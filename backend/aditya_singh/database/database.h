#ifndef DATABASE_H
#define DATABASE_H

#include <iostream>
#include <string>
#include <sqlite3.h>

// Simple Database class to manage SQLite connection & CRUD foundation
// Created by: Aditya Singh (Team Lead - Database & Backend Foundation)
class Database {
private:
    sqlite3* db;
    std::string dbPath;

public:
    // Constructor
    Database(const std::string& path = "evhub.db");

    // Destructor - safely closes connection
    ~Database();

    // Open connection to SQLite file
    bool open();

    // Close connection
    void close();

    // Execute simple SQL command like INSERT, UPDATE, DELETE, CREATE TABLE
    bool execute(const std::string& sql);

    // Read and run SQL file (like schema.sql)
    bool initSchema(const std::string& schemaFilePath);

    // Transaction Management Helpers
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // Metadata & Helper queries
    bool tableExists(const std::string& tableName);
    long long getLastInsertId();

    // Check if database is open
    bool isOpen() const;

    // Get sqlite3 pointer for queries
    sqlite3* getDbHandle();
};

#endif
