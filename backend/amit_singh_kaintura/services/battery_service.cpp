#include "battery_service.h"
#include <iostream>
#include <ctime>

// ============================================================================
// Smart EV Hub - Battery Service Implementation
// Module Owner: Amit Singh Kaintura (B.Tech 2nd Year, Section G, Student ID: 2518188)
// Role: Battery Inventory & Battery Swapping Module
// ============================================================================

BatteryService::BatteryService(Database* database) {
    this->db = database;
    this->batteryCount = 0;

    // Initialize default local battery inventory
    seedDefaultBatteries();
}

void BatteryService::seedDefaultBatteries() {
    batteryCount = 0;

    // Initial 6 batteries matching the EV Hub fleet specs
    batteryArray[0] = Battery("BAT-01", 50.0, 100.0, "available", 98.5, 42);
    batteryArray[1] = Battery("BAT-02", 50.0, 100.0, "available", 97.0, 58);
    batteryArray[2] = Battery("BAT-03", 60.0, 100.0, "available", 99.2, 18);
    batteryArray[3] = Battery("BAT-04", 50.0, 44.0,  "charging",  95.0, 89);
    batteryArray[4] = Battery("BAT-05", 40.0, 15.0,  "charging",  92.4, 120);
    batteryArray[5] = Battery("BAT-06", 70.0, 88.0,  "inuse",     96.8, 65, "EV-103");
    batteryCount = 6;
}

// ----------------------------------------------------------------------------
// 1. Retrieve all batteries from Database or local array (TCS 302 Linear Scan)
// ----------------------------------------------------------------------------
int BatteryService::getAllBatteries(Battery outList[], int maxCapacity) {
    int count = 0;

    // Primary: Load fresh records directly from SQLite database if connected
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id, capacity, charge_level, status, health, cycle_count, current_vehicle FROM Battery;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW && count < maxCapacity) {
                const char* idText = (const char*)sqlite3_column_text(stmt, 0);
                std::string id = idText ? idText : "";
                double cap = sqlite3_column_double(stmt, 1);
                double charge = sqlite3_column_double(stmt, 2);
                const char* statusText = (const char*)sqlite3_column_text(stmt, 3);
                std::string status = statusText ? statusText : "available";
                double health = sqlite3_column_double(stmt, 4);
                int cycles = sqlite3_column_int(stmt, 5);

                const char* vehText = (const char*)sqlite3_column_text(stmt, 6);
                std::string vehicle = vehText ? vehText : "";

                outList[count] = Battery(id, cap, charge, status, health, cycles, vehicle);
                count++;
            }
            sqlite3_finalize(stmt);

            // Also synchronize our local array for quick access
            batteryCount = 0;
            for (int i = 0; i < count && i < MAX_BATTERIES; i++) {
                batteryArray[i] = outList[i];
                batteryCount++;
            }

            return count;
        }
    }

    // Fallback: Return from local in-memory array if database is not active
    for (int i = 0; i < batteryCount && i < maxCapacity; i++) {
        outList[i] = batteryArray[i];
        count++;
    }

    return count;
}

// ----------------------------------------------------------------------------
// 2. Linear search by Battery ID (TCS 302 Search Algorithm: O(N) complexity)
// ----------------------------------------------------------------------------
bool BatteryService::getBatteryById(const std::string& id, Battery& outBattery) {
    Battery all[MAX_BATTERIES];
    int count = getAllBatteries(all, MAX_BATTERIES);

    for (int i = 0; i < count; i++) {
        if (all[i].getId() == id) {
            outBattery = all[i];
            return true;
        }
    }
    return false;
}

// ----------------------------------------------------------------------------
// 3. Count ready batteries
// ----------------------------------------------------------------------------
int BatteryService::getReadyCount() {
    Battery all[MAX_BATTERIES];
    int count = getAllBatteries(all, MAX_BATTERIES);
    int ready = 0;

    for (int i = 0; i < count; i++) {
        if (all[i].isReady()) {
            ready++;
        }
    }
    return ready;
}

// ----------------------------------------------------------------------------
// 4. Find first available ready battery (Basic)
// ----------------------------------------------------------------------------
bool BatteryService::findReadyBattery(Battery& outBattery) {
    Battery all[MAX_BATTERIES];
    int count = getAllBatteries(all, MAX_BATTERIES);

    for (int i = 0; i < count; i++) {
        if (all[i].isReady()) {
            outBattery = all[i];
            return true;
        }
    }
    return false;
}

