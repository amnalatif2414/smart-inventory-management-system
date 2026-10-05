#ifndef INVENTORY_CSV_STORAGE_HPP
#define INVENTORY_CSV_STORAGE_HPP

// ---------------------------------------------------------------------------
// csv_storage.hpp -- the data layer's file I/O (Phase 1, section 4).
//
// Three CSV files, each with a header row and one record per line. Fields that
// contain a comma, a quote or a newline are quoted, and an embedded quote is
// doubled (RFC 4180).
//
// Failure policy: a missing file yields an empty collection with a warning; a
// row with the wrong column count or a non-numeric number is skipped with a
// warning naming its line number; saving writes to "<path>.tmp" and then
// renames, so an interrupted save cannot corrupt the data.
// ---------------------------------------------------------------------------

#include "inventory/inventory.hpp"

#include <string>
#include <vector>

namespace inv {

struct LoadResult {
    bool                     ok = true;
    std::vector<std::string> warnings;
};

class CsvStorage {
public:
    /// @param dir  directory holding products.csv, suppliers.csv, transactions.csv
    explicit CsvStorage(std::string dir = ".");

    LoadResult  load(Inventory& inv) const;
    std::string save(const Inventory& inv) const;   ///< "" on success

    std::string products_path()     const;
    std::string suppliers_path()    const;
    std::string transactions_path() const;

private:
    std::string dir_;
};

// -- exposed for the unit tests ---------------------------------------------
std::string              csv_quote(const std::string& raw);
std::vector<std::string> csv_split(const std::string& line);

} // namespace inv
#endif
