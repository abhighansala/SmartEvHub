#include "station_service.h"
#include <iostream>
#include <ctime>
#include <algorithm>

static int s_sessionCounter = 1010;
static int s_requestCounter = 1010;

StationService::StationService(Database* database) {
    db = database;
    stationCount = 0;
    sessionCount = 0;
    requestCount = 0;

    // Initialize with default demo stations if starting without database
    stationArray[0] = ChargingStation("CS-1", "DC Fast Charger 1", 60.0, "CCS2", "available", "", "Bay A-1");
    stationArray[1] = ChargingStation("CS-2", "DC Fast Charger 2", 60.0, "CCS2", "available", "", "Bay A-2");
    stationArray[2] = ChargingStation("CS-3", "Ultra Fast Charger 3", 120.0, "CCS2", "available", "", "Bay B-1");
    stationArray[3] = ChargingStation("CS-4", "AC Type 2 Charger 4", 22.0, "Type 2", "maintenance", "", "Bay B-2");
    stationCount = 4;

    sessionArray[0] = ChargingSession("SES-1001", "EV-101", "Tata Nexon EV Max", "CS-1", 12.0, 90.0, "2026-10-19 10:00:00", "", 0.0, 14.5, 290.0, "active", 9, "REQ-1001", "Bay A-1");
    sessionArray[1] = ChargingSession("SES-1000", "EV-103", "Hyundai Ioniq 5", "CS-2", 20.0, 80.0, "2026-10-19 08:30:00", "2026-10-19 09:30:00", 80.0, 43.5, 870.0, "completed", 11, "", "Bay A-2");
    sessionCount = 2;

    requestArray[0] = ChargingRequest("REQ-1001", 9, "EV-101", "CS-1", "Bay A-1", 90.0, "active", "2026-10-19 09:55:00");
    requestArray[1] = ChargingRequest("REQ-1002", 10, "EV-102", "CS-2", "Bay A-2", 85.0, "queued", "2026-10-19 10:05:00");
    requestCount = 2;

    // Auto-migrate SQLite schema if open
    ensureSchemaMigrations();
}

void StationService::ensureSchemaMigrations() {
    if (db == nullptr || !db->isOpen()) return;
    sqlite3* handle = db->getDbHandle();

    // 1. Ensure ChargingRequest table exists
    std::string reqTableSql = 
        "CREATE TABLE IF NOT EXISTS ChargingRequest ("
        "id TEXT PRIMARY KEY, "
        "user_id INTEGER NOT NULL, "
        "vehicle_id TEXT NOT NULL, "
        "station_id TEXT DEFAULT NULL, "
        "bay_id TEXT DEFAULT NULL, "
        "target_charge REAL NOT NULL DEFAULT 85.0, "
        "status TEXT NOT NULL DEFAULT 'queued' CHECK(status IN ('queued', 'allocated', 'active', 'completed', 'cancelled', 'failed')), "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "FOREIGN KEY(user_id) REFERENCES Users(id) ON DELETE CASCADE, "
        "FOREIGN KEY(vehicle_id) REFERENCES EV(id) ON DELETE CASCADE, "
        "FOREIGN KEY(station_id) REFERENCES ChargingStation(id) ON DELETE SET NULL"
        ");";
    sqlite3_exec(handle, reqTableSql.c_str(), nullptr, nullptr, nullptr);

    // 2. Ensure ChargingSession columns: user_id, request_id
    bool hasSesUserId = false;
    bool hasSesReqId = false;
    sqlite3_stmt* sesStmt = nullptr;
    if (sqlite3_prepare_v2(handle, "PRAGMA table_info(ChargingSession);", -1, &sesStmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(sesStmt) == SQLITE_ROW) {
            const char* colName = (const char*)sqlite3_column_text(sesStmt, 1);
            if (colName) {
                if (std::string(colName) == "user_id") hasSesUserId = true;
                if (std::string(colName) == "request_id") hasSesReqId = true;
            }
        }
        sqlite3_finalize(sesStmt);
    }
    if (!hasSesUserId) {
        sqlite3_exec(handle, "ALTER TABLE ChargingSession ADD COLUMN user_id INTEGER DEFAULT NULL;", nullptr, nullptr, nullptr);
    }
    if (!hasSesReqId) {
        sqlite3_exec(handle, "ALTER TABLE ChargingSession ADD COLUMN request_id TEXT DEFAULT NULL;", nullptr, nullptr, nullptr);
    }

    // 3. Ensure TransactionLog columns: user_id, session_id
    bool hasTxnUserId = false;
    bool hasTxnSesId = false;
    sqlite3_stmt* txnStmt = nullptr;
    if (sqlite3_prepare_v2(handle, "PRAGMA table_info(TransactionLog);", -1, &txnStmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(txnStmt) == SQLITE_ROW) {
            const char* colName = (const char*)sqlite3_column_text(txnStmt, 1);
            if (colName) {
                if (std::string(colName) == "user_id") hasTxnUserId = true;
                if (std::string(colName) == "session_id") hasTxnSesId = true;
            }
        }
        sqlite3_finalize(txnStmt);
    }
    if (!hasTxnUserId) {
        sqlite3_exec(handle, "ALTER TABLE TransactionLog ADD COLUMN user_id INTEGER DEFAULT NULL;", nullptr, nullptr, nullptr);
    }
    if (!hasTxnSesId) {
        sqlite3_exec(handle, "ALTER TABLE TransactionLog ADD COLUMN session_id TEXT DEFAULT NULL;", nullptr, nullptr, nullptr);
    }

    // Backfill user_id in ChargingSession and TransactionLog from EV table if null
    sqlite3_exec(handle, "UPDATE ChargingSession SET user_id = (SELECT user_id FROM EV WHERE EV.id = ChargingSession.vehicle_id) WHERE user_id IS NULL;", nullptr, nullptr, nullptr);
    sqlite3_exec(handle, "UPDATE TransactionLog SET user_id = (SELECT user_id FROM EV WHERE EV.id = TransactionLog.vehicle_id) WHERE user_id IS NULL;", nullptr, nullptr, nullptr);
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

    for (int i = 0; i < stationCount; i++) {
        if (stationArray[i].getId() == id) {
            stationArray[i].setStatus(newStatus);
            if (newStatus == "available") {
                stationArray[i].setCurrentVehicleId("");
                stationArray[i].setSessionStart("");
            }
            return true;
        }
    }
    return dbUpdated;
}