// ----------------------------------------------------------------------------
// 5. Overloaded find: ready battery matching requested capacity (TCS 307: Overloading)
// ----------------------------------------------------------------------------
bool BatteryService::findReadyBattery(double requiredCapacity, Battery& outBattery) {
    Battery all[MAX_BATTERIES];
    int count = getAllBatteries(all, MAX_BATTERIES);

    // First attempt: Exact or close capacity match (within 5 kWh)
    if (requiredCapacity > 0.0) {
        for (int i = 0; i < count; i++) {
            if (all[i].isReady()) {
                double diff = all[i].getCapacity() - requiredCapacity;
                if (diff < 0) diff = -diff;
                if (diff <= 5.0) {
                    outBattery = all[i];
                    return true;
                }
            }
        }
    }

    // Second attempt: Fallback to any ready battery
    return findReadyBattery(outBattery);
}

// ----------------------------------------------------------------------------
// 6. Standard Battery Swap
// ----------------------------------------------------------------------------
bool BatteryService::performSwap(const std::string& evId, Battery& outGivenBattery, Transaction* outTxn) {
    return performSwap(evId, "", 0.0, outGivenBattery, outTxn);
}

// ----------------------------------------------------------------------------
// 7. Full Battery Swap Workflow (Atomic SQLite Transaction with Rollback Safety)
// ----------------------------------------------------------------------------
bool BatteryService::performSwap(const std::string& evId, const std::string& oldBatteryId,
                                 double requiredCapacity, Battery& outGivenBattery, Transaction* outTxn) {
    // Step 1: Find a ready battery matching requested specifications
    bool found = false;
    if (requiredCapacity > 0.0) {
        found = findReadyBattery(requiredCapacity, outGivenBattery);
    } else {
        found = findReadyBattery(outGivenBattery);
    }

    if (!found) {
        std::cout << "[Battery Service] No ready battery available in inventory for EV: " << evId << "\n";
        return false;
    }

    std::string newBatteryId = outGivenBattery.getId();
    static int swapTxCounter = 7000;
    std::string txnId = "TXN-SWAP-" + std::to_string(swapTxCounter++) + "-" + std::to_string(time(nullptr) % 10000);
    double swapAmount = Transaction::calculateTariff(outGivenBattery.getCapacity(), "battery-swap");
    double energyDelivered = outGivenBattery.getCapacity() * 0.8;
    int duration = 5; // standard swap time in minutes
    std::string desc = "Battery swap: fresh " + newBatteryId + " (" + std::to_string((int)outGivenBattery.getCapacity()) + " kWh)";

    // Step 2: Database updates if SQLite is active (Atomic Transaction with Rollback)
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        if (!db->beginTransaction()) {
            return false;
        }

        bool okNew = false;
        // 2a. Mark newly allocated battery as 'inuse' and assign vehicle ID
        std::string sqlNew = "UPDATE Battery SET status = 'inuse', current_vehicle = ? WHERE id = ?;";
        sqlite3_stmt* stmtNew = nullptr;
        if (sqlite3_prepare_v2(handle, sqlNew.c_str(), -1, &stmtNew, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmtNew, 1, evId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmtNew, 2, newBatteryId.c_str(), -1, SQLITE_STATIC);
            int rc = sqlite3_step(stmtNew);
            sqlite3_finalize(stmtNew);
            if (rc == SQLITE_DONE) okNew = true;
        }

        // 2b. If old battery ID was specified, place it into the charging dock
        bool okOld = true;
        if (!oldBatteryId.empty()) {
            okOld = false;
            std::string sqlOld = "UPDATE Battery SET status = 'charging', charge_level = 15.0, current_vehicle = NULL WHERE id = ?;";
            sqlite3_stmt* stmtOld = nullptr;
            if (sqlite3_prepare_v2(handle, sqlOld.c_str(), -1, &stmtOld, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmtOld, 1, oldBatteryId.c_str(), -1, SQLITE_STATIC);
                int rc = sqlite3_step(stmtOld);
                sqlite3_finalize(stmtOld);
                if (rc == SQLITE_DONE) okOld = true;
            }
        }

        // 2c. Update vehicle status to available and battery charge to fresh battery's charge level in EV table
        bool okEV = false;
        std::string sqlEV = "UPDATE EV SET current_charge = ?, status = 'available', last_activity = datetime('now') WHERE id = ?;";
        sqlite3_stmt* stmtEV = nullptr;
        if (sqlite3_prepare_v2(handle, sqlEV.c_str(), -1, &stmtEV, nullptr) == SQLITE_OK) {
            sqlite3_bind_double(stmtEV, 1, outGivenBattery.getChargeLevel());
            sqlite3_bind_text(stmtEV, 2, evId.c_str(), -1, SQLITE_STATIC);
            int rc = sqlite3_step(stmtEV);
            sqlite3_finalize(stmtEV);
            if (rc == SQLITE_DONE) okEV = true;
        }

        // 2d. Insert completed TransactionLog record
        bool okTxn = false;
        std::string sqlTxn = "INSERT INTO TransactionLog (id, vehicle_id, type, station_or_battery, description, amount, energy_delivered, duration, timestamp, status) "
                             "VALUES (?, ?, 'battery-swap', ?, ?, ?, ?, ?, datetime('now'), 'completed');";
        sqlite3_stmt* stmtTxn = nullptr;
        if (sqlite3_prepare_v2(handle, sqlTxn.c_str(), -1, &stmtTxn, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmtTxn, 1, txnId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmtTxn, 2, evId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmtTxn, 3, newBatteryId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmtTxn, 4, desc.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmtTxn, 5, swapAmount);
            sqlite3_bind_double(stmtTxn, 6, energyDelivered);
            sqlite3_bind_int(stmtTxn, 7, duration);
            int rc = sqlite3_step(stmtTxn);
            sqlite3_finalize(stmtTxn);
            if (rc == SQLITE_DONE) okTxn = true;
        }

        if (okNew && okOld && okEV && okTxn) {
            db->commitTransaction();
        } else {
            db->rollbackTransaction();
            std::cerr << "[Battery Service Error] Battery swap transaction failed! Changes rolled back.\n";
            return false;
        }
    }

    // Step 3: Synchronize local in-memory array state
    for (int i = 0; i < batteryCount; i++) {
        if (batteryArray[i].getId() == newBatteryId) {
            batteryArray[i].setStatus("inuse");
            batteryArray[i].setCurrentVehicle(evId);
            outGivenBattery = batteryArray[i];
        }
        if (!oldBatteryId.empty() && batteryArray[i].getId() == oldBatteryId) {
            batteryArray[i].setStatus("charging");
            batteryArray[i].setChargeLevel(15.0);
            batteryArray[i].setCurrentVehicle("");
        }
    }

    if (outTxn != nullptr) {
        *outTxn = Transaction(txnId, evId, "battery-swap", newBatteryId, desc, swapAmount, energyDelivered, duration, "completed");
    }

    std::cout << "[Battery Service] Swap successful: EV " << evId
              << " assigned fresh battery " << newBatteryId << ".\n";
    return true;
}

