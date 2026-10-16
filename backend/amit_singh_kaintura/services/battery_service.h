#ifndef BATTERY_SERVICE_H
#define BATTERY_SERVICE_H

#include "../models/battery.h"
#include "../../aditya_singh/database/database.h"
#include <string>

// ============================================================================
// Smart EV Hub - Battery Service
// Module Owner: Amit Singh Kaintura (B.Tech 2nd Year, Section G, Student ID: 2518188)
// Role: Battery Inventory & Battery Swapping Module
//
// Syllabus References:
// - TCS 307 (OOP with C++):
//   * Classes & Objects, Encapsulation, Function Overloading (findReadyBattery, performSwap)
//   * Const correctness, Array of objects as data members, passing/returning objects.
// - TCS 302 (Data Structures with C):
//   * Linear Array Data Structure (fixed-size array batteryArray[MAX_BATTERIES])
//   * Linear Search Algorithm (O(N) time complexity to find available batteries)
//   * Sequential Record Management (traversing, inserting, and updating array elements)
// ============================================================================

const int MAX_BATTERIES = 50;

class BatteryService {
private:
    Database* db;                           // Pointer to shared SQLite database
    Battery batteryArray[MAX_BATTERIES];    // Fixed-size array of Battery objects (no vectors)
    int batteryCount;                       // Current count of batteries in array

    // Helper method to populate initial mock inventory if DB is empty or unavailable
    void seedDefaultBatteries();

public:
    // -------------------------------------------------------------------------
    // Constructor: accepts optional Database pointer
    // -------------------------------------------------------------------------
    BatteryService(Database* database = nullptr);

    // -------------------------------------------------------------------------
    // 1. Inventory Retrieval Operations (TCS 302: Linear Traversal)
    // -------------------------------------------------------------------------
    
    // Copies all batteries into outList array, returns total count copied
    int getAllBatteries(Battery outList[], int maxCapacity);

    // Linear search for battery by ID (O(N) search)
    bool getBatteryById(const std::string& id, Battery& outBattery);

    // Total count of batteries currently tracked
    int getBatteryCount() const { return batteryCount; }

    // Count of batteries currently ready for an immediate swap
    int getReadyCount();

    // -------------------------------------------------------------------------
    // 2. Battery Selection Algorithms (TCS 307: Function Overloading)
    // -------------------------------------------------------------------------

    // Basic find: selects first battery ready for swap (available, SoC >= 95%, SoH >= 80%)
    bool findReadyBattery(Battery& outBattery);

    // Overloaded find: selects ready battery matching desired capacity (kWh)
    bool findReadyBattery(double requiredCapacity, Battery& outBattery);

    // -------------------------------------------------------------------------
    // 3. Swap Workflow Operations (TCS 307: Function Overloading)
    // -------------------------------------------------------------------------

    // Standard swap: allocates ready battery to EV, marks battery as 'inuse'
    bool performSwap(const std::string& evId, Battery& outGivenBattery);

    // Enhanced swap: handles incoming depleted battery, allocates fresh battery
    bool performSwap(const std::string& evId, const std::string& oldBatteryId,
                     double requiredCapacity, Battery& outGivenBattery);

    // -------------------------------------------------------------------------
    // 4. Inventory Mutation Operations (Add, Update, Charge)
    // -------------------------------------------------------------------------

    // Add a new battery to inventory (updates array and SQLite if open)
    bool addBattery(const Battery& b);

    // Update existing battery attributes
    bool updateBattery(const Battery& b);

    // Mark a battery as charging in a charging bay
    bool markBatteryCharging(const std::string& batteryId);
};

#endif // BATTERY_SERVICE_H
