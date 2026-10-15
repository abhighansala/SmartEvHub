#include "transaction_service.h"
#include <iostream>

TransactionService::TransactionService(Database* database) {
    db = database;
    transactionCount = 0;

    // Seed local array with default transactions matching React prototype
    transactionArray[0] = Transaction("TXN-5001", "EV-103", "charging", "CS-2", "Fast charging 20% to 80%", 870.0, 43.5, 60, "2026-10-15 14:30:00");
    transactionArray[1] = Transaction("TXN-5002", "EV-105", "battery-swap", "BAT-03", "Swapped depleted battery with BAT-03 (100%)", 450.0, 60.0, 5, "2026-10-15 13:00:00");
    transactionArray[2] = Transaction("TXN-5003", "EV-106", "charging", "CS-3", "Ultra-fast charge 40% to 91%", 680.0, 39.4, 35, "2026-10-15 11:00:00");
    transactionCount = 3;
}

int TransactionService::getAllTransactions(Transaction outList[], int maxCapacity) {
    int count = 0;

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id, vehicle_id, type, station_or_battery, description, amount, energy_delivered, duration, timestamp, status FROM TransactionLog ORDER BY timestamp DESC;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW && count < maxCapacity) {
                std::string id = (const char*)sqlite3_column_text(stmt, 0);
                std::string vId = (const char*)sqlite3_column_text(stmt, 1);
                std::string type = (const char*)sqlite3_column_text(stmt, 2);
                std::string target = (const char*)sqlite3_column_text(stmt, 3);
                
                const char* descText = (const char*)sqlite3_column_text(stmt, 4);
                std::string desc = descText ? descText : "";
                
                double amount = sqlite3_column_double(stmt, 5);
                double energy = sqlite3_column_double(stmt, 6);
                int duration = sqlite3_column_int(stmt, 7);
                
                const char* timeText = (const char*)sqlite3_column_text(stmt, 8);
                std::string timeStr = timeText ? timeText : "";
                
                const char* statText = (const char*)sqlite3_column_text(stmt, 9);
                std::string status = statText ? statText : "completed";

                outList[count] = Transaction(id, vId, type, target, desc, amount, energy, duration, timeStr, status);
                count++;
            }
            sqlite3_finalize(stmt);
            return count;
        }
    }

    // Fallback: copy from array
    for (int i = 0; i < transactionCount && i < maxCapacity; i++) {
        outList[i] = transactionArray[i];
        count++;
    }

    return count;
}

bool TransactionService::getTransactionById(const std::string& id, Transaction& outTxn) {
    Transaction all[MAX_TRANSACTIONS];
    int count = getAllTransactions(all, MAX_TRANSACTIONS);
    for (int i = 0; i < count; i++) {
        if (all[i].getId() == id) {
            outTxn = all[i];
            return true;
        }
    }
    return false;
}

bool TransactionService::addTransaction(const Transaction& txn) {
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "INSERT INTO TransactionLog (id, vehicle_id, type, station_or_battery, description, amount, energy_delivered, duration, status) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, txn.getId().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, txn.getVehicleId().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, txn.getType().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, txn.getStationOrBattery().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 5, txn.getDescription().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 6, txn.getAmount());
            sqlite3_bind_double(stmt, 7, txn.getEnergyDelivered());
            sqlite3_bind_int(stmt, 8, txn.getDuration());
            sqlite3_bind_text(stmt, 9, txn.getStatus().c_str(), -1, SQLITE_STATIC);

            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc == SQLITE_DONE) return true;
        }
    }

    if (transactionCount < MAX_TRANSACTIONS) {
        transactionArray[transactionCount] = txn;
        transactionCount++;
        return true;
    }
    return false;
}

double TransactionService::getTotalRevenue() {
    Transaction all[MAX_TRANSACTIONS];
    int count = getAllTransactions(all, MAX_TRANSACTIONS);
    double total = 0.0;
    for (int i = 0; i < count; i++) {
        total += all[i].getAmount();
    }
    return total;
}

double TransactionService::getTotalEnergy() {
    Transaction all[MAX_TRANSACTIONS];
    int count = getAllTransactions(all, MAX_TRANSACTIONS);
    double total = 0.0;
    for (int i = 0; i < count; i++) {
        total += all[i].getEnergyDelivered();
    }
    return total;
}

int TransactionService::getTotalSessions() {
    Transaction all[MAX_TRANSACTIONS];
    return getAllTransactions(all, MAX_TRANSACTIONS);
}

double TransactionService::getAverageWaitingMinutes() {
    return 8.5; // Standard operational average for EV hub simulation
}
