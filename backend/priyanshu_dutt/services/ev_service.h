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
class EVService {
private:
    Database* db;

    // Fallback simple array in case database is not available
    EV evArray[MAX_EVS];
    int evCount;

public:
    EVService(Database* database = nullptr);

    // Fills outList array and returns total number of EVs
    int getAllEVs(EV outList[], int maxCapacity);

    // Finds a single EV by ID
    bool getEVById(const std::string& id, EV& outEV);

    // Adds a new EV
    bool createEV(const EV& ev, std::string& outError);

    // Updates an existing EV
    bool updateEV(const std::string& id, const EV& updatedEV, std::string& outError);

    // Deletes an EV
    bool deleteEV(const std::string& id);

    // Get current in-memory count
    int getCount() const { return evCount; }
};

#endif
