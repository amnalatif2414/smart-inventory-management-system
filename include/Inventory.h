#ifndef INVENTORY_H
#define INVENTORY_H

#include <vector>
#include "Product.h"
#include "Supplier.h"
#include "Transaction.h"

class Inventory {
public:
    void addProduct(const Product& p);
    // more methods to come in Phase 2

private:
    std::vector<Product> products;
    std::vector<Supplier> suppliers;
    std::vector<Transaction> transactions;
};

#endif