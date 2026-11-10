#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include "../models/user.h"
#include "../database/database.h"
#include <string>

// Authentication & Secure User Management Service
// Created by: Aditya Singh (Team Lead - Database & Backend Foundation)
// Features: Bcrypt password hashing, legacy plaintext automatic migration, input validation
class AuthService {
private:
    Database* db;

public:
    AuthService(Database* database);

    // Checks email or username and password against the Users table (bcrypt + auto-migration)
    // Returns true if valid, and fills outUser with details (password field sanitized)
    bool login(const std::string& emailOrName, const std::string& password, User& outUser);

    // Inserts a new user into the database with bcrypt password hashing
    bool registerUser(const std::string& name, const std::string& email, const std::string& password, const std::string& role);

    // Fetch user by unique ID
    bool getUserById(int id, User& outUser);

    // Check if email already registered
    bool userExists(const std::string& email);

    // Fetch all users into fixed array
    int getAllUsers(User outUsers[], int maxCapacity);

    // Transparently migrates any legacy plaintext seed passwords in SQLite to secure bcrypt hashes
    int migratePlaintextPasswords();
};

#endif
