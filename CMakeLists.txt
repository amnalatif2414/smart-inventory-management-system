cmake_minimum_required(VERSION 3.10)
project(SmartInventoryManagementSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

add_executable(inventory_app
    src/main.cpp
    src/Inventory.cpp
    src/Product.cpp
    src/Supplier.cpp
    src/Transaction.cpp
)