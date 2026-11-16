#include "station_service.h"
#include <iostream>
#include <ctime>
#include <algorithm>
#include <vector>
#include <cmath>

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

    requestArray[0] = ChargingRequest("REQ-1001", 9, "EV-101", "CS-1", "Bay A-1", 90.0, "active", "2026-10-19 09:55:00", "CCS2");
    requestArray[1] = ChargingRequest("REQ-1002", 10, "EV-102", "CS-2", "Bay A-2", 85.0, "queued", "2026-10-19 10:05:00", "CCS2");
    requestCount = 2;

    // Configurable simulation parameters
    m_speedMultiplier = 20.0; // default demonstration speed (20x)
    const char* envMult = std::getenv("CHARGING_SIMULATION_SPEED_MULTIPLIER");
    if (envMult != nullptr) {
        try {
            double v = std::stod(envMult);
            if (v > 0.0) m_speedMultiplier = v;
        } catch (...) {}
    }

    m_chargingEfficiency = 0.90; // default 90%
    const char* envEff = std::getenv("CHARGING_EFFICIENCY");
    if (envEff != nullptr) {
        try {
            double v = std::stod(envEff);
            if (v > 0.0 && v <= 1.0) m_chargingEfficiency = v;
        } catch (...) {}
    }

    m_tariffRate = 0.0; // 0.0 = standard tiered tariff
    const char* envTariff = std::getenv("CHARGING_TARIFF_RATE");
    if (envTariff != nullptr) {
        try {
            double v = std::stod(envTariff);
            if (v >= 0.0) m_tariffRate = v;
        } catch (...) {}
    }

    // Auto-migrate SQLite schema if open
    ensureSchemaMigrations();
}

void StationService::ensureSchemaMigrations() {
    if (db == nullptr || !db->isOpen()) return;
    sqlite3* handle = db->getDbHandle();

    // 1. Ensure ChargingRequest table exists with connector_type
    std::string reqTableSql = 
        "CREATE TABLE IF NOT EXISTS ChargingRequest ("
        "id TEXT PRIMARY KEY, "
        "user_id INTEGER NOT NULL, "
        "vehicle_id TEXT NOT NULL, "
        "station_id TEXT DEFAULT NULL, "
        "bay_id TEXT DEFAULT NULL, "
        "target_charge REAL NOT NULL DEFAULT 85.0, "
        "connector_type TEXT NOT NULL DEFAULT 'CCS2' CHECK(connector_type IN ('Type 2', 'CCS2', 'CHAdeMO', 'GB/T')), "
        "status TEXT NOT NULL DEFAULT 'queued' CHECK(status IN ('queued', 'allocated', 'active', 'completed', 'cancelled', 'failed')), "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "FOREIGN KEY(user_id) REFERENCES Users(id) ON DELETE CASCADE, "
        "FOREIGN KEY(vehicle_id) REFERENCES EV(id) ON DELETE CASCADE, "
        "FOREIGN KEY(station_id) REFERENCES ChargingStation(id) ON DELETE SET NULL"
        ");";
    sqlite3_exec(handle, reqTableSql.c_str(), nullptr, nullptr, nullptr);

    // Check ChargingRequest columns: connector_type
    bool hasReqConnType = false;
    sqlite3_stmt* reqStmt = nullptr;
    if (sqlite3_prepare_v2(handle, "PRAGMA table_info(ChargingRequest);", -1, &reqStmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(reqStmt) == SQLITE_ROW) {
            const char* colName = (const char*)sqlite3_column_text(reqStmt, 1);
            if (colName && std::string(colName) == "connector_type") {
                hasReqConnType = true;
            }
        }
        sqlite3_finalize(reqStmt);
    }
    if (!hasReqConnType) {
        sqlite3_exec(handle, "ALTER TABLE ChargingRequest ADD COLUMN connector_type TEXT NOT NULL DEFAULT 'CCS2';", nullptr, nullptr, nullptr);
    }

    // Check EV columns: connector_type
    bool hasEvConnType = false;
    sqlite3_stmt* evColStmt = nullptr;
    if (sqlite3_prepare_v2(handle, "PRAGMA table_info(EV);", -1, &evColStmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(evColStmt) == SQLITE_ROW) {
            const char* colName = (const char*)sqlite3_column_text(evColStmt, 1);
            if (colName && std::string(colName) == "connector_type") {
                hasEvConnType = true;
            }
        }
        sqlite3_finalize(evColStmt);
    }
    if (!hasEvConnType) {
        sqlite3_exec(handle, "ALTER TABLE EV ADD COLUMN connector_type TEXT NOT NULL DEFAULT 'CCS2';", nullptr, nullptr, nullptr);
    }

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

                outList[count] = ChargingStation(
                    idText ? idText : "",
                    nameText ? nameText : "",
                    power,
                    connText ? connText : "CCS2",
                    statusText ? statusText : "available",
                    vehText ? vehText : "",
                    locText ? locText : "Bay 1",
                    sesStartText ? sesStartText : ""
                );
                count++;
            }
            sqlite3_finalize(stmt);
            return count;
        }
    }

    // In-memory fallback
    for (int i = 0; i < stationCount && count < maxCapacity; i++) {
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

bool StationService::updateStationStatus(const std::string& id, const std::string& newStatus, ChargingQueue* queue) {
    bool dbUpdated = false;
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE ChargingStation SET status = ?, available_ports = (CASE WHEN ? = 'available' THEN 1 ELSE 0 END) WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, newStatus.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, newStatus.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, id.c_str(), -1, SQLITE_STATIC);
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
            }
            dbUpdated = true;
            break;
        }
    }

    if (newStatus == "available" && queue != nullptr) {
        processQueueForAvailableStations(queue);
    }

    return dbUpdated;
}

bool StationService::assignVehicle(const std::string& stationId, const std::string& vehicleId) {
    bool dbUpdated = false;
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE ChargingStation SET status = 'charging', current_vehicle_id = ?, "
                          "session_start = datetime('now'), available_ports = 0 WHERE id = ? AND status = 'available';";
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

bool StationService::releaseStation(const std::string& stationId, ChargingQueue* queue) {
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
            dbUpdated = true;
            break;
        }
    }

    if (queue != nullptr) {
        processQueueForAvailableStations(queue);
    }

    return dbUpdated;
}

