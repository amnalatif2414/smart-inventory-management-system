#ifndef INVENTORY_CONSOLE_UI_HPP
#define INVENTORY_CONSOLE_UI_HPP

// ---------------------------------------------------------------------------
// console_ui.hpp -- the interface layer.
// Holds no data of its own: it reads the menu choice, validates the input and
// calls into Inventory and CsvStorage.
// ---------------------------------------------------------------------------

#include "inventory/csv_storage.hpp"
#include "inventory/inventory.hpp"

namespace inv {

class ConsoleUI {
public:
    ConsoleUI(Inventory& inv, const CsvStorage& store);
    void run();                        ///< main menu loop

private:
    void show_menu() const;
    void dispatch(const std::string& choice, bool& running);

    void add_product();
    void edit_product();
    void remove_product();
    void movement(TxType type);
    void do_search();
    void do_sort();
    void stock_report() const;
    void low_stock_report() const;
    void history() const;
    void add_supplier();
    void save_now() const;

    Inventory&        inv_;
    const CsvStorage& store_;
};

} // namespace inv
#endif
