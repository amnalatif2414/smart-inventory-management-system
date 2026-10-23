# Smart Inventory Management System

## 1. Project Overview

A C++17 command-line application for managing inventory in a small retail shop.
It maintains products, suppliers and a stock transaction history, generates stock
reports, and stores everything in three CSV files so the data survives a restart.

Single-user, no graphical interface, no database and no external libraries.

## 2. Features

- **Products** — add, edit, remove, search by ID or name, sort by name or quantity
- **Suppliers** — store supplier records and link them to products
- **Stock transactions** — record stock-in and stock-out movements with a full history
- **Reports** — current stock with total value, and low-stock items with their supplier
- **Validation** — rejects duplicate IDs, invalid fields and non-numeric input
- **Persistence** — CSV files loaded at start-up and saved after every change

## 3. Requirements

- A C++ compiler supporting C++17 (GCC, Clang or MSVC)
- CMake 3.16 or later

Nothing else — the project has no external dependencies.

## 4. Build

Run from the repository root.

**Linux / macOS**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

**Windows** (Visual Studio Developer Command Prompt)

```bat
cmake -S . -B build
cmake --build build --config Debug
```

## 5. Run

```bash
./build/inventory                 # Windows:  build\Debug\inventory.exe
```

The application reads and writes `products.csv`, `suppliers.csv` and
`transactions.csv` in its **current working directory**. To start from the bundled
sample data, copy it there first:

```bash
cp data/*.csv .                   # Windows:  copy data\*.csv .
```

Field definitions and the full file-format specification are in
[`docs/CODEBOOK.md`](docs/CODEBOOK.md).

## 6. Testing

```bash
ctest --test-dir build --output-on-failure
```

Or run the test executable directly:

```bash
./build/inventory_tests           # Windows:  build\Debug\inventory_tests.exe
```

Expected result:

```
ALL TESTS PASSED  (270 checks, 10 test groups)
```

The suite covers an empty inventory, duplicate IDs, invalid fields, negative and
excessive quantities, CSV quoting, missing and malformed files, and persistence
across a restart. It exits with a non-zero status if any check fails.

## 7. Application Architecture

Three layers, each in its own header and source file:

| Layer | Files | Responsibility |
|---|---|---|
| Interface | `console_ui.cpp`, `main.cpp` | Reads and validates menu input |
| Logic | `inventory.cpp` | Owns the records and enforces the rules |
| Data | `types.hpp`, `csv_storage.cpp` | Record types and CSV persistence |

```text
include/inventory/    public headers
src/                  implementation
tests/                automated test suite
docs/CODEBOOK.md      data dictionary and file-format reference
data/                 sample CSV data
```

Records are held in `std::vector` containers that own them, with an
`unordered_map` from ID to position for fast lookup. Entities reference each other
by ID rather than by pointer, so the relationships survive being written to disk
and read back.

## 8. Business Rules

- **Stock can never go negative** — a stock-out larger than the quantity on hand is refused
- **Quantities change only through a recorded transaction**, so the log always explains the stock level
- **Duplicate product or supplier IDs are rejected**
- **Non-numeric input is re-prompted**, not accepted
- **A missing CSV file** gives an empty collection and a warning rather than a crash
- **A malformed row** is skipped and reported with its line number
- **Saving is atomic** — written to a temporary file and then renamed, so an
  interrupted save cannot corrupt the data
