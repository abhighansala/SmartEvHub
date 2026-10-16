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

    std::cout << "\n========================================================\n";
    std::cout << " >>> ALL TESTS PASSED FOR AGREEV SINGH (ROLE 6)! <<<\n";
    std::cout << "========================================================\n";
    return 0;
}
