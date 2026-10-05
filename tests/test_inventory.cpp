// ---------------------------------------------------------------------------
// test_inventory.cpp -- assert-based test executable (Phase 1, section 5).
// Covers product operations, stock calculations, search and sort, reports,
// invalid input, an empty inventory, missing and malformed files, and
// persistence across a restart. Returns 0 only if every check passes.
// ---------------------------------------------------------------------------

#include "inventory/csv_storage.hpp"
#include "inventory/inventory.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

using namespace inv;

static int g_checks = 0;
#define CHECK(expr) do { ++g_checks; assert(expr); } while (0)

static void banner(const char* name) { std::cout << "  [ RUN ] " << name << "\n"; }

static Product mk(const std::string& id, int min_stock = 5) {
    Product p;
    p.product_id = id;
    p.name       = "Item " + id;
    p.price      = 9.99;
    p.min_stock  = min_stock;
    p.supplier_id= "S001";
    return p;
}

// --- 1. empty inventory ----------------------------------------------------
static void test_empty() {
    banner("empty inventory");
    Inventory inv;
    CHECK(inv.empty());
    CHECK(inv.products().empty());
    CHECK(inv.low_stock().empty());
    CHECK(inv.search("anything").empty());
    CHECK(inv.sorted(SortKey::Name).empty());
}

// --- 2. add / duplicate / invalid ------------------------------------------
static void test_add_validation() {
    banner("add, duplicate ID and invalid fields");
    Inventory inv;
    CHECK(inv.add_product(mk("P001")) == Status::Ok);
    CHECK(inv.add_product(mk("P001")) == Status::DuplicateId);   // duplicate rejected
    CHECK(inv.products().size() == 1);

    CHECK(inv.add_product(Product{}) == Status::InvalidField);   // empty ID and name

    Product bad = mk("P002");
    bad.price = -1.0;
    CHECK(inv.add_product(bad) == Status::InvalidField);         // negative price

    bad = mk("P003", -4);
    CHECK(inv.add_product(bad) == Status::InvalidField);         // negative threshold
}

// --- 3. edit and remove ----------------------------------------------------
static void test_edit_remove() {
    banner("edit and remove");
    Inventory inv;
    CHECK(inv.add_product(mk("P001")) == Status::Ok);
    CHECK(inv.record_transaction("P001", TxType::In, 10, "2026-01-01") == Status::Ok);

    Product e = mk("P001", 3);
    e.name = "Renamed";
    e.quantity = 999;                    // must be ignored by edit_product
    CHECK(inv.edit_product(e) == Status::Ok);
    CHECK(inv.find_product("P001")->name == "Renamed");
    CHECK(inv.find_product("P001")->min_stock == 3);
    CHECK(inv.find_product("P001")->quantity == 10);   // unchanged: log is the truth

    CHECK(inv.edit_product(mk("NOPE")) == Status::NotFound);
    CHECK(inv.remove_product("NOPE")   == Status::NotFound);
    CHECK(inv.remove_product("P001")   == Status::Ok);
    CHECK(inv.find_product("P001")     == nullptr);
}

// --- 4. stock calculations -------------------------------------------------
static void test_stock() {
    banner("stock-in, stock-out and the no-negative-stock rule");
    Inventory inv;
    CHECK(inv.add_product(mk("P010")) == Status::Ok);

    CHECK(inv.record_transaction("P010", TxType::In,  0, "2026-01-01") == Status::NonPositiveQty);
    CHECK(inv.record_transaction("P010", TxType::In, -5, "2026-01-01") == Status::NonPositiveQty);
    CHECK(inv.record_transaction("NOPE", TxType::In,  5, "2026-01-01") == Status::NotFound);

    CHECK(inv.record_transaction("P010", TxType::In, 20, "2026-01-01") == Status::Ok);
    CHECK(inv.find_product("P010")->quantity == 20);

    CHECK(inv.record_transaction("P010", TxType::Out, 21, "2026-01-02") == Status::InsufficientStock);
    CHECK(inv.find_product("P010")->quantity == 20);          // unchanged after refusal
    CHECK(inv.transactions().size() == 1);                    // failure is not logged

    CHECK(inv.record_transaction("P010", TxType::Out, 8, "2026-01-02") == Status::Ok);
    CHECK(inv.find_product("P010")->quantity == 12);
    CHECK(inv.history_for("P010").size() == 2);
}

// --- 5. index integrity across reallocation --------------------------------
static void test_index() {
    banner("ID index survives vector growth and erase");
    Inventory inv;
    for (int i = 0; i < 200; ++i)                       // forces reallocation
        CHECK(inv.add_product(mk("P" + std::to_string(i))) == Status::Ok);
    CHECK(inv.find_product("P0")   != nullptr);
    CHECK(inv.find_product("P199") != nullptr);

    CHECK(inv.remove_product("P0") == Status::Ok);
    CHECK(inv.find_product("P0")   == nullptr);
    CHECK(inv.find_product("P199") != nullptr);         // index repaired after erase
    CHECK(inv.find_product("P199")->product_id == "P199");
}

