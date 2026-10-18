#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include "../models/user.h"
#include "../database/database.h"
#include <string>

// Simple Authentication & User Management Service
// Created by: Aditya Singh (Team Lead - Database & Backend Foundation)
class AuthService {
private:
    Database* db;

public:
    AuthService(Database* database);

    // Checks email or username and password against the Users table
    // Returns true if valid, and fills outUser with details
    bool login(const std::string& emailOrName, const std::string& password, User& outUser);

    // Inserts a new user into the database
    bool registerUser(const std::string& name, const std::string& email, const std::string& password, const std::string& role);

    // Fetch user by unique ID
    bool getUserById(int id, User& outUser);

    // Check if email already registered
    bool userExists(const std::string& email);

    // Fetch all users into fixed array
    int getAllUsers(User outUsers[], int maxCapacity);
};

#endif
