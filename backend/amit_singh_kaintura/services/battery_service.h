#ifndef BATTERY_SERVICE_H
#define BATTERY_SERVICE_H

#include "../models/battery.h"
#include "../../aditya_singh/database/database.h"
#include <string>

const int MAX_BATTERIES = 30;

// Simple Service to manage Battery Inventory and Swap operations
// Uses simple C++ arrays (no vectors)
// Created by: Amit Singh Kaintura (Battery & Swap Module Owner, ID: 2518188)
class BatteryService {
private:
    Database* db;
    Battery batteryArray[MAX_BATTERIES];
    int batteryCount;

public:
    BatteryService(Database* database = nullptr);

    // Get all batteries into outList, returns total count
    int getAllBatteries(Battery outList[], int maxCapacity);

    // Get battery by ID
    bool getBatteryById(const std::string& id, Battery& outBattery);

    // Find the first battery with status == "available" and charge >= 95%
    bool findReadyBattery(Battery& outBattery);

    // Swap battery for an EV: marks ready battery as "inuse", returns the given battery
    bool performSwap(const std::string& evId, Battery& outGivenBattery);

    // Return count of batteries ready for swap
    int getReadyCount();

    // Add new battery to local array
    bool addBattery(const Battery& b);
};

#endif
