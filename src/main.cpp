// Entry point: wires the three layers together and starts the menu loop.
#include "inventory/console_ui.hpp"

#include <iostream>

int main() {
    inv::Inventory  inventory;
    inv::CsvStorage storage(".");          // CSV files in the working directory

    const inv::LoadResult r = storage.load(inventory);
    for (const std::string& w : r.warnings) std::cout << "  ~ " << w << "\n";
    std::cout << "  > loaded " << inventory.products().size() << " products, "
              << inventory.suppliers().size() << " suppliers, "
              << inventory.transactions().size() << " transactions\n";

    inv::ConsoleUI ui(inventory, storage);
    ui.run();
    return 0;
}
