#include "swap_routes.h"

// ============================================================================
// Smart EV Hub - Battery Swap Workflow REST API Endpoints
// Module Owner: Amit Singh Kaintura (B.Tech 2nd Year, Section G, Student ID: 2518188)
// Role: Battery Inventory & Battery Swapping Module
//
// Crow Helper API:
// Communicates between C++ backend and React frontend via JSON.
// ============================================================================

void registerSwapRoutes(crow::App<crow::CORSHandler>& app,
                        BatteryService& batteryService,
                        SwapQueue& swapQueue,
                        TransactionStack* txnStack) {

    // -------------------------------------------------------------------------
    // 1. POST /api/swap/request - Request an immediate or queued battery swap
    // -------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/swap/request").methods(crow::HTTPMethod::POST)
    ([&batteryService, &swapQueue, txnStack](const crow::request& req) {
        crow::json::wvalue res;
        auto json = crow::json::load(req.body);
        if (!json || !json.has("evId")) {
            res["success"] = false;
            res["message"] = "Field 'evId' is required in JSON payload";
            return crow::response(400, res);
        }

        std::string evId = std::string(json["evId"].s());
        if (evId.empty()) {
            res["success"] = false;
            res["message"] = "Vehicle ID cannot be empty";
            return crow::response(400, res);
        }

        // Prevent duplicate queue entry
        if (swapQueue.contains(evId)) {
            res["success"] = false;
            res["swapped"] = false;
            res["queued"] = true;
            res["message"] = "Vehicle " + evId + " is already waiting in the Swap Queue.";
            return crow::response(400, res);
        }

        std::string currentBattery = "";
        if (json.has("currentBattery")) {
            currentBattery = std::string(json["currentBattery"].s());
        }

        double requiredCapacity = 0.0;
        if (json.has("requiredCapacity")) {
            requiredCapacity = json["requiredCapacity"].d();
        }

        // FIFO preservation: If queue is empty, attempt immediate swap with available inventory
        if (swapQueue.isEmpty()) {
            Battery readyBattery;
            bool found = false;
            if (requiredCapacity > 0.0) {
                found = batteryService.findReadyBattery(requiredCapacity, readyBattery);
            } else {
                found = batteryService.findReadyBattery(readyBattery);
            }

            if (found) {
                Transaction outTxn;
                bool swapped = batteryService.performSwap(evId, currentBattery, requiredCapacity, readyBattery, &outTxn);
                if (swapped) {
                    if (txnStack != nullptr) {
                        txnStack->push(outTxn.getId());
                    }

                    res["success"] = true;
                    res["swapped"] = true;
                    res["queued"] = false;
                    res["batteryId"] = readyBattery.getId();
                    res["chargeLevel"] = readyBattery.getChargeLevel();
                    res["capacity"] = readyBattery.getCapacity();
                    res["transactionId"] = outTxn.getId();
                    res["amount"] = outTxn.getAmount();
                    res["energyDelivered"] = outTxn.getEnergyDelivered();
                    res["message"] = "Swap completed! Assigned battery " + readyBattery.getId();
                    return crow::response(200, res);
                }
            }
        }

        // Either inventory is exhausted or vehicles are already waiting in queue (FIFO enforcement)
        if (swapQueue.isFull()) {
            res["success"] = false;
            res["swapped"] = false;
            res["queued"] = false;
            res["message"] = "Swap queue overflow! Queue is full.";
            return crow::response(400, res);
        }

        if (batteryService.enqueueWaitingSwap(evId, swapQueue)) {
            res["success"] = true;
            res["swapped"] = false;
            res["queued"] = true;
            res["queueSize"] = swapQueue.getSize();
            res["frontIndex"] = swapQueue.getFrontIndex();
            res["rearIndex"] = swapQueue.getRearIndex();
            res["message"] = "No charged batteries available in inventory. Added vehicle " + evId + " to Swap Queue.";
            return crow::response(200, res);
        } else {
            res["success"] = false;
            res["swapped"] = false;
            res["queued"] = false;
            res["message"] = "Failed to enqueue vehicle " + evId + " (vehicle may not exist, is already in queue, or is currently charging)";
            return crow::response(400, res);
        }
    });

    // -------------------------------------------------------------------------
    // 2. POST /api/swap/process - Process next pending swap request (FIFO Dequeue)
    // -------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/swap/process").methods(crow::HTTPMethod::POST)
    ([&batteryService, &swapQueue, txnStack](const crow::request& req) {
        crow::json::wvalue res;

        if (swapQueue.isEmpty()) {
            res["success"] = false;
            res["message"] = "Swap queue is empty! Underflow.";
            return crow::response(400, res);
        }

        std::string nextEvId = swapQueue.peek();
        Battery readyBattery;
        Transaction outTxn;

        if (batteryService.processNextSwap(swapQueue, readyBattery, outTxn, txnStack)) {
            res["success"] = true;
            res["message"] = "Processed next swap from queue for EV: " + nextEvId;
            res["evId"] = nextEvId;
            res["batteryId"] = readyBattery.getId();
            res["chargeLevel"] = readyBattery.getChargeLevel();
            res["capacity"] = readyBattery.getCapacity();
            res["transactionId"] = outTxn.getId();
            res["amount"] = outTxn.getAmount();
            res["energyDelivered"] = outTxn.getEnergyDelivered();
            res["remainingSize"] = swapQueue.getSize();
            res["frontIndex"] = swapQueue.getFrontIndex();
            res["rearIndex"] = swapQueue.getRearIndex();
            return crow::response(200, res);
        } else {
            res["success"] = false;
            res["message"] = "Inventory exhausted. Cannot process swap for vehicle " + nextEvId + ". Vehicle remains at FRONT of queue.";
            return crow::response(400, res);
        }
    });

    // -------------------------------------------------------------------------
    // 3. GET /api/swap/available-count - Quick check of ready swap batteries
    // -------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/swap/available-count").methods(crow::HTTPMethod::GET)
    ([&batteryService]() {
        crow::json::wvalue res;
        int readyCount = batteryService.getReadyCount();
        res["success"] = true;
        res["readyCount"] = readyCount;
        res["message"] = std::to_string(readyCount) + " battery(ies) ready for immediate swap";
        return crow::response(200, res);
    });
}
