// Entry point: wires the three layers together and starts the menu loop.
#include "inventory/console_ui.hpp"

#include <iostream>
#include <string>

int main() {
    inv::Inventory  inventory;
    inv::CsvStorage storage(".");          // CSV files in the working directory

    const inv::LoadResult r = storage.load(inventory);
    for (const std::string& w : r.warnings) std::cout << "  ~ " << w << "\n";
        auto plural = [](std::size_t n, const char* word) {
        return std::to_string(n) + " " + word + (n == 1 ? "" : "s");
    };
    std::cout << "  > loaded " << plural(inventory.products().size(),     "product")
              << ", "          << plural(inventory.suppliers().size(),    "supplier")
              << ", "          << plural(inventory.transactions().size(), "transaction")
              << "\n";

    inv::ConsoleUI ui(inventory, storage);
    ui.run();
    return 0;
}
