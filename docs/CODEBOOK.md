# Code book

Companion to the Phase 1 conceptual design. Field-by-field reference, file
format, and the public API.

## 1. Entities

### Product — primary key `product_id`

| Field | Type | Constraint | Meaning |
|---|---|---|---|
| `product_id` | string | non-empty, unique | e.g. `P001` |
| `name` | string | non-empty | description |
| `quantity` | int | `>= 0`, **derived** | stock on hand; changed only by a transaction |
| `price` | double | `>= 0` | unit price |
| `min_stock` | int | `>= 0` | low-stock threshold |
| `supplier_id` | string | empty or an existing ID | foreign key to Supplier |

### Supplier — primary key `supplier_id`

| Field | Type | Constraint |
|---|---|---|
| `supplier_id` | string | non-empty, unique |
| `name` | string | non-empty |
| `contact` | string | free text |

### Transaction — primary key `tx_id`, append-only

| Field | Type | Constraint |
|---|---|---|
| `tx_id` | string | assigned by the program, `T` + 5 digits |
| `product_id` | string | must exist when recorded |
| `type` | TxType | `IN` or `OUT` |
| `quantity` | int | `> 0`; direction comes from `type` |
| `date` | string | `YYYY-MM-DD` |

`IN` adds `quantity`; `OUT` subtracts it and is refused if the result would be
negative.

## 2. In-memory structures

| Member | Type | Role |
|---|---|---|
| `products_` | `std::vector<Product>` | **owns** the products |
| `suppliers_` | `std::vector<Supplier>` | **owns** the suppliers |
| `transactions_` | `std::vector<Transaction>` | **owns** the log, append-only |
| `product_ix_` | `unordered_map<string, size_t>` | `product_id` → index |
| `supplier_ix_` | `unordered_map<string, size_t>` | `supplier_id` → index |

Indices rather than pointers, because `push_back` may reallocate the vector and
invalidate every pointer into it. `reindex()` rebuilds both maps after an erase.
Lookup is O(1) average; add is O(1) amortised; remove is O(n).

## 3. Status codes

| Code | Raised when |
|---|---|
| `Ok` | operation succeeded |
| `DuplicateId` | the ID already exists |
| `NotFound` | no record with that ID |
| `InvalidField` | empty ID or name, negative price or threshold |
| `NonPositiveQty` | a movement of zero or fewer units |
| `InsufficientStock` | a stock-OUT larger than the stock on hand |

## 4. File format

Three CSV files, each with a header row and one record per line. A field
containing a comma, a double quote, CR or LF is wrapped in double quotes and an
embedded quote is doubled (RFC 4180).

```
products.csv      product_id,name,quantity,price,min_stock,supplier_id
suppliers.csv     supplier_id,name,contact
transactions.csv  tx_id,product_id,type,quantity,date
```

Example:

```
product_id,name,quantity,price,min_stock,supplier_id
P001,Rice basmati 5kg,26,189.5,10,S001
P002,Cooking oil 1L,12,49.9,15,S001
```

| Situation | Behaviour |
|---|---|
| File absent | empty collection, warning, program continues |
| Wrong column count | row skipped, warning names the line number |
| Non-numeric quantity or price | row skipped, warning names the line number |
| Unknown transaction type | row skipped, warning names the line number |
| Negative quantity in file | row skipped, warning names the line number |

`save()` writes `<file>.tmp`, closes it, then renames over the original.

## 5. Public API

```cpp
// Inventory -- logic layer
Status add_product(const Product&);
Status edit_product(const Product&);          // ignores p.quantity by design
Status remove_product(const std::string& id);
Product* find_product(const std::string& id); // BORROWED, may be nullptr
Status add_supplier(const Supplier&);
Status record_transaction(id, TxType, qty, date);
std::vector<Product>     search(text) / sorted(SortKey) / low_stock();
std::vector<Transaction> history_for(id);

// CsvStorage -- data layer
LoadResult  load(Inventory&) const;
std::string save(const Inventory&) const;     // "" on success
```

**Pointer contract.** `find_product()` and `find_supplier()` return a raw,
**non-owning** pointer. Do not delete it and do not hold it across any call that
adds or removes a record. `nullptr` means "not found".

## 6. Test coverage

`tests/test_inventory.cpp`, 10 groups, 270 assertions:

| Group | Covers |
|---|---|
| empty inventory | empty reports, no crash |
| add / duplicate / invalid | `DuplicateId`, `InvalidField`, negative price |
| edit and remove | quantity not editable, `NotFound` |
| stock calculations | zero, negative, unknown ID, stock underflow |
| index integrity | 200 inserts force reallocation; erase + reindex |
| search, sort, report | exact ID, case-insensitive name, threshold boundary |
| CSV quoting | comma, doubled quote round trip |
| persistence | save → reload equality across a simulated restart |
| missing files | warning, not an error |
| malformed rows | skipped with line number, good rows survive |
