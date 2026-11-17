#include "station_service.h"
#include <iostream>
#include <ctime>
#include <cmath>
#include <algorithm>

static int s_sessionCounter = 1010;

StationService::StationService(Database* database) {
    db = database;
    stationCount = 0;
    sessionCount = 0;

    // Initialize with default demo stations if starting without database
    stationArray[0] = ChargingStation("CS-1", "Standard Bay 1", 25.0, "CCS2", "available", "", "Bay A-1", "", 1, 1);
    stationArray[1] = ChargingStation("CS-2", "Fast Bay 2", 29.0, "CCS2", "available", "", "Bay A-2", "", 1, 1);
    stationArray[2] = ChargingStation("CS-3", "Flex Bay 3", 28.0, "CCS2", "available", "", "Bay B-1", "", 1, 1);
    stationArray[3] = ChargingStation("CS-4", "AC Type 2 Charger 4", 26.0, "Type 2", "maintenance", "", "Bay B-2", "", 1, 0);
    stationCount = 4;

    sessionArray[0] = ChargingSession("SES-1001", "EV-101", "Tata Nexon EV Max", "CS-1", 12.0, 90.0, "2026-10-19 10:00:00", "", 0.0, 14.5, 290.0, "active", 25);
    sessionArray[1] = ChargingSession("SES-1000", "EV-103", "Hyundai Ioniq 5", "CS-2", 20.0, 80.0, "2026-10-19 08:30:00", "2026-10-19 09:30:00", 80.0, 43.5, 870.0, "completed", 60);
    sessionCount = 2;
}

// ─── Station Management (Phase 2 & Phase 3) ────────────────────────────────

bool StationService::createStation(const ChargingStation& station, std::string& outError) {
    std::string sId = station.getId();
    if (sId.empty()) {
        outError = "Station ID cannot be empty";
        return false;
    }
    if (station.getName().empty()) {
        outError = "Station name cannot be empty";
        return false;
    }
    if (station.getPower() <= 0.0) {
        outError = "Power rating must be greater than 0 kW";
        return false;
    }

    std::string conn = station.getConnectorType();
    if (conn != "Type 2" && conn != "CCS2" && conn != "CHAdeMO" && conn != "GB/T") {
        conn = "CCS2";
    }

    std::string stat = ChargingStation::normalizeStatus(station.getStatus());

    // Check if station with this ID already exists
    ChargingStation existing;
    if (getStationById(sId, existing)) {
        outError = "Station with ID '" + sId + "' already exists";
        return false;
    }

    // Persist to SQLite if connected
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "INSERT INTO ChargingStation (id, name, power, connector_type, status, current_vehicle_id, session_start, location, total_ports, available_ports) "
                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            outError = "Database error preparing station insert statement";
            return false;
        }

        sqlite3_bind_text(stmt, 1, sId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, station.getName().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 3, station.getPower());
        sqlite3_bind_text(stmt, 4, conn.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, stat.c_str(), -1, SQLITE_STATIC);

        if (station.getCurrentVehicleId().empty()) {
            sqlite3_bind_null(stmt, 6);
        } else {
            sqlite3_bind_text(stmt, 6, station.getCurrentVehicleId().c_str(), -1, SQLITE_STATIC);
        }

        if (station.getSessionStart().empty()) {
            sqlite3_bind_null(stmt, 7);
        } else {
            sqlite3_bind_text(stmt, 7, station.getSessionStart().c_str(), -1, SQLITE_STATIC);
        }

        sqlite3_bind_text(stmt, 8, station.getLocation().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 9, station.getTotalPorts());
        sqlite3_bind_int(stmt, 10, station.getAvailablePorts());

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE) {
            outError = "Failed to insert station into SQLite database";
            return false;
        }
    }

    // Update in-memory local array
    if (stationCount < MAX_STATIONS) {
        ChargingStation storedStation = station;
        storedStation.setConnectorType(conn);
        storedStation.setStatus(stat);
        stationArray[stationCount++] = storedStation;
    }

    return true;
}

