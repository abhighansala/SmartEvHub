#include "swap_routes.h"

void registerSwapRoutes(crow::App<crow::CORSHandler>& app, BatteryService& batteryService) {

    // POST /api/swap/request - Request a battery swap
    CROW_ROUTE(app, "/api/swap/request").methods(crow::HTTPMethod::POST)
    ([&batteryService](const crow::request& req){
        crow::json::wvalue res;
        auto json = crow::json::load(req.body);
        if (!json || !json.has("evId")) {
            res["success"] = false;
            res["message"] = "evId is required";
            return crow::response(400, res);
        }

        std::string evId = std::string(json["evId"].s());

        Battery readyBattery;
        if (batteryService.performSwap(evId, readyBattery)) {
            res["success"] = true;
            res["swapped"] = true;
            res["batteryId"] = readyBattery.getId();
            res["chargeLevel"] = readyBattery.getChargeLevel();
            res["message"] = "Swap completed! Assigned battery " + readyBattery.getId();
            return crow::response(200, res);
        } else {
            // No ready batteries
            res["success"] = true;
            res["swapped"] = false;
            res["queued"] = true;
            res["message"] = "No charged batteries available in inventory. Added to Swap Queue.";
            return crow::response(200, res);
        }
    });

    // POST /api/swap/process - Process swap
    CROW_ROUTE(app, "/api/swap/process").methods(crow::HTTPMethod::POST)
    ([&batteryService](const crow::request& req){
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
            return crow::response(200, res);
        } else {
            res["success"] = false;
            res["message"] = "Inventory exhausted. Cannot process swap.";
            return crow::response(400, res);
        }
    });
}