bool StationService::assignVehicle(const std::string& stationId, const std::string& vehicleId) {
    bool dbUpdated = false;
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE ChargingStation SET status = 'charging', current_vehicle_id = ?, "
                          "session_start = datetime('now'), available_ports = 0 WHERE id = ?;";
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
            stationArray[i].setSessionStart("2026-10-19 10:00:00");
            return true;
        }
    }
    return dbUpdated;
}

bool StationService::releaseStation(const std::string& stationId) {
    bool dbUpdated = false;
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE ChargingStation SET status = 'available', current_vehicle_id = NULL, "
                          "session_start = NULL, available_ports = 1 WHERE id = ?;";
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
            stationArray[i].setSessionStart("");
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
    if (stationCount >= MAX_STATIONS) return false;
    stationArray[stationCount++] = station;
    return true;
}

// ─── Customer Charging Request Management (Phase 5 & 6) ───────────────────
int StationService::getAllRequests(ChargingRequest outList[], int maxCapacity, int userId) {
    int count = 0;
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql;
        if (userId > 0) {
            sql = "SELECT id, user_id, vehicle_id, station_id, bay_id, target_charge, status, created_at "
                  "FROM ChargingRequest WHERE user_id = ? ORDER BY created_at DESC;";
        } else {
            sql = "SELECT id, user_id, vehicle_id, station_id, bay_id, target_charge, status, created_at "
                  "FROM ChargingRequest ORDER BY created_at DESC;";
        }

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (userId > 0) {
                sqlite3_bind_int(stmt, 1, userId);
            }
            while (sqlite3_step(stmt) == SQLITE_ROW && count < maxCapacity) {
                const char* rId = (const char*)sqlite3_column_text(stmt, 0);
                int uId = sqlite3_column_int(stmt, 1);
                const char* vId = (const char*)sqlite3_column_text(stmt, 2);
                const char* sId = (const char*)sqlite3_column_text(stmt, 3);
                const char* bId = (const char*)sqlite3_column_text(stmt, 4);
                double target = sqlite3_column_double(stmt, 5);
                const char* stat = (const char*)sqlite3_column_text(stmt, 6);
                const char* created = (const char*)sqlite3_column_text(stmt, 7);

                outList[count++] = ChargingRequest(rId ? rId : "", uId, vId ? vId : "",
                                                   sId ? sId : "", bId ? bId : "",
                                                   target, stat ? stat : "queued",
                                                   created ? created : "");
            }
            sqlite3_finalize(stmt);
            return count;
        }
    }

    // In-memory fallback
    for (int i = 0; i < requestCount && count < maxCapacity; i++) {
        if (userId <= 0 || requestArray[i].getUserId() == userId) {
            outList[count++] = requestArray[i];
        }
    }
    return count;
}

bool StationService::getRequestById(const std::string& id, ChargingRequest& outRequest) {
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id, user_id, vehicle_id, station_id, bay_id, target_charge, status, created_at "
                          "FROM ChargingRequest WHERE id = ? LIMIT 1;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* rId = (const char*)sqlite3_column_text(stmt, 0);
                int uId = sqlite3_column_int(stmt, 1);
                const char* vId = (const char*)sqlite3_column_text(stmt, 2);
                const char* sId = (const char*)sqlite3_column_text(stmt, 3);
                const char* bId = (const char*)sqlite3_column_text(stmt, 4);
                double target = sqlite3_column_double(stmt, 5);
                const char* stat = (const char*)sqlite3_column_text(stmt, 6);
                const char* created = (const char*)sqlite3_column_text(stmt, 7);

                outRequest = ChargingRequest(rId ? rId : "", uId, vId ? vId : "",
                                             sId ? sId : "", bId ? bId : "",
                                             target, stat ? stat : "queued",
                                             created ? created : "");
                sqlite3_finalize(stmt);
                return true;
            }
            sqlite3_finalize(stmt);
        }
    }

    for (int i = 0; i < requestCount; i++) {
        if (requestArray[i].getId() == id) {
            outRequest = requestArray[i];
            return true;
        }
    }
    return false;
}

