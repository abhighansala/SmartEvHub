#include "auth_service.h"
#include <iostream>

AuthService::AuthService(Database* database) {
    db = database;
}

bool AuthService::login(const std::string& email, const std::string& password, User& outUser) {
    if (db == nullptr || !db->isOpen()) {
        std::cout << "[Auth Error] Database is not connected.\n";
        return false;
    }

    sqlite3* handle = db->getDbHandle();
    std::string sql = "SELECT id, name, email, password, role FROM Users WHERE email = ?;";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "[Auth Error] Could not prepare query: " << sqlite3_errmsg(handle) << "\n";
        return false;
    }

    // Bind email to the first ?
    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_STATIC);

    bool success = false;
    // Execute query
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = (const char*)sqlite3_column_text(stmt, 1);
        std::string userEmail = (const char*)sqlite3_column_text(stmt, 2);
        std::string storedPassword = (const char*)sqlite3_column_text(stmt, 3);
        std::string role = (const char*)sqlite3_column_text(stmt, 4);

        // Check if passwords match
        if (storedPassword == password) {
            outUser.setId(id);
            outUser.setName(name);
            outUser.setEmail(userEmail);
            outUser.setPassword(""); // Do not expose password
            outUser.setRole(role);
            success = true;
        } else {
            std::cout << "[Auth] Password incorrect for user: " << email << "\n";
        }
    } else {
        std::cout << "[Auth] No user found with email: " << email << "\n";
    }

    sqlite3_finalize(stmt);
    return success;
}

bool AuthService::registerUser(const std::string& name, const std::string& email, const std::string& password, const std::string& role) {
    if (db == nullptr || !db->isOpen()) {
        return false;
    }

    sqlite3* handle = db->getDbHandle();
    std::string sql = "INSERT INTO Users (name, email, password, role) VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, password.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, role.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}
