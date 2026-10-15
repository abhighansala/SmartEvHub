#include <iostream>
#include <cassert>
#include "../models/transaction.h"
#include "../services/transaction_service.h"

// Standalone unit test for Agreev Singh's module
int main() {
    std::cout << "========================================\n";
    std::cout << " Running Tests: Agreev Singh (Transactions & Analytics)\n";
    std::cout << "========================================\n";

    // 1. Test Transaction Model
    Transaction txn1("TXN-TEST", "EV-101", "charging", "CS-1", "Test Charge", 500.0, 25.0, 30);
    assert(txn1.getId() == "TXN-TEST");
    assert(txn1.getAmount() == 500.0);
    assert(txn1.getEnergyDelivered() == 25.0);
    std::cout << "[PASS] Transaction model creation passed.\n";

    // 2. Test TransactionService (using local array)
    TransactionService service(nullptr);

    // Initial default count is 3
    assert(service.getTotalSessions() == 3);
    double initialRevenue = service.getTotalRevenue();
    assert(initialRevenue > 0.0);
    std::cout << "[PASS] Default transaction count and revenue verified.\n";

    // Add new transaction
    bool added = service.addTransaction(txn1);
    assert(added == true);
    assert(service.getTotalSessions() == 4);
    assert(service.getTotalRevenue() == initialRevenue + 500.0);
    std::cout << "[PASS] Add transaction and updated total revenue passed.\n";

    // Test find transaction by ID
    Transaction foundTxn;
    assert(service.getTransactionById("TXN-TEST", foundTxn) == true);
    assert(foundTxn.getVehicleId() == "EV-101");
    std::cout << "[PASS] Retrieve transaction by ID passed.\n";

    std::cout << "\n>>> ALL TESTS PASSED FOR AGREEV SINGH! <<<\n";
    return 0;
}
