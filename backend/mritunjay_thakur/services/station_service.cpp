#include "station_service.h"
#include <iostream>
#include <ctime>
#include <algorithm>

static int s_sessionCounter = 1010;

StationService::StationService(Database* database) {
    db = database;
    stationCount = 0;
    sessionCount = 0;

    // Initialize with default demo stations if starting without database
    stationArray[0] = ChargingStation("CS-1", "DC Fast Charger 1", 60.0, "CCS2", "available", "", "Bay A-1");
    stationArray[1] = ChargingStation("CS-2", "DC Fast Charger 2", 60.0, "CCS2", "available", "", "Bay A-2");
    stationArray[2] = ChargingStation("CS-3", "Ultra Fast Charger 3", 120.0, "CCS2", "available", "", "Bay B-1");
    stationArray[3] = ChargingStation("CS-4", "AC Type 2 Charger 4", 22.0, "Type 2", "maintenance", "", "Bay B-2");
    stationCount = 4;

    sessionArray[0] = ChargingSession("SES-1001", "EV-101", "Tata Nexon EV Max", "CS-1", 12.0, 90.0, "2026-10-19 10:00:00", "", 0.0, 14.5, 290.0, "active");
    sessionArray[1] = ChargingSession("SES-1000", "EV-103", "Hyundai Ioniq 5", "CS-2", 20.0, 80.0, "2026-10-19 08:30:00", "2026-10-19 09:30:00", 80.0, 43.5, 870.0, "completed");
    sessionCount = 2;
}

int StationService::getAllStations(ChargingStation outList[], int maxCapacity) {
    int count = 0;

    // Load from database if connected
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id, name, power, connector_type, status, current_vehicle_id, location, session_start FROM ChargingStation;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW && count < maxCapacity) {
                const char* idText = (const char*)sqlite3_column_text(stmt, 0);
                const char* nameText = (const char*)sqlite3_column_text(stmt, 1);
                double power = sqlite3_column_double(stmt, 2);
                const char* connText = (const char*)sqlite3_column_text(stmt, 3);
                const char* statusText = (const char*)sqlite3_column_text(stmt, 4);
                const char* vehText = (const char*)sqlite3_column_text(stmt, 5);
                const char* locText = (const char*)sqlite3_column_text(stmt, 6);
                const char* sesStartText = (const char*)sqlite3_column_text(stmt, 7);

                std::string id = idText ? idText : "";
                std::string name = nameText ? nameText : "";
                std::string conn = connText ? connText : "CCS2";
                std::string status = statusText ? statusText : "available";
                std::string vehicleId = vehText ? vehText : "";
                std::string location = locText ? locText : "Bay 1";
                std::string sessionStart = sesStartText ? sesStartText : "";

                outList[count] = ChargingStation(id, name, power, conn, status, vehicleId, location, sessionStart);
                count++;
            }
            sqlite3_finalize(stmt);
            return count;
        }
    }

    // Fallback: copy from array
    for (int i = 0; i < stationCount && i < maxCapacity; i++) {
        outList[i] = stationArray[i];
        count++;
    }

    return count;
}

bool StationService::getStationById(const std::string& id, ChargingStation& outStation) {
    ChargingStation all[MAX_STATIONS];
    int count = getAllStations(all, MAX_STATIONS);
    for (int i = 0; i < count; i++) {
        if (all[i].getId() == id) {
            outStation = all[i];
            return true;
        }
    }
    return false;
}

bool StationService::updateStationStatus(const std::string& id, const std::string& newStatus) {
    bool dbUpdated = false;
    // Update in database if connected
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE ChargingStation SET status = ? WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, newStatus.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_STATIC);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc == SQLITE_DONE && sqlite3_changes(handle) > 0) {
                dbUpdated = true;
            }
        }
    }

    // Update in local array
    for (int i = 0; i < stationCount; i++) {
        if (stationArray[i].getId() == id) {
            stationArray[i].setStatus(newStatus);
            return true;
        }
    }
    return dbUpdated;
}

