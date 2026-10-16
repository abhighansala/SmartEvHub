
cmake_minimum_required(VERSION 3.15)

project(SmartEVHub VERSION 1.0.0 LANGUAGES C CXX)

# ==================================================
# C++ configuration
# ==================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==================================================
# Project directories
# ==================================================
set(BACKEND_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(THIRD_PARTY_DIR "${BACKEND_DIR}/third_party")
set(SQLITE_DIR "${THIRD_PARTY_DIR}/sqlite")
set(CROW_INCLUDE_DIR "${THIRD_PARTY_DIR}/crow/include")
set(ASIO_INCLUDE_DIR "${THIRD_PARTY_DIR}/asio/include")

# ==================================================
# SQLite: bundled source, no system SQLite required
# ==================================================
if(NOT EXISTS "${SQLITE_DIR}/sqlite3.c"
   OR NOT EXISTS "${SQLITE_DIR}/sqlite3.h")
    message(FATAL_ERROR
        "SQLite source files are missing. Expected:\n"
        "  backend/third_party/sqlite/sqlite3.c\n"
        "  backend/third_party/sqlite/sqlite3.h"
    )
endif()

add_library(sqlite3_vendor STATIC
    "${SQLITE_DIR}/sqlite3.c"
)

target_include_directories(sqlite3_vendor PUBLIC
    "${SQLITE_DIR}"
)

# ==================================================
# Required dependencies
# ==================================================
find_package(Threads REQUIRED)

# Crow headers
if(NOT EXISTS "${CROW_INCLUDE_DIR}/crow.h")
    message(FATAL_ERROR
        "Crow header not found at ${CROW_INCLUDE_DIR}/crow.h"
    )
endif()

# Standalone Asio headers
if(NOT EXISTS "${ASIO_INCLUDE_DIR}/asio.hpp")
    message(FATAL_ERROR
        "Asio header not found at ${ASIO_INCLUDE_DIR}/asio.hpp"
    )
endif()

# ==================================================
# Main C++ REST API server
# ==================================================
add_executable(smart_ev_hub
    src/main.cpp
    src/db/Database.cpp
)

target_include_directories(smart_ev_hub PRIVATE
    "${BACKEND_DIR}/include"
    "${CROW_INCLUDE_DIR}"
    "${ASIO_INCLUDE_DIR}"
)

target_compile_definitions(smart_ev_hub PRIVATE
    ASIO_STANDALONE
)

if(WIN32)
    target_link_libraries(smart_ev_hub PRIVATE
        Threads::Threads
        sqlite3_vendor
        ws2_32
        mswsock
    )
else()
    target_link_libraries(smart_ev_hub PRIVATE
        Threads::Threads
        sqlite3_vendor
    )
endif()

# ==================================================
# Manual DSA tests
# ==================================================
add_executable(dsa_test
    tests/test_dsa.cpp
)

target_include_directories(dsa_test PRIVATE
    "${BACKEND_DIR}/include"
)

target_link_libraries(dsa_test PRIVATE
    Threads::Threads
)

# ==================================================
# Build summary
# ==================================================
message(STATUS "Smart EV Hub configuration:")
message(STATUS "  SQLite: bundled source")
message(STATUS "  Crow:   ${CROW_INCLUDE_DIR}")
message(STATUS "  Asio:   ${ASIO_INCLUDE_DIR}")
message(STATUS "  C++:    ${CMAKE_CXX_STANDARD}")
