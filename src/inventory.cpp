#include "inventory/inventory.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace inv {

std::string to_string(TxType t) { return t == TxType::In ? "IN" : "OUT"; }

bool parse_tx_type(const std::string& s, TxType& out) {
    if (s == "IN")  { out = TxType::In;  return true; }
    if (s == "OUT") { out = TxType::Out; return true; }
    return false;
}

const char* to_message(Status s) {
    switch (s) {
        case Status::Ok:                return "OK";
        case Status::DuplicateId:       return "a record with that ID already exists";
        case Status::NotFound:          return "no record with that ID";
        case Status::InvalidField:      return "a required field is empty or out of range";
        case Status::NonPositiveQty:    return "quantity must be greater than zero";
        case Status::InsufficientStock: return "not enough stock on hand for that movement";
    }
    return "unknown status";
}

namespace {
std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
} // namespace

void Inventory::reindex() {
    product_ix_.clear();
    supplier_ix_.clear();
    for (std::size_t i = 0; i < products_.size(); ++i)  product_ix_[products_[i].product_id] = i;
    for (std::size_t i = 0; i < suppliers_.size(); ++i) supplier_ix_[suppliers_[i].supplier_id] = i;
}

// -- products ---------------------------------------------------------------

Status Inventory::add_product(const Product& p) {
    if (p.product_id.empty() || p.name.empty())      return Status::InvalidField;
    if (p.quantity < 0 || p.min_stock < 0 || p.price < 0.0) return Status::InvalidField;
    if (product_ix_.count(p.product_id))             return Status::DuplicateId;

    products_.push_back(p);                              // vector takes ownership
    product_ix_[p.product_id] = products_.size() - 1;    // index, never a pointer
    return Status::Ok;
}

Status Inventory::edit_product(const Product& p) {
    Product* e = find_product(p.product_id);
    if (!e) return Status::NotFound;
    if (p.name.empty() || p.min_stock < 0 || p.price < 0.0) return Status::InvalidField;

    // quantity deliberately not copied -- owned by the transaction log
    e->name        = p.name;
    e->price       = p.price;
    e->min_stock   = p.min_stock;
    e->supplier_id = p.supplier_id;
    return Status::Ok;
}

Status Inventory::remove_product(const std::string& id) {
    auto it = product_ix_.find(id);
    if (it == product_ix_.end()) return Status::NotFound;
    products_.erase(products_.begin() + static_cast<long>(it->second));
    reindex();                 // indices after the erased one have shifted
    return Status::Ok;
}

Product* Inventory::find_product(const std::string& id) {
    auto it = product_ix_.find(id);
    return it == product_ix_.end() ? nullptr : &products_[it->second];
}
const Product* Inventory::find_product(const std::string& id) const {
    auto it = product_ix_.find(id);
    return it == product_ix_.end() ? nullptr : &products_[it->second];
}

// -- suppliers --------------------------------------------------------------

Status Inventory::add_supplier(const Supplier& s) {
    if (s.supplier_id.empty() || s.name.empty()) return Status::InvalidField;
    if (supplier_ix_.count(s.supplier_id))       return Status::DuplicateId;
    suppliers_.push_back(s);
    supplier_ix_[s.supplier_id] = suppliers_.size() - 1;
    return Status::Ok;
}

Supplier* Inventory::find_supplier(const std::string& id) {
    auto it = supplier_ix_.find(id);
    return it == supplier_ix_.end() ? nullptr : &suppliers_[it->second];
}
const Supplier* Inventory::find_supplier(const std::string& id) const {
    auto it = supplier_ix_.find(id);
    return it == supplier_ix_.end() ? nullptr : &suppliers_[it->second];
}

// -- transactions -----------------------------------------------------------

std::string Inventory::next_transaction_id() const {
    std::ostringstream os;
    os << "T" << std::setw(5) << std::setfill('0') << (transactions_.size() + 1);
    return os.str();
}

Status Inventory::record_transaction(const std::string& product_id, TxType type,
                                     int qty, const std::string& date) {
    if (qty <= 0) return Status::NonPositiveQty;

    Product* p = find_product(product_id);
    if (!p) return Status::NotFound;

    const int updated = (type == TxType::In) ? p->quantity + qty : p->quantity - qty;
    if (updated < 0) return Status::InsufficientStock;   // stock can never go negative

    p->quantity = updated;

    Transaction t;
    t.tx_id      = next_transaction_id();
    t.product_id = product_id;
    t.type       = type;
    t.quantity   = qty;
    t.date       = date;
    transactions_.push_back(t);
    return Status::Ok;
}

// -- queries and reports ----------------------------------------------------

std::vector<Product> Inventory::search(const std::string& text) const {
    std::vector<Product> out;
    if (text.empty()) return out;
    const std::string needle = lower(text);
    for (const Product& p : products_)
        if (lower(p.product_id) == needle || lower(p.name).find(needle) != std::string::npos)
            out.push_back(p);
    return out;
}

std::vector<Product> Inventory::sorted(SortKey key) const {
    std::vector<Product> out = products_;      // a copy: the caller cannot invalidate us
    if (key == SortKey::Name)
        std::sort(out.begin(), out.end(),
                  [](const Product& a, const Product& b) { return a.name < b.name; });
    else
        std::sort(out.begin(), out.end(),
                  [](const Product& a, const Product& b) { return a.quantity < b.quantity; });
    return out;
}

std::vector<Product> Inventory::low_stock() const {
    std::vector<Product> out;
    for (const Product& p : products_)
        if (p.quantity <= p.min_stock) out.push_back(p);
    return out;
}

std::vector<Transaction> Inventory::history_for(const std::string& product_id) const {
    std::vector<Transaction> out;
    for (const Transaction& t : transactions_)
        if (t.product_id == product_id) out.push_back(t);
    return out;    // already chronological: the log is append-only
}

// -- bulk -------------------------------------------------------------------

void Inventory::load_raw(std::vector<Supplier> s, std::vector<Product> p,
                         std::vector<Transaction> t) {
    suppliers_    = std::move(s);
    products_     = std::move(p);
    transactions_ = std::move(t);
    reindex();
}

void Inventory::clear() {
    suppliers_.clear(); products_.clear(); transactions_.clear();
    product_ix_.clear(); supplier_ix_.clear();
}

} // namespace inv