bool StationService::createChargingRequest(int userId,
                                          const std::string& evId,
                                          const std::string& stationId,
                                          double targetCharge,
                                          ChargingRequest& outRequest,
                                          ChargingSession& outSession,
                                          bool& outAllocated,
                                          ChargingQueue* queue) {
    outAllocated = false;

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();

        // 1. Validate EV in DB
        std::string evSql = "SELECT model, battery_capacity, current_charge, status, user_id FROM EV WHERE id = ?;";
        sqlite3_stmt* evStmt = nullptr;
        std::string evModel = "EV";
        double capacity = 50.0;
        double currentCharge = 20.0;
        std::string evStatus = "";
        int evOwnerId = 0;

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
            evOwnerId = sqlite3_column_int(evStmt, 4);
        } else {
            sqlite3_finalize(evStmt);
            return false; // EV not found
        }
        sqlite3_finalize(evStmt);

        // Security / Ownership check (Phase 14)
        if (userId > 0 && evOwnerId > 0 && evOwnerId != userId) {
            
            return false;
        }

        if (evStatus == "charging" || evStatus == "swapping") {
            
            return false;
        }

        if (queue != nullptr && queue->contains(evId)) {
            
            return false;
        }

        if (targetCharge <= 0.0 || targetCharge > 100.0) {
            targetCharge = 85.0;
        }
        if (targetCharge <= currentCharge) {
            
            return false;
        }

        // 2. Station selection and availability
        ChargingStation chosenStation;
        bool hasSpecificStation = (!stationId.empty() && stationId != "any" && stationId != "ANY");
        bool stationFound = false;

        if (hasSpecificStation) {
            if (getStationById(stationId, chosenStation)) {
                if (chosenStation.isAvailable()) {
                    stationFound = true;
                }
            } else {
                return false; // Specific station does not exist
            }
        } else {
            stationFound = findAvailableStation(chosenStation);
        }

        std::string reqId = "REQ-" + std::to_string(s_requestCounter++) + "-" + std::to_string(time(nullptr) % 10000);

        // 3. If station available -> ALLOCATE BAY & START ACTIVE SESSION
        if (stationFound) {
            std::string sesId = "SES-" + std::to_string(s_sessionCounter++) + "-" + std::to_string(time(nullptr) % 10000);
            double energyEst = ((targetCharge - currentCharge) / 100.0) * capacity;
            double costEst = Transaction::calculateTariff(energyEst, "charging");

            if (!db->beginTransaction()) return false;

            // A. Insert ChargingRequest as 'allocated'
            std::string insReqSql = "INSERT INTO ChargingRequest (id, user_id, vehicle_id, station_id, bay_id, target_charge, status, created_at) "
                                    "VALUES (?, ?, ?, ?, ?, ?, 'allocated', datetime('now'));";
            sqlite3_stmt* insReqStmt = nullptr;
            if (sqlite3_prepare_v2(handle, insReqSql.c_str(), -1, &insReqStmt, nullptr) != SQLITE_OK) {
                db->rollbackTransaction();
                return false;
            }
            sqlite3_bind_text(insReqStmt, 1, reqId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(insReqStmt, 2, (userId > 0) ? userId : evOwnerId);
            sqlite3_bind_text(insReqStmt, 3, evId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(insReqStmt, 4, chosenStation.getId().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(insReqStmt, 5, chosenStation.getLocation().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(insReqStmt, 6, targetCharge);
            int rc = sqlite3_step(insReqStmt);
            sqlite3_finalize(insReqStmt);
            if (rc != SQLITE_DONE) {
                db->rollbackTransaction();
                return false;
            }

            // B. Insert ChargingSession as 'active'
            std::string insSesSql = "INSERT INTO ChargingSession (id, vehicle_id, ev_model, station_id, start_time, end_time, "
                                    "battery_start, battery_end, target_charge, energy_consumed, estimated_cost, status, user_id, request_id) "
                                    "VALUES (?, ?, ?, ?, datetime('now'), NULL, ?, NULL, ?, 0.0, ?, 'active', ?, ?);";
            sqlite3_stmt* insSesStmt = nullptr;
            if (sqlite3_prepare_v2(handle, insSesSql.c_str(), -1, &insSesStmt, nullptr) != SQLITE_OK) {
                db->rollbackTransaction();
                return false;
            }
            sqlite3_bind_text(insSesStmt, 1, sesId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(insSesStmt, 2, evId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(insSesStmt, 3, evModel.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(insSesStmt, 4, chosenStation.getId().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(insSesStmt, 5, currentCharge);
            sqlite3_bind_double(insSesStmt, 6, targetCharge);
            sqlite3_bind_double(insSesStmt, 7, costEst);
            sqlite3_bind_int(insSesStmt, 8, (userId > 0) ? userId : evOwnerId);
            sqlite3_bind_text(insSesStmt, 9, reqId.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_step(insSesStmt);
            sqlite3_finalize(insSesStmt);
            if (rc != SQLITE_DONE) {
                db->rollbackTransaction();
                return false;
            }

            // C. Update ChargingStation
            std::string updStSql = "UPDATE ChargingStation SET status = 'charging', current_vehicle_id = ?, "
                                   "session_start = datetime('now'), available_ports = 0 WHERE id = ?;";
            sqlite3_stmt* updStStmt = nullptr;
            if (sqlite3_prepare_v2(handle, updStSql.c_str(), -1, &updStStmt, nullptr) != SQLITE_OK) {
                db->rollbackTransaction();
                return false;
            }
            sqlite3_bind_text(updStStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(updStStmt, 2, chosenStation.getId().c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_step(updStStmt);
            sqlite3_finalize(updStStmt);
            if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
                db->rollbackTransaction();
                return false;
            }

            // D. Update EV status
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

            outAllocated = true;
            outRequest = ChargingRequest(reqId, (userId > 0) ? userId : evOwnerId, evId, chosenStation.getId(), chosenStation.getLocation(), targetCharge, "allocated", "");
            outSession = ChargingSession(sesId, evId, evModel, chosenStation.getId(), currentCharge, targetCharge, "", "", 0.0, 0.0, costEst, "active", (userId > 0) ? userId : evOwnerId, reqId, chosenStation.getLocation());
            return true;
        } else {
            // 4. Stations busy -> ENQUEUE IN FIFO QUEUE
            if (queue != nullptr && queue->isFull()) {
                return false; // Queue capacity full
            }

            if (!db->beginTransaction()) return false;

            std::string insReqSql = "INSERT INTO ChargingRequest (id, user_id, vehicle_id, station_id, bay_id, target_charge, status, created_at) "
                                    "VALUES (?, ?, ?, ?, 'Queue Waiting', ?, 'queued', datetime('now'));";
            sqlite3_stmt* insReqStmt = nullptr;
            if (sqlite3_prepare_v2(handle, insReqSql.c_str(), -1, &insReqStmt, nullptr) != SQLITE_OK) {
                db->rollbackTransaction();
                return false;
            }
            sqlite3_bind_text(insReqStmt, 1, reqId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(insReqStmt, 2, (userId > 0) ? userId : evOwnerId);
            sqlite3_bind_text(insReqStmt, 3, evId.c_str(), -1, SQLITE_STATIC);
            if (hasSpecificStation) {
                sqlite3_bind_text(insReqStmt, 4, stationId.c_str(), -1, SQLITE_STATIC);
            } else {
                sqlite3_bind_null(insReqStmt, 4);
            }
            sqlite3_bind_double(insReqStmt, 5, targetCharge);
            int rc = sqlite3_step(insReqStmt);
            sqlite3_finalize(insReqStmt);
            if (rc != SQLITE_DONE) {
                db->rollbackTransaction();
                return false;
            }

            std::string updEvSql = "UPDATE EV SET status = 'waiting', last_activity = datetime('now') WHERE id = ?;";
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

            if (queue != nullptr) {
                queue->enqueue(evId);
            }

            outAllocated = false;
            outRequest = ChargingRequest(reqId, (userId > 0) ? userId : evOwnerId, evId, hasSpecificStation ? stationId : "", "Queue Waiting", targetCharge, "queued", "");
            return true;
        }
    }

    // In-memory fallback
    std::string reqId = "REQ-" + std::to_string(s_requestCounter++);
    outRequest = ChargingRequest(reqId, userId, evId, stationId, "Bay 1", targetCharge, "allocated", "2026-10-19 10:00:00");
    if (requestCount < MAX_REQUESTS) {
        requestArray[requestCount++] = outRequest;
    }
    outAllocated = true;
    return true;
}

// ─── Charging Session Management (Phase 7 & 8) ───────────────────────────
int StationService::getAllSessions(ChargingSession outList[], int maxCapacity, const std::string& statusFilter, int userId) {
    int count = 0;

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT ChargingSession.id, ChargingSession.vehicle_id, ChargingSession.ev_model, "
                          "ChargingSession.station_id, ChargingSession.start_time, ChargingSession.end_time, "
                          "ChargingSession.battery_start, ChargingSession.battery_end, ChargingSession.target_charge, "
                          "ChargingSession.energy_consumed, ChargingSession.estimated_cost, ChargingSession.status, "
                          "ChargingSession.user_id, ChargingSession.request_id, ChargingStation.location "
                          "FROM ChargingSession "
                          "LEFT JOIN ChargingStation ON ChargingSession.station_id = ChargingStation.id ";

        bool hasWhere = false;
        if (userId > 0) {
            sql += "WHERE (ChargingSession.user_id = ? OR ChargingSession.vehicle_id IN (SELECT id FROM EV WHERE user_id = ?)) ";
            hasWhere = true;
        }

        if (!statusFilter.empty()) {
            sql += hasWhere ? "AND ChargingSession.status = ? " : "WHERE ChargingSession.status = ? ";
        }

        sql += "ORDER BY ChargingSession.start_time DESC;";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            int bindIdx = 1;
            if (userId > 0) {
                sqlite3_bind_int(stmt, bindIdx++, userId);
                sqlite3_bind_int(stmt, bindIdx++, userId);
            }
            if (!statusFilter.empty()) {
                sqlite3_bind_text(stmt, bindIdx++, statusFilter.c_str(), -1, SQLITE_STATIC);
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
                int uId               = sqlite3_column_int(stmt, 12);
                const char* reqText   = (const char*)sqlite3_column_text(stmt, 13);
                const char* locText   = (const char*)sqlite3_column_text(stmt, 14);

                outList[count] = ChargingSession(idText ? idText : "",
                                                 vIdText ? vIdText : "",
                                                 mText ? mText : "",
                                                 sIdText ? sIdText : "",
                                                 bStart, target,
                                                 sTimeText ? sTimeText : "",
                                                 eTimeText ? eTimeText : "",
                                                 bEnd, energy, cost,
                                                 statText ? statText : "active",
                                                 uId,
                                                 reqText ? reqText : "",
                                                 locText ? locText : "Bay 1");
                count++;
            }
            sqlite3_finalize(stmt);
            return count;
        }
    }

    // In-memory fallback
    for (int i = 0; i < sessionCount && count < maxCapacity; i++) {
        bool statusMatch = (statusFilter.empty() || sessionArray[i].getStatus() == statusFilter);
        bool userMatch = (userId <= 0 || sessionArray[i].getUserId() == userId);
        if (statusMatch && userMatch) {
            outList[count++] = sessionArray[i];
        }
    }
    return count;
}

bool StationService::getSessionById(const std::string& id, ChargingSession& outSession) {
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT ChargingSession.id, ChargingSession.vehicle_id, ChargingSession.ev_model, "
                          "ChargingSession.station_id, ChargingSession.start_time, ChargingSession.end_time, "
                          "ChargingSession.battery_start, ChargingSession.battery_end, ChargingSession.target_charge, "
                          "ChargingSession.energy_consumed, ChargingSession.estimated_cost, ChargingSession.status, "
                          "ChargingSession.user_id, ChargingSession.request_id, ChargingStation.location "
                          "FROM ChargingSession "
                          "LEFT JOIN ChargingStation ON ChargingSession.station_id = ChargingStation.id "
                          "WHERE ChargingSession.id = ? LIMIT 1;";
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
                int uId               = sqlite3_column_int(stmt, 12);
                const char* reqText   = (const char*)sqlite3_column_text(stmt, 13);
                const char* locText   = (const char*)sqlite3_column_text(stmt, 14);

                outSession = ChargingSession(idText ? idText : "",
                                             vIdText ? vIdText : "",
                                             mText ? mText : "",
                                             sIdText ? sIdText : "",
                                             bStart, target,
                                             sTimeText ? sTimeText : "",
                                             eTimeText ? eTimeText : "",
                                             bEnd, energy, cost,
                                             statText ? statText : "active",
                                             uId,
                                             reqText ? reqText : "",
                                             locText ? locText : "Bay 1");
                sqlite3_finalize(stmt);
                return true;
            }
            sqlite3_finalize(stmt);
        }
    }

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
        std::string sql = "SELECT ChargingSession.id, ChargingSession.vehicle_id, ChargingSession.ev_model, "
                          "ChargingSession.station_id, ChargingSession.start_time, ChargingSession.end_time, "
                          "ChargingSession.battery_start, ChargingSession.battery_end, ChargingSession.target_charge, "
                          "ChargingSession.energy_consumed, ChargingSession.estimated_cost, ChargingSession.status, "
                          "ChargingSession.user_id, ChargingSession.request_id, ChargingStation.location "
                          "FROM ChargingSession "
                          "LEFT JOIN ChargingStation ON ChargingSession.station_id = ChargingStation.id "
                          "WHERE ChargingSession.station_id = ? AND ChargingSession.status = 'active' LIMIT 1;";
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
                int uId               = sqlite3_column_int(stmt, 12);
                const char* reqText   = (const char*)sqlite3_column_text(stmt, 13);
                const char* locText   = (const char*)sqlite3_column_text(stmt, 14);

                outSession = ChargingSession(idText ? idText : "",
                                             vIdText ? vIdText : "",
                                             mText ? mText : "",
                                             sIdText ? sIdText : "",
                                             bStart, target,
                                             sTimeText ? sTimeText : "",
                                             eTimeText ? eTimeText : "",
                                             bEnd, energy, cost,
                                             statText ? statText : "active",
                                             uId,
                                             reqText ? reqText : "",
                                             locText ? locText : "Bay 1");
                sqlite3_finalize(stmt);
                return true;
            }
            sqlite3_finalize(stmt);
        }
    }

    for (int i = 0; i < sessionCount; i++) {
        if (sessionArray[i].getStationId() == stationId && sessionArray[i].getStatus() == "active") {
            outSession = sessionArray[i];
            return true;
        }
    }
    return false;
}

bool StationService::createChargingSession(const std::string& evId,
                                          const std::string& stationId,
                                          double targetCharge,
                                          ChargingSession& outSession,
                                          int userId,
                                          const std::string& requestId) {
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();

        // 1. Validate EV status in DB
        std::string evSql = "SELECT model, battery_capacity, current_charge, status, user_id FROM EV WHERE id = ?;";
        sqlite3_stmt* evStmt = nullptr;
        std::string evModel = "EV";
        double capacity = 50.0;
        double currentCharge = 20.0;
        std::string evStatus = "";
        int evOwnerId = 0;

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
            evOwnerId = sqlite3_column_int(evStmt, 4);
        } else {
            sqlite3_finalize(evStmt);
            return false; // EV not found
        }
        sqlite3_finalize(evStmt);

        if (evStatus == "charging" || evStatus == "swapping") {
            return false;
        }

        // 2. Validate Station status
        std::string stSql = "SELECT status, current_vehicle_id, location FROM ChargingStation WHERE id = ?;";
        sqlite3_stmt* stStmt = nullptr;
        std::string stStatus = "";
        std::string stCurVeh = "";
        std::string stLocation = "Bay 1";
        if (sqlite3_prepare_v2(handle, stSql.c_str(), -1, &stStmt, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_text(stStmt, 1, stationId.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stStmt) == SQLITE_ROW) {
            const char* sTxt = (const char*)sqlite3_column_text(stStmt, 0);
            stStatus = sTxt ? sTxt : "";
            const char* vTxt = (const char*)sqlite3_column_text(stStmt, 1);
            stCurVeh = vTxt ? vTxt : "";
            const char* lTxt = (const char*)sqlite3_column_text(stStmt, 2);
            stLocation = lTxt ? lTxt : "Bay 1";
        } else {
            sqlite3_finalize(stStmt);
            return false;
        }
        sqlite3_finalize(stStmt);

        if (stStatus != "available" || !stCurVeh.empty()) {
            return false;
        }

        if (targetCharge <= 0.0 || targetCharge > 100.0) {
            targetCharge = 85.0;
        }
        if (targetCharge <= currentCharge) {
            targetCharge = std::min(100.0, currentCharge + 20.0);
        }

        double energyEst = ((targetCharge - currentCharge) / 100.0) * capacity;
        double costEst = Transaction::calculateTariff(energyEst, "charging");

        std::string sesId = "SES-" + std::to_string(s_sessionCounter++) + "-" + std::to_string(time(nullptr) % 10000);
        int finalUserId = (userId > 0) ? userId : evOwnerId;

        // 3. Atomic Multi-record SQLite Transaction
        if (!db->beginTransaction()) {
            return false;
        }

        std::string insertSesSql = "INSERT INTO ChargingSession (id, vehicle_id, ev_model, station_id, "
                                   "start_time, end_time, battery_start, battery_end, target_charge, "
                                   "energy_consumed, estimated_cost, status, user_id, request_id) "
                                   "VALUES (?, ?, ?, ?, datetime('now'), NULL, ?, NULL, ?, 0.0, ?, 'active', ?, ?);";
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
        sqlite3_bind_int(insStmt, 8, finalUserId);
        sqlite3_bind_text(insStmt, 9, requestId.c_str(), -1, SQLITE_STATIC);
        int rc = sqlite3_step(insStmt);
        sqlite3_finalize(insStmt);
        if (rc != SQLITE_DONE) {
            db->rollbackTransaction();
            return false;
        }

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

        if (!getSessionById(sesId, outSession)) {
            outSession = ChargingSession(sesId, evId, evModel, stationId, currentCharge, targetCharge,
                                         "", "", 0.0, 0.0, costEst, "active", finalUserId, requestId, stLocation);
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
                                         "2026-10-19 10:00:00", "", 0.0, 0.0, 150.0, "active", userId, requestId, stationArray[i].getLocation());
            if (sessionCount < MAX_SESSIONS) {
                sessionArray[sessionCount++] = outSession;
            }
            return true;
        }
    }
    return false;
}

