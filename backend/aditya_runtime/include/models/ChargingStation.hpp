#pragma once

#include "Types.hpp"
#include <string>
#include <stdexcept>

namespace evhub {

/**
 * @brief Domain Entity representing a physical Charging Station / Dispenser.
 * Module Owner: Mritunjay Thakur (Charging Stations).
 * Enforces valid finite-state machine transitions.
 */
class ChargingStation {
private:
    int id_;
    std::string name_;
    PortType portType_;
    double maxPowerKw_;
    StationStatus status_;
    int currentSessionId_;
    int assignedEvId_;
    double totalEnergyDispensedKwh_;

public:
    ChargingStation()
        : id_(0), name_(""), portType_(PortType::CCS2), maxPowerKw_(50.0),
          status_(StationStatus::AVAILABLE), currentSessionId_(0), assignedEvId_(0), totalEnergyDispensedKwh_(0.0) {}

    ChargingStation(int id, std::string name, PortType portType, double maxPowerKw,
                    StationStatus status = StationStatus::AVAILABLE,
                    int currentSessionId = 0, int assignedEvId = 0, double totalEnergyDispensed = 0.0)
        : id_(id), name_(std::move(name)), portType_(portType), maxPowerKw_(maxPowerKw),
          status_(status), currentSessionId_(currentSessionId), assignedEvId_(assignedEvId),
          totalEnergyDispensedKwh_(totalEnergyDispensed) {}

    // State machine transitions
    bool isAvailable() const {
        return status_ == StationStatus::AVAILABLE && currentSessionId_ == 0;
    }

    bool assignSession(int sessionId, int evId) {
        if (!isAvailable()) {
            return false;
        }
        status_ = StationStatus::CHARGING;
        currentSessionId_ = sessionId;
        assignedEvId_ = evId;
        return true;
    }

    bool releaseSession(double dispensedEnergyKwh) {
        if (status_ != StationStatus::CHARGING) {
            return false;
        }
        status_ = StationStatus::AVAILABLE;
        currentSessionId_ = 0;
        assignedEvId_ = 0;
        totalEnergyDispensedKwh_ += dispensedEnergyKwh;
        return true;
    }

    void setStatus(StationStatus newStatus) {
        if (status_ == StationStatus::CHARGING && newStatus != StationStatus::CHARGING) {
            throw std::runtime_error("Cannot change station status while an active charging session is in progress");
        }
        status_ = newStatus;
    }

    // Getters
    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    PortType getPortType() const { return portType_; }
    double getMaxPowerKw() const { return maxPowerKw_; }
    StationStatus getStatus() const { return status_; }
    int getCurrentSessionId() const { return currentSessionId_; }
    int getAssignedEvId() const { return assignedEvId_; }
    double getTotalEnergyDispensedKwh() const { return totalEnergyDispensedKwh_; }

    void setId(int id) { id_ = id; }
};

} // namespace evhub