bool StationService::findAvailableStation(ChargingStation& outStation,
                                         const std::string& preferredStationId,
                                         const std::string& requiredConnectorType) {
    if (!preferredStationId.empty() && preferredStationId != "any" && preferredStationId != "ANY") {
        if (getStationById(preferredStationId, outStation)) {
            if (outStation.getStatus() == "available" &&
                outStation.getCurrentVehicleId().empty() &&
                (requiredConnectorType.empty() || outStation.getConnectorType() == requiredConnectorType)) {
                return true;
            }
        }
        return false;
    }

    ChargingStation all[MAX_STATIONS];
    int count = getAllStations(all, MAX_STATIONS);
    for (int i = 0; i < count; i++) {
        if (all[i].getStatus() == "available" &&
            all[i].getCurrentVehicleId().empty() &&
            (requiredConnectorType.empty() || all[i].getConnectorType() == requiredConnectorType)) {
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
            sql = "SELECT id, user_id, vehicle_id, station_id, bay_id, target_charge, status, created_at, connector_type "
                  "FROM ChargingRequest WHERE user_id = ? ORDER BY created_at DESC;";
        } else {
            sql = "SELECT id, user_id, vehicle_id, station_id, bay_id, target_charge, status, created_at, connector_type "
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
                const char* conn = (const char*)sqlite3_column_text(stmt, 8);

                outList[count++] = ChargingRequest(rId ? rId : "", uId, vId ? vId : "",
                                                   sId ? sId : "", bId ? bId : "",
                                                   target, stat ? stat : "queued",
                                                   created ? created : "",
                                                   conn ? conn : "CCS2");
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
        std::string sql = "SELECT id, user_id, vehicle_id, station_id, bay_id, target_charge, status, created_at, connector_type "
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
                const char* conn = (const char*)sqlite3_column_text(stmt, 8);

                outRequest = ChargingRequest(rId ? rId : "", uId, vId ? vId : "",
                                             sId ? sId : "", bId ? bId : "",
                                             target, stat ? stat : "queued",
                                             created ? created : "",
                                             conn ? conn : "CCS2");
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

int StationService::processQueueForAvailableStations(ChargingQueue* queue) {
    if (queue != nullptr && queue->isEmpty()) {
        return 0;
    }
    if (db == nullptr || !db->isOpen()) {
        return 0;
    }
    sqlite3* handle = db->getDbHandle();

    // 1. Fetch all operational available stations
    ChargingStation availStations[MAX_STATIONS];
    int availCount = 0;
    std::string stSql = "SELECT id, name, power, connector_type, status, current_vehicle_id, location, session_start "
                        "FROM ChargingStation WHERE status = 'available' AND (current_vehicle_id IS NULL OR current_vehicle_id = '') "
                        "AND available_ports > 0 ORDER BY id ASC;";
    sqlite3_stmt* stStmt = nullptr;
    if (sqlite3_prepare_v2(handle, stSql.c_str(), -1, &stStmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stStmt) == SQLITE_ROW && availCount < MAX_STATIONS) {
            const char* idText = (const char*)sqlite3_column_text(stStmt, 0);
            const char* nameText = (const char*)sqlite3_column_text(stStmt, 1);
            double power = sqlite3_column_double(stStmt, 2);
            const char* connText = (const char*)sqlite3_column_text(stStmt, 3);
            const char* statusText = (const char*)sqlite3_column_text(stStmt, 4);
            const char* vehText = (const char*)sqlite3_column_text(stStmt, 5);
            const char* locText = (const char*)sqlite3_column_text(stStmt, 6);
            const char* sesStartText = (const char*)sqlite3_column_text(stStmt, 7);

            availStations[availCount++] = ChargingStation(
                idText ? idText : "",
                nameText ? nameText : "",
                power,
                connText ? connText : "CCS2",
                statusText ? statusText : "available",
                vehText ? vehText : "",
                locText ? locText : "Bay 1",
                sesStartText ? sesStartText : ""
            );
        }
        sqlite3_finalize(stStmt);
    }

    if (availCount == 0) {
        return 0; // No available operational stations
    }

    // 2. Fetch queued requests in strict FIFO order
    ChargingRequest queuedReqs[MAX_REQUESTS];
    int queuedCount = 0;

    if (queue != nullptr) {
        std::string qVehicles[CHARGING_QUEUE_CAPACITY];
        int qvCount = queue->getItems(qVehicles, CHARGING_QUEUE_CAPACITY);
        for (int v = 0; v < qvCount && queuedCount < MAX_REQUESTS; v++) {
            std::string qvId = qVehicles[v];
            std::string rSql = "SELECT id, user_id, vehicle_id, station_id, bay_id, target_charge, status, created_at, connector_type "
                               "FROM ChargingRequest WHERE vehicle_id = ? AND status = 'queued' ORDER BY created_at ASC LIMIT 1;";
            sqlite3_stmt* rStmt = nullptr;
            bool foundReq = false;
            if (sqlite3_prepare_v2(handle, rSql.c_str(), -1, &rStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(rStmt, 1, qvId.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(rStmt) == SQLITE_ROW) {
                    foundReq = true;
                    const char* rId = (const char*)sqlite3_column_text(rStmt, 0);
                    int uId = sqlite3_column_int(rStmt, 1);
                    const char* vId = (const char*)sqlite3_column_text(rStmt, 2);
                    const char* sId = (const char*)sqlite3_column_text(rStmt, 3);
                    const char* bId = (const char*)sqlite3_column_text(rStmt, 4);
                    double target = sqlite3_column_double(rStmt, 5);
                    const char* stat = (const char*)sqlite3_column_text(rStmt, 6);
                    const char* created = (const char*)sqlite3_column_text(rStmt, 7);
                    const char* conn = (const char*)sqlite3_column_text(rStmt, 8);

                    queuedReqs[queuedCount++] = ChargingRequest(
                        rId ? rId : "",
                        uId,
                        vId ? vId : "",
                        sId ? sId : "",
                        bId ? bId : "",
                        target,
                        stat ? stat : "queued",
                        created ? created : "",
                        conn ? conn : "CCS2"
                    );
                }
                sqlite3_finalize(rStmt);
            }

            if (!foundReq && queuedCount < MAX_REQUESTS) {
                std::string evSql = "SELECT user_id, current_charge, model FROM EV WHERE id = ?;";
                sqlite3_stmt* evStmt = nullptr;
                if (sqlite3_prepare_v2(handle, evSql.c_str(), -1, &evStmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(evStmt, 1, qvId.c_str(), -1, SQLITE_STATIC);
                    if (sqlite3_step(evStmt) == SQLITE_ROW) {
                        int uId = sqlite3_column_int(evStmt, 0);
                        static int autoReqCounter = 9000;
                        std::string autoReqId = "REQ-A-" + std::to_string(autoReqCounter++) + "-" + std::to_string(time(nullptr) % 10000);

                        std::string insSql = "INSERT INTO ChargingRequest (id, user_id, vehicle_id, station_id, bay_id, target_charge, connector_type, status, created_at) "
                                             "VALUES (?, ?, ?, NULL, 'Queue Waiting', 85.0, 'CCS2', 'queued', datetime('now'));";
                        sqlite3_stmt* insStmt = nullptr;
                        if (sqlite3_prepare_v2(handle, insSql.c_str(), -1, &insStmt, nullptr) == SQLITE_OK) {
                            sqlite3_bind_text(insStmt, 1, autoReqId.c_str(), -1, SQLITE_STATIC);
                            sqlite3_bind_int(insStmt, 2, uId);
                            sqlite3_bind_text(insStmt, 3, qvId.c_str(), -1, SQLITE_STATIC);
                            sqlite3_step(insStmt);
                            sqlite3_finalize(insStmt);
                        }

                        queuedReqs[queuedCount++] = ChargingRequest(
                            autoReqId,
                            uId,
                            qvId,
                            "",
                            "Queue Waiting",
                            85.0,
                            "queued",
                            "",
                            "CCS2"
                        );
                    }
                    sqlite3_finalize(evStmt);
                }
            }
        }
    } else {
        std::string reqSql = "SELECT id, user_id, vehicle_id, station_id, bay_id, target_charge, status, created_at, connector_type "
                             "FROM ChargingRequest WHERE status = 'queued' ORDER BY created_at ASC, rowid ASC;";
        sqlite3_stmt* reqStmt = nullptr;
        if (sqlite3_prepare_v2(handle, reqSql.c_str(), -1, &reqStmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(reqStmt) == SQLITE_ROW && queuedCount < MAX_REQUESTS) {
                const char* rId = (const char*)sqlite3_column_text(reqStmt, 0);
                int uId = sqlite3_column_int(reqStmt, 1);
                const char* vId = (const char*)sqlite3_column_text(reqStmt, 2);
                const char* sId = (const char*)sqlite3_column_text(reqStmt, 3);
                const char* bId = (const char*)sqlite3_column_text(reqStmt, 4);
                double target = sqlite3_column_double(reqStmt, 5);
                const char* stat = (const char*)sqlite3_column_text(reqStmt, 6);
                const char* created = (const char*)sqlite3_column_text(reqStmt, 7);
                const char* conn = (const char*)sqlite3_column_text(reqStmt, 8);

                queuedReqs[queuedCount++] = ChargingRequest(
                    rId ? rId : "",
                    uId,
                    vId ? vId : "",
                    sId ? sId : "",
                    bId ? bId : "",
                    target,
                    stat ? stat : "queued",
                    created ? created : "",
                    conn ? conn : "CCS2"
                );
            }
            sqlite3_finalize(reqStmt);
        }
    }

    if (queuedCount == 0) {
        return 0; // No waiting requests
    }

    int processedCount = 0;
    bool stationClaimed[MAX_STATIONS] = { false };

    // 3. Match queued requests with compatible available stations
    for (int q = 0; q < queuedCount; q++) {
        std::string reqId = queuedReqs[q].getId();
        std::string evId = queuedReqs[q].getVehicleId();
        int reqUserId = queuedReqs[q].getUserId();
        double targetCharge = queuedReqs[q].getTargetCharge();
        std::string prefStationId = queuedReqs[q].getStationId();
        std::string reqConnector = queuedReqs[q].getConnectorType();
        if (reqConnector.empty()) reqConnector = "CCS2";

        int matchIdx = -1;
        // First try to match preferred station if specified and compatible
        if (!prefStationId.empty() && prefStationId != "any" && prefStationId != "ANY") {
            for (int s = 0; s < availCount; s++) {
                if (stationClaimed[s]) continue;
                if (availStations[s].getStatus() != "available") continue;
                if (!reqConnector.empty() && availStations[s].getConnectorType() != reqConnector) {
                    continue;
                }
                if (availStations[s].getId() == prefStationId) {
                    matchIdx = s;
                    break;
                }
            }
        }

        // If no preferred station matched, match any available compatible station
        if (matchIdx == -1) {
            for (int s = 0; s < availCount; s++) {
                if (stationClaimed[s]) continue;
                if (availStations[s].getStatus() != "available") continue;
                if (!reqConnector.empty() && availStations[s].getConnectorType() != reqConnector) {
                    continue;
                }
                matchIdx = s;
                break;
            }
        }

        if (matchIdx == -1) {
            // No compatible bay found for this request
            continue;
        }

        ChargingStation chosenStation = availStations[matchIdx];

        // Fetch EV data
        std::string evSql = "SELECT model, battery_capacity, current_charge, user_id FROM EV WHERE id = ?;";
        sqlite3_stmt* evStmt = nullptr;
        std::string evModel = "Electric Vehicle";
        double capacity = 50.0;
        double currentCharge = 20.0;
        int evOwnerId = reqUserId;

        if (sqlite3_prepare_v2(handle, evSql.c_str(), -1, &evStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(evStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(evStmt) == SQLITE_ROW) {
                const char* mTxt = (const char*)sqlite3_column_text(evStmt, 0);
                if (mTxt) evModel = mTxt;
                capacity = sqlite3_column_double(evStmt, 1);
                currentCharge = sqlite3_column_double(evStmt, 2);
                int oId = sqlite3_column_int(evStmt, 3);
                if (oId > 0) evOwnerId = oId;
            }
            sqlite3_finalize(evStmt);
        }

        if (targetCharge <= currentCharge) {
            targetCharge = std::min(100.0, currentCharge + 20.0);
        }
        double energyEst = ((targetCharge - currentCharge) / 100.0) * capacity;
        double costEst = Transaction::calculateTariff(energyEst, "charging");

        std::string sesId = "SES-" + std::to_string(s_sessionCounter++) + "-" + std::to_string(time(nullptr) % 10000);

        // 4. Atomic Multi-record SQLite Transaction
        if (!db->beginTransaction()) {
            continue;
        }

        // A. Lock ChargingStation atomically
        std::string updStSql = "UPDATE ChargingStation SET status = 'charging', current_vehicle_id = ?, "
                               "session_start = datetime('now'), available_ports = 0 WHERE id = ? AND status = 'available' "
                               "AND (current_vehicle_id IS NULL OR current_vehicle_id = '');";
        sqlite3_stmt* updStStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updStSql.c_str(), -1, &updStStmt, nullptr) != SQLITE_OK) {
            db->rollbackTransaction();
            continue;
        }
        sqlite3_bind_text(updStStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(updStStmt, 2, chosenStation.getId().c_str(), -1, SQLITE_STATIC);
        int rc = sqlite3_step(updStStmt);
        sqlite3_finalize(updStStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            // Concurrent race condition or bay occupied
            db->rollbackTransaction();
            continue;
        }

        // B. Update ChargingRequest atomically: queued -> allocated
        std::string updReqSql = "UPDATE ChargingRequest SET status = 'allocated', station_id = ?, bay_id = ? "
                                "WHERE id = ? AND status = 'queued';";
        sqlite3_stmt* updReqStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updReqSql.c_str(), -1, &updReqStmt, nullptr) != SQLITE_OK) {
            db->rollbackTransaction();
            continue;
        }
        sqlite3_bind_text(updReqStmt, 1, chosenStation.getId().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(updReqStmt, 2, chosenStation.getLocation().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(updReqStmt, 3, reqId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(updReqStmt);
        sqlite3_finalize(updReqStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            // Request was already allocated or cancelled concurrently
            db->rollbackTransaction();
            continue;
        }

        // C. Update EV status to charging
        std::string updEvSql = "UPDATE EV SET status = 'charging', last_activity = datetime('now') WHERE id = ?;";
        sqlite3_stmt* updEvStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updEvSql.c_str(), -1, &updEvStmt, nullptr) != SQLITE_OK) {
            db->rollbackTransaction();
            continue;
        }
        sqlite3_bind_text(updEvStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(updEvStmt);
        sqlite3_finalize(updEvStmt);
        if (rc != SQLITE_DONE) {
            db->rollbackTransaction();
            continue;
        }

        // D. Insert ChargingSession (status = 'active')
        std::string insSesSql = "INSERT INTO ChargingSession (id, vehicle_id, ev_model, station_id, start_time, end_time, "
                                "battery_start, battery_end, target_charge, energy_consumed, estimated_cost, status, user_id, request_id) "
                                "VALUES (?, ?, ?, ?, datetime('now'), NULL, ?, NULL, ?, 0.0, ?, 'active', ?, ?);";
        sqlite3_stmt* insSesStmt = nullptr;
        if (sqlite3_prepare_v2(handle, insSesSql.c_str(), -1, &insSesStmt, nullptr) != SQLITE_OK) {
            db->rollbackTransaction();
            continue;
        }
        sqlite3_bind_text(insSesStmt, 1, sesId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insSesStmt, 2, evId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insSesStmt, 3, evModel.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insSesStmt, 4, chosenStation.getId().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(insSesStmt, 5, currentCharge);
        sqlite3_bind_double(insSesStmt, 6, targetCharge);
        sqlite3_bind_double(insSesStmt, 7, costEst);
        sqlite3_bind_int(insSesStmt, 8, evOwnerId);
        sqlite3_bind_text(insSesStmt, 9, reqId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(insSesStmt);
        sqlite3_finalize(insSesStmt);
        if (rc != SQLITE_DONE) {
            db->rollbackTransaction();
            continue;
        }

        db->commitTransaction();

        stationClaimed[matchIdx] = true;
        if (queue != nullptr) {
            queue->remove(evId);
        }
        processedCount++;
    }

    return processedCount;
}

bool StationService::createChargingRequest(int userId,
                                          const std::string& evId,
                                          const std::string& stationId,
                                          double targetCharge,
                                          ChargingRequest& outRequest,
                                          ChargingSession& outSession,
                                          bool& outAllocated,
                                          ChargingQueue* queue,
                                          const std::string& connectorType) {
    outAllocated = false;

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();

        // 1. Validate EV in DB
        std::string evSql = "SELECT model, battery_capacity, current_charge, status, user_id, connector_type FROM EV WHERE id = ?;";
        sqlite3_stmt* evStmt = nullptr;
        std::string evModel = "EV";
        double capacity = 50.0;
        double currentCharge = 20.0;
        std::string evStatus = "";
        int evOwnerId = 0;
        std::string evConn = "CCS2";

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
            const char* cTxt = (const char*)sqlite3_column_text(evStmt, 5);
            if (cTxt) evConn = cTxt;
        } else {
            sqlite3_finalize(evStmt);
            return false; // EV not found
        }
        sqlite3_finalize(evStmt);

        // Security / Ownership check: Authenticated User must own the EV
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

        if (queue != nullptr && queue->isFull()) {
            return false; // Queue capacity overflow
        }

        std::string finalConn = connectorType.empty() ? evConn : connectorType;
        if (finalConn.empty()) finalConn = "CCS2";

        bool hasSpecificStation = (!stationId.empty() && stationId != "any" && stationId != "ANY");
        if (hasSpecificStation) {
            ChargingStation specStation;
            if (!getStationById(stationId, specStation)) {
                return false;
            }
        }

        std::string reqId = "REQ-" + std::to_string(s_requestCounter++) + "-" + std::to_string(time(nullptr) % 10000);

        // Insert ChargingRequest as 'queued' and update EV to 'waiting'
        if (!db->beginTransaction()) return false;

        std::string insReqSql = "INSERT INTO ChargingRequest (id, user_id, vehicle_id, station_id, bay_id, target_charge, connector_type, status, created_at) "
                                "VALUES (?, ?, ?, ?, 'Queue Waiting', ?, ?, 'queued', datetime('now'));";
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
        sqlite3_bind_text(insReqStmt, 6, finalConn.c_str(), -1, SQLITE_STATIC);
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

        // Automatic Queue Processing: check if a compatible bay is available
        processQueueForAvailableStations(queue);

        // Check if our request was allocated/activated
        if (getRequestById(reqId, outRequest)) {
            if (outRequest.getStatus() == "allocated" || outRequest.getStatus() == "active") {
                outAllocated = true;
                // Query active session
                std::string getSesSql = "SELECT id, vehicle_id, ev_model, station_id, start_time, end_time, "
                                        "battery_start, battery_end, target_charge, energy_consumed, estimated_cost, "
                                        "status, user_id, request_id FROM ChargingSession WHERE request_id = ? LIMIT 1;";
                sqlite3_stmt* gSesStmt = nullptr;
                if (sqlite3_prepare_v2(handle, getSesSql.c_str(), -1, &gSesStmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(gSesStmt, 1, reqId.c_str(), -1, SQLITE_STATIC);
                    if (sqlite3_step(gSesStmt) == SQLITE_ROW) {
                        const char* sId = (const char*)sqlite3_column_text(gSesStmt, 0);
                        const char* vId = (const char*)sqlite3_column_text(gSesStmt, 1);
                        const char* mTxt = (const char*)sqlite3_column_text(gSesStmt, 2);
                        const char* stId = (const char*)sqlite3_column_text(gSesStmt, 3);
                        const char* sTime = (const char*)sqlite3_column_text(gSesStmt, 4);
                        const char* eTime = (const char*)sqlite3_column_text(gSesStmt, 5);
                        double bStart = sqlite3_column_double(gSesStmt, 6);
                        double bEnd = sqlite3_column_double(gSesStmt, 7);
                        double tChg = sqlite3_column_double(gSesStmt, 8);
                        double eCons = sqlite3_column_double(gSesStmt, 9);
                        double estCost = sqlite3_column_double(gSesStmt, 10);
                        const char* stat = (const char*)sqlite3_column_text(gSesStmt, 11);
                        int uId = sqlite3_column_int(gSesStmt, 12);
                        const char* rId = (const char*)sqlite3_column_text(gSesStmt, 13);

                        outSession = ChargingSession(sId ? sId : "", vId ? vId : "", mTxt ? mTxt : "",
                                                     stId ? stId : "", bStart, tChg, sTime ? sTime : "",
                                                     eTime ? eTime : "", bEnd, eCons, estCost, stat ? stat : "active",
                                                     uId, rId ? rId : "", outRequest.getBayId());
                    }
                    sqlite3_finalize(gSesStmt);
                }
            } else {
                outAllocated = false;
            }
        } else {
            outRequest = ChargingRequest(reqId, (userId > 0) ? userId : evOwnerId, evId, hasSpecificStation ? stationId : "", "Queue Waiting", targetCharge, "queued", "", finalConn);
            outAllocated = false;
        }

        return true;
    }

    // In-memory fallback
    std::string reqId = "REQ-" + std::to_string(s_requestCounter++);
    outRequest = ChargingRequest(reqId, userId, evId, stationId, "Bay 1", targetCharge, "allocated", "2026-10-19 10:00:00", connectorType);
    if (requestCount < MAX_REQUESTS) {
        requestArray[requestCount++] = outRequest;
    }
    outAllocated = true;
    return true;
}

bool StationService::cancelChargingRequest(const std::string& requestId,
                                          int callerUserId,
                                          const std::string& callerRole,
                                          ChargingQueue* queue) {
    if (db == nullptr || !db->isOpen()) return false;
    sqlite3* handle = db->getDbHandle();

    ChargingRequest req;
    if (!getRequestById(requestId, req)) {
        return false;
    }

    // Role check: user can only cancel their own request
    if (callerRole == "user" && callerUserId > 0 && req.getUserId() != callerUserId) {
        return false;
    }

    if (req.getStatus() == "completed" || req.getStatus() == "cancelled") {
        return false;
    }

    std::string curStatus = req.getStatus();
    std::string vId = req.getVehicleId();
    std::string sId = req.getStationId();

    if (!db->beginTransaction()) return false;

    // Update request to cancelled
    std::string updReq = "UPDATE ChargingRequest SET status = 'cancelled' WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(handle, updReq.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        db->rollbackTransaction();
        return false;
    }
    sqlite3_bind_text(stmt, 1, requestId.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Update EV back to available
    std::string updEv = "UPDATE EV SET status = 'available', last_activity = datetime('now') WHERE id = ?;";
    sqlite3_stmt* evStmt = nullptr;
    if (sqlite3_prepare_v2(handle, updEv.c_str(), -1, &evStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(evStmt, 1, vId.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(evStmt);
        sqlite3_finalize(evStmt);
    }

    // If was allocated or active, release station
    if (curStatus == "allocated" || curStatus == "active") {
        if (!sId.empty()) {
            std::string updSt = "UPDATE ChargingStation SET status = 'available', current_vehicle_id = NULL, "
                                "session_start = NULL, available_ports = 1 WHERE id = ?;";
            sqlite3_stmt* stStmt = nullptr;
            if (sqlite3_prepare_v2(handle, updSt.c_str(), -1, &stStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stStmt, 1, sId.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(stStmt);
                sqlite3_finalize(stStmt);
            }
        }
        std::string updSes = "UPDATE ChargingSession SET status = 'completed', end_time = datetime('now') WHERE request_id = ? AND status = 'active';";
        sqlite3_stmt* sesStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updSes.c_str(), -1, &sesStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(sesStmt, 1, requestId.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(sesStmt);
            sqlite3_finalize(sesStmt);
        }
    }

    db->commitTransaction();

    if (queue != nullptr) {
        queue->remove(vId);
    }

    if (curStatus == "allocated" || curStatus == "active") {
        processQueueForAvailableStations(queue);
    }

    return true;
}

bool StationService::allocateBayManually(const std::string& requestId,
                                       const std::string& stationId,
                                       ChargingRequest& outRequest,
                                       ChargingSession& outSession,
                                       ChargingQueue* queue,
                                       int callerUserId,
                                       const std::string& callerRole) {
    if (callerRole != "operator" && callerRole != "admin") {
        return false;
    }

    if (db == nullptr || !db->isOpen()) return false;
    sqlite3* handle = db->getDbHandle();

    ChargingRequest req;
    if (!getRequestById(requestId, req)) return false;
    if (req.getStatus() != "queued") return false;

    ChargingStation st;
    if (!getStationById(stationId, st)) return false;
    if (st.getStatus() != "available" || !st.getCurrentVehicleId().empty()) return false;

    std::string reqConnector = req.getConnectorType();
    if (!reqConnector.empty() && st.getConnectorType() != reqConnector) {
        return false;
    }

    if (!db->beginTransaction()) return false;

    std::string updSt = "UPDATE ChargingStation SET status = 'charging', current_vehicle_id = ?, "
                        "session_start = datetime('now'), available_ports = 0 WHERE id = ? AND status = 'available';";
    sqlite3_stmt* stStmt = nullptr;
    if (sqlite3_prepare_v2(handle, updSt.c_str(), -1, &stStmt, nullptr) != SQLITE_OK) {
        db->rollbackTransaction();
        return false;
    }
    sqlite3_bind_text(stStmt, 1, req.getVehicleId().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stStmt, 2, stationId.c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(stStmt);
    sqlite3_finalize(stStmt);
    if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
        db->rollbackTransaction();
        return false;
    }

    std::string updReq = "UPDATE ChargingRequest SET status = 'active', station_id = ?, bay_id = ? "
                         "WHERE id = ? AND status = 'queued';";
    sqlite3_stmt* reqStmt = nullptr;
    if (sqlite3_prepare_v2(handle, updReq.c_str(), -1, &reqStmt, nullptr) != SQLITE_OK) {
        db->rollbackTransaction();
        return false;
    }
    sqlite3_bind_text(reqStmt, 1, stationId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(reqStmt, 2, st.getLocation().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(reqStmt, 3, requestId.c_str(), -1, SQLITE_STATIC);
    rc = sqlite3_step(reqStmt);
    sqlite3_finalize(reqStmt);
    if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
        db->rollbackTransaction();
        return false;
    }

    std::string updEv = "UPDATE EV SET status = 'charging', last_activity = datetime('now') WHERE id = ?;";
    sqlite3_stmt* evStmt = nullptr;
    if (sqlite3_prepare_v2(handle, updEv.c_str(), -1, &evStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(evStmt, 1, req.getVehicleId().c_str(), -1, SQLITE_STATIC);
        sqlite3_step(evStmt);
        sqlite3_finalize(evStmt);
    }

    std::string sesId = "SES-" + std::to_string(s_sessionCounter++) + "-" + std::to_string(time(nullptr) % 10000);
    std::string insSes = "INSERT INTO ChargingSession (id, vehicle_id, ev_model, station_id, start_time, end_time, "
                         "battery_start, battery_end, target_charge, energy_consumed, estimated_cost, status, user_id, request_id) "
                         "VALUES (?, ?, 'Electric Vehicle', ?, datetime('now'), NULL, 20.0, NULL, ?, 0.0, 150.0, 'active', ?, ?);";
    sqlite3_stmt* sesStmt = nullptr;
    if (sqlite3_prepare_v2(handle, insSes.c_str(), -1, &sesStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(sesStmt, 1, sesId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(sesStmt, 2, req.getVehicleId().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(sesStmt, 3, stationId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(sesStmt, 4, req.getTargetCharge());
        sqlite3_bind_int(sesStmt, 5, req.getUserId());
        sqlite3_bind_text(sesStmt, 6, requestId.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(sesStmt);
        sqlite3_finalize(sesStmt);
    }

    db->commitTransaction();

    if (queue != nullptr) {
        queue->remove(req.getVehicleId());
    }

    getRequestById(requestId, outRequest);
    getSessionById(sesId, outSession);
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
                          "LEFT JOIN ChargingStation ON ChargingSession.station_id = ChargingStation.id "
                          "WHERE 1=1 ";

        if (!statusFilter.empty()) {
            sql += "AND ChargingSession.status = ? ";
        }
        if (userId > 0) {
            sql += "AND ChargingSession.user_id = ? ";
        }
        sql += "ORDER BY ChargingSession.start_time DESC;";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            int bindIndex = 1;
            if (!statusFilter.empty()) {
                sqlite3_bind_text(stmt, bindIndex++, statusFilter.c_str(), -1, SQLITE_STATIC);
            }
            if (userId > 0) {
                sqlite3_bind_int(stmt, bindIndex++, userId);
            }

            while (sqlite3_step(stmt) == SQLITE_ROW && count < maxCapacity) {
                const char* idText = (const char*)sqlite3_column_text(stmt, 0);
                const char* vehText = (const char*)sqlite3_column_text(stmt, 1);
                const char* modText = (const char*)sqlite3_column_text(stmt, 2);
                const char* stText = (const char*)sqlite3_column_text(stmt, 3);
                const char* startText = (const char*)sqlite3_column_text(stmt, 4);
                const char* endText = (const char*)sqlite3_column_text(stmt, 5);
                double bStart = sqlite3_column_double(stmt, 6);
                double bEnd = sqlite3_column_double(stmt, 7);
                double tCharge = sqlite3_column_double(stmt, 8);
                double energy = sqlite3_column_double(stmt, 9);
                double cost = sqlite3_column_double(stmt, 10);
                const char* statText = (const char*)sqlite3_column_text(stmt, 11);
                int uId = sqlite3_column_int(stmt, 12);
                const char* reqIdText = (const char*)sqlite3_column_text(stmt, 13);
                const char* bayText = (const char*)sqlite3_column_text(stmt, 14);

                outList[count] = ChargingSession(
                    idText ? idText : "",
                    vehText ? vehText : "",
                    modText ? modText : "Electric Vehicle",
                    stText ? stText : "",
                    bStart,
                    tCharge,
                    startText ? startText : "",
                    endText ? endText : "",
                    bEnd,
                    energy,
                    cost,
                    statText ? statText : "active",
                    uId,
                    reqIdText ? reqIdText : "",
                    bayText ? bayText : "Bay 1"
                );
                count++;
            }
            sqlite3_finalize(stmt);
            return count;
        }
    }

    // In-memory fallback
    for (int i = 0; i < sessionCount && count < maxCapacity; i++) {
        bool matchesStatus = statusFilter.empty() || sessionArray[i].getStatus() == statusFilter;
        bool matchesUser   = (userId <= 0) || sessionArray[i].getUserId() == userId;
        if (matchesStatus && matchesUser) {
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
                const char* idText = (const char*)sqlite3_column_text(stmt, 0);
                const char* vehText = (const char*)sqlite3_column_text(stmt, 1);
                const char* modText = (const char*)sqlite3_column_text(stmt, 2);
                const char* stText = (const char*)sqlite3_column_text(stmt, 3);
                const char* startText = (const char*)sqlite3_column_text(stmt, 4);
                const char* endText = (const char*)sqlite3_column_text(stmt, 5);
                double bStart = sqlite3_column_double(stmt, 6);
                double bEnd = sqlite3_column_double(stmt, 7);
                double tCharge = sqlite3_column_double(stmt, 8);
                double energy = sqlite3_column_double(stmt, 9);
                double cost = sqlite3_column_double(stmt, 10);
                const char* statText = (const char*)sqlite3_column_text(stmt, 11);
                int uId = sqlite3_column_int(stmt, 12);
                const char* reqIdText = (const char*)sqlite3_column_text(stmt, 13);
                const char* bayText = (const char*)sqlite3_column_text(stmt, 14);

                outSession = ChargingSession(
                    idText ? idText : "",
                    vehText ? vehText : "",
                    modText ? modText : "Electric Vehicle",
                    stText ? stText : "",
                    bStart,
                    tCharge,
                    startText ? startText : "",
                    endText ? endText : "",
                    bEnd,
                    energy,
                    cost,
                    statText ? statText : "active",
                    uId,
                    reqIdText ? reqIdText : "",
                    bayText ? bayText : "Bay 1"
                );
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
                          "WHERE ChargingSession.station_id = ? AND ChargingSession.status = 'active' "
                          "ORDER BY ChargingSession.start_time DESC LIMIT 1;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, stationId.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* idText = (const char*)sqlite3_column_text(stmt, 0);
                const char* vehText = (const char*)sqlite3_column_text(stmt, 1);
                const char* modText = (const char*)sqlite3_column_text(stmt, 2);
                const char* stText = (const char*)sqlite3_column_text(stmt, 3);
                const char* startText = (const char*)sqlite3_column_text(stmt, 4);
                const char* endText = (const char*)sqlite3_column_text(stmt, 5);
                double bStart = sqlite3_column_double(stmt, 6);
                double bEnd = sqlite3_column_double(stmt, 7);
                double tCharge = sqlite3_column_double(stmt, 8);
                double energy = sqlite3_column_double(stmt, 9);
                double cost = sqlite3_column_double(stmt, 10);
                const char* statText = (const char*)sqlite3_column_text(stmt, 11);
                int uId = sqlite3_column_int(stmt, 12);
                const char* reqIdText = (const char*)sqlite3_column_text(stmt, 13);
                const char* bayText = (const char*)sqlite3_column_text(stmt, 14);

                outSession = ChargingSession(
                    idText ? idText : "",
                    vehText ? vehText : "",
                    modText ? modText : "Electric Vehicle",
                    stText ? stText : "",
                    bStart,
                    tCharge,
                    startText ? startText : "",
                    endText ? endText : "",
                    bEnd,
                    energy,
                    cost,
                    statText ? statText : "active",
                    uId,
                    reqIdText ? reqIdText : "",
                    bayText ? bayText : "Bay 1"
                );
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

    elapsedSeconds = 0;
    if (db != nullptr && db->isOpen()) {
        sqlite3_stmt* tStmt = nullptr;
        std::string sql = "SELECT cast(max(0, strftime('%s', 'now') - strftime('%s', start_time)) as integer) FROM ChargingSession WHERE id = ?;";
        if (sqlite3_prepare_v2(db->getDbHandle(), sql.c_str(), -1, &tStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(tStmt, 1, sessionId.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(tStmt) == SQLITE_ROW) {
                int secs = sqlite3_column_int(tStmt, 0);
                if (secs >= 0) elapsedSeconds = secs;
            }
            sqlite3_finalize(tStmt);
        }
    }

    // Physical Charging Model with simulation acceleration:
    double efficiency = m_chargingEfficiency;
    double simulatedElapsedSeconds = elapsedSeconds * m_speedMultiplier;
    double simulatedElapsedHours = simulatedElapsedSeconds / 3600.0;
    double batterySidePower = powerKW * efficiency;
    double energyStored = batterySidePower * simulatedElapsedHours;
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
    currentCost = Transaction::calculateTariff(gridEnergy, "charging", m_tariffRate);

    return true;
}

int StationService::advanceActiveSessions(ChargingQueue* queue) {
    if (db == nullptr || !db->isOpen()) return 0;
    sqlite3* handle = db->getDbHandle();

    // Query active sessions from SQLite
    std::string sql = "SELECT id, vehicle_id, station_id, battery_start, target_charge, "
                      "cast(max(0, strftime('%s', 'now') - strftime('%s', start_time)) as integer) "
                      "FROM ChargingSession WHERE status = 'active';";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    struct ActiveItem {
        std::string sessionId;
        std::string evId;
        std::string stationId;
        double batteryStart;
        double targetCharge;
        int elapsedSeconds;
    };
    std::vector<ActiveItem> items;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* sId = (const char*)sqlite3_column_text(stmt, 0);
        const char* vId = (const char*)sqlite3_column_text(stmt, 1);
        const char* stId = (const char*)sqlite3_column_text(stmt, 2);
        double bStart = sqlite3_column_double(stmt, 3);
        double tChg = sqlite3_column_double(stmt, 4);
        int elapsed = sqlite3_column_int(stmt, 5);

        items.push_back({
            sId ? sId : "",
            vId ? vId : "",
            stId ? stId : "",
            bStart,
            (tChg > 0.0 && tChg <= 100.0) ? tChg : 85.0,
            elapsed
        });
    }
    sqlite3_finalize(stmt);

    int autoCompleted = 0;

    for (const auto& item : items) {
        // Query station power rating
        double powerKW = 25.0; // standard fallback
        ChargingStation st;
        if (getStationById(item.stationId, st) && st.getPower() > 0.0) {
            powerKW = st.getPower();
        }

        // Query EV battery capacity
        double capacityKWh = 50.0;
        std::string evCapSql = "SELECT battery_capacity FROM EV WHERE id = ?;";
        sqlite3_stmt* evStmt = nullptr;
        if (sqlite3_prepare_v2(handle, evCapSql.c_str(), -1, &evStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(evStmt, 1, item.evId.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(evStmt) == SQLITE_ROW) {
                double cap = sqlite3_column_double(evStmt, 0);
                if (cap > 0.0) capacityKWh = cap;
            }
            sqlite3_finalize(evStmt);
        }

        // Physical Constant-Power charging physics with configurable demonstration speed:
        double efficiency = m_chargingEfficiency;
        double simulatedElapsedSeconds = item.elapsedSeconds * m_speedMultiplier;
        double simulatedElapsedHours = simulatedElapsedSeconds / 3600.0;
        double batterySidePower = powerKW * efficiency;
        double energyStored = batterySidePower * simulatedElapsedHours;
        double currentSoC = item.batteryStart + (energyStored / capacityKWh) * 100.0;

        if (currentSoC < item.batteryStart) currentSoC = item.batteryStart;
        if (currentSoC > item.targetCharge) currentSoC = item.targetCharge;
        if (currentSoC > 100.0) currentSoC = 100.0;

        double energyDelivered = ((std::min(currentSoC, item.targetCharge) - item.batteryStart) / 100.0) * capacityKWh;
        if (energyDelivered < 0.0) energyDelivered = 0.0;
        double gridEnergy = (efficiency > 0.0) ? (energyDelivered / efficiency) : energyDelivered;
        double currentCost = Transaction::calculateTariff(gridEnergy, "charging", m_tariffRate);

        if (currentSoC >= item.targetCharge) {
            // Target charge reached! Auto-complete session and automatically dispatch FIFO queue
            ChargingSession completedSession;
            Transaction completedTxn;
            if (completeChargingSession(item.sessionId, completedSession, completedTxn, queue)) {
                autoCompleted++;
            }
        } else {
            // Persist progressive metrics in SQLite for EV and Session
            std::string updEv = "UPDATE EV SET current_charge = ?, last_activity = datetime('now') WHERE id = ?;";
            sqlite3_stmt* uEvStmt = nullptr;
            if (sqlite3_prepare_v2(handle, updEv.c_str(), -1, &uEvStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_double(uEvStmt, 1, currentSoC);
                sqlite3_bind_text(uEvStmt, 2, item.evId.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(uEvStmt);
                sqlite3_finalize(uEvStmt);
            }

            std::string updSes = "UPDATE ChargingSession SET battery_end = ?, energy_consumed = ?, estimated_cost = ? WHERE id = ? AND status = 'active';";
            sqlite3_stmt* uSesStmt = nullptr;
            if (sqlite3_prepare_v2(handle, updSes.c_str(), -1, &uSesStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_double(uSesStmt, 1, currentSoC);
                sqlite3_bind_double(uSesStmt, 2, energyDelivered);
                sqlite3_bind_double(uSesStmt, 3, currentCost);
                sqlite3_bind_text(uSesStmt, 4, item.sessionId.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(uSesStmt);
                sqlite3_finalize(uSesStmt);
            }
        }
    }

    return autoCompleted;
}

bool StationService::completeChargingSession(const std::string& stationOrSessionId,
                                            ChargingSession& outCompletedSession,
                                            Transaction& outTxn,
                                            ChargingQueue* queue,
                                            ChargingSession* outDispatchedSession,
                                            int callerUserId,
                                            const std::string& callerRole) {
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

        // Ownership / RBAC Authorization Guard: User cannot complete another customer's session
        if (callerRole == "user" && callerUserId > 0) {
            int sessionOwnerId = activeSession.getUserId();
            if (sessionOwnerId > 0 && sessionOwnerId != callerUserId) {
                return false;
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
        double batteryEnergyDelivered = ((finalSoC - batteryStart) / 100.0) * capacity;
        if (batteryEnergyDelivered < 0.0) batteryEnergyDelivered = 0.0;
        double gridEnergyConsumed = (m_chargingEfficiency > 0.0) ? (batteryEnergyDelivered / m_chargingEfficiency) : batteryEnergyDelivered;
        double finalAmount = Transaction::calculateTariff(gridEnergyConsumed, "charging", m_tariffRate);
        double energyDelivered = gridEnergyConsumed;

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
            db->rollbackTransaction();
            return false;
        }

        // B. Update completed EV to available with finalSoC
        std::string updEvSql = "UPDATE EV SET status = 'available', current_charge = ?, last_activity = datetime('now') WHERE id = ?;";
        sqlite3_stmt* updEvStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updEvSql.c_str(), -1, &updEvStmt, nullptr) != SQLITE_OK) {
            db->rollbackTransaction();
            return false;
        }
        sqlite3_bind_double(updEvStmt, 1, finalSoC);
        sqlite3_bind_text(updEvStmt, 2, evId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(updEvStmt);
        sqlite3_finalize(updEvStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            db->rollbackTransaction();
            return false;
        }

        // C. Insert TransactionLog record with user_id and session_id
        std::string insTxnSql = "INSERT INTO TransactionLog (id, vehicle_id, type, station_or_battery, "
                                "description, amount, energy_delivered, duration, timestamp, status, user_id, session_id) "
                                "VALUES (?, ?, 'charging', ?, ?, ?, ?, "
                                "?, "
                                "datetime('now'), 'completed', ?, ?);";
        sqlite3_stmt* insTxnStmt = nullptr;
        if (sqlite3_prepare_v2(handle, insTxnSql.c_str(), -1, &insTxnStmt, nullptr) != SQLITE_OK) {
            db->rollbackTransaction();
            return false;
        }
        sqlite3_bind_text(insTxnStmt, 1, txnId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insTxnStmt, 2, evId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insTxnStmt, 3, stationId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insTxnStmt, 4, desc.c_str(), -1, SQLITE_STATIC);
        // Compute physical simulated duration in minutes
        int realElapsedSecs = 60;
        std::string durSql = "SELECT cast(max(1, strftime('%s', 'now') - strftime('%s', start_time)) as integer) FROM ChargingSession WHERE id = ?;";
        sqlite3_stmt* dStmt = nullptr;
        if (sqlite3_prepare_v2(handle, durSql.c_str(), -1, &dStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(dStmt, 1, sessionId.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(dStmt) == SQLITE_ROW) {
                realElapsedSecs = sqlite3_column_int(dStmt, 0);
            }
            sqlite3_finalize(dStmt);
        }
        int simulatedDurationMins = static_cast<int>(std::round((realElapsedSecs * m_speedMultiplier) / 60.0));
        if (simulatedDurationMins < 1) simulatedDurationMins = 1;

        sqlite3_bind_double(insTxnStmt, 5, finalAmount);
        sqlite3_bind_double(insTxnStmt, 6, energyDelivered);
        sqlite3_bind_int(insTxnStmt, 7, simulatedDurationMins);
        sqlite3_bind_int(insTxnStmt, 8, finalUserId);
        sqlite3_bind_text(insTxnStmt, 9, sessionId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(insTxnStmt);
        sqlite3_finalize(insTxnStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
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

        // E. Release Station back to available
        std::string freeStSql = "UPDATE ChargingStation SET status = 'available', current_vehicle_id = NULL, "
                                "session_start = NULL, available_ports = 1 WHERE id = ?;";
        sqlite3_stmt* freeStStmt = nullptr;
        if (sqlite3_prepare_v2(handle, freeStSql.c_str(), -1, &freeStStmt, nullptr) != SQLITE_OK) {
            db->rollbackTransaction();
            return false;
        }
        sqlite3_bind_text(freeStStmt, 1, stationId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(freeStStmt);
        sqlite3_finalize(freeStStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            db->rollbackTransaction();
            return false;
        }

        db->commitTransaction();

        // F. Trigger Automatic FIFO Queue Processing for newly freed bay
        processQueueForAvailableStations(queue);

        // Check if an eligible waiting request was auto-dispatched to this released station
        if (outDispatchedSession != nullptr) {
            ChargingSession newlyActive;
            if (getActiveSessionByStationId(stationId, newlyActive)) {
                *outDispatchedSession = newlyActive;
            }
        }

        outCompletedSession = activeSession;
        outCompletedSession.setStatus("completed");
        outCompletedSession.setBatteryEnd(finalSoC);
        outCompletedSession.setEnergyConsumed(energyDelivered);
        outCompletedSession.setEstimatedCost(finalAmount);

        outTxn = Transaction(txnId, evId, "charging", stationId, desc, finalAmount, energyDelivered, simulatedDurationMins, "", "completed", finalUserId, sessionId);
        return true;
    }

    // In-memory fallback
    for (int i = 0; i < sessionCount; i++) {
        if ((sessionArray[i].getStationId() == stationOrSessionId || sessionArray[i].getId() == stationOrSessionId) &&
            sessionArray[i].getStatus() == "active") {

            sessionArray[i].setStatus("completed");
            sessionArray[i].setBatteryEnd(85.0);
            sessionArray[i].setEndTime("2026-10-19 11:30:00");

            releaseStation(sessionArray[i].getStationId(), queue);

            outCompletedSession = sessionArray[i];
            static int inMemTxnCounter = 9000;
            outTxn = Transaction("TXN-" + std::to_string(inMemTxnCounter++), sessionArray[i].getVehicleId(),
                                 "charging", sessionArray[i].getStationId(), "Charging completed",
                                 sessionArray[i].getEstimatedCost(), 30.0, 60, "2026-10-19 11:30:00",
                                 "completed", sessionArray[i].getUserId(), sessionArray[i].getId());
            return true;
        }
    }
    return false;
}

bool StationService::enqueueWaitingEV(const std::string& evId, ChargingQueue& queue) {
    if (queue.isFull()) return false;
    if (queue.contains(evId)) return false;

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE EV SET status = 'waiting', last_activity = datetime('now') WHERE id = ?;";
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

int StationService::rehydrateChargingQueue(ChargingQueue& queue) {
    queue.clear();
    int count = 0;

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        // Strict FIFO ordering by created_at / last_activity
        std::string sql = "SELECT id FROM EV WHERE status = 'waiting' ORDER BY last_activity ASC, registered_at ASC, rowid ASC;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW && count < CHARGING_QUEUE_CAPACITY) {
                const char* idText = (const char*)sqlite3_column_text(stmt, 0);
                if (idText) {
                    queue.enqueue(idText);
                    count++;
                }
            }
            sqlite3_finalize(stmt);
        }
    } else {
        queue.enqueue("EV-102");
        queue.enqueue("EV-104");
        count = 2;
    }

    return count;
}