// ─── Live Progress Simulation (Phase 8) ───────────────────────────────────
bool StationService::getSessionProgress(const std::string& sessionId,
                                        ChargingSession& outSession,
                                        double& currentSoC,
                                        double& energyDelivered,
                                        double& currentCost,
                                        int& elapsedSeconds) {
    if (!getSessionById(sessionId, outSession)) {
        return false;
    }

    if (outSession.getStatus() == "completed") {
        currentSoC = outSession.getBatteryEnd();
        energyDelivered = outSession.getEnergyConsumed();
        currentCost = outSession.getEstimatedCost();
        elapsedSeconds = 0;
        return true;
    }

    double powerKW = 50.0;
    ChargingStation st;
    if (getStationById(outSession.getStationId(), st)) {
        if (st.getPower() > 0.0) powerKW = st.getPower();
    }

    double capacityKWh = 50.0;
    if (db != nullptr && db->isOpen()) {
        sqlite3_stmt* evStmt = nullptr;
        std::string sql = "SELECT battery_capacity FROM EV WHERE id = ?;";
        if (sqlite3_prepare_v2(db->getDbHandle(), sql.c_str(), -1, &evStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(evStmt, 1, outSession.getVehicleId().c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(evStmt) == SQLITE_ROW) {
                capacityKWh = sqlite3_column_double(evStmt, 0);
            }
            sqlite3_finalize(evStmt);
        }
    }

    elapsedSeconds = 60; // minimum default
    if (db != nullptr && db->isOpen()) {
        sqlite3_stmt* tStmt = nullptr;
        std::string sql = "SELECT cast(max(60, strftime('%s', 'now') - strftime('%s', start_time)) as integer) FROM ChargingSession WHERE id = ?;";
        if (sqlite3_prepare_v2(db->getDbHandle(), sql.c_str(), -1, &tStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(tStmt, 1, sessionId.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(tStmt) == SQLITE_ROW) {
                int secs = sqlite3_column_int(tStmt, 0);
                if (secs > 0) elapsedSeconds = secs;
            }
            sqlite3_finalize(tStmt);
        }
    }

    // Academic Constant-Power Simulation Model (Phase 8):
    double efficiency = 0.92;
    double elapsedHours = elapsedSeconds / 3600.0;
    double energyStored = powerKW * elapsedHours * efficiency;
    double startingSoC = outSession.getBatteryStart();
    double targetSoC = outSession.getTargetCharge();
    if (targetSoC <= 0.0 || targetSoC > 100.0) targetSoC = 85.0;

    currentSoC = startingSoC + (energyStored / capacityKWh) * 100.0;
    if (currentSoC > targetSoC) currentSoC = targetSoC;
    if (currentSoC > 100.0) currentSoC = 100.0;
    if (currentSoC < startingSoC) currentSoC = startingSoC;

    energyDelivered = ((currentSoC - startingSoC) / 100.0) * capacityKWh;
    if (energyDelivered < 0.0) energyDelivered = 0.0;

    double gridEnergy = (efficiency > 0.0) ? (energyDelivered / efficiency) : energyDelivered;
    currentCost = Transaction::calculateTariff(gridEnergy, "charging");

    return true;
}

// ─── Complete Charging Session (Phase 9 & 10) ─────────────────────────────
bool StationService::completeChargingSession(const std::string& stationOrSessionId,
                                            ChargingSession& outCompletedSession,
                                            Transaction& outTxn,
                                            ChargingQueue* queue,
                                            ChargingSession* outDispatchedSession,
                                            int callerUserId,
                                            const std::string& callerRole) {
    ChargingQueue queueBackup;
    if (queue != nullptr) {
        queueBackup = *queue;
    }

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();

        ChargingSession activeSession;
        bool found = false;

        // Check if argument is sessionId or stationId
        if (stationOrSessionId.rfind("SES-", 0) == 0) {
            found = getSessionById(stationOrSessionId, activeSession);
            if (found && activeSession.getStatus() != "active") {
                found = false;
            }
        } else {
            found = getActiveSessionByStationId(stationOrSessionId, activeSession);
        }

        if (!found) {
            return false;
        }

        std::string sessionId = activeSession.getId();
        std::string stationId = activeSession.getStationId();
        std::string evId      = activeSession.getVehicleId();
        double batteryStart   = activeSession.getBatteryStart();
        double targetCharge   = activeSession.getTargetCharge();
        if (targetCharge <= 0.0 || targetCharge > 100.0) {
            targetCharge = 85.0;
        }

        // Ownership / RBAC Authorization Guard (Phase 14)
        if (callerRole == "user" && callerUserId > 0) {
            int sessionOwnerId = activeSession.getUserId();
            if (sessionOwnerId > 0 && sessionOwnerId != callerUserId) {
                return false; // User cannot stop another user's session
            }
        }

        // Fetch EV capacity
        double capacity = 50.0;
        int evOwnerId = 0;
        std::string evSql = "SELECT battery_capacity, user_id FROM EV WHERE id = ?;";
        sqlite3_stmt* evStmt = nullptr;
        if (sqlite3_prepare_v2(handle, evSql.c_str(), -1, &evStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(evStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(evStmt) == SQLITE_ROW) {
                capacity = sqlite3_column_double(evStmt, 0);
                evOwnerId = sqlite3_column_int(evStmt, 1);
            }
            sqlite3_finalize(evStmt);
        }

        // Compute final energy, SoC, and billing
        double finalSoC = targetCharge;
        double energyDelivered = ((finalSoC - batteryStart) / 100.0) * capacity;
        if (energyDelivered < 0.0) energyDelivered = 0.0;
        double finalAmount = Transaction::calculateTariff(energyDelivered, "charging");

        static int txCounter = 6000;
        std::string txnId = "TXN-" + std::to_string(txCounter++) + "-" + std::to_string(time(nullptr) % 10000);
        std::string desc = "Charging completed " + std::to_string((int)batteryStart) + "% to " + std::to_string((int)finalSoC) + "%";
        int finalUserId = (activeSession.getUserId() > 0) ? activeSession.getUserId() : ((callerUserId > 0) ? callerUserId : evOwnerId);

        // Atomic Multi-Record SQLite Transaction
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
        sqlite3_bind_double(updSesStmt, 1, finalSoC);
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

        // B. Update completed EV to available with finalSoC
        std::string updEvSql = "UPDATE EV SET status = 'available', current_charge = ?, last_activity = datetime('now') WHERE id = ?;";
        sqlite3_stmt* updEvStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updEvSql.c_str(), -1, &updEvStmt, nullptr) != SQLITE_OK) {
            if (queue != nullptr) *queue = queueBackup;
            db->rollbackTransaction();
            return false;
        }
        sqlite3_bind_double(updEvStmt, 1, finalSoC);
        sqlite3_bind_text(updEvStmt, 2, evId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(updEvStmt);
        sqlite3_finalize(updEvStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            if (queue != nullptr) *queue = queueBackup;
            db->rollbackTransaction();
            return false;
        }

        // C. Insert TransactionLog record with user_id and session_id
        std::string insTxnSql = "INSERT INTO TransactionLog (id, vehicle_id, type, station_or_battery, "
                                "description, amount, energy_delivered, duration, timestamp, status, user_id, session_id) "
                                "VALUES (?, ?, 'charging', ?, ?, ?, ?, "
                                "max(1, cast((strftime('%s', 'now') - strftime('%s', (SELECT start_time FROM ChargingSession WHERE id = ?))) / 60 as integer)), "
                                "datetime('now'), 'completed', ?, ?);";
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
        sqlite3_bind_int(insTxnStmt, 8, finalUserId);
        sqlite3_bind_text(insTxnStmt, 9, sessionId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(insTxnStmt);
        sqlite3_finalize(insTxnStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            if (queue != nullptr) *queue = queueBackup;
            db->rollbackTransaction();
            return false;
        }

        // D. Mark associated ChargingRequest as 'completed'
        std::string updReqSql = "UPDATE ChargingRequest SET status = 'completed' "
                                "WHERE (id = (SELECT request_id FROM ChargingSession WHERE id = ?) "
                                "OR (vehicle_id = ? AND status IN ('allocated', 'active', 'queued')));";
        sqlite3_stmt* updReqStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updReqSql.c_str(), -1, &updReqStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(updReqStmt, 1, sessionId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(updReqStmt, 2, evId.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(updReqStmt);
            sqlite3_finalize(updReqStmt);
        }

        // E. Automatic FIFO Queue Dispatch if waiting EVs exist
        bool dispatched = false;
        if (queue != nullptr && !queue->isEmpty()) {
            while (!queue->isEmpty()) {
                std::string waitingEvId = queue->dequeue();

                std::string checkNextSql = "SELECT model, battery_capacity, current_charge, status, user_id FROM EV WHERE id = ?;";
                sqlite3_stmt* nextStmt = nullptr;
                std::string nextModel = "";
                double nextCap = 50.0;
                double nextCharge = 20.0;
                std::string nextStatus = "";
                int nextOwnerId = 0;

                if (sqlite3_prepare_v2(handle, checkNextSql.c_str(), -1, &nextStmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(nextStmt, 1, waitingEvId.c_str(), -1, SQLITE_STATIC);
                    if (sqlite3_step(nextStmt) == SQLITE_ROW) {
                        const char* mTxt = (const char*)sqlite3_column_text(nextStmt, 0);
                        nextModel = mTxt ? mTxt : "Electric Vehicle";
                        nextCap = sqlite3_column_double(nextStmt, 1);
                        nextCharge = sqlite3_column_double(nextStmt, 2);
                        const char* sTxt = (const char*)sqlite3_column_text(nextStmt, 3);
                        nextStatus = sTxt ? sTxt : "";
                        nextOwnerId = sqlite3_column_int(nextStmt, 4);
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
                                             "energy_consumed, estimated_cost, status, user_id) "
                                             "VALUES (?, ?, ?, ?, datetime('now'), NULL, ?, NULL, ?, 0.0, ?, 'active', ?);";
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
                    sqlite3_bind_int(insNextStmt, 8, nextOwnerId);
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

                    // Update next request status to allocated
                    std::string updNextReq = "UPDATE ChargingRequest SET status = 'allocated', station_id = ?, bay_id = (SELECT location FROM ChargingStation WHERE id = ?) "
                                             "WHERE vehicle_id = ? AND status = 'queued';";
                    sqlite3_stmt* updNextReqStmt = nullptr;
                    if (sqlite3_prepare_v2(handle, updNextReq.c_str(), -1, &updNextReqStmt, nullptr) == SQLITE_OK) {
                        sqlite3_bind_text(updNextReqStmt, 1, stationId.c_str(), -1, SQLITE_STATIC);
                        sqlite3_bind_text(updNextReqStmt, 2, stationId.c_str(), -1, SQLITE_STATIC);
                        sqlite3_bind_text(updNextReqStmt, 3, waitingEvId.c_str(), -1, SQLITE_STATIC);
                        sqlite3_step(updNextReqStmt);
                        sqlite3_finalize(updNextReqStmt);
                    }

                    dispatched = true;
                    if (outDispatchedSession != nullptr) {
                        *outDispatchedSession = ChargingSession(nextSesId, waitingEvId, nextModel, stationId,
                                                                nextCharge, nextTarget, "", "", 0.0, 0.0, nextCost, "active", nextOwnerId);
                    }
                    break;
                }
            }
        }

        if (!dispatched) {
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

        db->commitTransaction();

        outCompletedSession = activeSession;
        outCompletedSession.setStatus("completed");
        outCompletedSession.setBatteryEnd(finalSoC);
        outCompletedSession.setEnergyConsumed(energyDelivered);
        outCompletedSession.setEstimatedCost(finalAmount);

        outTxn = Transaction(txnId, evId, "charging", stationId, desc, finalAmount, energyDelivered, 15, "", "completed", finalUserId, sessionId);
        return true;
    }

    // In-memory fallback
    for (int i = 0; i < sessionCount; i++) {
        if ((sessionArray[i].getStationId() == stationOrSessionId || sessionArray[i].getId() == stationOrSessionId) &&
            sessionArray[i].getStatus() == "active") {
            sessionArray[i].setStatus("completed");
            sessionArray[i].setBatteryEnd(sessionArray[i].getTargetCharge());
            sessionArray[i].setEnergyConsumed(25.0);
            sessionArray[i].setEstimatedCost(Transaction::calculateTariff(25.0, "charging"));

            outCompletedSession = sessionArray[i];
            outTxn = Transaction("TXN-" + std::to_string(5000 + i), sessionArray[i].getVehicleId(),
                                 "charging", sessionArray[i].getStationId(), "Session completed",
                                 outCompletedSession.getEstimatedCost(), 25.0, 30, "", "completed", callerUserId, sessionArray[i].getId());

            releaseStation(sessionArray[i].getStationId());
            return true;
        }
    }
    return false;
}

// ─── FIFO Queue Helpers ───────────────────────────────────────────────────
bool StationService::enqueueWaitingEV(const std::string& evId, ChargingQueue& queue) {
    if (queue.isFull()) return false;
    if (queue.contains(evId)) return false;

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE EV SET status = 'waiting', last_activity = datetime('now') "
                          "WHERE id = ? AND status != 'charging' AND status != 'swapping';";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_text(stmt, 1, evId.c_str(), -1, SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc == SQLITE_DONE && sqlite3_changes(handle) > 0) {
            queue.enqueue(evId);
            return true;
        }
        return false;
    }

    queue.enqueue(evId);
    return true;
}

int StationService::rehydrateChargingQueue(ChargingQueue& queue) {
    queue.clear();
    int count = 0;
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id FROM EV WHERE status = 'waiting' ORDER BY last_activity ASC, registered_at ASC, rowid ASC;";
        sqlite3_stmt* stmt = nullptr;
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
    }
    return count;
}