// ----------------------------------------------------------------------------
// 8. Rehydrate FIFO Swap Queue from SQLite (Deterministic Tie-Breaker)
// ----------------------------------------------------------------------------
int BatteryService::rehydrateSwapQueue(SwapQueue& queue) {
    queue.clear();
    int count = 0;
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        // Deterministic FIFO: last_activity ASC, registered_at ASC, rowid ASC
        std::string sql = "SELECT id FROM EV WHERE status = 'swapping' ORDER BY last_activity ASC, registered_at ASC, rowid ASC;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW && count < SWAP_QUEUE_CAPACITY) {
                const char* idText = (const char*)sqlite3_column_text(stmt, 0);
                if (idText && !queue.isFull()) {
                    queue.enqueue(idText);
                    count++;
                }
            }
            sqlite3_finalize(stmt);
        }
    }
    return count;
}

// ----------------------------------------------------------------------------
// 9. Enqueue EV when no ready battery is available
// ----------------------------------------------------------------------------
bool BatteryService::enqueueWaitingSwap(const std::string& evId, SwapQueue& queue) {
    if (queue.isFull()) {
        return false;
    }
    if (queue.contains(evId)) {
        return false;
    }

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        // Verify EV exists and is not currently charging
        std::string checkSql = "SELECT status FROM EV WHERE id = ?;";
        sqlite3_stmt* checkStmt = nullptr;
        if (sqlite3_prepare_v2(handle, checkSql.c_str(), -1, &checkStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(checkStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(checkStmt) == SQLITE_ROW) {
                const char* statText = (const char*)sqlite3_column_text(checkStmt, 0);
                std::string st = statText ? statText : "";
                sqlite3_finalize(checkStmt);
                if (st == "charging") {
                    return false; // Cannot swap while actively charging
                }
            } else {
                sqlite3_finalize(checkStmt);
                return false; // EV does not exist
            }
        }

        std::string sql = "UPDATE EV SET status = 'swapping', last_activity = datetime('now') WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, evId.c_str(), -1, SQLITE_STATIC);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
                return false;
            }
        } else {
            return false;
        }
    }

    return queue.enqueue(evId);
}

