#ifndef CHARGING_SESSION_H
#define CHARGING_SESSION_H

#include <string>
#include <algorithm>

// Charging Session Domain Model
// Created by: Mritunjay Thakur (Charging Stations Module Owner, Student ID: 2519425, Section ML-5)
class ChargingSession {
private:
    std::string id;            // e.g., "SES-1001"
    std::string vehicleId;     // e.g., "EV-101"
    std::string evModel;       // e.g., "Tata Nexon EV Max"
    std::string stationId;     // e.g., "CS-1"
    std::string startTime;     // e.g., "2026-10-19 10:00:00"
    std::string endTime;       // e.g., "2026-10-19 10:30:00" or empty
    double batteryStart;       // Starting SoC % (e.g., 20.0)
    double batteryEnd;         // Ending SoC % (e.g., 85.0)
    double targetCharge;       // Target SoC % (default 85.0, max 100.0)
    double energyConsumed;     // Energy consumed in kWh
    int duration;              // Charging duration in minutes
    double estimatedCost;      // Total billed cost in INR
    std::string status;        // "active", "completed"
    int userId;                // ID of owner user
    std::string requestId;     // Optional associated ChargingRequest ID
    std::string bayId;         // Bay / Location string

public:
    ChargingSession() {
        id = "";
        vehicleId = "";
        evModel = "";
        stationId = "";
        startTime = "";
        endTime = "";
        batteryStart = 0.0;
        batteryEnd = 0.0;
        targetCharge = 85.0;
        energyConsumed = 0.0;
        duration = 0;
        estimatedCost = 0.0;
        status = "active";
        userId = 0;
        requestId = "";
        bayId = "";
    }

    ChargingSession(std::string sesId, std::string vId, std::string model,
                    std::string sId, double bStart, double target = 85.0,
                    std::string sTime = "", std::string eTime = "",
                    double bEnd = 0.0, double energy = 0.0, double cost = 0.0,
                    std::string stat = "active", int uId = 0,
                    std::string reqId = "", std::string bId = "", int dur = 0) {
        id = sesId;
        vehicleId = vId;
        evModel = model.empty() ? "Electric Vehicle" : model;
        stationId = sId;
        startTime = sTime;
        endTime = eTime;
        batteryStart = (bStart >= 0.0) ? bStart : 0.0;
        batteryEnd = (bEnd >= 0.0) ? bEnd : 0.0;
        targetCharge = (target > 0.0 && target <= 100.0) ? target : 85.0;
        energyConsumed = (energy >= 0.0) ? energy : 0.0;
        estimatedCost = (cost >= 0.0) ? cost : 0.0;
        status = (stat == "completed") ? "completed" : "active";
        userId = uId;
        requestId = reqId;
        bayId = bId;
        duration = (dur >= 0) ? dur : 0;
    }

    // Getters
    std::string getId() const { return id; }
    std::string getVehicleId() const { return vehicleId; }
    std::string getEvModel() const { return evModel; }
    std::string getStationId() const { return stationId; }
    std::string getStartTime() const { return startTime; }
    std::string getEndTime() const { return endTime; }
    double getBatteryStart() const { return batteryStart; }
    double getBatteryEnd() const { return batteryEnd; }
    double getTargetCharge() const { return targetCharge; }
    double getEnergyConsumed() const { return energyConsumed; }
    int getDuration() const { return duration; }
    double getEstimatedCost() const { return estimatedCost; }
    std::string getStatus() const { return status; }
    int getUserId() const { return userId; }
    std::string getRequestId() const { return requestId; }
    std::string getBayId() const { return bayId; }

    // Setters
    void setId(const std::string& newId) { id = newId; }
    void setVehicleId(const std::string& vId) { vehicleId = vId; }
    void setEvModel(const std::string& model) { evModel = model; }
    void setStationId(const std::string& sId) { stationId = sId; }
    void setStartTime(const std::string& sTime) { startTime = sTime; }
    void setEndTime(const std::string& eTime) { endTime = eTime; }
    void setBatteryStart(double start) { batteryStart = (start >= 0.0) ? start : 0.0; }
    void setBatteryEnd(double end) { batteryEnd = (end >= 0.0) ? end : 0.0; }
    void setTargetCharge(double target) { targetCharge = (target > 0.0 && target <= 100.0) ? target : 85.0; }
    void setEnergyConsumed(double energy) { energyConsumed = (energy >= 0.0) ? energy : 0.0; }
    void setDuration(int dur) { duration = (dur >= 0) ? dur : 0; }
    void setEstimatedCost(double cost) { estimatedCost = (cost >= 0.0) ? cost : 0.0; }
    void setStatus(const std::string& newStatus) { status = newStatus; }
    void setUserId(int uId) { userId = uId; }
    void setRequestId(const std::string& reqId) { requestId = reqId; }
    void setBayId(const std::string& bId) { bayId = bId; }

    bool isActive() const { return status == "active"; }
    bool isCompleted() const { return status == "completed"; }
};

#endif // CHARGING_SESSION_H
