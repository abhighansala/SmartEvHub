#pragma once

#include "Types.hpp"
#include <string>
#include <stdexcept>

namespace evhub {

/**
 * @brief Domain Entity representing a Charging Session lifecycle.
 * Invariant: A session cannot be completed twice, and energy delivered cannot be negative.
 */
class ChargingSession {
private:
    int id_;
    int stationId_;
    int evId_;
    int userId_;
    std::string startTime_;
    std::string endTime_;
    double initialSocPercent_;
    double finalSocPercent_;
    double energyDeliveredKwh_;
    double totalCost_;
    SessionStatus status_;

public:
    ChargingSession()
        : id_(0), stationId_(0), evId_(0), userId_(0),
          initialSocPercent_(0.0), finalSocPercent_(0.0), energyDeliveredKwh_(0.0),
          totalCost_(0.0), status_(SessionStatus::REQUESTED) {}

    ChargingSession(int id, int stationId, int evId, int userId,
                    std::string startTime, double initialSocPercent,
                    SessionStatus status = SessionStatus::IN_PROGRESS)
        : id_(id), stationId_(stationId), evId_(evId), userId_(userId),
          startTime_(std::move(startTime)), endTime_(""), initialSocPercent_(initialSocPercent),
          finalSocPercent_(initialSocPercent), energyDeliveredKwh_(0.0), totalCost_(0.0), status_(status) {}

    bool complete(std::string endTime, double finalSoc, double energyKwh, double cost) {
        if (status_ == SessionStatus::COMPLETED) {
            throw std::logic_error("Invariant violation: Charging session is already completed");
        }
        if (status_ != SessionStatus::IN_PROGRESS) {
            return false;
        }
        if (finalSoc < initialSocPercent_) {
            throw std::invalid_argument("Final SOC cannot be lower than initial SOC for charging session");
        }
        endTime_ = std::move(endTime);
        finalSocPercent_ = finalSoc;
        energyDeliveredKwh_ = energyKwh;
        totalCost_ = cost;
        status_ = SessionStatus::COMPLETED;
        return true;
    }

    void cancel() {
        if (status_ == SessionStatus::COMPLETED) {
            throw std::logic_error("Cannot cancel an already completed charging session");
        }
        status_ = SessionStatus::CANCELLED;
    }

    // Getters
    int getId() const { return id_; }
    int getStationId() const { return stationId_; }
    int getEvId() const { return evId_; }
    int getUserId() const { return userId_; }
    const std::string& getStartTime() const { return startTime_; }
    const std::string& getEndTime() const { return endTime_; }
    double getInitialSocPercent() const { return initialSocPercent_; }
    double getFinalSocPercent() const { return finalSocPercent_; }
    double getEnergyDeliveredKwh() const { return energyDeliveredKwh_; }
    double getTotalCost() const { return totalCost_; }
    SessionStatus getStatus() const { return status_; }

    void setId(int id) { id_ = id; }
    void setStationId(int stationId) { stationId_ = stationId; }
    void setStatus(SessionStatus st) { status_ = st; }
};

} // namespace evhub
