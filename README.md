# Smart Inventory Management System

A command-line inventory management application written in C++17 for a small retail shop. The system allows the owner to manage products and suppliers, record stock-in/stock-out transactions, search and sort the product list, and generate simple reports, with all data persisted between runs in CSV files.

## Project Status

Current phase: Conception phase — build skeleton with entity stubs (Product, Supplier, Transaction, Inventory). Full functionality (persistence, operations, search/sort, reporting, and the menu interface) will be implemented in the development phase.

## Architecture

The application is organized into three layers:

* **Data layer** — entity structs and CSV file I/O (load/save)
* **Logic layer** — product/supplier/transaction operations, stock calculations, search, sort, and reporting
* **Interface layer** — command-line menu (`main.cpp`)

## Entities

* **Product** — id, name, quantity, price, min stock level, max stock level, supplier ID
* **Supplier** — id, name, contact details
* **Transaction** — id, product ID, transaction type (`TransactionType` enum: `IN` / `OUT`), quantity, date (YYYY-MM-DD)

Relationships: one supplier supplies many products, and one product has many transactions (both 1:n), expressed via stored IDs rather than raw pointers.

## Persistence

Data is stored in three CSV files, each with a header row and one record per line:

* `products.csv`
* `suppliers.csv`
* `transactions.csv`

Files are loaded at startup and written back after changes. A missing file is treated as an empty inventory; malformed lines are skipped with a warning naming the line number. Saving writes to a temporary file that then replaces the original.

## Testing

Planned: a separate assert-based test executable covering product operations, stock calculations, search/sort, reports, and edge cases (invalid input, empty inventory, missing/malformed files, persistence across restarts), supplemented by manual menu testing.

## Build Instructions

Requires CMake (3.10+) and a C++17 compiler.

```
mkdir build
cd build
cmake ..
cmake --build .
```

## Run

```
# From the build folder
Debug\inventory_app.exe      # Windows
./inventory_app              # Linux/Mac
```