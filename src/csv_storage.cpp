#include "inventory/csv_storage.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace inv {
namespace {

const char* kProductsHeader     = "product_id,name,quantity,price,min_stock,supplier_id";
const char* kSuppliersHeader    = "supplier_id,name,contact";
const char* kTransactionsHeader = "tx_id,product_id,type,quantity,date";

void strip_cr(std::string& s) { if (!s.empty() && s.back() == '\r') s.pop_back(); }

bool to_int(const std::string& s, int& out) {
    if (s.empty()) return false;
    try {
        std::size_t pos = 0;
        const int v = std::stoi(s, &pos);
        if (pos != s.size()) return false;
        out = v;
        return true;
    } catch (...) {
        return false;
    }
}

bool to_double(const std::string& s, double& out) {
    if (s.empty()) return false;
    try {
        std::size_t pos = 0;
        const double v = std::stod(s, &pos);
        if (pos != s.size()) return false;
        out = v;
        return true;
    } catch (...) {
        return false;
    }
}

std::string warn(const std::string& file, std::size_t line, const std::string& why) {
    std::ostringstream os;
    os << file << " line " << line << ": " << why << " (row skipped)";
    return os.str();
}

/// Read every line of a CSV file. ok=false means the file is absent.
bool read_lines(const std::string& path, std::vector<std::string>& out) {
    std::ifstream in(path);
    if (!in) return false;
    std::string line;
    while (std::getline(in, line)) { strip_cr(line); out.push_back(line); }
    return true;
}

} // namespace

// -- RFC 4180 quoting -------------------------------------------------------

std::string csv_quote(const std::string& raw) {
    if (raw.find_first_of(",\"\r\n") == std::string::npos) return raw;
    std::string out = "\"";
    for (char c : raw) { if (c == '"') out += "\"\""; else out += c; }
    out += '"';
    return out;
}

std::vector<std::string> csv_split(const std::string& line) {
    std::vector<std::string> f;
    std::string cur;
    bool quoted = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (quoted) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') { cur += '"'; ++i; }
                else quoted = false;
            } else cur += c;
        } else {
            if      (c == '"') quoted = true;
            else if (c == ',') { f.push_back(cur); cur.clear(); }
            else               cur += c;
        }
    }
    f.push_back(cur);
    return f;
}

// -- construction -----------------------------------------------------------

CsvStorage::CsvStorage(std::string dir) : dir_(std::move(dir)) {}

std::string CsvStorage::products_path()     const { return dir_ + "/products.csv"; }
std::string CsvStorage::suppliers_path()    const { return dir_ + "/suppliers.csv"; }
std::string CsvStorage::transactions_path() const { return dir_ + "/transactions.csv"; }

// -- load -------------------------------------------------------------------

LoadResult CsvStorage::load(Inventory& inv) const {
    LoadResult r;
    inv.clear();

    std::vector<Supplier>    sup;
    std::vector<Product>     prod;
    std::vector<Transaction> tx;
    std::vector<std::string> lines;

    // ---- suppliers.csv ----
    lines.clear();
    if (!read_lines(suppliers_path(), lines)) {
        r.warnings.push_back("suppliers.csv not found - starting with no suppliers");
    } else {
        for (std::size_t i = 1; i < lines.size(); ++i) {     // row 0 is the header
            if (lines[i].empty()) continue;
            std::vector<std::string> f = csv_split(lines[i]);
            if (f.size() != 3) { r.warnings.push_back(warn("suppliers.csv", i + 1, "expected 3 columns")); continue; }
            if (f[0].empty())  { r.warnings.push_back(warn("suppliers.csv", i + 1, "empty supplier_id")); continue; }
            sup.push_back(Supplier{f[0], f[1], f[2]});
        }
    }

    // ---- products.csv ----
    lines.clear();
    if (!read_lines(products_path(), lines)) {
        r.warnings.push_back("products.csv not found - starting with an empty inventory");
    } else {
        for (std::size_t i = 1; i < lines.size(); ++i) {
            if (lines[i].empty()) continue;
            std::vector<std::string> f = csv_split(lines[i]);
            if (f.size() != 6) { r.warnings.push_back(warn("products.csv", i + 1, "expected 6 columns")); continue; }
            Product p;
            p.product_id = f[0];
            p.name       = f[1];
            if (!to_int(f[2], p.quantity) || !to_double(f[3], p.price) || !to_int(f[4], p.min_stock)) {
                r.warnings.push_back(warn("products.csv", i + 1, "quantity, price or min_stock is not a number"));
                continue;
            }
            p.supplier_id = f[5];
            if (p.product_id.empty()) { r.warnings.push_back(warn("products.csv", i + 1, "empty product_id")); continue; }
            if (p.quantity < 0)       { r.warnings.push_back(warn("products.csv", i + 1, "negative quantity")); continue; }
            prod.push_back(p);
        }
    }

    // ---- transactions.csv ----
    lines.clear();
    if (read_lines(transactions_path(), lines)) {
        for (std::size_t i = 1; i < lines.size(); ++i) {
            if (lines[i].empty()) continue;
            std::vector<std::string> f = csv_split(lines[i]);
            if (f.size() != 5) { r.warnings.push_back(warn("transactions.csv", i + 1, "expected 5 columns")); continue; }
            Transaction t;
            t.tx_id      = f[0];
            t.product_id = f[1];
            if (!parse_tx_type(f[2], t.type)) {
                r.warnings.push_back(warn("transactions.csv", i + 1, "type must be IN or OUT")); continue;
            }
            if (!to_int(f[3], t.quantity)) {
                r.warnings.push_back(warn("transactions.csv", i + 1, "quantity is not a number")); continue;
            }
            t.date = f[4];
            tx.push_back(t);
        }
    }

    inv.load_raw(std::move(sup), std::move(prod), std::move(tx));
    return r;
}

// -- save -------------------------------------------------------------------

namespace {
/// Write `body` to `path` atomically: temp file first, then rename.
std::string atomic_write(const std::string& path, const std::string& body) {
    const std::string tmp = path + ".tmp";
    { std::ofstream out(tmp, std::ios::binary);
      if (!out) return "cannot open " + tmp + " for writing";
      out << body;
      out.flush();
      if (!out) return "write failed for " + tmp; }
    std::remove(path.c_str());                       // Windows rename needs this
    if (std::rename(tmp.c_str(), path.c_str()) != 0)
        return "could not replace " + path;
    return "";
}
} // namespace

std::string CsvStorage::save(const Inventory& inv) const {
    std::ostringstream s;
    s << kSuppliersHeader << "\n";
    for (const Supplier& x : inv.suppliers())
        s << csv_quote(x.supplier_id) << ',' << csv_quote(x.name) << ','
          << csv_quote(x.contact) << '\n';
    std::string err = atomic_write(suppliers_path(), s.str());
    if (!err.empty()) return err;

    std::ostringstream p;
    p << kProductsHeader << "\n";
    for (const Product& x : inv.products())
        p << csv_quote(x.product_id) << ',' << csv_quote(x.name) << ','
          << x.quantity << ',' << x.price << ',' << x.min_stock << ','
          << csv_quote(x.supplier_id) << '\n';
    err = atomic_write(products_path(), p.str());
    if (!err.empty()) return err;

    std::ostringstream t;
    t << kTransactionsHeader << "\n";
    for (const Transaction& x : inv.transactions())
        t << csv_quote(x.tx_id) << ',' << csv_quote(x.product_id) << ','
          << to_string(x.type) << ',' << x.quantity << ',' << csv_quote(x.date) << '\n';
    return atomic_write(transactions_path(), t.str());
}

} // namespace inv