bool StationService::assignVehicle(const std::string& stationId, const std::string& vehicleId) {
    bool dbUpdated = false;
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE ChargingStation SET status = 'charging', current_vehicle_id = ?, session_start = datetime('now') WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, vehicleId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, stationId.c_str(), -1, SQLITE_STATIC);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc == SQLITE_DONE && sqlite3_changes(handle) > 0) {
                dbUpdated = true;
            }
        }
    }

    for (int i = 0; i < stationCount; i++) {
        if (stationArray[i].getId() == stationId) {
            stationArray[i].setStatus("charging");
            stationArray[i].setCurrentVehicleId(vehicleId);
            return true;
        }
    }
    return dbUpdated;
}

bool StationService::releaseStation(const std::string& stationId) {
    bool dbUpdated = false;
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE ChargingStation SET status = 'available', current_vehicle_id = NULL, session_start = NULL WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, stationId.c_str(), -1, SQLITE_STATIC);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc == SQLITE_DONE && sqlite3_changes(handle) > 0) {
                dbUpdated = true;
            }
        }
    }

    for (int i = 0; i < stationCount; i++) {
        if (stationArray[i].getId() == stationId) {
            stationArray[i].setStatus("available");
            stationArray[i].setCurrentVehicleId("");
            return true;
        }
    }
    return dbUpdated;
}

bool StationService::findAvailableStation(ChargingStation& outStation) {
    ChargingStation all[MAX_STATIONS];
    int count = getAllStations(all, MAX_STATIONS);
    for (int i = 0; i < count; i++) {
        if (all[i].isAvailable()) {
            outStation = all[i];
            return true;
        }
    }
    return false;
}

bool StationService::addStation(const ChargingStation& station) {
    if (stationCount < MAX_STATIONS) {
        stationArray[stationCount] = station;
        stationCount++;
        return true;
    }
    return false;
}

// ─── Charging Session Management (Phase B) ─────────────────────────

int StationService::getAllSessions(ChargingSession outList[], int maxCapacity, const std::string& statusFilter) {
    int count = 0;

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql;
        if (statusFilter.empty()) {
            sql = "SELECT id, vehicle_id, ev_model, station_id, start_time, end_time, "
                  "battery_start, battery_end, target_charge, energy_consumed, estimated_cost, status "
                  "FROM ChargingSession ORDER BY start_time DESC;";
        } else {
            sql = "SELECT id, vehicle_id, ev_model, station_id, start_time, end_time, "
                  "battery_start, battery_end, target_charge, energy_consumed, estimated_cost, status "
                  "FROM ChargingSession WHERE status = ? ORDER BY start_time DESC;";
        }

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (!statusFilter.empty()) {
                sqlite3_bind_text(stmt, 1, statusFilter.c_str(), -1, SQLITE_STATIC);
            }

            while (sqlite3_step(stmt) == SQLITE_ROW && count < maxCapacity) {
                const char* idText    = (const char*)sqlite3_column_text(stmt, 0);
                const char* vIdText   = (const char*)sqlite3_column_text(stmt, 1);
                const char* mText     = (const char*)sqlite3_column_text(stmt, 2);
                const char* sIdText   = (const char*)sqlite3_column_text(stmt, 3);
                const char* sTimeText = (const char*)sqlite3_column_text(stmt, 4);
                const char* eTimeText = (const char*)sqlite3_column_text(stmt, 5);
                double bStart         = sqlite3_column_double(stmt, 6);
                double bEnd           = (sqlite3_column_type(stmt, 7) == SQLITE_NULL) ? 0.0 : sqlite3_column_double(stmt, 7);
                double target         = sqlite3_column_double(stmt, 8);
                double energy         = sqlite3_column_double(stmt, 9);
                double cost           = sqlite3_column_double(stmt, 10);
                const char* statText  = (const char*)sqlite3_column_text(stmt, 11);

                std::string id        = idText ? idText : "";
                std::string vehicleId = vIdText ? vIdText : "";
                std::string evModel   = mText ? mText : "";
                std::string stationId = sIdText ? sIdText : "";
                std::string startTime = sTimeText ? sTimeText : "";
                std::string endTime   = eTimeText ? eTimeText : "";
                std::string status    = statText ? statText : "active";

                outList[count] = ChargingSession(id, vehicleId, evModel, stationId, bStart, target,
                                                 startTime, endTime, bEnd, energy, cost, status);
                count++;
            }
            sqlite3_finalize(stmt);
            return count;
        }
    }

    // In-memory fallback
    for (int i = 0; i < sessionCount && count < maxCapacity; i++) {
        if (statusFilter.empty() || sessionArray[i].getStatus() == statusFilter) {
            outList[count] = sessionArray[i];
            count++;
        }
    }
    return count;
}

