#ifndef BATTERY_H
#define BATTERY_H

#include <string>

// Simple Battery Domain Model for Swap Inventory
// Created by: Amit Singh Kaintura (Battery & Swap Module Owner, ID: 2518188)
class Battery {
private:
    std::string id;              // e.g., "BAT-01"
    double capacity;             // in kWh (e.g., 50.0)
    double chargeLevel;          // Current % (0.0 to 100.0)
    std::string status;          // "available" (READY), "inuse", "charging", "maintenance"
    double health;               // State of Health % (e.g., 98.5)
    int cycleCount;              // Lifetime charge cycles
    std::string currentVehicle;  // Vehicle ID if currently assigned

public:
    Battery() {
        id = "";
        capacity = 50.0;
        chargeLevel = 100.0;
        status = "available";
        health = 100.0;
        cycleCount = 0;
        currentVehicle = "";
    }

    Battery(std::string bId, double bCap, double bCharge,
            std::string bStatus = "available", double bHealth = 100.0,
            int cycles = 0, std::string vehicle = "") {
        id = bId;
        capacity = bCap;
        chargeLevel = bCharge;
        status = bStatus;
        health = bHealth;
        cycleCount = cycles;
        currentVehicle = vehicle;
    }

    // Getters
    std::string getId() const { return id; }
    double getCapacity() const { return capacity; }
    double getChargeLevel() const { return chargeLevel; }
    std::string getStatus() const { return status; }
    double getHealth() const { return health; }
    int getCycleCount() const { return cycleCount; }
    std::string getCurrentVehicle() const { return currentVehicle; }

    // Setters
    void setId(const std::string& newId) { id = newId; }
    void setCapacity(double cap) { capacity = cap; }
    void setChargeLevel(double charge) { chargeLevel = charge; }
    void setStatus(const std::string& newStatus) { status = newStatus; }
    void setHealth(double newHealth) { health = newHealth; }
    void setCycleCount(int count) { cycleCount = count; }
    void setCurrentVehicle(const std::string& vehicle) { currentVehicle = vehicle; }

    // Helper: is battery ready to be swapped into an EV?
    bool isReady() const {
        return (status == "available" && chargeLevel >= 95.0 && health >= 80.0);
    }
};

#endif
