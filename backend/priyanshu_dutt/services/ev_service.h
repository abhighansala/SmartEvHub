#ifndef EV_SERVICE_H
#define EV_SERVICE_H

#include "../models/ev.h"
#include "../../aditya_singh/database/database.h"
#include <string>

// Maximum number of EVs stored in simple array
const int MAX_EVS = 50;

// Simple Service to manage Electric Vehicle operations
// Uses simple C++ arrays and loops (no complex vectors)
// Created by: Priyanshu Dutt (EV Management Module Owner, ID: 2519460)
// Enhanced for User Data Isolation & Role-Based Ownership Verification
class EVService {
private:
    Database* db;

    // Fallback simple array in case database is not available
    EV evArray[MAX_EVS];
    int evCount;

    // Safe migration/initialization for user_id column
    void ensureSchema();

public:
    EVService(Database* database = nullptr);

    // Fills outList array and returns total number of EVs.
    // If userId > 0, returns only EVs owned by that user (Data Isolation for USER role).
    // If userId == 0 (default), returns all EVs (Admin / Operator view).
    int getAllEVs(EV outList[], int maxCapacity, int userId = 0);

    // Explicit convenience method to retrieve EVs for a specific user ID
    int getEVsByUserId(EV outList[], int maxCapacity, int userId);

    // Finds a single EV by ID
    bool getEVById(const std::string& id, EV& outEV);

    // Verifies whether a given userId and role can access/modify the vehicle
    bool checkOwnership(const std::string& id, int userId, const std::string& role, std::string& outError);

    // Finds a single EV by ID enforcing ownership for normal users
    bool getEVByIdForUser(const std::string& id, int userId, const std::string& role, EV& outEV, std::string& outError);

    // Adds a new EV
    bool createEV(const EV& ev, std::string& outError);

    // Updates an existing EV
    bool updateEV(const std::string& id, const EV& updatedEV, std::string& outError);

    // Updates an existing EV enforcing ownership for normal users
    bool updateEVForUser(const std::string& id, const EV& updatedEV, int userId, const std::string& role, std::string& outError);

    // Deletes an EV
    bool deleteEV(const std::string& id);
    bool deleteEV(const std::string& id, std::string& outError);

    // Deletes an EV enforcing ownership for normal users
    bool deleteEVForUser(const std::string& id, int userId, const std::string& role, std::string& outError);

    // Get current in-memory count
    int getCount() const { return evCount; }
};

#endif
