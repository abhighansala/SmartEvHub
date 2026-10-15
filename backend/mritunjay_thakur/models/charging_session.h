#ifndef CHARGING_SESSION_H
#define CHARGING_SESSION_H

#include <string>

// Simple Charging Session Model
// Created by: Mritunjay Thakur (Charging Stations Module Owner, ID: 2519425)
class ChargingSession {
private:
    std::string id;            // e.g., "SES-1001"
    std::string vehicleId;     // e.g., "EV-101"
    std::string evModel;       // e.g., "Tata Nexon EV Max"
    std::string stationId;     // e.g., "CS-1"
    double batteryStart;       // Starting %
    double batteryEnd;         // Ending %
    double targetCharge;       // Target % (default 100.0)
    double energyConsumed;     // in kWh
    double estimatedCost;      // in INR
    std::string status;        // "active", "completed"

public:
    ChargingSession() {
        id = "";
        vehicleId = "";
        evModel = "";
        stationId = "";
        batteryStart = 0.0;
        batteryEnd = 0.0;
        targetCharge = 100.0;
        energyConsumed = 0.0;
        estimatedCost = 0.0;
        status = "active";
    }

    ChargingSession(std::string sesId, std::string vId, std::string model,
                    std::string sId, double bStart, double target = 100.0) {
        id = sesId;
        vehicleId = vId;
        evModel = model;
        stationId = sId;
        batteryStart = bStart;
        batteryEnd = 0.0;
        targetCharge = target;
        energyConsumed = 0.0;
        estimatedCost = 0.0;
        status = "active";
    }

    // Getters
    std::string getId() const { return id; }
    std::string getVehicleId() const { return vehicleId; }
    std::string getEvModel() const { return evModel; }
    std::string getStationId() const { return stationId; }
    double getBatteryStart() const { return batteryStart; }
    double getBatteryEnd() const { return batteryEnd; }
    double getTargetCharge() const { return targetCharge; }
    double getEnergyConsumed() const { return energyConsumed; }
    double getEstimatedCost() const { return estimatedCost; }
    std::string getStatus() const { return status; }

    // Setters
    void setId(const std::string& newId) { id = newId; }
    void setVehicleId(const std::string& vId) { vehicleId = vId; }
    void setEvModel(const std::string& model) { evModel = model; }
    void setStationId(const std::string& sId) { stationId = sId; }
    void setBatteryStart(double start) { batteryStart = start; }
    void setBatteryEnd(double end) { batteryEnd = end; }
    void setTargetCharge(double target) { targetCharge = target; }
    void setEnergyConsumed(double energy) { energyConsumed = energy; }
    void setEstimatedCost(double cost) { estimatedCost = cost; }
    void setStatus(const std::string& newStatus) { status = newStatus; }
};

#endif
