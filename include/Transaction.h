#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

enum class TransactionType { IN, OUT };

struct Transaction {
    int id;
    int productId;
    TransactionType type;
    int quantity;
    std::string date;
};

#endif