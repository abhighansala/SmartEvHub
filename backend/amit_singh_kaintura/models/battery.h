#ifndef BATTERY_H
#define BATTERY_H

#include <string>
#include <iostream>

// ============================================================================
// Smart EV Hub - Battery Domain Model
// Module Owner: Amit Singh Kaintura (B.Tech 2nd Year, Section G, Student ID: 2518188)
// Role: Battery Inventory & Battery Swapping Module
//
// Syllabus References (TCS 307 - Object Oriented Programming with C++):
// - Unit 1 & 2: Classes & Objects, Encapsulation, Constructors (Default, Parameterized, Copy),
//   Destructors, 'this' pointer, Member functions, Data Hiding, Arrays of Objects.
// - Unit 2: Operator Overloading (operator==, operator<< using friend function).
// ============================================================================

class Battery {
private:
    // -------------------------------------------------------------------------
    // Private Attributes (Encapsulation / Data Hiding)
    // -------------------------------------------------------------------------
    std::string id;              // Unique identifier, e.g. "BAT-01"
    double capacity;             // Total capacity in kWh (e.g. 50.0)
    double chargeLevel;          // Current charge percentage (0.0% - 100.0%)
    std::string status;          // "available" (ready for swap), "inuse", "charging", "maintenance"
    double health;               // State of Health (SoH) percentage (0.0% - 100.0%)
    int cycleCount;              // Lifetime charge cycles completed
    std::string currentVehicle;  // Vehicle ID if currently fitted in an EV (e.g. "EV-102")
    std::string chargingStart;   // Timestamp when battery began charging in dock
    double initialCharge;        // Starting charge percentage when placed in charging dock

public:
    // -------------------------------------------------------------------------
    // 1. Default Constructor
    // Initializes a default healthy, fully charged battery
    // -------------------------------------------------------------------------
    Battery() {
        this->id = "";
        this->capacity = 50.0;
        this->chargeLevel = 100.0;
        this->status = "available";
        this->health = 100.0;
        this->cycleCount = 0;
        this->currentVehicle = "";
        this->chargingStart = "";
        this->initialCharge = 100.0;
    }

    // -------------------------------------------------------------------------
    // 2. Parameterized Constructor
    // Initializes a battery with specific domain values
    // Demonstrates 'this' pointer usage (Syllabus TCS 307 Unit 2)
    // -------------------------------------------------------------------------
    Battery(std::string id, double capacity, double chargeLevel,
            std::string status = "available", double health = 100.0,
            int cycleCount = 0, std::string currentVehicle = "",
            std::string chargingStart = "", double initialCharge = 0.0) {
        this->id = id;
        this->capacity = capacity;
        this->chargeLevel = chargeLevel;
        this->status = status;
        this->health = health;
        this->cycleCount = cycleCount;
        this->currentVehicle = currentVehicle;
        this->chargingStart = chargingStart;
        this->initialCharge = (initialCharge > 0.0 ? initialCharge : chargeLevel);
    }

    // -------------------------------------------------------------------------
    // 3. Copy Constructor (Syllabus TCS 307 Unit 2)
    // -------------------------------------------------------------------------
    Battery(const Battery& other) {
        this->id = other.id;
        this->capacity = other.capacity;
        this->chargeLevel = other.chargeLevel;
        this->status = other.status;
        this->health = other.health;
        this->cycleCount = other.cycleCount;
        this->currentVehicle = other.currentVehicle;
        this->chargingStart = other.chargingStart;
        this->initialCharge = other.initialCharge;
    }

    // -------------------------------------------------------------------------
    // 4. Destructor (Syllabus TCS 307 Unit 2)
    // -------------------------------------------------------------------------
    ~Battery() {
        // Clean destructor for lifecycle completeness
    }

    // -------------------------------------------------------------------------
    // 5. Getters (Const Correctness for safe read-only access)
    // -------------------------------------------------------------------------
    std::string getId() const { return id; }
    double getCapacity() const { return capacity; }
    double getChargeLevel() const { return chargeLevel; }
    std::string getStatus() const { return status; }
    double getHealth() const { return health; }
    int getCycleCount() const { return cycleCount; }
    std::string getCurrentVehicle() const { return currentVehicle; }

    // -------------------------------------------------------------------------
    // 6. Setters (Encapsulation with validation)
    // -------------------------------------------------------------------------
    void setId(const std::string& newId) {
        this->id = newId;
    }