bool StationService::addStation(const ChargingStation& station) {
    std::string dummyErr;
    return createStation(station, dummyErr);
}

int StationService::getAllStations(ChargingStation outList[], int maxCapacity) {
    int count = 0;

    // Load from database if connected
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id, name, power, connector_type, status, current_vehicle_id, location, session_start, total_ports, available_ports FROM ChargingStation ORDER BY id ASC;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW && count < maxCapacity) {
                const char* idText       = (const char*)sqlite3_column_text(stmt, 0);
                const char* nameText     = (const char*)sqlite3_column_text(stmt, 1);
                double power             = sqlite3_column_double(stmt, 2);
                const char* connText     = (const char*)sqlite3_column_text(stmt, 3);
                const char* statusText   = (const char*)sqlite3_column_text(stmt, 4);
                const char* vehText      = (const char*)sqlite3_column_text(stmt, 5);
                const char* locText      = (const char*)sqlite3_column_text(stmt, 6);
                const char* sesStartText = (const char*)sqlite3_column_text(stmt, 7);
                int tPorts               = sqlite3_column_int(stmt, 8);
                int aPorts               = sqlite3_column_int(stmt, 9);

                std::string id           = idText ? idText : "";
                std::string name         = nameText ? nameText : "";
                std::string conn         = connText ? connText : "CCS2";
                std::string status       = statusText ? statusText : "available";
                std::string vehicleId    = vehText ? vehText : "";
                std::string location     = locText ? locText : "Bay 1";
                std::string sessionStart = sesStartText ? sesStartText : "";

                outList[count] = ChargingStation(id, name, power, conn, status, vehicleId, location, sessionStart, tPorts, aPorts);
                count++;
            }
            sqlite3_finalize(stmt);
            return count;
        }
    }

    // Fallback: copy from in-memory array
    for (int i = 0; i < stationCount && i < maxCapacity; i++) {
        outList[i] = stationArray[i];
        count++;
    }

    return count;
}

bool StationService::getStationById(const std::string& id, ChargingStation& outStation) {
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id, name, power, connector_type, status, current_vehicle_id, location, session_start, total_ports, available_ports FROM ChargingStation WHERE id = ? LIMIT 1;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* idText       = (const char*)sqlite3_column_text(stmt, 0);
                const char* nameText     = (const char*)sqlite3_column_text(stmt, 1);
                double power             = sqlite3_column_double(stmt, 2);
                const char* connText     = (const char*)sqlite3_column_text(stmt, 3);
                const char* statusText   = (const char*)sqlite3_column_text(stmt, 4);
                const char* vehText      = (const char*)sqlite3_column_text(stmt, 5);
                const char* locText      = (const char*)sqlite3_column_text(stmt, 6);
                const char* sesStartText = (const char*)sqlite3_column_text(stmt, 7);
                int tPorts               = sqlite3_column_int(stmt, 8);
                int aPorts               = sqlite3_column_int(stmt, 9);

                outStation = ChargingStation(idText ? idText : "",
                                             nameText ? nameText : "",
                                             power,
                                             connText ? connText : "CCS2",
                                             statusText ? statusText : "available",
                                             vehText ? vehText : "",
                                             locText ? locText : "Bay 1",
                                             sesStartText ? sesStartText : "",
                                             tPorts, aPorts);
                sqlite3_finalize(stmt);
                return true;
            }
            sqlite3_finalize(stmt);
        }
    }

    // In-memory fallback
    for (int i = 0; i < stationCount; i++) {
        if (stationArray[i].getId() == id) {
            outStation = stationArray[i];
            return true;
        }
    }
    return false;
}

