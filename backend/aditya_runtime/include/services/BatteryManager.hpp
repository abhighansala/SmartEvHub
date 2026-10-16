#pragma once

#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include <algorithm>
#include <stdexcept>

#include "../models/Battery.hpp"
#include "../models/Types.hpp"
#include "../models/Transaction.hpp"
#include "../dsa/ManualStack.hpp"

namespace evhub {

/**
 * @brief Coordinates Battery Swap Station Inventory & Swap Workflows.
 * Module Owner: Amit Singh Kaintura (Battery & Battery Swapping).
 * Enforces battery safety criteria: SOC >= 90% and SOH >= 80% for swap readiness.
 */
class BatteryManager {
private:
    std::vector<Battery> inventory_;
    mutable std::mutex batteryMutex_;
    const double SWAP_FEE = 12.00; // Flat swap rate

public:
    BatteryManager() {
        // Initialize sample battery rack
        inventory_.emplace_back(1, "BAT-LFP-8001", 60.0, 98.5, 96.0, BatteryStatus::READY);
        inventory_.emplace_back(2, "BAT-LFP-8002", 60.0, 97.0, 94.0, BatteryStatus::READY);
        inventory_.emplace_back(3, "BAT-LFP-8003", 60.0, 99.0, 45.0, BatteryStatus::CHARGING);
        inventory_.emplace_back(4, "BAT-NMC-8004", 75.0, 95.0, 92.0, BatteryStatus::READY);
        inventory_.emplace_back(5, "BAT-NMC-8005", 75.0, 89.0, 30.0, BatteryStatus::CHARGING);
        inventory_.emplace_back(6, "BAT-LFP-8006", 60.0, 72.0, 10.0, BatteryStatus::FAULTY);
    }

    std::vector<Battery> getAllBatteries() const {
        std::lock_guard<std::mutex> lock(batteryMutex_);
        return inventory_;
    }

    size_t getReadyCount() const {
        std::lock_guard<std::mutex> lock(batteryMutex_);
        size_t count = 0;
        for (const auto& b : inventory_) {
            if (b.isReadyForSwap()) count++;
        }
        return count;
    }

    struct SwapResult {
        bool success;
        int installedBatteryId;
        double amountCharged;
        std::string message;
    };

    SwapResult executeSwap(int evId, double depletedSoc, ManualStack<Transaction>& txStack, int nextTxId, const std::string& timestamp) {
        std::lock_guard<std::mutex> lock(batteryMutex_);
        SwapResult result{false, 0, 0.0, ""};

        // 1. Find an eligible READY battery
        auto it = std::find_if(inventory_.begin(), inventory_.end(), [](const Battery& b) {
            return b.isReadyForSwap();
        });

        if (it == inventory_.end()) {
            result.message = "No fully charged (>90% SOC) and healthy (>80% SOH) battery pack available in rack.";
            return result;
        }

        // 2. Perform swap: mark pack as IN_USE
        it->setStatus(BatteryStatus::IN_USE);
        int installedId = it->getId();

        // 3. Mark the removed depleted battery as CHARGING in the bay
        auto depletedIt = std::find_if(inventory_.begin(), inventory_.end(), [](const Battery& b) {
            return b.getStatus() == BatteryStatus::IN_USE;
        });
        if (depletedIt != inventory_.end()) {
            depletedIt->setStatus(BatteryStatus::CHARGING);
            depletedIt->setCurrentCharge(depletedSoc);
        }

        // 4. Create Swap Transaction and push to LIFO Stack
        Transaction tx(nextTxId, 0, 1, evId, 0, SWAP_FEE, 60.0, timestamp, "PAID");
        txStack.push(tx);

        result.success = true;
        result.installedBatteryId = installedId;
        result.amountCharged = SWAP_FEE;
        result.message = "Battery successfully swapped in 3 minutes. Installed Pack #" + std::to_string(installedId) + " at 96% SOC.";
        return result;
    }
};

} // namespace evhub
