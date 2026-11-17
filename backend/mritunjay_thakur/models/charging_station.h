#ifndef CHARGING_STATION_H
#define CHARGING_STATION_H

#include <string>
#include <algorithm>

// Charging Station Domain Model
// Created by: Mritunjay Thakur (Charging Stations Module Owner, Student ID: 2519425, Section ML-5)
class ChargingStation {
private:
    std::string id;               // e.g., "CS-1"
    std::string name;             // e.g., "DC Fast Charger 1"
    double power;                 // kW rating (e.g., 50.0, 120.0)
    std::string connectorType;    // "Type 2", "CCS2", "CHAdeMO", "GB/T"
    std::string status;           // "available", "charging", "maintenance", "offline"
    std::string currentVehicleId; // ID of EV currently charging, or empty
    std::string location;         // e.g., "Bay A-1"
    std::string sessionStart;     // e.g., "2026-10-19 10:00:00" or empty
    int totalPorts;               // Default: 1
    int availablePorts;           // 0 when busy/offline/maintenance, 1 when available
    std::string createdAt;        // Creation timestamp
    std::string updatedAt;        // Last updated timestamp

public:
    ChargingStation() {
        id = "";
        name = "";
        power = 50.0;
        connectorType = "CCS2";
        status = "available";
        currentVehicleId = "";
        location = "Bay 1";
        sessionStart = "";
        totalPorts = 1;
        availablePorts = 1;
        createdAt = "";
        updatedAt = "";
    }

    ChargingStation(std::string sId, std::string sName, double sPower,
                    std::string sConnector, std::string sStatus = "available",
                    std::string vehicleId = "", std::string sLocation = "Bay 1",
                    std::string sSessionStart = "", int tPorts = 1, int aPorts = 1,
                    std::string cAt = "", std::string uAt = "") {
        id = sId;
        name = sName;
        power = (sPower > 0.0) ? sPower : 50.0;
        connectorType = sConnector.empty() ? "CCS2" : sConnector;
        status = normalizeStatus(sStatus.empty() ? "available" : sStatus);
        currentVehicleId = vehicleId;
        location = sLocation.empty() ? "Bay 1" : sLocation;
        sessionStart = sSessionStart;
        totalPorts = (tPorts > 0) ? tPorts : 1;
        availablePorts = (status == "available" && currentVehicleId.empty()) ? totalPorts : 0;
        createdAt = cAt;
        updatedAt = uAt;
    }

    // Static helper to normalize and validate status strings
    static std::string normalizeStatus(std::string raw) {
        std::string s = raw;
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        if (s == "available" || s == "charging" || s == "maintenance" || s == "offline") {
            return s;
        }
        return "available";
    }

    static bool isValidStatus(const std::string& raw) {
        std::string s = raw;
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return (s == "available" || s == "charging" || s == "maintenance" || s == "offline");
    }

    // Getters
    std::string getId() const { return id; }
    std::string getName() const { return name; }
    double getPower() const { return power; }
    std::string getConnectorType() const { return connectorType; }
    std::string getStatus() const { return status; }
    std::string getCurrentVehicleId() const { return currentVehicleId; }
    std::string getLocation() const { return location; }
    std::string getSessionStart() const { return sessionStart; }
    int getTotalPorts() const { return totalPorts; }
    int getAvailablePorts() const { return availablePorts; }
    std::string getCreatedAt() const { return createdAt; }
    std::string getUpdatedAt() const { return updatedAt; }

    // Setters
    void setId(const std::string& newId) { id = newId; }
    void setName(const std::string& newName) { name = newName; }
    void setPower(double newPower) { if (newPower > 0.0) power = newPower; }
    void setConnectorType(const std::string& newType) { connectorType = newType; }
    void setStatus(const std::string& newStatus) {
        status = normalizeStatus(newStatus);
        availablePorts = (status == "available" && currentVehicleId.empty()) ? totalPorts : 0;
    }
    void setCurrentVehicleId(const std::string& vehicleId) {
        currentVehicleId = vehicleId;
        availablePorts = (status == "available" && currentVehicleId.empty()) ? totalPorts : 0;
    }
    void setLocation(const std::string& newLocation) { location = newLocation; }
    void setSessionStart(const std::string& start) { sessionStart = start; }
    void setTotalPorts(int ports) { totalPorts = ports; }
    void setAvailablePorts(int ports) { availablePorts = ports; }
    void setCreatedAt(const std::string& dt) { createdAt = dt; }
    void setUpdatedAt(const std::string& dt) { updatedAt = dt; }

    // State transition helpers
    bool isAvailable() const {
        return status == "available" && currentVehicleId.empty();
    }

    bool isCharging() const {
        return status == "charging";
    }

    bool isMaintenance() const {
        return status == "maintenance";
    }

    bool isOffline() const {
        return status == "offline";
    }
};

#endif // CHARGING_STATION_H