bool StationService::updateStationStatus(const std::string& id, const std::string& newStatus, std::string& outError) {
    if (!ChargingStation::isValidStatus(newStatus)) {
        outError = "Invalid station status: '" + newStatus + "'. Supported: available, charging, maintenance, offline";
        return false;
    }

    std::string normStatus = ChargingStation::normalizeStatus(newStatus);

    ChargingStation currentStation;
    if (!getStationById(id, currentStation)) {
        outError = "Station not found with ID: " + id;
        return false;
    }

    std::string currentStatus = currentStation.getStatus();

    // ── Validated State Transition Rules (Phase 3) ─────────────────────
    // Rule 1: A charging station cannot be manually marked available while its session is active.
    // A session must be completed before releasing its station.
    if (currentStatus == "charging") {
        if (normStatus == "available") {
            outError = "Cannot manually mark station available while charging session is active. Session must be completed first.";
            return false;
        }
        if (normStatus == "maintenance" || normStatus == "offline") {
            outError = "Cannot transition station to " + normStatus + " while actively charging an EV. Complete the session first.";
            return false;
        }
        if (normStatus == "charging") {
            return true; // No change
        }
    }

    // Rule 2: Cannot manually mark station as 'charging' without session allocation.
    if (normStatus == "charging" && currentStatus != "charging") {
        outError = "Cannot set station status to charging manually. Station must be allocated through charging request.";
        return false;
    }

    // Determine available ports based on target operational status
    int availPorts = (normStatus == "available" && currentStation.getCurrentVehicleId().empty()) ? currentStation.getTotalPorts() : 0;

    // Update in database if connected
    bool dbUpdated = false;
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE ChargingStation SET status = ?, available_ports = ? WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, normStatus.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, availPorts);
            sqlite3_bind_text(stmt, 3, id.c_str(), -1, SQLITE_STATIC);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc == SQLITE_DONE && sqlite3_changes(handle) > 0) {
                dbUpdated = true;
            }
        }
    }

    // Update in local in-memory array
    for (int i = 0; i < stationCount; i++) {
        if (stationArray[i].getId() == id) {
            stationArray[i].setStatus(normStatus);
            stationArray[i].setAvailablePorts(availPorts);
            return true;
        }
    }

    return dbUpdated;
}

bool StationService::updateStationStatus(const std::string& id, const std::string& newStatus) {
    std::string dummyErr;
    return updateStationStatus(id, newStatus, dummyErr);
}

bool StationService::assignVehicle(const std::string& stationId, const std::string& vehicleId) {
    bool dbUpdated = false;
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE ChargingStation SET status = 'charging', current_vehicle_id = ?, session_start = datetime('now'), available_ports = 0 WHERE id = ?;";
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
            stationArray[i].setAvailablePorts(0);
            return true;
        }
    }
    return dbUpdated;
}

bool StationService::releaseStation(const std::string& stationId) {
    bool dbUpdated = false;
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE ChargingStation SET status = 'available', current_vehicle_id = NULL, session_start = NULL, available_ports = total_ports WHERE id = ?;";
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
            stationArray[i].setAvailablePorts(stationArray[i].getTotalPorts());
            return true;
        }
    }
    return dbUpdated;
}

bool StationService::findAvailableStation(ChargingStation& outStation) {
    ChargingStation all[MAX_STATIONS];
    int count = getAllStations(all, MAX_STATIONS);
    for (int i = 0; i < count; i++) {
        // Only available stations can accept EVs. Maintenance and offline stations are rejected.
        if (all[i].isAvailable()) {
            outStation = all[i];
            return true;
        }
    }
    return false;
}

// ─── Charging Session Management (Phase 4 & Phase 5) ───────────────────────

