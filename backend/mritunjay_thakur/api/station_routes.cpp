#include "station_routes.h"

void registerStationRoutes(crow::App<crow::CORSHandler>& app, StationService& stationService) {

    // GET /api/stations - List all stations
    CROW_ROUTE(app, "/api/stations").methods(crow::HTTPMethod::GET)
    ([&stationService](){
        ChargingStation list[MAX_STATIONS];
        int count = stationService.getAllStations(list, MAX_STATIONS);

        crow::json::wvalue res;
        for (int i = 0; i < count; i++) {
            res[i]["id"] = list[i].getId();
            res[i]["name"] = list[i].getName();
            res[i]["power"] = list[i].getPower();
            res[i]["connectorType"] = list[i].getConnectorType();
            res[i]["status"] = list[i].getStatus();
            res[i]["currentEV"] = list[i].getCurrentVehicleId();
            res[i]["location"] = list[i].getLocation();
        }

        return crow::response(200, res);
    });

    // GET /api/stations/<string> - Single station
    CROW_ROUTE(app, "/api/stations/<string>").methods(crow::HTTPMethod::GET)
    ([&stationService](std::string id){
        ChargingStation station;
        if (stationService.getStationById(id, station)) {
            crow::json::wvalue res;
            res["id"] = station.getId();
            res["name"] = station.getName();
            res["power"] = station.getPower();
            res["connectorType"] = station.getConnectorType();
            res["status"] = station.getStatus();
            res["currentEV"] = station.getCurrentVehicleId();
            res["location"] = station.getLocation();
            return crow::response(200, res);
        } else {
            crow::json::wvalue res;
            res["success"] = false;
            res["message"] = "Station not found with ID: " + id;
            return crow::response(404, res);
        }
    });

    // PUT /api/stations/<string>/status - Update station status
    CROW_ROUTE(app, "/api/stations/<string>/status").methods(crow::HTTPMethod::PUT)
    ([&stationService](const crow::request& req, std::string id){
        crow::json::wvalue res;
        auto json = crow::json::load(req.body);
        if (!json || !json.has("status")) {
            res["success"] = false;
            res["message"] = "Status field is required";
            return crow::response(400, res);
        }

        std::string newStatus = json["status"].s();
        if (stationService.updateStationStatus(id, newStatus)) {
            res["success"] = true;
            res["message"] = "Station status updated to " + newStatus;
            return crow::response(200, res);
        } else {
            res["success"] = false;
            res["message"] = "Station not found with ID: " + id;
            return crow::response(404, res);
        }
    });
}
