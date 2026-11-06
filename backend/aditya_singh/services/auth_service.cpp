#include "auth_service.h"
#include <bcrypt/bcrypt.h>
#include <iostream>
#include <vector>

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
        const char* nameText = (const char*)sqlite3_column_text(stmt, 1);
        const char* emailText = (const char*)sqlite3_column_text(stmt, 2);
        const char* passText = (const char*)sqlite3_column_text(stmt, 3);
        const char* roleText = (const char*)sqlite3_column_text(stmt, 4);
        const char* createdText = (const char*)sqlite3_column_text(stmt, 5);

        std::string name = nameText ? nameText : "";
        std::string userEmail = emailText ? emailText : "";
        std::string storedPassword = passText ? passText : "";
        std::string role = roleText ? roleText : "user";
        std::string createdAt = createdText ? createdText : "";

        bool passwordMatches = false;

        // 1. If stored password is a bcrypt hash ($2a$ or $2b$)
        if (storedPassword.rfind("$2", 0) == 0) {
            passwordMatches = bcrypt::validatePassword(password, storedPassword);
        } else {
            // 2. Backward compatibility & Transparent Auto-Migration:
            // If stored password is older legacy plaintext, check match and automatically upgrade to bcrypt
            if (password == storedPassword) {
                passwordMatches = true;
                std::string newBcryptHash = bcrypt::generateHash(password, 10);
                std::string updateSql = "UPDATE Users SET password = ? WHERE id = ?;";
                sqlite3_stmt* updateStmt = nullptr;
                if (sqlite3_prepare_v2(handle, updateSql.c_str(), -1, &updateStmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(updateStmt, 1, newBcryptHash.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_int(updateStmt, 2, id);
                    sqlite3_step(updateStmt);
                    sqlite3_finalize(updateStmt);
                    std::cout << "[Auth Migration] Transparently upgraded user " << userEmail << " (ID: " << id << ") to secure bcrypt hash.\n";
                }
            }
        }

        if (passwordMatches) {
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
    if (name.length() < 2) {
        std::cout << "[Auth Error] Name must be at least 2 characters.\n";
        return false;
    }

    if (email.find('@') == std::string::npos || email.find('.') == std::string::npos) {
        std::cout << "[Auth Error] Invalid email address format.\n";
        return false;
    }

    if (password.length() < 6) {
        std::cout << "[Auth Error] Password must be at least 6 characters.\n";
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

    // Validate role against allowable set
    std::string userRole = "user";
    if (role == "admin" || role == "operator") {
        userRole = role;
    }

    // Secure password hashing with bcrypt (rounds = 10)
    std::string hashedPassword = bcrypt::generateHash(password, 10);

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, hashedPassword.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, userRole.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);

    if (success) {
        std::cout << "[Auth] New user registered with bcrypt hash: " << email << " (ID: " << sqlite3_last_insert_rowid(handle) << ")\n";
    } else {
        std::cout << "[Auth Error] Failed to register user: " << sqlite3_errmsg(handle) << "\n";
    }

    sqlite3_finalize(stmt);
    return success;
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
        const char* nameText = (const char*)sqlite3_column_text(stmt, 1);
        const char* emailText = (const char*)sqlite3_column_text(stmt, 2);
        const char* roleText = (const char*)sqlite3_column_text(stmt, 3);
        const char* dt = (const char*)sqlite3_column_text(stmt, 4);
        outUser.setId(sqlite3_column_int(stmt, 0));
        outUser.setName(nameText ? nameText : "");
        outUser.setEmail(emailText ? emailText : "");
        outUser.setRole(roleText ? roleText : "user");
        outUser.setCreatedAt(dt ? dt : "");
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
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
        const char* nameText = (const char*)sqlite3_column_text(stmt, 1);
        const char* emailText = (const char*)sqlite3_column_text(stmt, 2);
        const char* roleText = (const char*)sqlite3_column_text(stmt, 3);
        const char* dt = (const char*)sqlite3_column_text(stmt, 4);
        std::string name = nameText ? nameText : "";
        std::string email = emailText ? emailText : "";
        std::string role = roleText ? roleText : "user";
        std::string createdAt = dt ? dt : "";

        outUsers[count] = User(id, name, email, "", role, createdAt);
        count++;
    }

    sqlite3_finalize(stmt);
    return count;
}

int AuthService::migratePlaintextPasswords() {
    if (db == nullptr || !db->isOpen()) return 0;

    sqlite3* handle = db->getDbHandle();
    std::string sql = "SELECT id, password FROM Users;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    struct MigrationItem {
        int id;
        std::string plain;
    };
    std::vector<MigrationItem> toMigrate;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* passText = (const char*)sqlite3_column_text(stmt, 1);
        std::string pass = passText ? passText : "";
        if (!pass.empty() && pass.rfind("$2", 0) != 0) {
            toMigrate.push_back({ id, pass });
        }
    }
    sqlite3_finalize(stmt);

    int count = 0;
    for (const auto& item : toMigrate) {
        std::string hashed = bcrypt::generateHash(item.plain, 10);
        std::string updateSql = "UPDATE Users SET password = ? WHERE id = ?;";
        sqlite3_stmt* upStmt = nullptr;
        if (sqlite3_prepare_v2(handle, updateSql.c_str(), -1, &upStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(upStmt, 1, hashed.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(upStmt, 2, item.id);
            if (sqlite3_step(upStmt) == SQLITE_DONE) {
                count++;
            }
            sqlite3_finalize(upStmt);
        }
    }

    if (count > 0) {
        std::cout << "[Auth Migration] Successfully upgraded " << count << " legacy seed password(s) to bcrypt hashes.\n";
    }
    return count;
}
