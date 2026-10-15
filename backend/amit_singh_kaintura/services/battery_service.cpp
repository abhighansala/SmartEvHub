#include "battery_service.h"
#include <iostream>

BatteryService::BatteryService(Database* database) {
    db = database;
    batteryCount = 0;

    // Seed local array with default batteries in case running without database
    batteryArray[0] = Battery("BAT-01", 50.0, 100.0, "available", 98.5, 42);
    batteryArray[1] = Battery("BAT-02", 50.0, 100.0, "available", 97.0, 58);
    batteryArray[2] = Battery("BAT-03", 60.0, 100.0, "available", 99.2, 18);
    batteryArray[3] = Battery("BAT-04", 50.0, 44.0, "charging", 95.0, 89);
    batteryArray[4] = Battery("BAT-05", 40.0, 15.0, "charging", 92.4, 120);
    batteryCount = 5;
}

int BatteryService::getAllBatteries(Battery outList[], int maxCapacity) {
    int count = 0;

    // Load from database if connected
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id, capacity, charge_level, status, health, cycle_count, current_vehicle FROM Battery;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW && count < maxCapacity) {
                std::string id = (const char*)sqlite3_column_text(stmt, 0);
                double cap = sqlite3_column_double(stmt, 1);
                double charge = sqlite3_column_double(stmt, 2);
                std::string status = (const char*)sqlite3_column_text(stmt, 3);
                double health = sqlite3_column_double(stmt, 4);
                int cycles = sqlite3_column_int(stmt, 5);

                const char* vehText = (const char*)sqlite3_column_text(stmt, 6);
                std::string vehicle = vehText ? vehText : "";

                outList[count] = Battery(id, cap, charge, status, health, cycles, vehicle);
                count++;
            }
            sqlite3_finalize(stmt);
            return count;
        }
    }

    // Fallback: copy from array
    for (int i = 0; i < batteryCount && i < maxCapacity; i++) {
        outList[i] = batteryArray[i];
        count++;
    }

    return count;
}

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

bool BatteryService::performSwap(const std::string& evId, Battery& outGivenBattery) {
    // 1. Find a ready battery
    if (!findReadyBattery(outGivenBattery)) {
        std::cout << "[Battery Error] No fully charged battery available for swap!\n";
        return false;
    }

    std::string batteryId = outGivenBattery.getId();

    // 2. Mark battery as "inuse" and assign vehicle in database
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE Battery SET status = 'inuse', current_vehicle = ? WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, evId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, batteryId.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    // Update in local array
    for (int i = 0; i < batteryCount; i++) {
        if (batteryArray[i].getId() == batteryId) {
            batteryArray[i].setStatus("inuse");
            batteryArray[i].setCurrentVehicle(evId);
            outGivenBattery = batteryArray[i];
            return true;
        }
    }

    return true;
}

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

bool BatteryService::addBattery(const Battery& b) {
    if (batteryCount < MAX_BATTERIES) {
        batteryArray[batteryCount] = b;
        batteryCount++;
        return true;
    }
    return false;
}
