#include "station_service.h"
#include <iostream>

StationService::StationService(Database* database) {
    db = database;
    stationCount = 0;

    // Initialize with default demo stations if starting without database
    stationArray[0] = ChargingStation("CS-1", "DC Fast Charger 1", 60.0, "CCS2", "available", "", "Bay A-1");
    stationArray[1] = ChargingStation("CS-2", "DC Fast Charger 2", 60.0, "CCS2", "available", "", "Bay A-2");
    stationArray[2] = ChargingStation("CS-3", "Ultra Fast Charger 3", 120.0, "CCS2", "available", "", "Bay B-1");
    stationArray[3] = ChargingStation("CS-4", "AC Type 2 Charger 4", 22.0, "Type 2", "maintenance", "", "Bay B-2");
    stationCount = 4;
}

int StationService::getAllStations(ChargingStation outList[], int maxCapacity) {
    int count = 0;

    // Load from database if connected
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id, name, power, connector_type, status, current_vehicle_id, location FROM ChargingStation;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW && count < maxCapacity) {
                std::string id = (const char*)sqlite3_column_text(stmt, 0);
                std::string name = (const char*)sqlite3_column_text(stmt, 1);
                double power = sqlite3_column_double(stmt, 2);
                std::string conn = (const char*)sqlite3_column_text(stmt, 3);
                std::string status = (const char*)sqlite3_column_text(stmt, 4);
                
                const char* vehText = (const char*)sqlite3_column_text(stmt, 5);
                std::string vehicleId = vehText ? vehText : "";
                
                const char* locText = (const char*)sqlite3_column_text(stmt, 6);
                std::string location = locText ? locText : "Bay 1";

                outList[count] = ChargingStation(id, name, power, conn, status, vehicleId, location);
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
            if (rc == SQLITE_DONE) return true;
        }
    }

    // Update in local array
    for (int i = 0; i < stationCount; i++) {
        if (stationArray[i].getId() == id) {
            stationArray[i].setStatus(newStatus);
            return true;
        }
    }
    return false;
}

bool StationService::assignVehicle(const std::string& stationId, const std::string& vehicleId) {
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE ChargingStation SET status = 'charging', current_vehicle_id = ?, session_start = datetime('now') WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, vehicleId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, stationId.c_str(), -1, SQLITE_STATIC);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc == SQLITE_DONE) return true;
        }
    }

    for (int i = 0; i < stationCount; i++) {
        if (stationArray[i].getId() == stationId) {
            stationArray[i].setStatus("charging");
            stationArray[i].setCurrentVehicleId(vehicleId);
            return true;
        }
    }
    return false;
}

bool StationService::releaseStation(const std::string& stationId) {
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE ChargingStation SET status = 'available', current_vehicle_id = NULL, session_start = NULL WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, stationId.c_str(), -1, SQLITE_STATIC);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc == SQLITE_DONE) return true;
        }
    }

    for (int i = 0; i < stationCount; i++) {
        if (stationArray[i].getId() == stationId) {
            stationArray[i].setStatus("available");
            stationArray[i].setCurrentVehicleId("");
            return true;
        }
    }
    return false;
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