// --- 6. search, sort, reports ----------------------------------------------
static void test_queries() {
    banner("search, sort and the low-stock report");
    Inventory inv;
    CHECK(inv.add_product(mk("P100", 5)) == Status::Ok);
    CHECK(inv.add_product(mk("P200", 2)) == Status::Ok);
    CHECK(inv.record_transaction("P100", TxType::In, 30, "2026-01-01") == Status::Ok);

    CHECK(inv.search("P100").size() == 1);              // by exact ID
    CHECK(inv.search("item").size() == 2);              // by name, case-insensitive
    CHECK(inv.search("").empty());

    const std::vector<Product> byqty = inv.sorted(SortKey::Quantity);
    CHECK(byqty.front().product_id == "P200");          // 0 before 30

    const std::vector<Product> low = inv.low_stock();
    CHECK(low.size() == 1);
    CHECK(low[0].product_id == "P200");                 // 0 <= 2
}

// --- 7. CSV quoting --------------------------------------------------------
static void test_quoting() {
    banner("CSV quoting of commas, quotes and newlines");
    const std::string awkward = "Rice, basmati 5kg \"premium\"";
    const std::vector<std::string> f = csv_split(csv_quote(awkward) + ",plain");
    CHECK(f.size() == 2);
    CHECK(f[0] == awkward);
    CHECK(f[1] == "plain");
    CHECK(csv_quote("simple") == "simple");             // not quoted when not needed
}

// --- 8. persistence across a restart ---------------------------------------
static void test_persistence() {
    banner("save, reload and persistence across a restart");
    CsvStorage store("./test_data");
    std::remove(store.products_path().c_str());
    std::remove(store.suppliers_path().c_str());
    std::remove(store.transactions_path().c_str());

    Inventory a;
    CHECK(a.add_supplier(Supplier{"S001", "Oslo Engros, AS", "post@engros.no"}) == Status::Ok);
    CHECK(a.add_product(mk("P-500", 4)) == Status::Ok);
    CHECK(a.record_transaction("P-500", TxType::In, 15, "2026-02-03") == Status::Ok);
    CHECK(store.save(a).empty());

    Inventory b;                                        // simulates a restart
    const LoadResult r = store.load(b);
    CHECK(r.warnings.empty());
    CHECK(b.products().size()     == 1);
    CHECK(b.suppliers().size()    == 1);
    CHECK(b.transactions().size() == 1);
    CHECK(b.find_product("P-500")->quantity == 15);     // quantity persisted
    CHECK(b.find_supplier("S001")->name == "Oslo Engros, AS");   // comma survived quoting
}

// --- 9. missing files ------------------------------------------------------
static void test_missing_files() {
    banner("missing files are not an error");
    CsvStorage store("./test_missing");
    Inventory inv;
    const LoadResult r = store.load(inv);
    CHECK(r.ok);
    CHECK(!r.warnings.empty());                         // warned, not crashed
    CHECK(inv.empty());
}

// --- 10. malformed rows ----------------------------------------------------
static void test_malformed() {
    banner("malformed rows are skipped and reported by line number");
    const std::string dir = "./test_bad";
    std::remove((dir + "/products.csv").c_str());
    {
        std::ofstream out(dir + "/products.csv");
        out << "product_id,name,quantity,price,min_stock,supplier_id\n"
               "P001,Good item,5,2.50,2,S001\n"
               "P002,too few columns\n"
               "P003,Bad qty,abc,2.50,2,S001\n"
               ",No id,1,1.00,1,S001\n"
               "P005,Also good,1,3.00,4,S001\n";
    }
    Inventory inv;
    CsvStorage store(dir);
    const LoadResult r = store.load(inv);
    CHECK(r.ok);
    CHECK(inv.products().size() == 2);                  // the two good rows survived
    CHECK(inv.find_product("P001") != nullptr);
    CHECK(inv.find_product("P005") != nullptr);
    // three bad product rows, plus a warning for the absent transactions file
    CHECK(r.warnings.size() >= 3);
}

int main() {
    std::cout << "Running Smart Inventory Management System tests\n\n";
    test_empty();
    test_add_validation();
    test_edit_remove();
    test_stock();
    test_index();
    test_queries();
    test_quoting();
    test_persistence();
    test_missing_files();
    test_malformed();
    std::cout << "\n  ALL TESTS PASSED  (" << g_checks << " checks, 10 test groups)\n";
    return 0;
}
