#ifndef CHARGING_REQUEST_H
#define CHARGING_REQUEST_H

#include <string>

// ============================================================================
// Smart EV Hub - Charging Request Domain Model
// Represents a customer intake charging request before or during bay allocation
// ============================================================================
class ChargingRequest {
private:
    std::string id;           // Unique ID e.g. "REQ-1001"
    int userId;               // ID of the authenticated user
    std::string vehicleId;    // Vehicle ID e.g. "EV-101"
    std::string stationId;    // Requested or assigned station ID
    std::string bayId;        // Bay / Location string
    double targetCharge;      // Requested target SoC percentage (e.g. 85.0)
    std::string status;       // "queued", "allocated", "active", "completed", "cancelled", "failed"
    std::string createdAt;    // Server creation timestamp

public:
    ChargingRequest()
        : id(""), userId(0), vehicleId(""), stationId(""), bayId(""),
          targetCharge(85.0), status("queued"), createdAt("") {}

    ChargingRequest(std::string rId, int uId, std::string vId, std::string sId,
                    std::string bId, double target, std::string stat = "queued",
                    std::string created = "")
        : id(rId), userId(uId), vehicleId(vId), stationId(sId), bayId(bId),
          targetCharge((target > 0.0 && target <= 100.0) ? target : 85.0),
          status(stat.empty() ? "queued" : stat), createdAt(created) {}

    // Getters
    std::string getId() const { return id; }
    int getUserId() const { return userId; }
    std::string getVehicleId() const { return vehicleId; }
    std::string getStationId() const { return stationId; }
    std::string getBayId() const { return bayId; }
    double getTargetCharge() const { return targetCharge; }
    std::string getStatus() const { return status; }
    std::string getCreatedAt() const { return createdAt; }

    // Setters
    void setId(const std::string& newId) { id = newId; }
    void setUserId(int uId) { userId = uId; }
    void setVehicleId(const std::string& vId) { vehicleId = vId; }
    void setStationId(const std::string& sId) { stationId = sId; }
    void setBayId(const std::string& bId) { bayId = bId; }
    void setTargetCharge(double target) { targetCharge = target; }
    void setStatus(const std::string& stat) { status = stat; }
    void setCreatedAt(const std::string& created) { createdAt = created; }
};

#endif // CHARGING_REQUEST_H