int StationService::getAllSessions(ChargingSession outList[], int maxCapacity, const std::string& statusFilter) {
    int count = 0;

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql;
        if (statusFilter.empty()) {
            sql = "SELECT id, vehicle_id, ev_model, station_id, start_time, end_time, "
                  "battery_start, battery_end, target_charge, energy_consumed, estimated_cost, status, "
                  "max(1, cast((strftime('%s', coalesce(end_time, 'now')) - strftime('%s', start_time)) / 60 as integer)) "
                  "FROM ChargingSession ORDER BY start_time DESC;";
        } else {
            sql = "SELECT id, vehicle_id, ev_model, station_id, start_time, end_time, "
                  "battery_start, battery_end, target_charge, energy_consumed, estimated_cost, status, "
                  "max(1, cast((strftime('%s', coalesce(end_time, 'now')) - strftime('%s', start_time)) / 60 as integer)) "
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
                int dur               = sqlite3_column_int(stmt, 12);

                std::string id        = idText ? idText : "";
                std::string vehicleId = vIdText ? vIdText : "";
                std::string evModel   = mText ? mText : "";
                std::string stationId = sIdText ? sIdText : "";
                std::string startTime = sTimeText ? sTimeText : "";
                std::string endTime   = eTimeText ? eTimeText : "";
                std::string status    = statText ? statText : "active";

                outList[count] = ChargingSession(id, vehicleId, evModel, stationId, bStart, target,
                                                 startTime, endTime, bEnd, energy, cost, status, dur);
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
                          "battery_start, battery_end, target_charge, energy_consumed, estimated_cost, status, "
                          "max(1, cast((strftime('%s', coalesce(end_time, 'now')) - strftime('%s', start_time)) / 60 as integer)) "
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
                int dur               = sqlite3_column_int(stmt, 12);

                outSession = ChargingSession(idText ? idText : "",
                                             vIdText ? vIdText : "",
                                             mText ? mText : "",
                                             sIdText ? sIdText : "",
                                             bStart, target,
                                             sTimeText ? sTimeText : "",
                                             eTimeText ? eTimeText : "",
                                             bEnd, energy, cost,
                                             statText ? statText : "active", dur);
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
                          "battery_start, battery_end, target_charge, energy_consumed, estimated_cost, status, "
                          "max(1, cast((strftime('%s', 'now') - strftime('%s', start_time)) / 60 as integer)) "
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
                int dur               = sqlite3_column_int(stmt, 12);

                outSession = ChargingSession(idText ? idText : "",
                                             vIdText ? vIdText : "",
                                             mText ? mText : "",
                                             sIdText ? sIdText : "",
                                             bStart, target,
                                             sTimeText ? sTimeText : "",
                                             eTimeText ? eTimeText : "",
                                             bEnd, energy, cost,
                                             statText ? statText : "active", dur);
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

bool StationService::createChargingSession(const std::string& evId,
                                          const std::string& stationId,
                                          double targetCharge,
                                          ChargingSession& outSession,
                                          std::string& outError) {
    if (evId.empty()) {
        outError = "EV ID cannot be empty";
        return false;
    }
    if (stationId.empty()) {
        outError = "Station ID cannot be empty";
        return false;
    }

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();

        // 1. Validate EV exists and is not currently charging/swapping
        std::string evSql = "SELECT model, battery_capacity, current_charge, status FROM EV WHERE id = ?;";
        sqlite3_stmt* evStmt = nullptr;
        std::string evModel = "EV";
        double capacity = 50.0;
        double currentCharge = 20.0;
        std::string evStatus = "";

        if (sqlite3_prepare_v2(handle, evSql.c_str(), -1, &evStmt, nullptr) != SQLITE_OK) {
            outError = "Database error verifying vehicle";
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
            outError = "Vehicle " + evId + " not found";
            return false;
        }
        sqlite3_finalize(evStmt);

        if (evStatus == "charging" || evStatus == "swapping") {
            outError = "Vehicle " + evId + " already has an active session (status: " + evStatus + ")";
            return false;
        }

        // Also check if EV already has an active session in ChargingSession table
        std::string dupSql = "SELECT id FROM ChargingSession WHERE vehicle_id = ? AND status = 'active' LIMIT 1;";
        sqlite3_stmt* dupStmt = nullptr;
        if (sqlite3_prepare_v2(handle, dupSql.c_str(), -1, &dupStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(dupStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(dupStmt) == SQLITE_ROW) {
                sqlite3_finalize(dupStmt);
                outError = "Vehicle " + evId + " already has an active charging session";
                return false;
            }
            sqlite3_finalize(dupStmt);
        }

        // 2. Validate Station exists and is available
        std::string stSql = "SELECT status, current_vehicle_id FROM ChargingStation WHERE id = ?;";
        sqlite3_stmt* stStmt = nullptr;
        std::string stStatus = "";
        std::string stCurVeh = "";
        if (sqlite3_prepare_v2(handle, stSql.c_str(), -1, &stStmt, nullptr) != SQLITE_OK) {
            outError = "Database error verifying station";
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
            outError = "Station " + stationId + " not found";
            return false;
        }
        sqlite3_finalize(stStmt);

        if (stStatus != "available" || !stCurVeh.empty()) {
            outError = "Station " + stationId + " is not available (current status: " + stStatus + ")";
            return false;
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
            outError = "Failed to begin database transaction";
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
            outError = "Failed to prepare session insert";
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
            outError = "Failed to insert charging session record";
            return false;
        }

        // B. Update ChargingStation
        std::string updStSql = "UPDATE ChargingStation SET status = 'charging', current_vehicle_id = ?, "
                               "session_start = datetime('now'), available_ports = 0 WHERE id = ?;";
        sqlite3_stmt* updStStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updStSql.c_str(), -1, &updStStmt, nullptr) != SQLITE_OK) {
            db->rollbackTransaction();
            outError = "Failed to prepare station update";
            return false;
        }
        sqlite3_bind_text(updStStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(updStStmt, 2, stationId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(updStStmt);
        sqlite3_finalize(updStStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            db->rollbackTransaction();
            outError = "Failed to update station state in database";
            return false;
        }

        // C. Update EV status
        std::string updEvSql = "UPDATE EV SET status = 'charging', last_activity = datetime('now') WHERE id = ?;";
        sqlite3_stmt* updEvStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updEvSql.c_str(), -1, &updEvStmt, nullptr) != SQLITE_OK) {
            db->rollbackTransaction();
            outError = "Failed to prepare EV status update";
            return false;
        }
        sqlite3_bind_text(updEvStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(updEvStmt);
        sqlite3_finalize(updEvStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            db->rollbackTransaction();
            outError = "Failed to update vehicle state in database";
            return false;
        }

        if (!db->commitTransaction()) {
            db->rollbackTransaction();
            outError = "Failed to commit database transaction";
            return false;
        }

        // Retrieve recorded session
        if (!getSessionById(sesId, outSession)) {
            outSession = ChargingSession(sesId, evId, evModel, stationId, currentCharge, targetCharge,
                                         "", "", 0.0, 0.0, costEst, "active", 0);
        }
        return true;
    }

    // In-memory fallback
    for (int i = 0; i < stationCount; i++) {
        if (stationArray[i].getId() == stationId && stationArray[i].isAvailable()) {
            stationArray[i].setStatus("charging");
            stationArray[i].setCurrentVehicleId(evId);
            stationArray[i].setAvailablePorts(0);

            std::string sesId = "SES-" + std::to_string(1000 + sessionCount);
            outSession = ChargingSession(sesId, evId, "Electric Vehicle", stationId, 25.0, targetCharge,
                                         "2026-10-19 10:00:00", "", 0.0, 0.0, 150.0, "active", 0);
            if (sessionCount < MAX_SESSIONS) {
                sessionArray[sessionCount++] = outSession;
            }
            return true;
        }
    }
    outError = "No matching available station found in memory";
    return false;
}

bool StationService::createChargingSession(const std::string& evId,
                                          const std::string& stationId,
                                          double targetCharge,
                                          ChargingSession& outSession) {
    std::string dummyErr;
    return createChargingSession(evId, stationId, targetCharge, outSession, dummyErr);
}

bool StationService::completeChargingSession(const std::string& stationOrSessionId,
                                            ChargingSession& outCompletedSession,
                                            Transaction& outTxn,
                                            ChargingQueue* queue,
                                            ChargingSession* outDispatchedSession,
                                            std::string& outError) {
    ChargingQueue queueBackup;
    if (queue != nullptr) {
        queueBackup = *queue; // Snapshot in-memory queue to guarantee rollback safety
    }

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();

        // 1. Resolve active session by sessionId or stationId
        ChargingSession activeSession;
        bool foundSession = false;

        if (stationOrSessionId.rfind("SES-", 0) == 0) {
            // Identifier is a session ID
            if (getSessionById(stationOrSessionId, activeSession) && activeSession.isActive()) {
                foundSession = true;
            }
        } else {
            // Identifier is a station ID
            if (getActiveSessionByStationId(stationOrSessionId, activeSession)) {
                foundSession = true;
            }
        }

        if (!foundSession) {
            outError = "No active charging session found for identifier '" + stationOrSessionId + "'";
            return false; // Prevents double completion
        }

        std::string sessionId = activeSession.getId();
        std::string stationId = activeSession.getStationId();
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

        // 3. Compute energy consumed and billing tariff
        double batteryEnd = targetCharge;
        double energyDelivered = ((batteryEnd - batteryStart) / 100.0) * capacity;
        if (energyDelivered < 0.0) energyDelivered = 0.0;
        double finalAmount = Transaction::calculateTariff(energyDelivered, "charging");

        static int txCounter = 6000;
        std::string txnId = "TXN-" + std::to_string(txCounter++) + "-" + std::to_string(time(nullptr) % 10000);
        std::string desc = "Charging completed " + std::to_string((int)batteryStart) + "% to " + std::to_string((int)batteryEnd) + "%";

        // 4. Begin Atomic SQLite Transaction
        if (!db->beginTransaction()) {
            outError = "Failed to begin database transaction";
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
            outError = "Failed to prepare session completion statement";
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
            outError = "Failed to update session or session was already completed (double completion rejected)";
            return false;
        }

        // B. Update completed EV to available with final target SoC
        std::string updEvSql = "UPDATE EV SET status = 'available', current_charge = ?, last_activity = datetime('now') WHERE id = ?;";
        sqlite3_stmt* updEvStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updEvSql.c_str(), -1, &updEvStmt, nullptr) != SQLITE_OK) {
            if (queue != nullptr) *queue = queueBackup;
            db->rollbackTransaction();
            outError = "Failed to prepare EV update statement";
            return false;
        }
        sqlite3_bind_double(updEvStmt, 1, batteryEnd);
        sqlite3_bind_text(updEvStmt, 2, evId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(updEvStmt);
        sqlite3_finalize(updEvStmt);
        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            if (queue != nullptr) *queue = queueBackup;
            db->rollbackTransaction();
            outError = "Failed to update completed vehicle state";
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
            outError = "Failed to prepare transaction log insert statement";
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
            outError = "Failed to insert transaction log record";
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
                        outError = "Failed to prepare next session insert";
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
                        outError = "Failed to insert next dispatched session";
                        return false;
                    }

                    // Update Station to new vehicle
                    std::string updStNext = "UPDATE ChargingStation SET status = 'charging', current_vehicle_id = ?, "
                                           "session_start = datetime('now'), available_ports = 0 WHERE id = ?;";
                    sqlite3_stmt* updStNextStmt = nullptr;
                    if (sqlite3_prepare_v2(handle, updStNext.c_str(), -1, &updStNextStmt, nullptr) != SQLITE_OK) {
                        if (queue != nullptr) *queue = queueBackup;
                        db->rollbackTransaction();
                        outError = "Failed to prepare station dispatch update";
                        return false;
                    }
                    sqlite3_bind_text(updStNextStmt, 1, waitingEvId.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_text(updStNextStmt, 2, stationId.c_str(), -1, SQLITE_STATIC);
                    rc = sqlite3_step(updStNextStmt);
                    sqlite3_finalize(updStNextStmt);
                    if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
                        if (queue != nullptr) *queue = queueBackup;
                        db->rollbackTransaction();
                        outError = "Failed to update station state for dispatched EV";
                        return false;
                    }

                    // Update next EV to charging
                    std::string updEvNext = "UPDATE EV SET status = 'charging', last_activity = datetime('now') WHERE id = ?;";
                    sqlite3_stmt* updEvNextStmt = nullptr;
                    if (sqlite3_prepare_v2(handle, updEvNext.c_str(), -1, &updEvNextStmt, nullptr) != SQLITE_OK) {
                        if (queue != nullptr) *queue = queueBackup;
                        db->rollbackTransaction();
                        outError = "Failed to update dispatched EV status";
                        return false;
                    }
                    sqlite3_bind_text(updEvNextStmt, 1, waitingEvId.c_str(), -1, SQLITE_STATIC);
                    rc = sqlite3_step(updEvNextStmt);
                    sqlite3_finalize(updEvNextStmt);
                    if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
                        if (queue != nullptr) *queue = queueBackup;
                        db->rollbackTransaction();
                        outError = "Failed to update dispatched EV status";
                        return false;
                    }

                    dispatched = true;
                    if (outDispatchedSession != nullptr) {
                        *outDispatchedSession = ChargingSession(nextSesId, waitingEvId, nextModel, stationId,
                                                                nextCharge, nextTarget, "", "", 0.0, 0.0, nextCost, "active", 0);
                    }
                    break;
                }
            }
        }

        if (!dispatched) {
            // No waiting EV dispatched: station is freed back to available
            std::string freeStSql = "UPDATE ChargingStation SET status = 'available', current_vehicle_id = NULL, "
                                    "session_start = NULL, available_ports = total_ports WHERE id = ?;";
            sqlite3_stmt* freeStStmt = nullptr;
            if (sqlite3_prepare_v2(handle, freeStSql.c_str(), -1, &freeStStmt, nullptr) != SQLITE_OK) {
                if (queue != nullptr) *queue = queueBackup;
                db->rollbackTransaction();
                outError = "Failed to prepare station release statement";
                return false;
            }
            sqlite3_bind_text(freeStStmt, 1, stationId.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_step(freeStStmt);
            sqlite3_finalize(freeStStmt);
            if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
                if (queue != nullptr) *queue = queueBackup;
                db->rollbackTransaction();
                outError = "Failed to release charging station";
                return false;
            }
        }

        if (!db->commitTransaction()) {
            if (queue != nullptr) *queue = queueBackup;
            db->rollbackTransaction();
            outError = "Failed to commit completion transaction";
            return false;
        }

        // Fetch duration in minutes
        int durationMinutes = 15;
        std::string durSql = "SELECT duration FROM TransactionLog WHERE id = ?;";
        sqlite3_stmt* durStmt = nullptr;
        if (sqlite3_prepare_v2(handle, durSql.c_str(), -1, &durStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(durStmt, 1, txnId.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(durStmt) == SQLITE_ROW) {
                durationMinutes = sqlite3_column_int(durStmt, 0);
            }
            sqlite3_finalize(durStmt);
        }

        // Populate outputs
        outCompletedSession = activeSession;
        outCompletedSession.setStatus("completed");
        outCompletedSession.setBatteryEnd(batteryEnd);
        outCompletedSession.setEnergyConsumed(energyDelivered);
        outCompletedSession.setEstimatedCost(finalAmount);
        outCompletedSession.setDuration(durationMinutes);

        outTxn = Transaction(txnId, evId, "charging", stationId, desc, finalAmount, energyDelivered, durationMinutes, "", "completed");
        return true;
    }

    // In-memory fallback
    for (int i = 0; i < sessionCount; i++) {
        if ((sessionArray[i].getStationId() == stationOrSessionId || sessionArray[i].getId() == stationOrSessionId) && sessionArray[i].getStatus() == "active") {
            sessionArray[i].setStatus("completed");
            sessionArray[i].setBatteryEnd(sessionArray[i].getTargetCharge());
            sessionArray[i].setDuration(15);
            outCompletedSession = sessionArray[i];

            std::string stId = sessionArray[i].getStationId();
            for (int j = 0; j < stationCount; j++) {
                if (stationArray[j].getId() == stId) {
                    stationArray[j].setStatus("available");
                    stationArray[j].setCurrentVehicleId("");
                    stationArray[j].setAvailablePorts(stationArray[j].getTotalPorts());
                    break;
                }
            }

            outTxn = Transaction("TXN-FALLBACK", sessionArray[i].getVehicleId(), "charging",
                                 stId, "Charging complete", sessionArray[i].getEstimatedCost(),
                                 15.0, 15, "", "completed");
            return true;
        }
    }
    outError = "No matching active session found in memory";
    return false;
}

