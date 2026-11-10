#include <iostream>
#include <cassert>
#include <cmath>
#include "../models/transaction.h"
#include "../services/transaction_service.h"

// ============================================================================
// Comprehensive Standalone Unit Test Suite
// Module Owner: Agreev Singh (Transactions + Reports + Analytics, ID: 2518161)
// ----------------------------------------------------------------------------
// Tests:
// 1. Transaction Domain Model (Constructors, Encapsulation, Validation, Tariff, Operators)
// 2. TransactionService (Array Storage, Linear Search, Filtering, DB Fallback)
// 3. KPI & Analytics Calculations (Revenue, Energy, Sessions, Averages, Max Extrema)
// ============================================================================

int main() {
    std::cout << "========================================================\n";
    std::cout << " Running Unit Tests: Agreev Singh (Transactions & Analytics)\n";
    std::cout << " Student ID: 2518161 | Section: F\n";
    std::cout << "========================================================\n\n";

    // ------------------------------------------------------------------------
    // TEST SECTION 1: Transaction Domain Model & OOP Concepts
    // ------------------------------------------------------------------------
    std::cout << ">>> [TEST 1] Testing Transaction Domain Model...\n";

    // A. Default Constructor
    Transaction defaultTxn;
    assert(defaultTxn.getId() == "");
    assert(defaultTxn.getAmount() == 0.0);
    assert(defaultTxn.getType() == "charging");
    assert(defaultTxn.getStatus() == "completed");
    std::cout << "  ✓ Default constructor sets sensible defaults.\n";

    // B. Parameterized Constructor
    Transaction txn1("TXN-TEST-01", "EV-101", "charging", "CS-1",
                     "Fast DC charge 20% to 80%", 540.0, 30.0, 45,
                     "2026-10-15 10:00:00", "completed");
    assert(txn1.getId() == "TXN-TEST-01");
    assert(txn1.getVehicleId() == "EV-101");
    assert(txn1.getType() == "charging");
    assert(txn1.getStationOrBattery() == "CS-1");
    assert(txn1.getAmount() == 540.0);
    assert(txn1.getEnergyDelivered() == 30.0);
    assert(txn1.getDuration() == 45);
    assert(txn1.getStatus() == "completed");
    std::cout << "  ✓ Parameterized constructor properly initializes all fields.\n";

    // C. Validation Logic (isValid)
    assert(txn1.isValid() == true);

    Transaction invalidTxn("", "EV-101", "charging", "CS-1", "Invalid empty ID", 100.0);
    assert(invalidTxn.isValid() == false);

    Transaction invalidTypeTxn("TXN-INV", "EV-101", "invalid_type", "CS-1", "Wrong type", 100.0);
    assert(invalidTypeTxn.isValid() == false);
    std::cout << "  ✓ Transaction validation (isValid) correctly checks required constraints.\n";

    // D. Tariff & Billing Calculation (OOP Lab 1 & 10)
    double chargeTariff = Transaction::calculateTariff(25.0, "charging");
    assert(chargeTariff == 25.0 * 18.0); // ₹450

    double swapTariff = Transaction::calculateTariff(50.0, "battery-swap");
    assert(swapTariff == 300.0 + (50.0 * 5.0)); // ₹550
    std::cout << "  ✓ Tariff calculation correctly computes charging & swap billing.\n";

    // E. Operator Overloading (OOP Lab 14)
    Transaction txn1Duplicate("TXN-TEST-01", "EV-999", "charging", "CS-2", "Diff obj same ID", 900.0);
    Transaction txn2("TXN-TEST-02", "EV-102", "battery-swap", "BAT-01", "Battery swap", 400.0);
    assert(txn1 == txn1Duplicate);
    assert(txn1 != txn2);
    std::cout << "  ✓ Operator overloading (== and !=) matches transactions by ID.\n";

    // ------------------------------------------------------------------------
    // TEST SECTION 2: TransactionService Fixed Array Operations
    // ------------------------------------------------------------------------
    std::cout << "\n>>> [TEST 2] Testing TransactionService & Array Traversal...\n";

    TransactionService service(nullptr); // Using in-memory fallback array

    // Initial seed check
    int initialCount = service.getTotalSessions();
    assert(initialCount == 4);
    std::cout << "  ✓ Default seed data loaded (" << initialCount << " transactions).\n";

    // Linear Search for Transaction by ID (Data Structures Week 1)
    Transaction found;
    bool foundSuccess = service.getTransactionById("TXN-5001", found);
    assert(foundSuccess == true);
    assert(found.getVehicleId() == "EV-103");
    assert(found.getAmount() == 870.0);

    bool notFound = service.getTransactionById("TXN-DOES-NOT-EXIST", found);
    assert(notFound == false);
    std::cout << "  ✓ Linear search by ID correctly locates existing records and handles misses.\n";

    // Add new transaction
    double revenueBefore = service.getTotalRevenue();
    double energyBefore  = service.getTotalEnergy();

    bool addSuccess = service.addTransaction(txn1);
    assert(addSuccess == true);
    assert(service.getTotalSessions() == initialCount + 1);
    assert(std::abs(service.getTotalRevenue() - (revenueBefore + 540.0)) < 0.001);
    assert(std::abs(service.getTotalEnergy() - (energyBefore + 30.0)) < 0.001);
    std::cout << "  ✓ Adding transaction updates session count, total revenue, and energy.\n";

    // ------------------------------------------------------------------------
    // TEST SECTION 3: Filtering & Query Operations (Data Structures)
    // ------------------------------------------------------------------------
    std::cout << "\n>>> [TEST 3] Testing Transaction Filtering...\n";

    Transaction chargingList[MAX_TRANSACTIONS];
    int chargingCount = service.getTransactionsByType("charging", chargingList, MAX_TRANSACTIONS);
    assert(chargingCount == 4); // 3 from seed + txn1
    for (int i = 0; i < chargingCount; i++) {
        assert(chargingList[i].getType() == "charging");
    }

    Transaction swapList[MAX_TRANSACTIONS];
    int swapCount = service.getTransactionsByType("battery-swap", swapList, MAX_TRANSACTIONS);
    assert(swapCount == 1); // TXN-5002
    assert(swapList[0].getId() == "TXN-5002");
    std::cout << "  ✓ Filtering by transaction type ('charging' and 'battery-swap') verified.\n";

    Transaction evList[MAX_TRANSACTIONS];
    int evCount = service.getTransactionsByVehicleId("EV-103", evList, MAX_TRANSACTIONS);
    assert(evCount == 1);
    assert(evList[0].getId() == "TXN-5001");
    std::cout << "  ✓ Filtering by vehicle ID correctly retrieves vehicle history.\n";

    // ------------------------------------------------------------------------
    // TEST SECTION 4: KPI & Extrema Analytics
    // ------------------------------------------------------------------------
    std::cout << "\n>>> [TEST 4] Testing Operational Analytics & Extrema...\n";

    assert(service.getChargingSessionCount() == 4);
    assert(service.getBatterySwapCount() == 1);
    assert(service.getAverageDurationMinutes() > 0.0);
    assert(service.getAverageWaitingMinutes() == 8.5);
    assert(service.getStationUtilization() == 84.5);

    // Extremum scan: Find highest revenue transaction (OOP Lab 4 Best index)
    Transaction topTxn;
    bool topFound = service.getHighestRevenueTransaction(topTxn);
    assert(topFound == true);
    assert(topTxn.getId() == "TXN-5001"); // 870.0 is the highest amount
    assert(topTxn.getAmount() == 870.0);
    std::cout << "  ✓ Extremum search successfully identified highest revenue transaction.\n";

    // Next ID generation
    std::string nextId = service.generateNextId();
    assert(nextId.rfind("TXN-", 0) == 0);
    std::cout << "  ✓ Automatic ID generator produced: " << nextId << "\n";

    // ------------------------------------------------------------------------
    // Receipt Display Verification (OOP Lab 6 & 7)
    // ------------------------------------------------------------------------
    std::cout << "\n>>> [DEMO] Formatted Receipt Output:\n";
    txn1.displayReceipt();

    // ------------------------------------------------------------------------
    // TEST SECTION 5: Real SQLite Persistence & Foreign-Key Enforcement
    // ------------------------------------------------------------------------
    std::cout << "\n>>> [TEST 5] Testing Real SQLite Persistence & Foreign Keys...\n";
    std::string dbFile = "test_agreev.db";
    std::remove(dbFile.c_str());

    Database db(dbFile);
    assert(db.open());

    bool schemaLoaded = db.initSchema("../database/schema.sql") ||
                        db.initSchema("backend/database/schema.sql") ||
                        db.initSchema("database/schema.sql");
    assert(schemaLoaded && "Failed to load database schema for TransactionService!");
    std::cout << "  ✓ Database connected and schema initialized.\n";

    TransactionService dbService(&db);
    Transaction dbTxns[MAX_TRANSACTIONS];
    int seedCount = dbService.getAllTransactions(dbTxns, MAX_TRANSACTIONS);
    assert(seedCount == 3); // Seed has TXN-5001, TXN-5002, TXN-5003
    std::cout << "  ✓ Loaded " << seedCount << " seed transactions from SQLite.\n";

    // Verify revenue calculation directly from SQLite
    double dbRevenue = dbService.getTotalRevenue();
    assert(std::abs(dbRevenue - 2000.0) < 0.001); // 870 + 450 + 680 = 2000
    std::cout << "  ✓ Real SQLite revenue computed: INR " << dbRevenue << "\n";

    // Add new transaction into SQLite (EV-101 exists in EV table)
    Transaction realTxn("TXN-5004", "EV-101", "charging", "CS-1",
                        "Standard fast charge", 520.0, 28.0, 45,
                        "2026-10-15 09:15:00", "completed");
    assert(dbService.addTransaction(realTxn));
    assert(dbService.getTotalSessions() == 4);
    assert(std::abs(dbService.getTotalRevenue() - 2520.0) < 0.001);
    std::cout << "  ✓ Transaction added to SQLite, new total revenue: INR 2520\n";

    // ------------------------------------------------------------------------
    // TEST SECTION 6: Foreign Key Constraint Enforcement
    // ------------------------------------------------------------------------
    std::cout << "\n>>> [TEST 6] Testing Foreign Key Enforcement (PRAGMA foreign_keys = ON)...\n";
    // Attempting to insert a transaction referencing a non-existent EV must fail FK check
    Transaction badFkTxn("TXN-BAD-FK", "EV-GHOST-999", "charging", "CS-1",
                         "Invalid vehicle reference", 100.0, 10.0, 15,
                         "2026-10-15 12:00:00", "completed");
    bool badFkResult = dbService.addTransaction(badFkTxn);
    assert(badFkResult == false);
    std::cout << "  ✓ FOREIGN KEY ENFORCEMENT VERIFIED: Non-existent vehicle was rejected by SQLite.\n";

    // ------------------------------------------------------------------------
    // TEST SECTION 7: Persistence across Connection Close / Reopen
    // ------------------------------------------------------------------------
    std::cout << "\n>>> [TEST 7] Testing Persistence across Connection Close / Reopen...\n";
    db.close();

    Database reopenedDb(dbFile);
    assert(reopenedDb.open());
    TransactionService reopenedService(&reopenedDb);

    Transaction persistedTxn;
    assert(reopenedService.getTransactionById("TXN-5004", persistedTxn));
    assert(persistedTxn.getVehicleId() == "EV-101");
    assert(persistedTxn.getAmount() == 520.0);
    assert(reopenedService.getTotalSessions() == 4);
    std::cout << "  ✓ PERSISTENCE VERIFIED: 4 transactions survived connection restart!\n";

    reopenedDb.close();
    std::remove(dbFile.c_str());

    std::cout << "\n========================================================\n";
    std::cout << " >>> ALL 7 TEST SUITES PASSED FOR AGREEV SINGH (ROLE 6)! <<<\n";
    std::cout << "========================================================\n";
    return 0;
}
