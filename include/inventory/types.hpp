#ifndef INVENTORY_TYPES_HPP
#define INVENTORY_TYPES_HPP

// ---------------------------------------------------------------------------
// types.hpp -- the three record types from the Phase 1 data model.
// Plain structs held BY VALUE inside the Inventory class. They never point at
// one another: Product refers to Supplier by supplier_id and Transaction
// refers to Product by product_id, so the relationships survive being written
// to and read back from the CSV files.
// ---------------------------------------------------------------------------

#include <string>

namespace inv {

/// Direction of a stock movement (Phase 1: "type (IN/OUT as an enum)").
enum class TxType { In, Out };

std::string to_string(TxType t);              ///< "IN" / "OUT"
bool parse_tx_type(const std::string& s, TxType& out);

/// A supplier of goods. Primary key: supplier_id.
struct Supplier {
    std::string supplier_id;   ///< unique, non-empty
    std::string name;
    std::string contact;
};

/// A stock item. Primary key: product_id. Foreign key: supplier_id.
struct Product {
    std::string product_id;    ///< unique, non-empty
    std::string name;
    int         quantity  = 0; ///< never negative; changed only by a Transaction
    double      price     = 0.0;
    int         min_stock = 0; ///< low-stock threshold
    std::string supplier_id;   ///< may be empty
};

/// One recorded stock movement. Primary key: tx_id. Foreign key: product_id.
struct Transaction {
    std::string tx_id;
    std::string product_id;
    TxType      type = TxType::In;
    int         quantity = 0;  ///< always positive; direction is in `type`
    std::string date;          ///< YYYY-MM-DD
};

} // namespace inv
#endif
