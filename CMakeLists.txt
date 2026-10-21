cmake_minimum_required(VERSION 3.16)
project(smart_inventory_management_system VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(ENABLE_ASAN "Build with AddressSanitizer and UBSan" OFF)
if(ENABLE_ASAN AND NOT MSVC)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()

# ---- core library: data layer + logic layer -------------------------------
add_library(inventory_core
    src/inventory.cpp
    src/csv_storage.cpp
)
target_include_directories(inventory_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ---- application: interface layer + entry point ---------------------------
add_executable(inventory src/console_ui.cpp src/main.cpp)
target_link_libraries(inventory PRIVATE inventory_core)

# NOTE: the target must NOT be called "run_tests" -- enable_testing() makes the
# Visual Studio generator create a built-in RUN_TESTS target, and the two
# collide on Windows's case-insensitive filesystem.
# ---- assert-based test executable -----------------------------------------
enable_testing()
add_executable(inventory_tests tests/test_inventory.cpp)
target_link_libraries(inventory_tests PRIVATE inventory_core)
add_test(NAME unit_tests COMMAND inventory_tests)

# the test suite writes into these directories
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_data)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_bad)
