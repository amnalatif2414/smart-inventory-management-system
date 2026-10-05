#ifndef INVENTORY_INVENTORY_HPP
#define INVENTORY_INVENTORY_HPP

// ---------------------------------------------------------------------------
// inventory.hpp -- the logic layer.
//
// MEMORY MODEL (Phase 1, section 3)
//   Inventory OWNS every record, by value, in std::vector members. Lifetime is
//   handled by RAII; there is no new/delete in this project. Lookup is O(1)
//   through unordered_map<string, size_t> holding INDICES, because a pointer
//   into a vector is invalidated when the vector reallocates, whereas an index
//   stays meaningful and is repaired after an erase.
//
//   find_product() / find_supplier() return a NON-OWNING, BORROWED pointer.
//   Do not delete it and do not hold it across any call that adds or removes a
//   record. nullptr means "not found".
// ---------------------------------------------------------------------------

#include "inventory/types.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace inv {

/// Result of an operation. Expected user errors are returned, never thrown.
enum class Status {
    Ok,
    DuplicateId,        ///< a record with that key already exists
    NotFound,           ///< no record with that key
    InvalidField,       ///< empty key/name, or a negative price or threshold
    NonPositiveQty,     ///< a movement of zero or fewer units
    InsufficientStock   ///< an OUT movement larger than the stock on hand
};

const char* to_message(Status s);

enum class SortKey { Name, Quantity };

class Inventory {
public:
    // ---- products ---------------------------------------------------------
    Status add_product(const Product& p);

    /// Updates the descriptive fields only. `p.quantity` is IGNORED: stock
    /// changes exclusively through record_transaction(), so the transaction
    /// log stays the single source of truth for every quantity.
    Status edit_product(const Product& p);

    Status remove_product(const std::string& product_id);

    Product*       find_product(const std::string& id);        ///< borrowed
    const Product* find_product(const std::string& id) const;  ///< borrowed

    // ---- suppliers --------------------------------------------------------
    Status add_supplier(const Supplier& s);
    Supplier*       find_supplier(const std::string& id);
    const Supplier* find_supplier(const std::string& id) const;

    // ---- transactions -----------------------------------------------------
    /// Apply a stock movement and append it to the log. The only way a
    /// quantity ever changes. An OUT larger than the stock on hand is refused.
    Status record_transaction(const std::string& product_id, TxType type,
                              int qty, const std::string& date);

    // ---- queries and reports ---------------------------------------------
    std::vector<Product> search(const std::string& text) const;   ///< by ID or name
    std::vector<Product> sorted(SortKey key) const;
    std::vector<Product> low_stock() const;                       ///< qty <= min_stock
    std::vector<Transaction> history_for(const std::string& product_id) const;

    // ---- accessors --------------------------------------------------------
    const std::vector<Product>&     products()     const { return products_; }
    const std::vector<Supplier>&    suppliers()    const { return suppliers_; }
    const std::vector<Transaction>& transactions() const { return transactions_; }

    /// Used by CsvStorage after a load: replaces the contents wholesale so
    /// that saved quantities are not re-derived by replaying the log.
    void load_raw(std::vector<Supplier> s, std::vector<Product> p,
                  std::vector<Transaction> t);
    void clear();
    bool empty() const { return products_.empty() && suppliers_.empty(); }

    std::string next_transaction_id() const;

private:
    void reindex();

    std::vector<Supplier>    suppliers_;
    std::vector<Product>     products_;
    std::vector<Transaction> transactions_;              ///< append-only

    std::unordered_map<std::string, std::size_t> product_ix_;
    std::unordered_map<std::string, std::size_t> supplier_ix_;
};

} // namespace inv
#endif
