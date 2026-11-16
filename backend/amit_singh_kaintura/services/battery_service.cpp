#include "battery_service.h"
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <vector>

// ============================================================================
// Smart EV Hub - Battery Service Implementation
// Module Owner: Amit Singh Kaintura (B.Tech 2nd Year, Section G, Student ID: 2518188)
// Role: Battery Inventory & Battery Swapping Module
// ============================================================================

BatteryService::BatteryService(Database* database) {
    this->db = database;
    this->batteryCount = 0;
    this->m_chargingPower = 25.0; // 25 kW standard dock charging rate
    this->m_speedMultiplier = 20.0; // 20x demonstration acceleration

    const char* envPower = std::getenv("BATTERY_CHARGING_POWER");
    if (envPower && std::string(envPower).length() > 0) {
        try { m_chargingPower = std::stod(envPower); } catch (...) {}
    }

    const char* envMult = std::getenv("CHARGING_SIMULATION_SPEED_MULTIPLIER");
    if (envMult && std::string(envMult).length() > 0) {
        try { m_speedMultiplier = std::stod(envMult); } catch (...) {}
    }

    // Initialize default local battery inventory
    seedDefaultBatteries();

    // Ensure SQLite schema columns exist and reconcile multi-battery anomalies
    ensureSchemaAndReconcile();
}

void BatteryService::seedDefaultBatteries() {
    batteryCount = 0;

    // Initial 6 batteries matching the EV Hub fleet specs
    batteryArray[0] = Battery("BAT-01", 50.0, 100.0, "available", 98.5, 42);
    batteryArray[1] = Battery("BAT-02", 50.0, 100.0, "available", 97.0, 58);
    batteryArray[2] = Battery("BAT-03", 60.0, 100.0, "available", 99.2, 18);
    batteryArray[3] = Battery("BAT-04", 50.0, 44.0,  "charging",  95.0, 89, "", "", 20.0);
    batteryArray[4] = Battery("BAT-05", 40.0, 15.0,  "charging",  92.4, 120, "", "", 10.0);
    batteryArray[5] = Battery("BAT-06", 70.0, 88.0,  "inuse",     96.8, 65, "EV-103");
    batteryCount = 6;
}

void BatteryService::ensureSchemaAndReconcile() {
    if (db == nullptr || !db->isOpen()) return;
    sqlite3* handle = db->getDbHandle();

    // 1. Add missing schema columns if not present
    sqlite3_exec(handle, "ALTER TABLE Battery ADD COLUMN charging_start DATETIME DEFAULT NULL;", nullptr, nullptr, nullptr);
    sqlite3_exec(handle, "ALTER TABLE Battery ADD COLUMN initial_charge REAL DEFAULT 20.0;", nullptr, nullptr, nullptr);

    // 2. Reconcile inventory
    reconcileInventory();
}

