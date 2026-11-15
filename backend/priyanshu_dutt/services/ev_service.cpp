#include "ev_service.h"
#include <iostream>

EVService::EVService(Database* database) {
    db = database;
    evCount = 0;
    ensureSchema();
}

void EVService::ensureSchema() {
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        bool hasUserId = false;
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(handle, "PRAGMA table_info(EV);", -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* colName = (const char*)sqlite3_column_text(stmt, 1);
                if (colName && std::string(colName) == "user_id") {
                    hasUserId = true;
                    break;
                }
            }
            sqlite3_finalize(stmt);
        }

        if (!hasUserId) {
            db->execute("ALTER TABLE EV ADD COLUMN user_id INTEGER DEFAULT NULL;");
        }

        // Backfill legacy seed EVs if user_id is NULL to preserve consistent initial data
        db->execute("UPDATE EV SET user_id = 9 WHERE id = 'EV-101' AND (user_id IS NULL OR user_id = 0);");
        db->execute("UPDATE EV SET user_id = 10 WHERE id = 'EV-102' AND (user_id IS NULL OR user_id = 0);");
        db->execute("UPDATE EV SET user_id = 11 WHERE id = 'EV-103' AND (user_id IS NULL OR user_id = 0);");
        db->execute("UPDATE EV SET user_id = 12 WHERE id = 'EV-104' AND (user_id IS NULL OR user_id = 0);");
        db->execute("UPDATE EV SET user_id = 13 WHERE id = 'EV-105' AND (user_id IS NULL OR user_id = 0);");
        db->execute("UPDATE EV SET user_id = 14 WHERE id = 'EV-106' AND (user_id IS NULL OR user_id = 0);");
        db->execute("UPDATE EV SET user_id = 11 WHERE id = 'EV-125' AND (user_id IS NULL OR user_id = 0);");
    }
}

int EVService::getAllEVs(EV outList[], int maxCapacity, int userId) {
    int count = 0;

    // First try to load from SQLite database if connected
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql;
        if (userId > 0) {
            // Data Isolation: return ONLY vehicles owned by this authenticated user
            sql = "SELECT id, owner_name, model, battery_capacity, current_charge, status, last_activity, registered_at, user_id FROM EV WHERE user_id = ?;";
        } else {
            // Unfiltered view for Admin / Operator
            sql = "SELECT id, owner_name, model, battery_capacity, current_charge, status, last_activity, registered_at, user_id FROM EV;";
        }
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (userId > 0) {
                sqlite3_bind_int(stmt, 1, userId);
            }
            while (sqlite3_step(stmt) == SQLITE_ROW && count < maxCapacity) {
                const char* idText = (const char*)sqlite3_column_text(stmt, 0);
                const char* ownerText = (const char*)sqlite3_column_text(stmt, 1);
                const char* modelText = (const char*)sqlite3_column_text(stmt, 2);
                double cap = sqlite3_column_double(stmt, 3);
                double charge = sqlite3_column_double(stmt, 4);
                const char* statusText = (const char*)sqlite3_column_text(stmt, 5);
                const char* actText = (const char*)sqlite3_column_text(stmt, 6);
                const char* regText = (const char*)sqlite3_column_text(stmt, 7);
                int uid = (sqlite3_column_type(stmt, 8) != SQLITE_NULL) ? sqlite3_column_int(stmt, 8) : 0;

                std::string id = idText ? idText : "";
                std::string owner = ownerText ? ownerText : "";
                std::string model = modelText ? modelText : "";
                std::string status = statusText ? statusText : "available";
                std::string lastAct = actText ? actText : "";
                std::string regAt = regText ? regText : "";

                outList[count] = EV(id, owner, model, cap, charge, status, lastAct, regAt, uid);
                count++;
            }
            sqlite3_finalize(stmt);
            return count;
        }
    }

    // Fallback: Copy from local simple array
    for (int i = 0; i < evCount && count < maxCapacity; i++) {
        if (userId <= 0 || evArray[i].getUserId() == userId) {
            outList[count] = evArray[i];
            count++;
        }
    }

    return count;
}

