#include "inventory/console_ui.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>

namespace inv {
namespace {

std::string today() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[16];
    std::strftime(buf, sizeof buf, "%Y-%m-%d", &tm);
    return buf;
}

std::string ask(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    return s;
}

/// Re-prompts until the input really is a whole number (validation N3).
int ask_int(const std::string& prompt) {
    while (true) {
        const std::string s = ask(prompt);
        try { std::size_t pos = 0; int v = std::stoi(s, &pos); if (pos == s.size()) return v; }
        catch (...) {}
        std::cout << "  ! please enter a whole number\n";
    }
}

double ask_double(const std::string& prompt) {
    while (true) {
        const std::string s = ask(prompt);
        try { std::size_t pos = 0; double v = std::stod(s, &pos); if (pos == s.size()) return v; }
        catch (...) {}
        std::cout << "  ! please enter a number\n";
    }
}

void say(Status st, const char* ok_text) {
    if (st == Status::Ok) std::cout << "  > " << ok_text << "\n";
    else                  std::cout << "  ! " << to_message(st) << "\n";
}

void table(const std::vector<Product>& v) {
    if (v.empty()) { std::cout << "  (no products)\n"; return; }
    std::cout << "\n  " << std::left
              << std::setw(10) << "ID" << std::setw(26) << "NAME" << std::right
              << std::setw(7) << "QTY" << std::setw(10) << "PRICE"
              << std::setw(6) << "MIN" << "   " << std::left << "SUPPLIER\n";
    std::cout << "  " << std::string(70, '-') << "\n";
    for (const Product& p : v)
        std::cout << "  " << std::left
                  << std::setw(10) << p.product_id << std::setw(26) << p.name << std::right
                  << std::setw(7) << p.quantity
                  << std::setw(10) << std::fixed << std::setprecision(2) << p.price
                  << std::setw(6) << p.min_stock << "   " << std::left << p.supplier_id << "\n";
    std::cout << "\n";
}

} // namespace

ConsoleUI::ConsoleUI(Inventory& inv, const CsvStorage& store)
    : inv_(inv), store_(store) {}

void ConsoleUI::show_menu() const {
    std::cout <<
        "\n=====================================================\n"
        "   SMART INVENTORY MANAGEMENT SYSTEM\n"
        "=====================================================\n"
        "   1  List all products        7  Stock report\n"
        "   2  Add product              8  Low-stock report\n"
        "   3  Edit product             9  Transaction history\n"
        "   4  Remove product          10  Add supplier\n"
        "   5  Record stock-IN         11  Save now\n"
        "   6  Record stock-OUT         0  Save and exit\n"
        "  12  Search    13  Sort\n"
        "-----------------------------------------------------\n"
        "  choice > ";
}

void ConsoleUI::add_product() {
    Product p;
    p.product_id = ask("  Product ID      : ");
    p.name       = ask("  Name            : ");
    p.price      = ask_double("  Price           : ");
    p.min_stock  = ask_int("  Minimum stock   : ");
    p.supplier_id= ask("  Supplier ID     : ");
    p.quantity   = 0;                       // stock arrives only through an IN movement
    say(inv_.add_product(p), "product added with zero stock - record a stock-IN next");
}

void ConsoleUI::edit_product() {
    Product p;
    p.product_id = ask("  Product ID      : ");
    p.name       = ask("  New name        : ");
    p.price      = ask_double("  New price       : ");
    p.min_stock  = ask_int("  New minimum     : ");
    p.supplier_id= ask("  New supplier ID : ");
    say(inv_.edit_product(p), "product updated");
}

void ConsoleUI::remove_product() {
    say(inv_.remove_product(ask("  Product ID to remove : ")), "product removed");
}

void ConsoleUI::movement(TxType type) {
    const std::string id = ask("  Product ID      : ");
    const int qty = ask_int("  Quantity        : ");
    const Status st = inv_.record_transaction(id, type, qty, today());
    if (st == Status::Ok) {
        const Product* p = inv_.find_product(id);
        std::cout << "  > " << to_string(type) << " recorded; " << id
                  << " now at " << (p ? p->quantity : 0) << "\n";
    } else {
        std::cout << "  ! " << to_message(st) << "\n";
    }
}

void ConsoleUI::do_search() { table(inv_.search(ask("  ID or name contains : "))); }

void ConsoleUI::do_sort() {
    const std::string k = ask("  Sort by [1] name  [2] quantity : ");
    table(inv_.sorted(k == "2" ? SortKey::Quantity : SortKey::Name));
}

void ConsoleUI::stock_report() const {
    double value = 0.0;
    int units = 0;
    for (const Product& p : inv_.products()) { value += p.quantity * p.price; units += p.quantity; }
    std::cout << "\n  STOCK REPORT\n";
    table(inv_.products());
    std::cout << "  Products: " << inv_.products().size()
              << "   Units on hand: " << units
              << "   Stock value: " << std::fixed << std::setprecision(2) << value << "\n\n";
}

void ConsoleUI::low_stock_report() const {
    const std::vector<Product> low = inv_.low_stock();
    std::cout << "\n  LOW-STOCK REPORT (" << low.size()
              << " item(s) at or below the minimum)\n";
    table(low);
    for (const Product& p : low) {
        const Supplier* s = inv_.find_supplier(p.supplier_id);
        std::cout << "   - reorder " << p.product_id << " from "
                  << (s ? s->name : std::string("(no supplier set)")) << "\n";
    }
    std::cout << "\n";
}

void ConsoleUI::history() const {
    const std::string id = ask("  Product ID      : ");
    const std::vector<Transaction> h = inv_.history_for(id);
    if (h.empty()) { std::cout << "  (no movements recorded for " << id << ")\n"; return; }
    for (const Transaction& t : h)
        std::cout << "   " << t.tx_id << "  " << std::setw(4) << std::left << to_string(t.type)
                  << "  " << std::setw(6) << std::right << t.quantity << "  " << t.date << "\n";
}

void ConsoleUI::add_supplier() {
    Supplier s;
    s.supplier_id = ask("  Supplier ID     : ");
    s.name        = ask("  Name            : ");
    s.contact     = ask("  Contact         : ");
    say(inv_.add_supplier(s), "supplier added");
}

void ConsoleUI::save_now() const {
    const std::string err = store_.save(inv_);
    std::cout << (err.empty() ? "  > saved to the CSV files" : "  ! " + err) << "\n";
}

void ConsoleUI::dispatch(const std::string& c, bool& running) {
    if      (c == "1")  table(inv_.products());
    else if (c == "2")  add_product();
    else if (c == "3")  edit_product();
    else if (c == "4")  remove_product();
    else if (c == "5")  movement(TxType::In);
    else if (c == "6")  movement(TxType::Out);
    else if (c == "7")  stock_report();
    else if (c == "8")  low_stock_report();
    else if (c == "9")  history();
    else if (c == "10") add_supplier();
    else if (c == "11") save_now();
    else if (c == "12") do_search();
    else if (c == "13") do_sort();
    else if (c == "0")  { save_now(); running = false; }
    else std::cout << "  ! unknown choice\n";
}

void ConsoleUI::run() {
    bool running = true;
    while (running) {
        show_menu();
        std::string choice;
        if (!std::getline(std::cin, choice)) break;
        dispatch(choice, running);
    }
}

} // namespace inv
