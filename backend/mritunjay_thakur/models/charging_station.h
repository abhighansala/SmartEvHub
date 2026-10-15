#ifndef CHARGING_STATION_H
#define CHARGING_STATION_H

#include <string>

// Simple Charging Station Domain Model
// Created by: Mritunjay Thakur (Charging Stations Module Owner, ID: 2519425)
class ChargingStation {
private:
    std::string id;               // e.g., "CS-1"
    std::string name;             // e.g., "DC Fast Charger 1"
    double power;                 // kW rating (e.g., 50.0, 120.0)
    std::string connectorType;    // "Type 2", "CCS2", "CHAdeMO", "GB/T"
    std::string status;           // "available", "charging", "maintenance", "offline"
    std::string currentVehicleId; // ID of EV currently charging, or empty
    std::string location;         // e.g., "Bay A-1"

public:
    ChargingStation() {
        id = "";
        name = "";
        power = 50.0;
        connectorType = "CCS2";
        status = "available";
        currentVehicleId = "";
        location = "Bay 1";
    }

    ChargingStation(std::string sId, std::string sName, double sPower,
                    std::string sConnector, std::string sStatus = "available",
                    std::string vehicleId = "", std::string sLocation = "Bay 1") {
        id = sId;
        name = sName;
        power = sPower;
        connectorType = sConnector;
        status = sStatus;
        currentVehicleId = vehicleId;
        location = sLocation;
    }

    // Getters
    std::string getId() const { return id; }
    std::string getName() const { return name; }
    double getPower() const { return power; }
    std::string getConnectorType() const { return connectorType; }
    std::string getStatus() const { return status; }
    std::string getCurrentVehicleId() const { return currentVehicleId; }
    std::string getLocation() const { return location; }

    // Setters
    void setId(const std::string& newId) { id = newId; }
    void setName(const std::string& newName) { name = newName; }
    void setPower(double newPower) { power = newPower; }
    void setConnectorType(const std::string& newType) { connectorType = newType; }
    void setStatus(const std::string& newStatus) { status = newStatus; }
    void setCurrentVehicleId(const std::string& vehicleId) { currentVehicleId = vehicleId; }
    void setLocation(const std::string& newLocation) { location = newLocation; }

    // Helper: is station ready to accept an EV?
    bool isAvailable() const {
        return status == "available" && currentVehicleId.empty();
    }
};

#endif