int EVService::getEVsByUserId(EV outList[], int maxCapacity, int userId) {
    return getAllEVs(outList, maxCapacity, userId);
}

bool EVService::getEVById(const std::string& id, EV& outEV) {
    // 1. Check in database
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql = "SELECT id, owner_name, model, battery_capacity, current_charge, status, last_activity, registered_at, user_id FROM EV WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* idText = (const char*)sqlite3_column_text(stmt, 0);
                const char* ownerText = (const char*)sqlite3_column_text(stmt, 1);
                const char* modelText = (const char*)sqlite3_column_text(stmt, 2);
                double cap = sqlite3_column_double(stmt, 3);
                double charge = sqlite3_column_double(stmt, 4);
                const char* statusText = (const char*)sqlite3_column_text(stmt, 5);
                const char* actText = (const char*)sqlite3_column_text(stmt, 6);
                const char* regText = (const char*)sqlite3_column_text(stmt, 7);
                int uid = (sqlite3_column_type(stmt, 8) != SQLITE_NULL) ? sqlite3_column_int(stmt, 8) : 0;

                outEV.setId(idText ? idText : "");
                outEV.setOwnerName(ownerText ? ownerText : "");
                outEV.setModel(modelText ? modelText : "");
                outEV.setBatteryCapacity(cap);
                outEV.setCurrentCharge(charge);
                outEV.setStatus(statusText ? statusText : "available");
                outEV.setLastActivity(actText ? actText : "");
                outEV.setRegisteredAt(regText ? regText : "");
                outEV.setUserId(uid);

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

bool EVService::checkOwnership(const std::string& id, int userId, const std::string& role, std::string& outError) {
    if (role == "admin" || role == "operator") {
        return true;
    }
    EV existing;
    if (!getEVById(id, existing)) {
        outError = "EV not found with ID: " + id;
        return false;
    }
    if (role == "user" && existing.getUserId() != userId) {
        outError = "Access forbidden: you do not have permission to access or modify this vehicle";
        return false;
    }
    return true;
}

bool EVService::getEVByIdForUser(const std::string& id, int userId, const std::string& role, EV& outEV, std::string& outError) {
    if (!getEVById(id, outEV)) {
        outError = "EV not found with ID: " + id;
        return false;
    }
    if (role == "user" && outEV.getUserId() != userId) {
        outError = "Access forbidden: you do not have permission to view this vehicle";
        return false;
    }
    return true;
}

bool EVService::createEV(const EV& ev, std::string& outError) {
    if (!ev.isValid()) {
        outError = "Invalid EV: Ensure battery charge is 0-100%, capacity is positive, and ID/model are not empty.";
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
        std::string sql = "INSERT INTO EV (id, owner_name, model, battery_capacity, current_charge, status, user_id) VALUES (?, ?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, ev.getId().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, ev.getOwnerName().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, ev.getModel().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 4, ev.getBatteryCapacity());
            sqlite3_bind_double(stmt, 5, ev.getCurrentCharge());
            sqlite3_bind_text(stmt, 6, ev.getStatus().c_str(), -1, SQLITE_TRANSIENT);
            if (ev.getUserId() > 0) {
                sqlite3_bind_int(stmt, 7, ev.getUserId());
            } else {
                sqlite3_bind_null(stmt, 7);
            }

            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc == SQLITE_DONE) {
                if (evCount < MAX_EVS) {
                    evArray[evCount] = ev;
                    evCount++;
                }
                return true;
            } else {
                outError = std::string("Database error: ") + sqlite3_errmsg(handle);
                return false;
            }
        } else {
            outError = std::string("Database error: ") + sqlite3_errmsg(handle);
            return false;
        }
    }

    // Fallback: Store in local array if database is not connected
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
        outError = "Invalid EV update data: Ensure battery charge is 0-100%, capacity is positive, and ID/model are not empty.";
        return false;
    }

    // Update in database if connected
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();
        std::string sql;
        if (updatedEV.getUserId() > 0) {
            sql = "UPDATE EV SET owner_name = ?, model = ?, battery_capacity = ?, current_charge = ?, status = ?, user_id = ? WHERE id = ?;";
        } else {
            sql = "UPDATE EV SET owner_name = ?, model = ?, battery_capacity = ?, current_charge = ?, status = ? WHERE id = ?;";
        }
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, updatedEV.getOwnerName().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, updatedEV.getModel().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 3, updatedEV.getBatteryCapacity());
            sqlite3_bind_double(stmt, 4, updatedEV.getCurrentCharge());
            sqlite3_bind_text(stmt, 5, updatedEV.getStatus().c_str(), -1, SQLITE_TRANSIENT);
            if (updatedEV.getUserId() > 0) {
                sqlite3_bind_int(stmt, 6, updatedEV.getUserId());
                sqlite3_bind_text(stmt, 7, id.c_str(), -1, SQLITE_TRANSIENT);
            } else {
                sqlite3_bind_text(stmt, 6, id.c_str(), -1, SQLITE_TRANSIENT);
            }

            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            if (rc != SQLITE_DONE) {
                outError = std::string("Database error: ") + sqlite3_errmsg(handle);
                return false;
            }

            int changes = sqlite3_changes(handle);
            if (changes > 0) {
                for (int i = 0; i < evCount; i++) {
                    if (evArray[i].getId() == id) {
                        evArray[i] = updatedEV;
                        break;
                    }
                }
                return true;
            }
        } else {
            outError = std::string("Database error: ") + sqlite3_errmsg(handle);
            return false;
        }
    }

    // Update in local fallback array
    for (int i = 0; i < evCount; i++) {
        if (evArray[i].getId() == id) {
            evArray[i] = updatedEV;
            return true;
        }
    }

    outError = "EV not found with ID: " + id;
    return false;
}

