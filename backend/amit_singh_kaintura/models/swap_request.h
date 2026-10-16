#ifndef SWAP_REQUEST_H
#define SWAP_REQUEST_H

#include <string>
#include <iostream>

// ============================================================================
// Smart EV Hub - Battery Swap Request Domain Model
// Module Owner: Amit Singh Kaintura (B.Tech 2nd Year, Section G, Student ID: 2518188)
// Role: Battery Inventory & Battery Swapping Module
//
// Syllabus References (TCS 307 - Object Oriented Programming with C++):
// - Unit 1 & 2: Classes & Objects, Encapsulation, Constructors (Default, Parameterized, Copy),
//   Destructors, 'this' pointer, Member functions, Data Hiding.
// - Unit 2: Operator Overloading (operator<< friend function).
// ============================================================================

class SwapRequest {
private:
    std::string id;               // e.g. "SWP-101"
    std::string vehicleId;        // EV requesting swap (e.g. "EV-102")
    std::string oldBatteryId;     // Depleted battery from the EV (e.g. "BAT-OLD")
    std::string newBatteryId;     // Fresh replacement battery allocated (e.g. "BAT-01")
    double requiredCapacity;      // Desired battery capacity in kWh (e.g. 50.0)
    std::string status;           // "pending", "completed", "queued", "cancelled"
    std::string timestamp;        // Request timestamp

public:
    // -------------------------------------------------------------------------
    // 1. Default Constructor
    // -------------------------------------------------------------------------
    SwapRequest() {
        this->id = "";
        this->vehicleId = "";
        this->oldBatteryId = "";
        this->newBatteryId = "";
        this->requiredCapacity = 50.0;
        this->status = "pending";
        this->timestamp = "";
    }

    // -------------------------------------------------------------------------
    // 2. Parameterized Constructor
    // Demonstrates 'this' pointer usage (Syllabus TCS 307 Unit 2)
    // -------------------------------------------------------------------------
    SwapRequest(std::string id, std::string vehicleId,
                std::string oldBatteryId = "", std::string newBatteryId = "",
                double requiredCapacity = 50.0, std::string status = "pending",
                std::string timestamp = "") {
        this->id = id;
        this->vehicleId = vehicleId;
        this->oldBatteryId = oldBatteryId;
        this->newBatteryId = newBatteryId;
        this->requiredCapacity = requiredCapacity;
        this->status = status;
        this->timestamp = timestamp;
    }

    // -------------------------------------------------------------------------
    // 3. Copy Constructor (Syllabus TCS 307 Unit 2)
    // -------------------------------------------------------------------------
    SwapRequest(const SwapRequest& other) {
        this->id = other.id;
        this->vehicleId = other.vehicleId;
        this->oldBatteryId = other.oldBatteryId;
        this->newBatteryId = other.newBatteryId;
        this->requiredCapacity = other.requiredCapacity;
        this->status = other.status;
        this->timestamp = other.timestamp;
    }

    // -------------------------------------------------------------------------
    // 4. Destructor
    // -------------------------------------------------------------------------
    ~SwapRequest() {
        // Clean destructor
    }

    // -------------------------------------------------------------------------
    // 5. Getters (Const Correctness)
    // -------------------------------------------------------------------------
    std::string getId() const { return id; }
    std::string getVehicleId() const { return vehicleId; }
    std::string getOldBatteryId() const { return oldBatteryId; }
    std::string getNewBatteryId() const { return newBatteryId; }
    double getRequiredCapacity() const { return requiredCapacity; }
    std::string getStatus() const { return status; }
    std::string getTimestamp() const { return timestamp; }

    // -------------------------------------------------------------------------
    // 6. Setters (Encapsulation)
    // -------------------------------------------------------------------------
    void setId(const std::string& newId) { this->id = newId; }
    void setVehicleId(const std::string& vId) { this->vehicleId = vId; }
    void setOldBatteryId(const std::string& bId) { this->oldBatteryId = bId; }
    void setNewBatteryId(const std::string& bId) { this->newBatteryId = bId; }
    void setRequiredCapacity(double cap) { if (cap > 0.0) this->requiredCapacity = cap; }
    void setStatus(const std::string& stat) { this->status = stat; }
    void setTimestamp(const std::string& time) { this->timestamp = time; }

    // -------------------------------------------------------------------------
    // 7. Domain Helper Methods
    // -------------------------------------------------------------------------
    bool isCompleted() const { return status == "completed"; }
    bool isQueued() const { return status == "queued"; }

    // -------------------------------------------------------------------------
    // 8. Stream Insertion Operator Overloading (Friend Function)
    // -------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, const SwapRequest& req) {
        os << "SwapRequest[ID: " << req.id
           << ", EV: " << req.vehicleId
           << ", OldBat: " << (req.oldBatteryId.empty() ? "N/A" : req.oldBatteryId)
           << ", NewBat: " << (req.newBatteryId.empty() ? "None" : req.newBatteryId)
           << ", ReqCap: " << req.requiredCapacity << "kWh"
           << ", Status: " << req.status
           << "]";
        return os;
    }
};

#endif // SWAP_REQUEST_H
