#ifndef EV_H
#define EV_H

#include <string>

// Simple Electric Vehicle (EV) Domain Model
// Created by: Priyanshu Dutt (EV Management Module Owner, ID: 2519460)
// Enhanced for User Data Isolation & Ownership Control
class EV {
private:
    std::string id;              // e.g., "EV-101"
    std::string ownerName;       // Owner's full name
    std::string model;           // e.g., "Tata Nexon EV Max"
    double batteryCapacity;      // Total capacity in kWh
    double currentCharge;        // Current battery percentage (0.0 to 100.0)
    std::string status;          // "available", "charging", "waiting", "swapping"
    std::string lastActivity;    // Date/time string
    std::string registeredAt;    // Date/time string
    int userId;                  // Associated User account ID (from Users table, 0 if unassigned)

public:
    EV() {
        id = "";
        ownerName = "";
        model = "";
        batteryCapacity = 50.0;
        currentCharge = 0.0;
        status = "available";
        lastActivity = "";
        registeredAt = "";
        userId = 0;
    }

    EV(std::string evId, std::string owner, std::string evModel,
       double capacity, double charge, std::string evStatus = "available",
       std::string activity = "", std::string registered = "", int uid = 0) {
        id = evId;
        ownerName = owner;
        model = evModel;
        batteryCapacity = capacity;
        currentCharge = charge;
        status = evStatus;
        lastActivity = activity;
        registeredAt = registered;
        userId = uid;
    }

    // Getters
    const std::string& getId() const { return id; }
    const std::string& getOwnerName() const { return ownerName; }
    const std::string& getModel() const { return model; }
    double getBatteryCapacity() const { return batteryCapacity; }
    double getCurrentCharge() const { return currentCharge; }
    const std::string& getStatus() const { return status; }
    const std::string& getLastActivity() const { return lastActivity; }
    const std::string& getRegisteredAt() const { return registeredAt; }
    int getUserId() const { return userId; }

    // Setters
    void setId(const std::string& newId) { id = newId; }
    void setOwnerName(const std::string& newOwner) { ownerName = newOwner; }
    void setModel(const std::string& newModel) { model = newModel; }
    void setBatteryCapacity(double cap) { batteryCapacity = cap; }
    void setCurrentCharge(double charge) { currentCharge = charge; }
    void setStatus(const std::string& newStatus) { status = newStatus; }
    void setLastActivity(const std::string& activity) { lastActivity = activity; }
    void setRegisteredAt(const std::string& registered) { registeredAt = registered; }
    void setUserId(int uid) { userId = uid; }

    // Validation according to PRD Rules:
    // - Battery percentage must be between 0 and 100
    // - ID and Model must not be empty
    bool isValid() const {
        if (id.empty() || model.empty()) return false;
        if (currentCharge < 0.0 || currentCharge > 100.0) return false;
        if (batteryCapacity <= 0.0) return false;
        return true;
    }
};

#endif
