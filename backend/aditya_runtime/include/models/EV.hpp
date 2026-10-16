#pragma once

#include "Types.hpp"
#include <string>
#include <stdexcept>

namespace evhub {

/**
 * @brief Domain Entity representing an Electric Vehicle.
 * Module Owner: Priyanshu Dutt (EV Management).
 */
class EV {
private:
    int id_;
    int userId_;
    std::string vin_;
    std::string make_;
    std::string model_;
    std::string licensePlate_;
    double batteryCapacityKwh_;
    double currentSocPercent_; // 0.0 to 100.0
    PortType supportedPort_;

public:
    EV() : id_(0), userId_(0), batteryCapacityKwh_(50.0), currentSocPercent_(20.0), supportedPort_(PortType::CCS2) {}

    EV(int id, int userId, std::string vin, std::string make, std::string model,
       std::string licensePlate, double batteryCapacityKwh, double currentSocPercent, PortType supportedPort)
        : id_(id), userId_(userId), vin_(std::move(vin)), make_(std::move(make)), model_(std::move(model)),
          licensePlate_(std::move(licensePlate)), supportedPort_(supportedPort) {
        setBatteryCapacityKwh(batteryCapacityKwh);
        setCurrentSocPercent(currentSocPercent);
    }

    // Encapsulation with business validations
    void setBatteryCapacityKwh(double capacity) {
        if (capacity <= 0.0 || capacity > 300.0) {
            throw std::invalid_argument("Battery capacity must be between 0.1 kWh and 300.0 kWh");
        }
        batteryCapacityKwh_ = capacity;
    }

    void setCurrentSocPercent(double soc) {
        if (soc < 0.0 || soc > 100.0) {
            throw std::invalid_argument("State of Charge (SOC) must be between 0% and 100%");
        }
        currentSocPercent_ = soc;
    }

    // Getters
    int getId() const { return id_; }
    int getUserId() const { return userId_; }
    const std::string& getVin() const { return vin_; }
    const std::string& getMake() const { return make_; }
    const std::string& getModel() const { return model_; }
    const std::string& getLicensePlate() const { return licensePlate_; }
    double getBatteryCapacityKwh() const { return batteryCapacityKwh_; }
    double getCurrentSocPercent() const { return currentSocPercent_; }
    PortType getSupportedPort() const { return supportedPort_; }

    void setId(int id) { id_ = id; }
    void setLicensePlate(const std::string& plate) { licensePlate_ = plate; }
};

} // namespace evhub
