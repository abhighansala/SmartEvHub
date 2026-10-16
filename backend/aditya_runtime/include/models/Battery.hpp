#pragma once

#include "Types.hpp"
#include <string>

namespace evhub {

/**
 * @brief Domain Entity for Battery Swapping.
 * Module Owner: Amit Singh Kaintura (Battery & Battery Swapping).
 */
class Battery {
private:
    int id_;
    std::string serialNumber_;
    double capacityKwh_;
    double healthPercent_; // State of Health (SOH)
    double currentChargePercent_; // State of Charge (SOC)
    BatteryStatus status_;
    int assignedEvId_;

public:
    Battery()
        : id_(0), capacityKwh_(60.0), healthPercent_(98.0), currentChargePercent_(100.0),
          status_(BatteryStatus::READY), assignedEvId_(0) {}

    Battery(int id, std::string serial, double capacity, double health, double charge,
            BatteryStatus status = BatteryStatus::READY, int assignedEvId = 0)
        : id_(id), serialNumber_(std::move(serial)), capacityKwh_(capacity),
          healthPercent_(health), currentChargePercent_(charge), status_(status), assignedEvId_(assignedEvId) {}

    bool isReadyForSwap() const {
        return status_ == BatteryStatus::READY && currentChargePercent_ >= 90.0 && healthPercent_ >= 80.0;
    }

    void setStatus(BatteryStatus st) { status_ = st; }
    void setCurrentCharge(double charge) { currentChargePercent_ = charge; }

    int getId() const { return id_; }
    const std::string& getSerialNumber() const { return serialNumber_; }
    double getCapacityKwh() const { return capacityKwh_; }
    double getHealthPercent() const { return healthPercent_; }
    double getCurrentChargePercent() const { return currentChargePercent_; }
    BatteryStatus getStatus() const { return status_; }
    int getAssignedEvId() const { return assignedEvId_; }
};

} // namespace evhub
