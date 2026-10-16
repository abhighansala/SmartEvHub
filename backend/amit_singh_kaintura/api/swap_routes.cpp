#include "swap_routes.h"

// ============================================================================
// Smart EV Hub - Battery Swap Workflow REST API Endpoints
// Module Owner: Amit Singh Kaintura (B.Tech 2nd Year, Section G, Student ID: 2518188)
// Role: Battery Inventory & Battery Swapping Module
//
// Crow Helper API:
// Communicates between C++ backend and React frontend via JSON.
// ============================================================================

void registerSwapRoutes(crow::App<crow::CORSHandler>& app, BatteryService& batteryService) {

    // -------------------------------------------------------------------------
    // 1. POST /api/swap/request - Request an immediate or queued battery swap
    // -------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/swap/request").methods(crow::HTTPMethod::POST)
    ([&batteryService](const crow::request& req) {
        crow::json::wvalue res;
        auto json = crow::json::load(req.body);
        if (!json || !json.has("evId")) {
            res["success"] = false;
            res["message"] = "Field 'evId' is required in JSON payload";
            return crow::response(400, res);
        }

        std::string evId = std::string(json["evId"].s());
        std::string currentBattery = "";
        if (json.has("currentBattery")) {
            currentBattery = std::string(json["currentBattery"].s());
        }

        double requiredCapacity = 0.0;
        if (json.has("requiredCapacity")) {
            requiredCapacity = json["requiredCapacity"].d();
        }

        Battery readyBattery;
        bool swapped = batteryService.performSwap(evId, currentBattery, requiredCapacity, readyBattery);

        if (swapped) {
            res["success"] = true;
            res["swapped"] = true;
            res["batteryId"] = readyBattery.getId();
            res["chargeLevel"] = readyBattery.getChargeLevel();
            res["capacity"] = readyBattery.getCapacity();
            res["message"] = "Swap completed! Assigned battery " + readyBattery.getId();
            return crow::response(200, res);
        } else {
            // No ready batteries in inventory - notify React to push to FIFO SwapQueue
            res["success"] = true;
            res["swapped"] = false;
            res["queued"] = true;
            res["message"] = "No charged batteries available in inventory. Added to Swap Queue.";
            return crow::response(200, res);
        }
    });

    // -------------------------------------------------------------------------
    // 2. POST /api/swap/process - Process next pending swap request
    // -------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/swap/process").methods(crow::HTTPMethod::POST)
    ([&batteryService](const crow::request& req) {
        crow::json::wvalue res;
        auto json = crow::json::load(req.body);

        std::string evId = "EV-102";
        if (json && json.has("evId")) {
            evId = std::string(json["evId"].s());
        }

        Battery readyBattery;
        if (batteryService.performSwap(evId, readyBattery)) {
            res["success"] = true;
            res["message"] = "Processed next swap from queue for EV: " + evId;
            res["batteryId"] = readyBattery.getId();
            res["chargeLevel"] = readyBattery.getChargeLevel();
            return crow::response(200, res);
        } else {
            res["success"] = false;
            res["message"] = "Inventory exhausted. Cannot process swap.";
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
