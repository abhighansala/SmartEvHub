#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <iostream>
#include <iomanip>

// ============================================================================
// Smart EV Hub - Transaction Domain Model
// Module Owner: Agreev Singh (Transactions + Reports + Analytics, ID: 2518161)
// ----------------------------------------------------------------------------
// Academic Syllabus Connections:
// 1. Data Abstraction & Encapsulation (OOP Lab 5 & 7):
//    Private data members with public getters/setters protecting internal state.
// 2. Constructors (OOP Lab 2 & 10):
//    Default constructor & parameterized constructor initializing all members.
// 3. Billing & Tariff Calculation (OOP Lab 1 Electricity Board & Lab 10 Invoice):
//    Calculates tiered charging costs, swap rates, and energy billing.
// 4. Operator Overloading (OOP Lab 14):
//    Overloading '==' and '!=' to compare transaction instances by unique ID.
// ============================================================================

class Transaction {
private:
    std::string id;                // Unique transaction ID (e.g. "TXN-5001")
    std::string vehicleId;         // EV identifier (e.g. "EV-103")
    std::string type;              // Operation type: "charging" or "battery-swap"
    std::string stationOrBattery;  // Resource ID: station bay (e.g. "CS-2") or battery (e.g. "BAT-03")
    std::string description;       // User-friendly activity description
    double amount;                 // Total billed amount in INR (₹)
    double energyDelivered;        // Energy transferred in kilowatt-hours (kWh)
    int duration;                  // Session duration in minutes
    std::string timestamp;         // Completion timestamp (YYYY-MM-DD HH:MM:SS)
    std::string status;            // Transaction status: "completed", "pending", "failed"

public:
    // ------------------------------------------------------------------------
    // Constructors (OOP Lab 2)
    // ------------------------------------------------------------------------
    
    // Default constructor: creates an empty transaction with sensible defaults
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

    // Parameterized constructor: fully initializes all transaction attributes
    Transaction(std::string tId, std::string vId, std::string tType,
                std::string target, std::string desc, double amt,
                double energy = 0.0, int dur = 0, std::string timeStr = "",
                std::string stat = "completed") {
        id = tId;
        vehicleId = vId;
        type = tType;
        stationOrBattery = target;
        description = desc;
        amount = (amt >= 0.0) ? amt : 0.0;
        energyDelivered = (energy >= 0.0) ? energy : 0.0;
        duration = (dur >= 0) ? dur : 0;
        timestamp = timeStr;
        status = stat.empty() ? "completed" : stat;
    }

    // ------------------------------------------------------------------------
    // Getters (Encapsulation - OOP Lab 5)
    // ------------------------------------------------------------------------
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

    // ------------------------------------------------------------------------
    // Setters (Encapsulation - OOP Lab 5)
    // ------------------------------------------------------------------------
    void setId(const std::string& newId) { id = newId; }
    void setVehicleId(const std::string& vId) { vehicleId = vId; }
    void setType(const std::string& newType) { type = newType; }
    void setStationOrBattery(const std::string& target) { stationOrBattery = target; }
    void setDescription(const std::string& desc) { description = desc; }
    void setAmount(double amt) { amount = (amt >= 0.0) ? amt : 0.0; }
    void setEnergyDelivered(double energy) { energyDelivered = (energy >= 0.0) ? energy : 0.0; }
    void setDuration(int dur) { duration = (dur >= 0) ? dur : 0; }
    void setTimestamp(const std::string& ts) { timestamp = ts; }
    void setStatus(const std::string& stat) { status = stat; }

    // ------------------------------------------------------------------------
    // Validation Rule Method
    // Ensures valid transactions before persisting to storage
    // ------------------------------------------------------------------------
    bool isValid() const {
        if (id.empty() || vehicleId.empty()) return false;
        if (type != "charging" && type != "battery-swap") return false;
        if (amount < 0.0 || energyDelivered < 0.0 || duration < 0) return false;
        return true;
    }

    // ------------------------------------------------------------------------
    // Tariff & Electricity Billing Calculator (OOP Lab 1 & Lab 10)
    // - For Charging: Base rate of ₹18/kWh. If energy > 30 kWh, 10% discount on units above 30.
    // - For Battery Swap: Fixed swap fee of ₹300 + ₹5/kWh of battery capacity.
    // ------------------------------------------------------------------------
    static double calculateTariff(double energyKwh, const std::string& opType) {
        if (energyKwh <= 0.0) return 50.0; // Minimum service fee

        if (opType == "battery-swap") {
            double baseSwapFee = 300.0;
            double capacityRate = energyKwh * 5.0;
            return baseSwapFee + capacityRate;
        } else {
            // Standard EV Charging tariff
            double baseRate = 18.0; // ₹18 per kWh
            double total = 0.0;
            if (energyKwh <= 30.0) {
                total = energyKwh * baseRate;
            } else {
                total = (30.0 * baseRate) + ((energyKwh - 30.0) * (baseRate * 0.90)); // 10% discount on higher usage
            }
            // Minimum billing floor of ₹100
            return (total < 100.0) ? 100.0 : total;
        }
    }

    // ------------------------------------------------------------------------
    // Operator Overloading (OOP Lab 14)
    // ------------------------------------------------------------------------
    bool operator==(const Transaction& other) const {
        return this->id == other.id;
    }

    bool operator!=(const Transaction& other) const {
        return !(*this == other);
    }

    // ------------------------------------------------------------------------
    // Formatted Receipt Display (OOP Lab 6 & 7)
    // ------------------------------------------------------------------------
    void displayReceipt() const {
        std::cout << "----------------------------------------\n";
        std::cout << "        SMART EV HUB RECEIPT            \n";
        std::cout << "----------------------------------------\n";
        std::cout << " Transaction ID : " << id << "\n";
        std::cout << " Vehicle ID     : " << vehicleId << "\n";
        std::cout << " Operation Type : " << type << "\n";
        std::cout << " Bay / Battery  : " << stationOrBattery << "\n";
        std::cout << " Energy (kWh)   : " << energyDelivered << " kWh\n";
        std::cout << " Duration       : " << duration << " mins\n";
        std::cout << " Total Amount   : INR " << amount << "\n";
        std::cout << " Status         : " << status << "\n";
        if (!timestamp.empty()) {
            std::cout << " Timestamp      : " << timestamp << "\n";
        }
        std::cout << "----------------------------------------\n";
    }
};

#endif // TRANSACTION_H
