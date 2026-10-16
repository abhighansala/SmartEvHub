#ifndef TRANSACTION_SERVICE_H
#define TRANSACTION_SERVICE_H

#include "../models/transaction.h"
#include "../../aditya_singh/database/database.h"
#include <string>

// Maximum number of transactions handled in the fixed-array in-memory buffer
const int MAX_TRANSACTIONS = 100;

// ============================================================================
// Smart EV Hub - Transaction & Reporting Analytics Service
// Module Owner: Agreev Singh (Transactions + Reports + Analytics, ID: 2518161)
// ----------------------------------------------------------------------------
// Academic Syllabus Connections:
// 1. Fixed Arrays & Sequential Traversal (Data Structures Week 1):
//    Standard C++ fixed-size arrays without using std::vector.
// 2. Linear Search Algorithm (Data Structures Week 1 & 2):
//    O(N) search to locate transactions by ID or filter by vehicle/type.
// 3. Extremum / Best Index Calculation (OOP Lab 4 & DS Week 1 Problem II):
//    Linear scan to determine the highest revenue transaction and peak metrics.
// 4. Persistence Integration:
//    Synchronizes with SQLite (evhub.db) with seamless array fallback.
// ============================================================================

class TransactionService {
private:
    Database* db;                                 // Pointer to unified SQLite database
    Transaction transactionArray[MAX_TRANSACTIONS]; // In-memory fixed array of transactions
    int transactionCount;                         // Number of active records in memory

public:
    // Constructor initializes database pointer and loads default seed transactions
    TransactionService(Database* database = nullptr);

    // ------------------------------------------------------------------------
    // Core Retrieval & Persistence Methods
    // ------------------------------------------------------------------------

    // Retrieves all transactions into outList array, returning the total count
    int getAllTransactions(Transaction outList[], int maxCapacity);

    // Finds a transaction by unique ID using linear search; returns true if found
    bool getTransactionById(const std::string& id, Transaction& outTxn);

    // Records a completed charging or swapping transaction
    bool addTransaction(const Transaction& txn);

    // ------------------------------------------------------------------------
    // Filtering Queries (Data Structures Linear Filter)
    // ------------------------------------------------------------------------

    // Filters transactions by operation type ("charging" vs "battery-swap")
    int getTransactionsByType(const std::string& type, Transaction outList[], int maxCapacity);

    // Filters transactions for a specific EV vehicle ID (e.g. "EV-103")
    int getTransactionsByVehicleId(const std::string& vehicleId, Transaction outList[], int maxCapacity);

    // ------------------------------------------------------------------------
    // KPI & Operational Analytics Calculations (For Reports & Dashboard)
    // ------------------------------------------------------------------------

    // Computes aggregate revenue across all completed transactions (in INR)
    double getTotalRevenue();

    // Computes aggregate electrical energy delivered (in kWh)
    double getTotalEnergy();

    // Returns total number of completed sessions
    int getTotalSessions();

    // Returns count of charging sessions
    int getChargingSessionCount();

    // Returns count of battery swapping sessions
    int getBatterySwapCount();

    // Calculates average session duration in minutes
    double getAverageDurationMinutes();

    // Returns simulated/calculated average queue waiting time in minutes
    double getAverageWaitingMinutes();

    // Returns station bays utilization percentage
    double getStationUtilization();

    // Linear search to find the transaction with the highest revenue
    bool getHighestRevenueTransaction(Transaction& outTxn);

    // Helper to generate the next unique Transaction ID string
    std::string generateNextId();

    // Returns current in-memory count
    int getCount() const { return transactionCount; }
};

#endif // TRANSACTION_SERVICE_H
