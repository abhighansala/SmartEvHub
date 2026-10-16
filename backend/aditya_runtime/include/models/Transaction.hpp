#pragma once

#include <string>
#include <iostream>

namespace evhub {

/**
 * @brief Domain Entity representing a Financial/Audit Transaction.
 * Module Owner: Agreev Singh (Transactions, Reports & Analytics).
 * Pushed onto the manual LIFO Stack upon session completion.
 */
class Transaction{
private:
    int id_;
    int sessionId_;
    int userId_;
    int evId_;
    int stationId_;
    double amount_;
    double energyKwh_;
    std::string timestamp_;
    std::string paymentStatus_; // "PAID", "PENDING", "FAILED"

public:
    Transaction()
        : id_(0), sessionId_(0), userId_(0), evId_(0), stationId_(0),
          amount_(0.0), energyKwh_(0.0), timestamp_(""), paymentStatus_("PAID") {}

    Transaction(int id, int sessionId, int userId, int evId, int stationId,
                double amount, double energyKwh, std::string timestamp, std::string paymentStatus = "PAID")
        : id_(id), sessionId_(sessionId), userId_(userId), evId_(evId), stationId_(stationId),
          amount_(amount), energyKwh_(energyKwh), timestamp_(std::move(timestamp)), paymentStatus_(std::move(paymentStatus)) {}

    // Getters
    int getId() const { return id_; }
    int getSessionId() const { return sessionId_; }
    int getUserId() const { return userId_; }
    int getEvId() const { return evId_; }
    int getStationId() const { return stationId_; }
    double getAmount() const { return amount_; }
    double getEnergyKwh() const { return energyKwh_; }
    const std::string& getTimestamp() const { return timestamp_; }
    const std::string& getPaymentStatus() const { return paymentStatus_; }

    void setId(int id) { id_ = id; }

    // Stream operator for manual Stack display
    friend std::ostream& operator<<(std::ostream& os, const Transaction& tx) {
        os << "TX#" << tx.id_ << " [Session:" << tx.sessionId_ << " | Energy:"
           << tx.energyKwh_ << "kWh | Cost:$" << tx.amount_ << "]";
        return os;
    }
};

} // namespace evhub
