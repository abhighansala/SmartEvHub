#include "auth_service.h"
#include <iostream>

AuthService::AuthService(Database* database) {
    db = database;
}

bool AuthService::login(const std::string& emailOrName, const std::string& password, User& outUser) {
    if (emailOrName.empty() || password.empty()) {
        std::cout << "[Auth Error] Email/Username and password must not be empty.\n";
        return false;
    }

    if (db == nullptr || !db->isOpen()) {
        std::cout << "[Auth Error] Database is not connected.\n";
        return false;
    }

    sqlite3* handle = db->getDbHandle();
    // Allow authentication by email OR by name for maximum usability
    std::string sql = "SELECT id, name, email, password, role, created_at FROM Users WHERE email = ? OR name = ?;";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "[Auth Error] Could not prepare query: " << sqlite3_errmsg(handle) << "\n";
        return false;
    }

    // Bind identifier to both params (1: email, 2: name)
    sqlite3_bind_text(stmt, 1, emailOrName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, emailOrName.c_str(), -1, SQLITE_STATIC);

    bool success = false;
    // Execute query
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = (const char*)sqlite3_column_text(stmt, 1);
        std::string userEmail = (const char*)sqlite3_column_text(stmt, 2);
        std::string storedPassword = (const char*)sqlite3_column_text(stmt, 3);
        std::string role = (const char*)sqlite3_column_text(stmt, 4);
        const char* createdText = (const char*)sqlite3_column_text(stmt, 5);
        std::string createdAt = createdText ? createdText : "";

        // Check if passwords match
        if (storedPassword == password) {
            outUser.setId(id);
            outUser.setName(name);
            outUser.setEmail(userEmail);
            outUser.setPassword(""); // Do not expose password in memory
            outUser.setRole(role);
            outUser.setCreatedAt(createdAt);
            success = true;
            std::cout << "[Auth] Login successful for: " << userEmail << " (" << role << ")\n";
        } else {
            std::cout << "[Auth] Password incorrect for user: " << emailOrName << "\n";
        }
    } else {
        std::cout << "[Auth] No user found with email or name: " << emailOrName << "\n";
    }

    sqlite3_finalize(stmt);
    return success;
}

bool AuthService::registerUser(const std::string& name, const std::string& email, const std::string& password, const std::string& role) {
    if (name.empty() || email.empty() || password.empty()) {
        std::cout << "[Auth Error] Required fields missing for registration.\n";
        return false;
    }

    if (db == nullptr || !db->isOpen()) {
        return false;
    }

    // Check if user already exists
    if (userExists(email)) {
        std::cout << "[Auth Error] User already registered with email: " << email << "\n";
        return false;
    }

    sqlite3* handle = db->getDbHandle();
    std::string sql = "INSERT INTO Users (name, email, password, role) VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    std::string userRole = role.empty() ? "user" : role;
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, password.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, userRole.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        std::cout << "[Auth] New user registered: " << email << " (ID: " << db->getLastInsertId() << ")\n";
        return true;
    }

    return false;
}

bool AuthService::userExists(const std::string& email) {
    if (db == nullptr || !db->isOpen()) return false;

    sqlite3* handle = db->getDbHandle();
    std::string sql = "SELECT id FROM Users WHERE email = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_STATIC);
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

bool AuthService::getUserById(int id, User& outUser) {
    if (db == nullptr || !db->isOpen()) return false;

    sqlite3* handle = db->getDbHandle();
    std::string sql = "SELECT id, name, email, role, created_at FROM Users WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outUser.setId(sqlite3_column_int(stmt, 0));
        outUser.setName((const char*)sqlite3_column_text(stmt, 1));
        outUser.setEmail((const char*)sqlite3_column_text(stmt, 2));
        outUser.setRole((const char*)sqlite3_column_text(stmt, 3));
        const char* dt = (const char*)sqlite3_column_text(stmt, 4);
        outUser.setCreatedAt(dt ? dt : "");
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

int AuthService::getAllUsers(User outUsers[], int maxCapacity) {
    if (db == nullptr || !db->isOpen()) return 0;

    sqlite3* handle = db->getDbHandle();
    std::string sql = "SELECT id, name, email, role, created_at FROM Users ORDER BY id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < maxCapacity) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = (const char*)sqlite3_column_text(stmt, 1);
        std::string email = (const char*)sqlite3_column_text(stmt, 2);
        std::string role = (const char*)sqlite3_column_text(stmt, 3);
        const char* dt = (const char*)sqlite3_column_text(stmt, 4);
        std::string createdAt = dt ? dt : "";

        outUsers[count] = User(id, name, email, "", role, createdAt);
        count++;
    }

    sqlite3_finalize(stmt);
    return count;
}