bool EVService::updateEVForUser(const std::string& id, const EV& updatedEV, int userId, const std::string& role, std::string& outError) {
    if (!checkOwnership(id, userId, role, outError)) {
        return false;
    }
    return updateEV(id, updatedEV, outError);
}

bool EVService::deleteEV(const std::string& id, std::string& outError) {
    bool dbDeleted = false;
    // Delete from database if connected
    if (db != nullptr && db->isOpen()) {
        sqlite3* handle = db->getDbHandle();

        // Release any charging station occupied by this vehicle before deleting it
        std::string freeStSql = "UPDATE ChargingStation SET status = 'available', current_vehicle_id = NULL, "
                                "session_start = NULL, available_ports = 1 WHERE current_vehicle_id = ?;";
        sqlite3_stmt* fsStmt = nullptr;
        if (sqlite3_prepare_v2(handle, freeStSql.c_str(), -1, &fsStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(fsStmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(fsStmt);
            sqlite3_finalize(fsStmt);
        }

        // Close any active session for this vehicle
        std::string endSesSql = "UPDATE ChargingSession SET status = 'completed', end_time = datetime('now') "
                                "WHERE vehicle_id = ? AND status = 'active';";
        sqlite3_stmt* esStmt = nullptr;
        if (sqlite3_prepare_v2(handle, endSesSql.c_str(), -1, &esStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(esStmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(esStmt);
            sqlite3_finalize(esStmt);
        }

        std::string sql = "DELETE FROM EV WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            if (rc != SQLITE_DONE) {
                outError = std::string("Database error: ") + sqlite3_errmsg(handle);
                return false;
            }

            int changes = sqlite3_changes(handle);
            if (changes > 0) {
                dbDeleted = true;
            }
        } else {
            outError = std::string("Database error: ") + sqlite3_errmsg(handle);
            return false;
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

    if (dbDeleted) {
        return true;
    }

    outError = "EV not found with ID: " + id;
    return false;
}

bool EVService::deleteEV(const std::string& id) {
    std::string err;
    return deleteEV(id, err);
}

bool EVService::deleteEVForUser(const std::string& id, int userId, const std::string& role, std::string& outError) {
    if (!checkOwnership(id, userId, role, outError)) {
        return false;
    }
    return deleteEV(id, outError);
}