bool StationService::enqueueWaitingEV(const std::string& evId, ChargingQueue& queue, std::string& outError) {
    if (evId.empty()) {
        outError = "Vehicle ID cannot be empty";
        return false;
    }
    if (queue.isFull()) {
        outError = "Charging queue is currently full";
        return false;
    }
    if (queue.contains(evId)) {
        outError = "Vehicle " + evId + " is already waiting in the charging queue";
        return false;
    }

    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();

        // 1. Verify EV exists in database and is not actively charging or swapping
        std::string checkSql = "SELECT status FROM EV WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        std::string currentStatus = "";

        if (sqlite3_prepare_v2(handle, checkSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            outError = "Database error verifying vehicle status";
            return false;
        }
        sqlite3_bind_text(stmt, 1, evId.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* sTxt = (const char*)sqlite3_column_text(stmt, 0);
            currentStatus = sTxt ? sTxt : "";
        } else {
            sqlite3_finalize(stmt);
            outError = "Vehicle " + evId + " does not exist in registry";
            return false;
        }
        sqlite3_finalize(stmt);

        if (currentStatus == "charging" || currentStatus == "swapping") {
            outError = "Vehicle " + evId + " is actively occupied (" + currentStatus + ") and cannot be queued";
            return false;
        }

        // 2. Atomically update EV status to 'waiting' with high-resolution timestamp
        std::string updateSql = "UPDATE EV SET status = 'waiting', last_activity = strftime('%Y-%m-%d %H:%M:%f', 'now') WHERE id = ?;";
        sqlite3_stmt* updStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updateSql.c_str(), -1, &updStmt, nullptr) != SQLITE_OK) {
            outError = "Failed to prepare vehicle queue update statement";
            return false;
        }
        sqlite3_bind_text(updStmt, 1, evId.c_str(), -1, SQLITE_STATIC);
        int rc = sqlite3_step(updStmt);
        sqlite3_finalize(updStmt);

        if (rc != SQLITE_DONE || sqlite3_changes(handle) != 1) {
            outError = "Failed to update vehicle status to waiting in database";
            return false;
        }
    }

    // 3. Only enqueue into memory after database persistence succeeds
    if (!queue.enqueue(evId)) {
        outError = "Failed to insert vehicle into memory queue buffer";
        return false;
    }

    return true;
}

bool StationService::enqueueWaitingEV(const std::string& evId, ChargingQueue& queue) {
    std::string dummyErr;
    return enqueueWaitingEV(evId, queue, dummyErr);
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
