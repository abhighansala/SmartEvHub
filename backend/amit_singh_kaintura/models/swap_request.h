#ifndef SWAP_REQUEST_H
#define SWAP_REQUEST_H

#include <string>

// Simple Model for Battery Swap Request
// Created by: Amit Singh Kaintura (Battery & Swap Module Owner, ID: 2518188)
class SwapRequest {
private:
    std::string id;            // e.g., "SWP-101"
    std::string vehicleId;     // e.g., "EV-102"
    std::string oldBatteryId;  // Depleted battery from the EV
    std::string newBatteryId;  // Replacement battery given
    std::string status;        // "pending", "completed", "queued"

public:
    SwapRequest() {
        id = "";
        vehicleId = "";
        oldBatteryId = "";
        newBatteryId = "";
        status = "pending";
    }

    SwapRequest(std::string rId, std::string vId, std::string oldBat = "", std::string newBat = "", std::string stat = "pending") {
        id = rId;
        vehicleId = vId;
        oldBatteryId = oldBat;
        newBatteryId = newBat;
        status = stat;
    }

    std::string getId() const { return id; }
    std::string getVehicleId() const { return vehicleId; }
    std::string getOldBatteryId() const { return oldBatteryId; }
    std::string getNewBatteryId() const { return newBatteryId; }
    std::string getStatus() const { return status; }

    void setId(const std::string& newId) { id = newId; }
    void setVehicleId(const std::string& vId) { vehicleId = vId; }
    void setOldBatteryId(const std::string& bId) { oldBatteryId = bId; }
    void setNewBatteryId(const std::string& bId) { newBatteryId = bId; }
    void setStatus(const std::string& stat) { status = stat; }
};

#endif
