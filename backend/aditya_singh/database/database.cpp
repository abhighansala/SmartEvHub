#include "database.h"
#include <fstream>
#include <sstream>

Database::Database(const std::string& path) {
    dbPath = path;
    db = nullptr;
}

Database::~Database() {
    close();
}

bool Database::open() {
    if (db != nullptr) {
        return true; // Already opened
    }

    int result = sqlite3_open(dbPath.c_str(), &db);
    if (result != SQLITE_OK) {
        std::cout << "[Database Error] Could not open database: " 
                  << (db ? sqlite3_errmsg(db) : "Unknown error") << "\n";
        db = nullptr;
        return false;
    }

    // Enable WAL journal mode and foreign keys for performance and data integrity
    execute("PRAGMA foreign_keys = ON;");
    execute("PRAGMA journal_mode = WAL;");

    std::cout << "[Database] Successfully connected to: " << dbPath << "\n";
    return true;
}

void Database::close() {
    if (db != nullptr) {
        sqlite3_close(db);
        db = nullptr;
        std::cout << "[Database] Connection closed.\n";
    }
}

bool Database::execute(const std::string& sql) {
    if (db == nullptr) {
        std::cout << "[Database Error] Database is not open!\n";
        return false;
    }

    char* errorMessage = nullptr;
    int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errorMessage);

    if (result != SQLITE_OK) {
        std::cout << "[Database Error] SQL failed: " << (errorMessage ? errorMessage : "unknown") << "\n";
        std::cout << "                  Query was: " << sql << "\n";
        if (errorMessage != nullptr) {
            sqlite3_free(errorMessage);
        }
        return false;
    }

    return true;
}

bool Database::initSchema(const std::string& schemaFilePath) {
    std::ifstream file(schemaFilePath);
    if (!file.is_open()) {
        std::cout << "[Database Info] Could not read file: " << schemaFilePath << "\n";
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sqlQueries = buffer.str();

    std::cout << "[Database] Running schema file: " << schemaFilePath << "\n";
    return execute(sqlQueries);
}

bool Database::beginTransaction() {
    return execute("BEGIN TRANSACTION;");
}

bool Database::commitTransaction() {
    return execute("COMMIT;");
}

bool Database::rollbackTransaction() {
    return execute("ROLLBACK;");
}

bool Database::tableExists(const std::string& tableName) {
    if (db == nullptr) return false;

    std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, tableName.c_str(), -1, SQLITE_STATIC);
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

long long Database::getLastInsertId() {
    if (db == nullptr) return 0;
    return (long long)sqlite3_last_insert_rowid(db);
}

bool Database::isOpen() const {
    return db != nullptr;
}

sqlite3* Database::getDbHandle() {
    return db;
}