// ----------------------------------------------------------------------------
// 10. Process oldest queued request atomically
// ----------------------------------------------------------------------------
bool BatteryService::processNextSwap(SwapQueue& queue, Battery& outGivenBattery, Transaction& outTxn, TransactionStack* txnStack) {
    if (queue.isEmpty()) {
        return false;
    }

    std::string evId = queue.peek();
    if (evId.empty()) return false;

    // Verify ready battery exists before mutating queue
    Battery checkBat;
    if (!findReadyBattery(checkBat)) {
        std::cerr << "[Battery Service] Inventory exhausted. Cannot process queued EV " << evId << "\n";
        return false;
    }

    // Snapshot queue for rollback safety across memory and SQLite
    SwapQueue queueBackup = queue;
    std::string dequeued = queue.dequeue();

    if (!performSwap(dequeued, "", 0.0, outGivenBattery, &outTxn)) {
        // Roll back in-memory queue so EV is never lost on database failure
        queue = queueBackup;
        std::cerr << "[Battery Service Error] performSwap failed for " << dequeued << ". Restored SwapQueue.\n";
        return false;
    }

    // Push transaction to stack ONLY after commit succeeds
    if (txnStack != nullptr) {
        txnStack->push(outTxn.getId());
    }

    return true;
}

// ----------------------------------------------------------------------------
// 11. Dispatch next swap if battery is available
// ----------------------------------------------------------------------------
bool BatteryService::dispatchNextSwapIfAvailable(SwapQueue& queue, Battery& outGivenBattery, Transaction& outTxn, TransactionStack* txnStack) {
    if (!queue.isEmpty() && getReadyCount() > 0) {
        return processNextSwap(queue, outGivenBattery, outTxn, txnStack);
    }
    return false;
}


// ----------------------------------------------------------------------------
// 8. Add a new battery to inventory
// ----------------------------------------------------------------------------
bool BatteryService::addBattery(const Battery& b) {
    // 1. Persist to SQLite if open
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "INSERT OR REPLACE INTO Battery (id, capacity, charge_level, status, health, cycle_count, current_vehicle) VALUES (?, ?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, b.getId().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 2, b.getCapacity());
            sqlite3_bind_double(stmt, 3, b.getChargeLevel());
            sqlite3_bind_text(stmt, 4, b.getStatus().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 5, b.getHealth());
            sqlite3_bind_int(stmt, 6, b.getCycleCount());
            if (!b.getCurrentVehicle().empty()) {
                sqlite3_bind_text(stmt, 7, b.getCurrentVehicle().c_str(), -1, SQLITE_STATIC);
            } else {
                sqlite3_bind_null(stmt, 7);
            }
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    // 2. Add or update in local array
    for (int i = 0; i < batteryCount; i++) {
        if (batteryArray[i].getId() == b.getId()) {
            batteryArray[i] = b;
            return true;
        }
    }

    if (batteryCount < MAX_BATTERIES) {
        batteryArray[batteryCount] = b;
        batteryCount++;
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
// 9. Update existing battery
// ----------------------------------------------------------------------------
bool BatteryService::updateBattery(const Battery& b) {
    return addBattery(b);
}

// ----------------------------------------------------------------------------
// 10. Mark battery as charging
// ----------------------------------------------------------------------------
bool BatteryService::markBatteryCharging(const std::string& batteryId) {
    Battery bat;
    if (!getBatteryById(batteryId, bat)) {
        return false;
    }

    bat.setStatus("charging");
    bat.setCurrentVehicle("");
    return updateBattery(bat);
}
