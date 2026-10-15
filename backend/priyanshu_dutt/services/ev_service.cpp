#include "ev_service.h"
#include <iostream>

EVService::EVService(Database* database) {
    db = database;
    evCount = 0;
}

int EVService::getAllEVs(EV outList[], int maxCapacity) {
    int count = 0;

    // First try to load from SQLite database if connected
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id, owner_name, model, battery_capacity, current_charge, status, last_activity, registered_at FROM EV;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW && count < maxCapacity) {
                std::string id = (const char*)sqlite3_column_text(stmt, 0);
                std::string owner = (const char*)sqlite3_column_text(stmt, 1);
                std::string model = (const char*)sqlite3_column_text(stmt, 2);
                double cap = sqlite3_column_double(stmt, 3);
                double charge = sqlite3_column_double(stmt, 4);
                std::string status = (const char*)sqlite3_column_text(stmt, 5);

                const char* actText = (const char*)sqlite3_column_text(stmt, 6);
                const char* regText = (const char*)sqlite3_column_text(stmt, 7);
                std::string lastAct = actText ? actText : "";
                std::string regAt = regText ? regText : "";

                outList[count] = EV(id, owner, model, cap, charge, status, lastAct, regAt);
                count++;
            }
            sqlite3_finalize(stmt);
            return count;
        }
    }

    // Fallback: Copy from local simple array
    for (int i = 0; i < evCount && i < maxCapacity; i++) {
        outList[i] = evArray[i];
        count++;
    }

    return count;
}

bool EVService::getEVById(const std::string& id, EV& outEV) {
    // 1. Check in database
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id, owner_name, model, battery_capacity, current_charge, status, last_activity, registered_at FROM EV WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                outEV.setId((const char*)sqlite3_column_text(stmt, 0));
                outEV.setOwnerName((const char*)sqlite3_column_text(stmt, 1));
                outEV.setModel((const char*)sqlite3_column_text(stmt, 2));
                outEV.setBatteryCapacity(sqlite3_column_double(stmt, 3));
                outEV.setCurrentCharge(sqlite3_column_double(stmt, 4));
                outEV.setStatus((const char*)sqlite3_column_text(stmt, 5));

                const char* actText = (const char*)sqlite3_column_text(stmt, 6);
                const char* regText = (const char*)sqlite3_column_text(stmt, 7);
                outEV.setLastActivity(actText ? actText : "");
                outEV.setRegisteredAt(regText ? regText : "");

                sqlite3_finalize(stmt);
                return true;
            }
            sqlite3_finalize(stmt);
        }
    }

    // 2. Check in local array
    for (int i = 0; i < evCount; i++) {
        if (evArray[i].getId() == id) {
            outEV = evArray[i];
            return true;
        }
    }

    return false;
}

bool EVService::createEV(const EV& ev, std::string& outError) {
    if (!ev.isValid()) {
        outError = "Invalid EV: Ensure battery charge is 0-100% and ID is not empty.";
        return false;
    }

    // Check if duplicate exists
    EV existing;
    if (getEVById(ev.getId(), existing)) {
        outError = "EV with ID " + ev.getId() + " already exists.";
        return false;
    }

    // Insert into database if available
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "INSERT INTO EV (id, owner_name, model, battery_capacity, current_charge, status) VALUES (?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, ev.getId().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, ev.getOwnerName().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, ev.getModel().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 4, ev.getBatteryCapacity());
            sqlite3_bind_double(stmt, 5, ev.getCurrentCharge());
            sqlite3_bind_text(stmt, 6, ev.getStatus().c_str(), -1, SQLITE_STATIC);

            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc == SQLITE_DONE) {
                return true;
            }
        }
    }

    // Store in local array
    if (evCount < MAX_EVS) {
        evArray[evCount] = ev;
        evCount++;
        return true;
    } else {
        outError = "Array is full! Cannot add more EVs.";
        return false;
    }
}

bool EVService::updateEV(const std::string& id, const EV& updatedEV, std::string& outError) {
    if (!updatedEV.isValid()) {
        outError = "Invalid EV update data.";
        return false;
    }

    // Update in database
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "UPDATE EV SET owner_name = ?, model = ?, battery_capacity = ?, current_charge = ?, status = ? WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, updatedEV.getOwnerName().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, updatedEV.getModel().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 3, updatedEV.getBatteryCapacity());
            sqlite3_bind_double(stmt, 4, updatedEV.getCurrentCharge());
            sqlite3_bind_text(stmt, 5, updatedEV.getStatus().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 6, id.c_str(), -1, SQLITE_STATIC);

            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return (rc == SQLITE_DONE);
        }
    }

    // Update in local array
    for (int i = 0; i < evCount; i++) {
        if (evArray[i].getId() == id) {
            evArray[i] = updatedEV;
            return true;
        }
    }

    outError = "EV not found with ID: " + id;
    return false;
}

bool EVService::deleteEV(const std::string& id) {
    // Delete from database
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "DELETE FROM EV WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return (rc == SQLITE_DONE);
        }
    }

    // Delete from local array by shifting elements
    for (int i = 0; i < evCount; i++) {
        if (evArray[i].getId() == id) {
            for (int j = i; j < evCount - 1; j++) {
                evArray[j] = evArray[j + 1];
            }
            evCount--;
            return true;
        }
    }

    return false;
}