bool BatteryService::reconcileInventory() {
    if (db == nullptr || !db->isOpen()) return false;
    sqlite3* handle = db->getDbHandle();

    // Find any vehicles that have more than one battery assigned
    std::string sqlFindDup = 
        "SELECT current_vehicle, COUNT(*) FROM Battery "
        "WHERE current_vehicle IS NOT NULL AND current_vehicle != '' "
        "GROUP BY current_vehicle HAVING COUNT(*) > 1;";
    sqlite3_stmt* stmt = nullptr;
    std::vector<std::string> dupVehicles;
    if (sqlite3_prepare_v2(handle, sqlFindDup.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* vText = (const char*)sqlite3_column_text(stmt, 0);
            if (vText) dupVehicles.push_back(vText);
        }
        sqlite3_finalize(stmt);
    }

    for (const auto& vId : dupVehicles) {
        // Query all batteries assigned to this vehicle ordered by last_updated DESC
        std::string sqlBats = "SELECT id, charge_level FROM Battery WHERE current_vehicle = ? ORDER BY last_updated DESC, rowid DESC;";
        sqlite3_stmt* bStmt = nullptr;
        if (sqlite3_prepare_v2(handle, sqlBats.c_str(), -1, &bStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(bStmt, 1, vId.c_str(), -1, SQLITE_STATIC);
            bool isFirst = true;
            std::vector<std::pair<std::string, double>> toDetach;
            while (sqlite3_step(bStmt) == SQLITE_ROW) {
                const char* bId = (const char*)sqlite3_column_text(bStmt, 0);
                double chg = sqlite3_column_double(bStmt, 1);
                if (isFirst) {
                    // Keep the first (most recent) battery assigned to this vehicle
                    isFirst = false;
                } else if (bId) {
                    toDetach.push_back({bId, chg});
                }
            }
            sqlite3_finalize(bStmt);

            // Detach extra batteries and set to charging or available
            for (const auto& item : toDetach) {
                std::string updSql;
                if (item.second >= 95.0) {
                    updSql = "UPDATE Battery SET current_vehicle = NULL, status = 'available', charging_start = NULL, initial_charge = NULL WHERE id = ?;";
                } else {
                    updSql = "UPDATE Battery SET current_vehicle = NULL, status = 'charging', charging_start = datetime('now'), initial_charge = ? WHERE id = ?;";
                }
                sqlite3_stmt* uStmt = nullptr;
                if (sqlite3_prepare_v2(handle, updSql.c_str(), -1, &uStmt, nullptr) == SQLITE_OK) {
                    if (item.second < 95.0) {
                        sqlite3_bind_double(uStmt, 1, item.second);
                        sqlite3_bind_text(uStmt, 2, item.first.c_str(), -1, SQLITE_STATIC);
                    } else {
                        sqlite3_bind_text(uStmt, 1, item.first.c_str(), -1, SQLITE_STATIC);
                    }
                    sqlite3_step(uStmt);
                    sqlite3_finalize(uStmt);
                }
            }
        }
    }

    // 3. Ensure any charging battery without charging_start has it initialized
    std::string sqlInitCharging = 
        "UPDATE Battery SET charging_start = COALESCE(last_updated, datetime('now')), "
        "initial_charge = charge_level "
        "WHERE status = 'charging' AND (charging_start IS NULL OR charging_start = '');";
    sqlite3_exec(handle, sqlInitCharging.c_str(), nullptr, nullptr, nullptr);

    return true;
}

int BatteryService::advanceChargingBatteries(SwapQueue* swapQueue, TransactionStack* txnStack) {
    if (db == nullptr || !db->isOpen()) return 0;
    sqlite3* handle = db->getDbHandle();

    struct ChargingBat {
        std::string id;
        double capacity;
        double chargeLevel;
        double initialCharge;
        int elapsedSeconds;
    };

    std::vector<ChargingBat> chargingList;
    std::string sql = 
        "SELECT id, capacity, charge_level, initial_charge, "
        "CAST((strftime('%s', 'now') - strftime('%s', charging_start)) AS INTEGER) "
        "FROM Battery WHERE status = 'charging' AND charging_start IS NOT NULL;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ChargingBat b;
            const char* idText = (const char*)sqlite3_column_text(stmt, 0);
            b.id = idText ? idText : "";
            b.capacity = sqlite3_column_double(stmt, 1);
            b.chargeLevel = sqlite3_column_double(stmt, 2);
            b.initialCharge = sqlite3_column_double(stmt, 3);
            b.elapsedSeconds = sqlite3_column_int(stmt, 4);
            if (b.elapsedSeconds < 0) b.elapsedSeconds = 0;
            chargingList.push_back(b);
        }
        sqlite3_finalize(stmt);
    }

    int completedCount = 0;
    for (const auto& b : chargingList) {
        double simElapsedSeconds = b.elapsedSeconds * m_speedMultiplier;
        double simElapsedHours = simElapsedSeconds / 3600.0;
        double energyDelivered = m_chargingPower * simElapsedHours;
        double addedPercentage = (b.capacity > 0.0) ? ((energyDelivered / b.capacity) * 100.0) : 0.0;
        double currentCharge = b.initialCharge + addedPercentage;
        if (currentCharge > 100.0) currentCharge = 100.0;

        if (currentCharge >= 95.0) {
            // Target swap charge reached! Transition to available!
            std::string upd = 
                "UPDATE Battery SET status = 'available', charge_level = 100.0, "
                "charging_start = NULL, initial_charge = NULL, cycle_count = cycle_count + 1, "
                "last_updated = datetime('now') WHERE id = ?;";
            sqlite3_stmt* uStmt = nullptr;
            if (sqlite3_prepare_v2(handle, upd.c_str(), -1, &uStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(uStmt, 1, b.id.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(uStmt);
                sqlite3_finalize(uStmt);
                completedCount++;
            }

            // If vehicles are waiting in the Swap Queue, auto-dispatch!
            if (swapQueue != nullptr && !swapQueue->isEmpty()) {
                Battery outGiven;
                Transaction outTxn;
                dispatchNextSwapIfAvailable(*swapQueue, outGiven, outTxn, txnStack);
            }
        } else {
            // Persist progressive charge level in SQLite
            std::string upd = "UPDATE Battery SET charge_level = ?, last_updated = datetime('now') WHERE id = ?;";
            sqlite3_stmt* uStmt = nullptr;
            if (sqlite3_prepare_v2(handle, upd.c_str(), -1, &uStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_double(uStmt, 1, currentCharge);
                sqlite3_bind_text(uStmt, 2, b.id.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(uStmt);
                sqlite3_finalize(uStmt);
            }
        }
    }

    return completedCount;
}

// ----------------------------------------------------------------------------
// 1. Retrieve all batteries from Database or local array (TCS 302 Linear Scan)
// ----------------------------------------------------------------------------
int BatteryService::getAllBatteries(Battery outList[], int maxCapacity) {
    int count = 0;

    // Primary: Load fresh records directly from SQLite database if connected
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id, capacity, charge_level, status, health, cycle_count, current_vehicle, charging_start, initial_charge FROM Battery;";
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

                const char* chgStartText = (const char*)sqlite3_column_text(stmt, 7);
                std::string chgStart = chgStartText ? chgStartText : "";
                double initChg = sqlite3_column_double(stmt, 8);

                outList[count] = Battery(id, cap, charge, status, health, cycles, vehicle, chgStart, initChg);
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
        if (all[i].isReady() && all[i].getCurrentVehicle().empty()) {
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
        if (all[i].isReady() && all[i].getCurrentVehicle().empty()) {
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
            if (all[i].isReady() && all[i].getCurrentVehicle().empty()) {
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
    return performSwap(evId, "", 0.0, outGivenBattery, outTxn, 0, "admin");
}

// ----------------------------------------------------------------------------
// 7. Full Battery Swap Workflow (Atomic SQLite Transaction with Rollback Safety)
// ----------------------------------------------------------------------------
bool BatteryService::performSwap(const std::string& evId, const std::string& oldBatteryId,
                                 double requiredCapacity, Battery& outGivenBattery, Transaction* outTxn,
                                 int callerUserId, const std::string& callerRole) {
    if (evId.empty()) return false;

    // 1. Vehicle existence, ownership and status verification
    double evCurrentCharge = 20.0;
    double evCapacity = 50.0;
    std::string evStatus = "available";
    int evUserId = 0;

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        sqlite3_stmt* evStmt = nullptr;
        std::string evSql = "SELECT current_charge, battery_capacity, status, user_id FROM EV WHERE id = ?;";
        if (sqlite3_prepare_v2(handle, evSql.c_str(), -1, &evStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(evStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(evStmt) == SQLITE_ROW) {
                evCurrentCharge = sqlite3_column_double(evStmt, 0);
                evCapacity = sqlite3_column_double(evStmt, 1);
                const char* st = (const char*)sqlite3_column_text(evStmt, 2);
                if (st) evStatus = st;
                evUserId = sqlite3_column_int(evStmt, 3);
            } else {
                sqlite3_finalize(evStmt);
                std::cerr << "[Battery Service] EV not found: " << evId << "\n";
                return false;
            }
            sqlite3_finalize(evStmt);
        }

        // Ownership enforcement for normal user accounts
        if (callerRole == "user" && callerUserId > 0) {
            if (evUserId > 0 && evUserId != callerUserId) {
                std::cerr << "[Battery Service] Ownership denied: User " << callerUserId 
                          << " cannot swap EV " << evId << " (owned by " << evUserId << ")\n";
                return false;
            }
        }

        // Cannot swap an EV actively charging at a charging station
        if (evStatus == "charging") {
            std::cerr << "[Battery Service] Vehicle " << evId << " is currently charging at a station.\n";
            return false;
        }
    }

    // 2. Automatically identify installed battery if not provided
    std::string effectiveOldBatteryId = oldBatteryId;
    if (effectiveOldBatteryId.empty() && db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        sqlite3_stmt* bStmt = nullptr;
        std::string bSql = "SELECT id, charge_level FROM Battery WHERE current_vehicle = ?;";
        if (sqlite3_prepare_v2(handle, bSql.c_str(), -1, &bStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(bStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(bStmt) == SQLITE_ROW) {
                const char* bId = (const char*)sqlite3_column_text(bStmt, 0);
                if (bId) effectiveOldBatteryId = bId;
            }
            sqlite3_finalize(bStmt);
        }
    }

    // 3. Find a ready replacement battery matching requested specifications
    double desiredCapacity = (requiredCapacity > 0.0 ? requiredCapacity : evCapacity);
    bool found = false;
    if (desiredCapacity > 0.0) {
        found = findReadyBattery(desiredCapacity, outGivenBattery);
    } else {
        found = findReadyBattery(outGivenBattery);
    }

    if (!found) {
        std::cout << "[Battery Service] No ready battery available in inventory for EV: " << evId << "\n";
        return false;
    }

    // Ensure the replacement battery is not currently assigned to another vehicle
    if (!outGivenBattery.getCurrentVehicle().empty() && outGivenBattery.getCurrentVehicle() != evId) {
        std::cerr << "[Battery Service] Replacement battery " << outGivenBattery.getId() 
                  << " is already assigned to " << outGivenBattery.getCurrentVehicle() << "\n";
        return false;
    }

    std::string newBatteryId = outGivenBattery.getId();
    static int swapTxCounter = 7000;
    std::string txnId = "TXN-SWAP-" + std::to_string(swapTxCounter++) + "-" + std::to_string(time(nullptr) % 10000);
    double swapAmount = Transaction::calculateTariff(outGivenBattery.getCapacity(), "battery-swap");
    double energyDelivered = outGivenBattery.getCapacity() * ((outGivenBattery.getChargeLevel() - evCurrentCharge) / 100.0);
    if (energyDelivered <= 0.0) energyDelivered = outGivenBattery.getCapacity() * 0.8;
    int duration = 5; // standard swap time in minutes
    std::string desc = "Battery swap: fresh " + newBatteryId + " (" + std::to_string((int)outGivenBattery.getCapacity()) + " kWh)";

    // 4. Database updates if SQLite is active (Atomic Transaction with Rollback)
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        if (!db->beginTransaction()) {
            return false;
        }

        // 4a. Detach any previous battery assigned to this vehicle
        std::string sqlDetach = "UPDATE Battery SET current_vehicle = NULL WHERE current_vehicle = ?;";
        sqlite3_stmt* dStmt = nullptr;
        if (sqlite3_prepare_v2(handle, sqlDetach.c_str(), -1, &dStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(dStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(dStmt);
            sqlite3_finalize(dStmt);
        }

        // 4b. Mark newly allocated battery as 'inuse' and assign vehicle ID
        bool okNew = false;
        std::string sqlNew = "UPDATE Battery SET status = 'inuse', current_vehicle = ?, "
                             "charging_start = NULL, initial_charge = NULL, last_updated = datetime('now') WHERE id = ?;";
        sqlite3_stmt* stmtNew = nullptr;
        if (sqlite3_prepare_v2(handle, sqlNew.c_str(), -1, &stmtNew, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmtNew, 1, evId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmtNew, 2, newBatteryId.c_str(), -1, SQLITE_STATIC);
            int rc = sqlite3_step(stmtNew);
            sqlite3_finalize(stmtNew);
            if (rc == SQLITE_DONE) okNew = true;
        }

        if (!okNew) {
            db->rollbackTransaction();
            return false;
        }

        // 4c. If old battery ID was specified or detected, place it into the charging dock or available
        bool okOld = true;
        if (!effectiveOldBatteryId.empty()) {
            okOld = false;
            std::string sqlOld;
            double removedCharge = evCurrentCharge;
            if (removedCharge < 95.0) {
                sqlOld = "UPDATE Battery SET status = 'charging', charge_level = ?, current_vehicle = NULL, "
                         "charging_start = datetime('now'), initial_charge = ?, last_updated = datetime('now') WHERE id = ?;";
            } else {
                sqlOld = "UPDATE Battery SET status = 'available', charge_level = ?, current_vehicle = NULL, "
                         "charging_start = NULL, initial_charge = NULL, last_updated = datetime('now') WHERE id = ?;";
            }
            sqlite3_stmt* stmtOld = nullptr;
            if (sqlite3_prepare_v2(handle, sqlOld.c_str(), -1, &stmtOld, nullptr) == SQLITE_OK) {
                sqlite3_bind_double(stmtOld, 1, removedCharge);
                if (removedCharge < 95.0) {
                    sqlite3_bind_double(stmtOld, 2, removedCharge);
                    sqlite3_bind_text(stmtOld, 3, effectiveOldBatteryId.c_str(), -1, SQLITE_STATIC);
                } else {
                    sqlite3_bind_text(stmtOld, 2, effectiveOldBatteryId.c_str(), -1, SQLITE_STATIC);
                }
                int rc = sqlite3_step(stmtOld);
                sqlite3_finalize(stmtOld);
                if (rc == SQLITE_DONE) okOld = true;
            }
        }

        if (!okOld) {
            db->rollbackTransaction();
            return false;
        }

        // 4d. Update vehicle status to available and battery charge to fresh battery's charge level in EV table
        bool okEV = false;
        std::string sqlEV = "UPDATE EV SET current_charge = ?, battery_capacity = ?, status = 'available', "
                            "last_activity = datetime('now') WHERE id = ?;";
        sqlite3_stmt* stmtEV = nullptr;
        if (sqlite3_prepare_v2(handle, sqlEV.c_str(), -1, &stmtEV, nullptr) == SQLITE_OK) {
            sqlite3_bind_double(stmtEV, 1, outGivenBattery.getChargeLevel());
            sqlite3_bind_double(stmtEV, 2, outGivenBattery.getCapacity());
            sqlite3_bind_text(stmtEV, 3, evId.c_str(), -1, SQLITE_STATIC);
            int rc = sqlite3_step(stmtEV);
            sqlite3_finalize(stmtEV);
            if (rc == SQLITE_DONE) okEV = true;
        }

        if (!okEV) {
            db->rollbackTransaction();
            return false;
        }

        // 4e. Insert completed TransactionLog record
        bool okTxn = false;
        std::string sqlTxn = "INSERT INTO TransactionLog (id, vehicle_id, type, station_or_battery, description, "
                             "amount, energy_delivered, duration, timestamp, status, user_id) "
                             "VALUES (?, ?, 'battery-swap', ?, ?, ?, ?, ?, datetime('now'), 'completed', ?);";
        sqlite3_stmt* stmtTxn = nullptr;
        if (sqlite3_prepare_v2(handle, sqlTxn.c_str(), -1, &stmtTxn, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmtTxn, 1, txnId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmtTxn, 2, evId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmtTxn, 3, newBatteryId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmtTxn, 4, desc.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmtTxn, 5, swapAmount);
            sqlite3_bind_double(stmtTxn, 6, energyDelivered);
            sqlite3_bind_int(stmtTxn, 7, duration);
            if (evUserId > 0) {
                sqlite3_bind_int(stmtTxn, 8, evUserId);
            } else if (callerUserId > 0) {
                sqlite3_bind_int(stmtTxn, 8, callerUserId);
            } else {
                sqlite3_bind_null(stmtTxn, 8);
            }
            int rc = sqlite3_step(stmtTxn);
            sqlite3_finalize(stmtTxn);
            if (rc == SQLITE_DONE) okTxn = true;
        }

        if (!okTxn) {
            db->rollbackTransaction();
            std::cerr << "[Battery Service Error] Battery swap transaction failed! Changes rolled back.\n";
            return false;
        }

        db->commitTransaction();
    }

    // Step 5: Synchronize local in-memory array state
    outGivenBattery.setStatus("inuse");
    outGivenBattery.setCurrentVehicle(evId);
    outGivenBattery.setChargingStart("");
    updateBattery(outGivenBattery);

    if (!effectiveOldBatteryId.empty()) {
        Battery oldBat;
        if (getBatteryById(effectiveOldBatteryId, oldBat)) {
            oldBat.setCurrentVehicle("");
            if (evCurrentCharge < 95.0) {
                oldBat.setStatus("charging");
                oldBat.setChargeLevel(evCurrentCharge);
                oldBat.setInitialCharge(evCurrentCharge);
            } else {
                oldBat.setStatus("available");
                oldBat.setChargeLevel(evCurrentCharge);
            }
            updateBattery(oldBat);
        }
    }

    if (outTxn != nullptr) {
        int finalUserId = (evUserId > 0 ? evUserId : callerUserId);
        *outTxn = Transaction(txnId, evId, "battery-swap", newBatteryId, desc, swapAmount, energyDelivered, duration, "", "completed", finalUserId);
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

    double reqCap = 50.0;
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(handle, "SELECT battery_capacity FROM EV WHERE id = ?;", -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(s, 1, dequeued.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(s) == SQLITE_ROW) {
                reqCap = sqlite3_column_double(s, 0);
            }
            sqlite3_finalize(s);
        }
    }

    if (!performSwap(dequeued, "", reqCap, outGivenBattery, &outTxn, 0, "admin")) {
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
        std::string sql = "INSERT OR REPLACE INTO Battery (id, capacity, charge_level, status, health, cycle_count, current_vehicle, charging_start, initial_charge, last_updated) "
                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'));";
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
            if (!b.getChargingStart().empty()) {
                sqlite3_bind_text(stmt, 8, b.getChargingStart().c_str(), -1, SQLITE_STATIC);
            } else {
                sqlite3_bind_null(stmt, 8);
            }
            sqlite3_bind_double(stmt, 9, b.getInitialCharge());
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
    bat.setChargingStart(std::to_string(time(nullptr)));
    bat.setInitialCharge(bat.getChargeLevel());
    return updateBattery(bat);
}
