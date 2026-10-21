# Smart Inventory Management System

Portfolio project for **DLBMINPAPCC01_E — Project: General Programming with C/C++**,
Task 3. Amna Latif, reg. no. 9211652.

A C++17 command-line inventory application for a small retail shop. It manages
products, suppliers and a stock transaction history, and persists everything to
three CSV files so the data survives a restart.

## Build and run

Requires a C++17 compiler and CMake 3.16+. No external dependencies.

### Linux / macOS
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/inventory
```

### Windows (Visual Studio Developer Command Prompt)
```bat
cmake -B build
cmake --build build --config Debug
build\Debug\inventory.exe
build\Debug\inventory_tests.exe
```

The program reads and writes `products.csv`, `suppliers.csv` and
`transactions.csv` **in the current working directory**. To start from the
bundled sample data:

```bash
cp data/*.csv .
./build/inventory
```

## Run the tests

```bash
cd build && ctest --output-on-failure
# or directly, for the per-group output:
./build/inventory_tests
```

Expected: `ALL TESTS PASSED (270 checks, 10 test groups)`.

Memory checks:
```bash
cmake -B build-asan -DENABLE_ASAN=ON && cmake --build build-asan -j
./build-asan/inventory_tests
valgrind --leak-check=full ./build/inventory_tests
```

## Layout

```
include/inventory/    public headers
  types.hpp             Product, Supplier, Transaction, TxType
  inventory.hpp         Inventory class - owns the records, enforces the rules
  csv_storage.hpp       CsvStorage class - load / save / RFC-4180 quoting
  console_ui.hpp        ConsoleUI class - the menu
src/                  implementation
tests/                assert-based test executable
docs/CODEBOOK.md      field-by-field reference and file format
data/                 sample CSV files
```

Layers: interface = `console_ui.cpp` + `main.cpp`; logic = `inventory.cpp`;
data = `types.hpp` + `csv_storage.cpp`.

## Rules the program enforces

- Stock can never become negative — a stock-OUT larger than the quantity on hand is refused.
- Quantities change **only** through a recorded transaction, so the log always explains the stock level.
- Duplicate product or supplier IDs are rejected.
- Non-numeric input is re-prompted rather than accepted.
- A missing CSV file gives an empty collection with a warning; a malformed row is skipped and reported with its line number.
- Saving writes to a temporary file and then renames, so an interrupted save cannot corrupt the data.
