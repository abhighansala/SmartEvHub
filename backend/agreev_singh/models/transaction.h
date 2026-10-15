#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

// Simple Transaction Domain Model
// Created by: Agreev Singh (Transactions + Reports + Analytics Owner, ID: 2518161)
class Transaction {
private:
    std::string id;                // e.g., "TXN-5001"
    std::string vehicleId;         // e.g., "EV-103"
    std::string type;              // "charging" or "battery-swap"
    std::string stationOrBattery;  // e.g., "CS-2" or "BAT-03"
    std::string description;       // e.g., "Fast charging 20% to 80%"
    double amount;                 // Cost in INR (e.g., 870.0)
    double energyDelivered;        // Energy in kWh (e.g., 43.5)
    int duration;                  // Duration in minutes (e.g., 60)
    std::string timestamp;         // Date/time string
    std::string status;            // "completed", "pending", "failed"

public:
    Transaction() {
        id = "";
        vehicleId = "";
        type = "charging";
        stationOrBattery = "";
        description = "";
        amount = 0.0;
        energyDelivered = 0.0;
        duration = 0;
        timestamp = "";
        status = "completed";
    }

    Transaction(std::string tId, std::string vId, std::string tType,
                std::string target, std::string desc, double amt,
                double energy = 0.0, int dur = 0, std::string timeStr = "",
                std::string stat = "completed") {
        id = tId;
        vehicleId = vId;
        type = tType;
        stationOrBattery = target;
        description = desc;
        amount = amt;
        energyDelivered = energy;
        duration = dur;
        timestamp = timeStr;
        status = stat;
    }

    // Getters
    std::string getId() const { return id; }
    std::string getVehicleId() const { return vehicleId; }
    std::string getType() const { return type; }
    std::string getStationOrBattery() const { return stationOrBattery; }
    std::string getDescription() const { return description; }
    double getAmount() const { return amount; }
    double getEnergyDelivered() const { return energyDelivered; }
    int getDuration() const { return duration; }
    std::string getTimestamp() const { return timestamp; }
    std::string getStatus() const { return status; }

    // Setters
    void setId(const std::string& newId) { id = newId; }
    void setVehicleId(const std::string& vId) { vehicleId = vId; }
    void setType(const std::string& newType) { type = newType; }
    void setStationOrBattery(const std::string& target) { stationOrBattery = target; }
    void setDescription(const std::string& desc) { description = desc; }
    void setAmount(double amt) { amount = amt; }
    void setEnergyDelivered(double energy) { energyDelivered = energy; }
    void setDuration(int dur) { duration = dur; }
    void setTimestamp(const std::string& ts) { timestamp = ts; }
    void setStatus(const std::string& stat) { status = stat; }
};

#endif