    void setCapacity(double cap) {
        if (cap > 0.0) {
            this->capacity = cap;
        }
    }

    void setChargeLevel(double charge) {
        if (charge < 0.0) charge = 0.0;
        if (charge > 100.0) charge = 100.0;
        this->chargeLevel = charge;
    }

    void setStatus(const std::string& newStatus) {
        this->status = newStatus;
    }

    void setHealth(double newHealth) {
        if (newHealth < 0.0) newHealth = 0.0;
        if (newHealth > 100.0) newHealth = 100.0;
        this->health = newHealth;
    }

    void setCycleCount(int count) {
        if (count >= 0) {
            this->cycleCount = count;
        }
    }

    void setCurrentVehicle(const std::string& vehicle) {
        this->currentVehicle = vehicle;
    }

    std::string getChargingStart() const { return chargingStart; }
    void setChargingStart(const std::string& start) { this->chargingStart = start; }

    double getInitialCharge() const { return initialCharge; }
    void setInitialCharge(double charge) {
        if (charge < 0.0) charge = 0.0;
        if (charge > 100.0) charge = 100.0;
        this->initialCharge = charge;
    }

    // -------------------------------------------------------------------------
    // 7. Domain Business Logic Methods & State Transitions
    // -------------------------------------------------------------------------
    bool isValidTransition(const std::string& targetStatus, const std::string& targetVehicle = "") const {
        if (targetStatus != "available" && targetStatus != "inuse" && 
            targetStatus != "charging" && targetStatus != "maintenance" && targetStatus != "unavailable") {
            return false;
        }
        if (targetStatus == status && targetVehicle == currentVehicle) {
            return true;
        }
        if (targetStatus == "inuse") {
            // Cannot be inuse without an assigned vehicle
            if (targetVehicle.empty()) return false;
            // Only available and ready batteries can transition to inuse
            if (status != "available" || !isReady()) return false;
            return true;
        }
        if (targetStatus == "available") {
            // An available battery must not have an active vehicle assignment
            if (!targetVehicle.empty()) return false;
            // Must have sufficient charge
            if (chargeLevel < 95.0) return false;
            return true;
        }
        if (targetStatus == "charging") {
            // A charging battery must be detached from any vehicle
            if (!targetVehicle.empty()) return false;
            return true;
        }
        if (targetStatus == "maintenance" || targetStatus == "unavailable") {
            return true;
        }
        return false;
    }

    // A battery is "Ready" for swapping only when:
    //  - Status is "available"
    //  - Charge level is >= 95.0%
    //  - State of Health (SoH) is >= 80.0%
    bool isReady() const {
        return (status == "available" && chargeLevel >= 95.0 && health >= 80.0);
    }

    // Check if the battery is depleted and needs charging (charge < 20%)
    bool isDepleted() const {
        return (chargeLevel < 20.0);
    }

    // Simulate charging the battery in a charging dock
    void chargeUp(double addedPercentage) {
        chargeLevel += addedPercentage;
        if (chargeLevel > 100.0) chargeLevel = 100.0;
        if (chargeLevel >= 95.0 && status == "charging") {
            status = "available";
        }
    }

    // Mark that a full charge cycle has concluded
    void incrementCycle() {
        cycleCount++;
        // Very slight health degradation simulation
        if (health > 60.0) {
            health -= 0.05;
        }
    }

    // -------------------------------------------------------------------------
    // 8. Operator Overloading (Syllabus TCS 307 Unit 2 & CO3)
    // -------------------------------------------------------------------------

    // Equality operator: batteries are identical if their IDs match
    bool operator==(const Battery& other) const {
        return this->id == other.id;
    }

    // Inequality operator
    bool operator!=(const Battery& other) const {
        return !(*this == other);
    }

    // Stream Insertion Operator (Friend function for easy printing & debugging)
    friend std::ostream& operator<<(std::ostream& os, const Battery& b) {
        os << "Battery[ID: " << b.id 
           << ", Cap: " << b.capacity << "kWh"
           << ", SoC: " << b.chargeLevel << "%"
           << ", SoH: " << b.health << "%"
           << ", Status: " << b.status
           << ", Vehicle: " << (b.currentVehicle.empty() ? "None" : b.currentVehicle)
           << "]";
        return os;
    }
};

#endif // BATTERY_H
