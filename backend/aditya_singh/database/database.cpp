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
        std::cout << "[Database Error] Could not open database: " << sqlite3_errmsg(db) << "\n";
        db = nullptr;
        return false;
    }

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
        std::cout << "[Database Error] Could not read file: " << schemaFilePath << "\n";
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sqlQueries = buffer.str();

    std::cout << "[Database] Running schema file: " << schemaFilePath << "\n";
    return execute(sqlQueries);
}

bool Database::isOpen() const {
    return db != nullptr;
}

sqlite3* Database::getDbHandle() {
    return db;
}
