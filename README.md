# smart-inventory-management-system

A command-line inventory management application written in C++17 for a small retail shop. 
The system allows the owner to manage products and suppliers, record stock-in/stock-out transactions, 
search and sort the product list, and generate simple reports, with all data persisted between runs 
in CSV files.

## Project Status

**Current phase:** Conception phase — build skeleton with entity stubs (Product, Supplier, Transaction, Inventory).
Full functionality (persistence, operations, search/sort, reporting, and the menu interface) will be implemented in the development phase.

## Architecture

The application is organized into three layers:
- **Data layer** — entity structs and CSV file I/O (load/save)
- **Logic layer** — product/supplier/transaction operations, stock calculations, search, sort, and reporting
- **Interface layer** — command-line menu (`main.cpp`)

## Entities

- **Product** — id, name, quantity, price, supplier ID
- **Supplier** — id, name, contact details
- **Transaction** — id, product ID, transaction type (IN/OUT), quantity, date

## Build Instructions

Requires CMake (3.10+) and a C++17 compiler.

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Run

```bash
# From the build folder
Debug\inventory_app.exe      # Windows
./inventory_app              # Linux/Mac
```
