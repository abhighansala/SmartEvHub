#ifndef TRANSACTION_SERVICE_H
#define TRANSACTION_SERVICE_H

#include "../models/transaction.h"
#include "../../aditya_singh/database/database.h"
#include <string>

const int MAX_TRANSACTIONS = 100;

// Simple Service to manage Transactions and Reporting Analytics
// Uses simple C++ arrays and loops (no vectors)
// Created by: Agreev Singh (Transactions + Reports + Analytics Owner, ID: 2518161)
class TransactionService {
private:
    Database* db;
    Transaction transactionArray[MAX_TRANSACTIONS];
    int transactionCount;

public:
    TransactionService(Database* database = nullptr);

    // Get all transactions into outList, returns total count
    int getAllTransactions(Transaction outList[], int maxCapacity);

    // Get single transaction by ID
    bool getTransactionById(const std::string& id, Transaction& outTxn);

    // Record a new completed transaction
    bool addTransaction(const Transaction& txn);

    // KPI Summary Calculations for Reports & Analytics
    double getTotalRevenue();
    double getTotalEnergy();
    int getTotalSessions();
    double getAverageWaitingMinutes();
};

#endif
