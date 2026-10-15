#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include "../models/user.h"
#include "../database/database.h"
#include <string>

// Simple Authentication Service
// Created by: Aditya Singh (Lead - Database & Backend Foundation)
class AuthService {
private:
    Database* db;

public:
    AuthService(Database* database);

    // Checks email and password against the Users table
    // Returns true if valid, and fills outUser with details
    bool login(const std::string& email, const std::string& password, User& outUser);

    // Inserts a new user into the database
    bool registerUser(const std::string& name, const std::string& email, const std::string& password, const std::string& role);
};

#endif
