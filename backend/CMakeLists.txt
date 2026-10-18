cmake_minimum_required(VERSION 3.20)

project(SmartEVHubBackend CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find threads
find_package(Threads REQUIRED)

# Common include directories
set(COMMON_INCLUDES
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/aditya_singh
    ${CMAKE_SOURCE_DIR}/aditya_singh/database
    ${CMAKE_SOURCE_DIR}/aditya_singh/models
    ${CMAKE_SOURCE_DIR}/aditya_singh/services
    ${CMAKE_SOURCE_DIR}/priyanshu_dutt
    ${CMAKE_SOURCE_DIR}/priyanshu_dutt/models
    ${CMAKE_SOURCE_DIR}/priyanshu_dutt/services
    ${CMAKE_SOURCE_DIR}/mritunjay_thakur
    ${CMAKE_SOURCE_DIR}/mritunjay_thakur/models
    ${CMAKE_SOURCE_DIR}/mritunjay_thakur/services
    ${CMAKE_SOURCE_DIR}/amit_singh_kaintura
    ${CMAKE_SOURCE_DIR}/amit_singh_kaintura/models
    ${CMAKE_SOURCE_DIR}/amit_singh_kaintura/services
    ${CMAKE_SOURCE_DIR}/abhishek_ghansala
    ${CMAKE_SOURCE_DIR}/abhishek_ghansala/dsa
    ${CMAKE_SOURCE_DIR}/agreev_singh
    ${CMAKE_SOURCE_DIR}/agreev_singh/models
    ${CMAKE_SOURCE_DIR}/agreev_singh/services
)

# Crow and Asio dependency resolution (fallback to bundled headers if system path not found)
if(EXISTS "/home/abhi/pbl/Crow/include")
    list(APPEND COMMON_INCLUDES "/home/abhi/pbl/Crow/include")
else()
    list(APPEND COMMON_INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/aditya_runtime/third_party/crow/include
        ${CMAKE_CURRENT_SOURCE_DIR}/aditya_runtime/third_party/asio/include
    )
endif()

add_compile_definitions(ASIO_STANDALONE)

# Platform-specific socket and dynamic linking libraries
if(WIN32)
    set(PLATFORM_LIBS ws2_32 mswsock)
else()
    set(PLATFORM_LIBS dl)
endif()

# -----------------------------------------------------------------------------
# 1. Main Unified Server Target
# -----------------------------------------------------------------------------
add_executable(smart_ev_hub_backend
    src/main.cpp
    src/sqlite3.c
    # Aditya Singh
    aditya_singh/database/database.cpp
    aditya_singh/services/auth_service.cpp
    aditya_singh/api/auth_routes.cpp
    # Priyanshu Dutt
    priyanshu_dutt/services/ev_service.cpp
    priyanshu_dutt/api/ev_routes.cpp
    # Mritunjay Thakur
    mritunjay_thakur/services/station_service.cpp
    mritunjay_thakur/api/station_routes.cpp
    mritunjay_thakur/api/charging_routes.cpp
    # Amit Singh Kaintura
    amit_singh_kaintura/services/battery_service.cpp
    amit_singh_kaintura/api/battery_routes.cpp
    amit_singh_kaintura/api/swap_routes.cpp
    # Abhishek Ghansala
    abhishek_ghansala/dsa/charging_queue.cpp
    abhishek_ghansala/dsa/swap_queue.cpp
    abhishek_ghansala/dsa/transaction_stack.cpp
    abhishek_ghansala/api/dsa_routes.cpp
    # Agreev Singh
    agreev_singh/services/transaction_service.cpp
    agreev_singh/api/transaction_routes.cpp
    agreev_singh/api/analytics_routes.cpp
)

target_include_directories(smart_ev_hub_backend PRIVATE ${COMMON_INCLUDES})
target_link_libraries(smart_ev_hub_backend PRIVATE Threads::Threads ${PLATFORM_LIBS})

# -----------------------------------------------------------------------------
# 2. Individual Member Standalone Test Targets
# -----------------------------------------------------------------------------

# Test 1: Aditya Singh (Database + Auth)
add_executable(test_aditya_singh
    aditya_singh/tests/test_aditya.cpp
    aditya_singh/database/database.cpp
    aditya_singh/services/auth_service.cpp
    src/sqlite3.c
)
target_include_directories(test_aditya_singh PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_aditya_singh PRIVATE Threads::Threads ${PLATFORM_LIBS})

# Test 2: Priyanshu Dutt (EV Management)
add_executable(test_priyanshu_dutt
    priyanshu_dutt/tests/test_priyanshu.cpp
    priyanshu_dutt/services/ev_service.cpp
    aditya_singh/database/database.cpp
    src/sqlite3.c
)
target_include_directories(test_priyanshu_dutt PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_priyanshu_dutt PRIVATE Threads::Threads ${PLATFORM_LIBS})

# Test 3: Mritunjay Thakur (Charging Stations)
add_executable(test_mritunjay_thakur
    mritunjay_thakur/tests/test_mritunjay.cpp
    mritunjay_thakur/services/station_service.cpp
    aditya_singh/database/database.cpp
    src/sqlite3.c
)
target_include_directories(test_mritunjay_thakur PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_mritunjay_thakur PRIVATE Threads::Threads ${PLATFORM_LIBS})

# Test 4: Amit Singh Kaintura (Battery + Battery Swap)
add_executable(test_amit_singh_kaintura
    amit_singh_kaintura/tests/test_amit.cpp
    amit_singh_kaintura/services/battery_service.cpp
    aditya_singh/database/database.cpp
    src/sqlite3.c
)
target_include_directories(test_amit_singh_kaintura PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_amit_singh_kaintura PRIVATE Threads::Threads ${PLATFORM_LIBS})

# Test 5: Abhishek Ghansala (UI + DSA)
add_executable(test_abhishek_ghansala
    abhishek_ghansala/tests/test_abhishek.cpp
    abhishek_ghansala/dsa/charging_queue.cpp
    abhishek_ghansala/dsa/swap_queue.cpp
    abhishek_ghansala/dsa/transaction_stack.cpp
)
target_include_directories(test_abhishek_ghansala PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_abhishek_ghansala PRIVATE Threads::Threads ${PLATFORM_LIBS})

# Test 6: Agreev Singh (Transactions + Analytics)
add_executable(test_agreev_singh
    agreev_singh/tests/test_agreev.cpp
    agreev_singh/services/transaction_service.cpp
    aditya_singh/database/database.cpp
    src/sqlite3.c
)
target_include_directories(test_agreev_singh PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_agreev_singh PRIVATE Threads::Threads ${PLATFORM_LIBS})