bool StationService::getSessionById(const std::string& id, ChargingSession& outSession) {
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id, vehicle_id, ev_model, station_id, start_time, end_time, "
                          "battery_start, battery_end, target_charge, energy_consumed, estimated_cost, status "
                          "FROM ChargingSession WHERE id = ? LIMIT 1;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* idText    = (const char*)sqlite3_column_text(stmt, 0);
                const char* vIdText   = (const char*)sqlite3_column_text(stmt, 1);
                const char* mText     = (const char*)sqlite3_column_text(stmt, 2);
                const char* sIdText   = (const char*)sqlite3_column_text(stmt, 3);
                const char* sTimeText = (const char*)sqlite3_column_text(stmt, 4);
                const char* eTimeText = (const char*)sqlite3_column_text(stmt, 5);
                double bStart         = sqlite3_column_double(stmt, 6);
                double bEnd           = (sqlite3_column_type(stmt, 7) == SQLITE_NULL) ? 0.0 : sqlite3_column_double(stmt, 7);
                double target         = sqlite3_column_double(stmt, 8);
                double energy         = sqlite3_column_double(stmt, 9);
                double cost           = sqlite3_column_double(stmt, 10);
                const char* statText  = (const char*)sqlite3_column_text(stmt, 11);

                outSession = ChargingSession(idText ? idText : "",
                                             vIdText ? vIdText : "",
                                             mText ? mText : "",
                                             sIdText ? sIdText : "",
                                             bStart, target,
                                             sTimeText ? sTimeText : "",
                                             eTimeText ? eTimeText : "",
                                             bEnd, energy, cost,
                                             statText ? statText : "active");
                sqlite3_finalize(stmt);
                return true;
            }
            sqlite3_finalize(stmt);
        }
    }

    // In-memory fallback
    for (int i = 0; i < sessionCount; i++) {
        if (sessionArray[i].getId() == id) {
            outSession = sessionArray[i];
            return true;
        }
    }
    return false;
}

bool StationService::getActiveSessionByStationId(const std::string& stationId, ChargingSession& outSession) {
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id, vehicle_id, ev_model, station_id, start_time, end_time, "
                          "battery_start, battery_end, target_charge, energy_consumed, estimated_cost, status "
                          "FROM ChargingSession WHERE station_id = ? AND status = 'active' LIMIT 1;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, stationId.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* idText    = (const char*)sqlite3_column_text(stmt, 0);
                const char* vIdText   = (const char*)sqlite3_column_text(stmt, 1);
                const char* mText     = (const char*)sqlite3_column_text(stmt, 2);
                const char* sIdText   = (const char*)sqlite3_column_text(stmt, 3);
                const char* sTimeText = (const char*)sqlite3_column_text(stmt, 4);
                const char* eTimeText = (const char*)sqlite3_column_text(stmt, 5);
                double bStart         = sqlite3_column_double(stmt, 6);
                double bEnd           = (sqlite3_column_type(stmt, 7) == SQLITE_NULL) ? 0.0 : sqlite3_column_double(stmt, 7);
                double target         = sqlite3_column_double(stmt, 8);
                double energy         = sqlite3_column_double(stmt, 9);
                double cost           = sqlite3_column_double(stmt, 10);
                const char* statText  = (const char*)sqlite3_column_text(stmt, 11);

                outSession = ChargingSession(idText ? idText : "",
                                             vIdText ? vIdText : "",
                                             mText ? mText : "",
                                             sIdText ? sIdText : "",
                                             bStart, target,
                                             sTimeText ? sTimeText : "",
                                             eTimeText ? eTimeText : "",
                                             bEnd, energy, cost,
                                             statText ? statText : "active");
                sqlite3_finalize(stmt);
                return true;
            }
            sqlite3_finalize(stmt);
        }
    }

    // In-memory fallback
    for (int i = 0; i < sessionCount; i++) {
        if (sessionArray[i].getStationId() == stationId && sessionArray[i].getStatus() == "active") {
            outSession = sessionArray[i];
            return true;
        }
    }
    return false;
}

