#include "charging_routes.h"

void registerChargingRoutes(crow::App<crow::CORSHandler>& app, StationService& stationService) {

    // POST /api/charging/request - Request a charging bay
    CROW_ROUTE(app, "/api/charging/request").methods(crow::HTTPMethod::POST)
    ([&stationService](const crow::request& req){
        crow::json::wvalue res;
        auto json = crow::json::load(req.body);
        if (!json || !json.has("evId")) {
            res["success"] = false;
            res["message"] = "evId is required";
            return crow::response(400, res);
        }

        std::string evId = json["evId"].s();

        // Check if any station is available
        ChargingStation freeStation;
        if (stationService.findAvailableStation(freeStation)) {
            // Assign immediately
            stationService.assignVehicle(freeStation.getId(), evId);
            res["success"] = true;
            res["assigned"] = true;
            res["stationId"] = freeStation.getId();
            res["message"] = "Assigned to Station " + freeStation.getId();
            return crow::response(200, res);
        } else {
            // All stations full: EV must wait in queue
            res["success"] = true;
            res["assigned"] = false;
            res["queued"] = true;
            res["message"] = "All stations currently busy. Added to Charging Queue.";
            return crow::response(200, res);
        }
    });

    // POST /api/charging/complete - Complete charging session & release station
    CROW_ROUTE(app, "/api/charging/complete").methods(crow::HTTPMethod::POST)
    ([&stationService](const crow::request& req){
        crow::json::wvalue res;
        auto json = crow::json::load(req.body);
        if (!json || !json.has("stationId")) {
            res["success"] = false;
            res["message"] = "stationId is required";
            return crow::response(400, res);
        }

        std::string stationId = json["stationId"].s();
        if (stationService.releaseStation(stationId)) {
            res["success"] = true;
            res["message"] = "Station " + stationId + " released and now available.";
            return crow::response(200, res);
        } else {
            res["success"] = false;
            res["message"] = "Could not release station " + stationId;
            return crow::response(400, res);
        }
    });
}