bool StationService::createChargingSession(const std::string& evId, const std::string& stationId, double targetCharge, ChargingSession& outSession) {
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();

        // 1. Validate EV status in DB
        std::string evSql = "SELECT model, battery_capacity, current_charge, status FROM EV WHERE id = ?;";
        sqlite3_stmt* evStmt = nullptr;
        std::string evModel = "EV";
        double capacity = 50.0;
        double currentCharge = 20.0;
        std::string evStatus = "";

        if (sqlite3_prepare_v2(handle, evSql.c_str(), -1, &evStmt, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_text(evStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(evStmt) == SQLITE_ROW) {
            const char* mTxt = (const char*)sqlite3_column_text(evStmt, 0);
            evModel = mTxt ? mTxt : "Electric Vehicle";
            capacity = sqlite3_column_double(evStmt, 1);
            currentCharge = sqlite3_column_double(evStmt, 2);
            const char* sTxt = (const char*)sqlite3_column_text(evStmt, 3);
            evStatus = sTxt ? sTxt : "";
        } else {
            sqlite3_finalize(evStmt);
            return false; // EV not found
        }
        sqlite3_finalize(evStmt);

        if (evStatus == "charging" || evStatus == "swapping") {
            return false; // EV is already in an active session
        }

        // 2. Validate Station status
        std::string stSql = "SELECT status, current_vehicle_id FROM ChargingStation WHERE id = ?;";
        sqlite3_stmt* stStmt = nullptr;
        std::string stStatus = "";
        std::string stCurVeh = "";
        if (sqlite3_prepare_v2(handle, stSql.c_str(), -1, &stStmt, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_text(stStmt, 1, stationId.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stStmt) == SQLITE_ROW) {
            const char* sTxt = (const char*)sqlite3_column_text(stStmt, 0);
            stStatus = sTxt ? sTxt : "";
            const char* vTxt = (const char*)sqlite3_column_text(stStmt, 1);
            stCurVeh = vTxt ? vTxt : "";
        } else {
            sqlite3_finalize(stStmt);
            return false; // Station not found
        }
        sqlite3_finalize(stStmt);

        if (stStatus != "available" || !stCurVeh.empty()) {
            return false; // Station is busy
        }

        // 3. Compute target charge and estimated cost
        if (targetCharge <= 0.0 || targetCharge > 100.0) {
            targetCharge = 85.0; // standard default charge
        }
        if (targetCharge <= currentCharge) {
            targetCharge = std::min(100.0, currentCharge + 20.0);
        }

        double energyEst = ((targetCharge - currentCharge) / 100.0) * capacity;
        double costEst = Transaction::calculateTariff(energyEst, "charging");

        std::string sesId = "SES-" + std::to_string(s_sessionCounter++) + "-" + std::to_string(time(nullptr) % 10000);

        // 4. Atomic Multi-record SQLite Transaction
        if (!db->beginTransaction()) {
            return false;
        }

        // A. Insert ChargingSession
        std::string insertSesSql = "INSERT INTO ChargingSession (id, vehicle_id, ev_model, station_id, "
                                   "start_time, end_time, battery_start, battery_end, target_charge, "
                                   "energy_consumed, estimated_cost, status) "
                                   "VALUES (?, ?, ?, ?, datetime('now'), NULL, ?, NULL, ?, 0.0, ?, 'active');";
        sqlite3_stmt* insStmt = nullptr;
        if (sqlite3_prepare_v2(handle, insertSesSql.c_str(), -1, &insStmt, nullptr) != SQLITE_OK) {
            db->rollbackTransaction();
            return false;
        }
        sqlite3_bind_text(insStmt, 1, sesId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insStmt, 2, evId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insStmt, 3, evModel.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insStmt, 4, stationId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(insStmt, 5, currentCharge);
        sqlite3_bind_double(insStmt, 6, targetCharge);
        sqlite3_bind_double(insStmt, 7, costEst);
        int rc = sqlite3_step(insStmt);
        sqlite3_finalize(insStmt);
        if (rc != SQLITE_DONE) {
            db->rollbackTransaction();
            return false;
        }

        // B. Update ChargingStation
        std::string updStSql = "UPDATE ChargingStation SET status = 'charging', current_vehicle_id = ?, "
                               "session_start = datetime('now'), available_ports = 0 WHERE id = ?;";
        sqlite3_stmt* updStStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updStSql.c_str(), -1, &updStStmt, nullptr) != SQLITE_OK) {
            db->rollbackTransaction();
            return false;
        }
        sqlite3_bind_text(updStStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(updStStmt, 2, stationId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(updStStmt);
        sqlite3_finalize(updStStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            db->rollbackTransaction();
            return false;
        }

        // C. Update EV status
        std::string updEvSql = "UPDATE EV SET status = 'charging', last_activity = datetime('now') WHERE id = ?;";
        sqlite3_stmt* updEvStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updEvSql.c_str(), -1, &updEvStmt, nullptr) != SQLITE_OK) {
            db->rollbackTransaction();
            return false;
        }
        sqlite3_bind_text(updEvStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(updEvStmt);
        sqlite3_finalize(updEvStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            db->rollbackTransaction();
            return false;
        }

        db->commitTransaction();

        // Retrieve recorded session
        if (!getSessionById(sesId, outSession)) {
            outSession = ChargingSession(sesId, evId, evModel, stationId, currentCharge, targetCharge,
                                         "", "", 0.0, 0.0, costEst, "active");
        }
        return true;
    }

    // In-memory fallback
    for (int i = 0; i < stationCount; i++) {
        if (stationArray[i].getId() == stationId && stationArray[i].isAvailable()) {
            stationArray[i].setStatus("charging");
            stationArray[i].setCurrentVehicleId(evId);

            std::string sesId = "SES-" + std::to_string(1000 + sessionCount);
            outSession = ChargingSession(sesId, evId, "Electric Vehicle", stationId, 25.0, targetCharge,
                                         "2026-10-19 10:00:00", "", 0.0, 0.0, 150.0, "active");
            if (sessionCount < MAX_SESSIONS) {
                sessionArray[sessionCount++] = outSession;
            }
            return true;
        }
    }
    return false;
}

bool StationService::completeChargingSession(const std::string& stationId,
                                            ChargingSession& outCompletedSession,
                                            Transaction& outTxn,
                                            ChargingQueue* queue,
                                            ChargingSession* outDispatchedSession) {
    ChargingQueue queueBackup;
    if (queue != nullptr) {
        queueBackup = *queue; // Snapshot in-memory queue to guarantee rollback safety
    }

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();

        // 1. Find active session for stationId
        ChargingSession activeSession;
        if (!getActiveSessionByStationId(stationId, activeSession)) {
            return false; // Prevents duplicate completion!
        }

        std::string sessionId = activeSession.getId();
        std::string evId      = activeSession.getVehicleId();
        double batteryStart   = activeSession.getBatteryStart();
        double targetCharge   = activeSession.getTargetCharge();
        if (targetCharge <= 0.0 || targetCharge > 100.0) {
            targetCharge = 85.0;
        }

        // 2. Fetch EV battery capacity
        double capacity = 50.0;
        std::string evSql = "SELECT battery_capacity FROM EV WHERE id = ?;";
        sqlite3_stmt* evStmt = nullptr;
        if (sqlite3_prepare_v2(handle, evSql.c_str(), -1, &evStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(evStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(evStmt) == SQLITE_ROW) {
                capacity = sqlite3_column_double(evStmt, 0);
            }
            sqlite3_finalize(evStmt);
        }

        // 3. Follow project tariff & energy rules (NOT hardcoding to 100%)
        double batteryEnd = targetCharge;
        double energyDelivered = ((batteryEnd - batteryStart) / 100.0) * capacity;
        if (energyDelivered < 0.0) energyDelivered = 0.0;
        double finalAmount = Transaction::calculateTariff(energyDelivered, "charging");

        static int txCounter = 6000;
        std::string txnId = "TXN-" + std::to_string(txCounter++) + "-" + std::to_string(time(nullptr) % 10000);
        std::string desc = "Charging completed " + std::to_string((int)batteryStart) + "% to " + std::to_string((int)batteryEnd) + "%";

        // 4. Begin Atomic SQLite Transaction
        if (!db->beginTransaction()) {
            return false;
        }

        // A. Complete ChargingSession
        std::string updSesSql = "UPDATE ChargingSession SET status = 'completed', end_time = datetime('now'), "
                                "battery_end = ?, energy_consumed = ?, estimated_cost = ? "
                                "WHERE id = ? AND status = 'active';";
        sqlite3_stmt* updSesStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updSesSql.c_str(), -1, &updSesStmt, nullptr) != SQLITE_OK) {
            if (queue != nullptr) *queue = queueBackup;
            db->rollbackTransaction();
            return false;
        }
        sqlite3_bind_double(updSesStmt, 1, batteryEnd);
        sqlite3_bind_double(updSesStmt, 2, energyDelivered);
        sqlite3_bind_double(updSesStmt, 3, finalAmount);
        sqlite3_bind_text(updSesStmt, 4, sessionId.c_str(), -1, SQLITE_STATIC);
        int rc = sqlite3_step(updSesStmt);
        sqlite3_finalize(updSesStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            if (queue != nullptr) *queue = queueBackup;
            db->rollbackTransaction();
            return false;
        }

        // B. Update completed EV to available with targetCharge
        std::string updEvSql = "UPDATE EV SET status = 'available', current_charge = ?, last_activity = datetime('now') WHERE id = ?;";
        sqlite3_stmt* updEvStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updEvSql.c_str(), -1, &updEvStmt, nullptr) != SQLITE_OK) {
            if (queue != nullptr) *queue = queueBackup;
            db->rollbackTransaction();
            return false;
        }
        sqlite3_bind_double(updEvStmt, 1, batteryEnd);
        sqlite3_bind_text(updEvStmt, 2, evId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(updEvStmt);
        sqlite3_finalize(updEvStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            if (queue != nullptr) *queue = queueBackup;
            db->rollbackTransaction();
            return false;
        }

        // C. Insert TransactionLog record
        std::string insTxnSql = "INSERT INTO TransactionLog (id, vehicle_id, type, station_or_battery, "
                                "description, amount, energy_delivered, duration, timestamp, status) "
                                "VALUES (?, ?, 'charging', ?, ?, ?, ?, "
                                "max(1, cast((strftime('%s', 'now') - strftime('%s', (SELECT start_time FROM ChargingSession WHERE id = ?))) / 60 as integer)), "
                                "datetime('now'), 'completed');";
        sqlite3_stmt* insTxnStmt = nullptr;
        if (sqlite3_prepare_v2(handle, insTxnSql.c_str(), -1, &insTxnStmt, nullptr) != SQLITE_OK) {
            if (queue != nullptr) *queue = queueBackup;
            db->rollbackTransaction();
            return false;
        }
        sqlite3_bind_text(insTxnStmt, 1, txnId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insTxnStmt, 2, evId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insTxnStmt, 3, stationId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insTxnStmt, 4, desc.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(insTxnStmt, 5, finalAmount);
        sqlite3_bind_double(insTxnStmt, 6, energyDelivered);
        sqlite3_bind_text(insTxnStmt, 7, sessionId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(insTxnStmt);
        sqlite3_finalize(insTxnStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            if (queue != nullptr) *queue = queueBackup;
            db->rollbackTransaction();
            return false;
        }

        // D. Automatic FIFO Queue Dispatch if waiting EVs exist
        bool dispatched = false;
        if (queue != nullptr && !queue->isEmpty()) {
            while (!queue->isEmpty()) {
                std::string waitingEvId = queue->dequeue();

                // Validate that EV exists and has status = 'waiting' in DB
                std::string checkNextSql = "SELECT model, battery_capacity, current_charge, status FROM EV WHERE id = ?;";
                sqlite3_stmt* nextStmt = nullptr;
                std::string nextModel = "";
                double nextCap = 50.0;
                double nextCharge = 20.0;
                std::string nextStatus = "";

                if (sqlite3_prepare_v2(handle, checkNextSql.c_str(), -1, &nextStmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(nextStmt, 1, waitingEvId.c_str(), -1, SQLITE_STATIC);
                    if (sqlite3_step(nextStmt) == SQLITE_ROW) {
                        const char* mTxt = (const char*)sqlite3_column_text(nextStmt, 0);
                        nextModel = mTxt ? mTxt : "Electric Vehicle";
                        nextCap = sqlite3_column_double(nextStmt, 1);
                        nextCharge = sqlite3_column_double(nextStmt, 2);
                        const char* sTxt = (const char*)sqlite3_column_text(nextStmt, 3);
                        nextStatus = sTxt ? sTxt : "";
                    }
                    sqlite3_finalize(nextStmt);
                }

                if (nextStatus == "waiting") {
                    double nextTarget = 85.0;
                    if (nextTarget <= nextCharge) nextTarget = std::min(100.0, nextCharge + 20.0);
                    double nextEnergy = ((nextTarget - nextCharge) / 100.0) * nextCap;
                    double nextCost = Transaction::calculateTariff(nextEnergy, "charging");

                    std::string nextSesId = "SES-" + std::to_string(s_sessionCounter++) + "-" + std::to_string(time(nullptr) % 10000);

                    // Insert next active session
                    std::string insNextSql = "INSERT INTO ChargingSession (id, vehicle_id, ev_model, station_id, "
                                             "start_time, end_time, battery_start, battery_end, target_charge, "
                                             "energy_consumed, estimated_cost, status) "
                                             "VALUES (?, ?, ?, ?, datetime('now'), NULL, ?, NULL, ?, 0.0, ?, 'active');";
                    sqlite3_stmt* insNextStmt = nullptr;
                    if (sqlite3_prepare_v2(handle, insNextSql.c_str(), -1, &insNextStmt, nullptr) != SQLITE_OK) {
                        if (queue != nullptr) *queue = queueBackup;
                        db->rollbackTransaction();
                        return false;
                    }
                    sqlite3_bind_text(insNextStmt, 1, nextSesId.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_text(insNextStmt, 2, waitingEvId.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_text(insNextStmt, 3, nextModel.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_text(insNextStmt, 4, stationId.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_double(insNextStmt, 5, nextCharge);
                    sqlite3_bind_double(insNextStmt, 6, nextTarget);
                    sqlite3_bind_double(insNextStmt, 7, nextCost);
                    rc = sqlite3_step(insNextStmt);
                    sqlite3_finalize(insNextStmt);
                    if (rc != SQLITE_DONE) {
                        if (queue != nullptr) *queue = queueBackup;
                        db->rollbackTransaction();
                        return false;
                    }

                    // Update Station to new vehicle
                    std::string updStNext = "UPDATE ChargingStation SET status = 'charging', current_vehicle_id = ?, "
                                           "session_start = datetime('now'), available_ports = 0 WHERE id = ?;";
                    sqlite3_stmt* updStNextStmt = nullptr;
                    if (sqlite3_prepare_v2(handle, updStNext.c_str(), -1, &updStNextStmt, nullptr) != SQLITE_OK) {
                        if (queue != nullptr) *queue = queueBackup;
                        db->rollbackTransaction();
                        return false;
                    }
                    sqlite3_bind_text(updStNextStmt, 1, waitingEvId.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_text(updStNextStmt, 2, stationId.c_str(), -1, SQLITE_STATIC);
                    rc = sqlite3_step(updStNextStmt);
                    sqlite3_finalize(updStNextStmt);
                    if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
                        if (queue != nullptr) *queue = queueBackup;
                        db->rollbackTransaction();
                        return false;
                    }

                    // Update next EV to charging
                    std::string updEvNext = "UPDATE EV SET status = 'charging', last_activity = datetime('now') WHERE id = ?;";
                    sqlite3_stmt* updEvNextStmt = nullptr;
                    if (sqlite3_prepare_v2(handle, updEvNext.c_str(), -1, &updEvNextStmt, nullptr) != SQLITE_OK) {
                        if (queue != nullptr) *queue = queueBackup;
                        db->rollbackTransaction();
                        return false;
                    }
                    sqlite3_bind_text(updEvNextStmt, 1, waitingEvId.c_str(), -1, SQLITE_STATIC);
                    rc = sqlite3_step(updEvNextStmt);
                    sqlite3_finalize(updEvNextStmt);
                    if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
                        if (queue != nullptr) *queue = queueBackup;
                        db->rollbackTransaction();
                        return false;
                    }

                    dispatched = true;
                    if (outDispatchedSession != nullptr) {
                        *outDispatchedSession = ChargingSession(nextSesId, waitingEvId, nextModel, stationId,
                                                                nextCharge, nextTarget, "", "", 0.0, 0.0, nextCost, "active");
                    }
                    break;
                }
            }
        }

        if (!dispatched) {
            // No waiting EV dispatched: station is freed back to available
            std::string freeStSql = "UPDATE ChargingStation SET status = 'available', current_vehicle_id = NULL, "
                                    "session_start = NULL, available_ports = 1 WHERE id = ?;";
            sqlite3_stmt* freeStStmt = nullptr;
            if (sqlite3_prepare_v2(handle, freeStSql.c_str(), -1, &freeStStmt, nullptr) != SQLITE_OK) {
                if (queue != nullptr) *queue = queueBackup;
                db->rollbackTransaction();
                return false;
            }
            sqlite3_bind_text(freeStStmt, 1, stationId.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_step(freeStStmt);
            sqlite3_finalize(freeStStmt);
            if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
                if (queue != nullptr) *queue = queueBackup;
                db->rollbackTransaction();
                return false;
            }
        }

        if (!db->commitTransaction()) {
            if (queue != nullptr) *queue = queueBackup;
            db->rollbackTransaction();
            return false;
        }

        // Populate outputs
        outCompletedSession = activeSession;
        outCompletedSession.setStatus("completed");
        outCompletedSession.setBatteryEnd(batteryEnd);
        outCompletedSession.setEnergyConsumed(energyDelivered);
        outCompletedSession.setEstimatedCost(finalAmount);

        outTxn = Transaction(txnId, evId, "charging", stationId, desc, finalAmount, energyDelivered, 15, "", "completed");
        return true;
    }

    // In-memory fallback
    for (int i = 0; i < sessionCount; i++) {
        if (sessionArray[i].getStationId() == stationId && sessionArray[i].getStatus() == "active") {
            sessionArray[i].setStatus("completed");
            sessionArray[i].setBatteryEnd(sessionArray[i].getTargetCharge());
            outCompletedSession = sessionArray[i];

            for (int j = 0; j < stationCount; j++) {
                if (stationArray[j].getId() == stationId) {
                    stationArray[j].setStatus("available");
                    stationArray[j].setCurrentVehicleId("");
                    break;
                }
            }

            outTxn = Transaction("TXN-FALLBACK", sessionArray[i].getVehicleId(), "charging",
                                 stationId, "Charging complete", sessionArray[i].getEstimatedCost(),
                                 15.0, 15, "", "completed");
            return true;
        }
    }
    return false;
}

bool StationService::enqueueWaitingEV(const std::string& evId, ChargingQueue& queue) {
    if (evId.empty() || queue.isFull() || queue.contains(evId)) {
        return false;
    }

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();

        // 1. Verify EV exists in database and is not actively charging or swapping
        std::string checkSql = "SELECT status FROM EV WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        std::string currentStatus = "";

        if (sqlite3_prepare_v2(handle, checkSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_text(stmt, 1, evId.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* sTxt = (const char*)sqlite3_column_text(stmt, 0);
            currentStatus = sTxt ? sTxt : "";
        } else {
            sqlite3_finalize(stmt);
            return false; // EV does not exist in DB
        }
        sqlite3_finalize(stmt);

        if (currentStatus == "charging" || currentStatus == "swapping") {
            return false; // Cannot queue an actively occupied EV
        }

        // 2. Atomically update EV status to 'waiting' with high-resolution timestamp
        std::string updateSql = "UPDATE EV SET status = 'waiting', last_activity = strftime('%Y-%m-%d %H:%M:%f', 'now') WHERE id = ?;";
        sqlite3_stmt* updStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updateSql.c_str(), -1, &updStmt, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_text(updStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
        int rc = sqlite3_step(updStmt);
        sqlite3_finalize(updStmt);

        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            return false;
        }
    }

    // 3. Only enqueue into memory after database persistence succeeds
    return queue.enqueue(evId);
}

int StationService::rehydrateChargingQueue(ChargingQueue& queue) {
    queue.clear();
    if (db == nullptr || !db->isOpen()) {
        return 0;
    }

    sqlite3* handle = db->getDbHandle();
    // Deterministic FIFO queue reconstruction by last_activity ASC, registered_at ASC, rowid ASC
    // Even if two vehicles have identical last_activity timestamps, registered_at and immutable rowid provide a stable tie-breaker.
    std::string sql = "SELECT id FROM EV WHERE status = 'waiting' ORDER BY last_activity ASC, registered_at ASC, rowid ASC;";
    sqlite3_stmt* stmt = nullptr;
    int count = 0;

    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* idText = (const char*)sqlite3_column_text(stmt, 0);
            if (idText) {
                queue.enqueue(idText);
                count++;
            }
        }
        sqlite3_finalize(stmt);
    }
    return count;
}
