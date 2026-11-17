#include "station_routes.h"

void registerStationRoutes(crow::App<crow::CORSHandler>& app, StationService& stationService) {

    // GET /api/stations - List all stations
    CROW_ROUTE(app, "/api/stations").methods(crow::HTTPMethod::GET)
    ([&stationService](){
        stationService.advanceActiveSessions();
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
            res[i]["totalPorts"] = list[i].getTotalPorts();
            res[i]["availablePorts"] = list[i].getAvailablePorts();
            if (list[i].getSessionStart().empty()) {
                res[i]["sessionStart"] = nullptr;
            } else {
                res[i]["sessionStart"] = list[i].getSessionStart();
            }
        }

        return crow::response(200, res);
    });

    // GET /api/stations/<string> - Single station details
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
            res["totalPorts"] = station.getTotalPorts();
            res["availablePorts"] = station.getAvailablePorts();
            if (station.getSessionStart().empty()) {
                res["sessionStart"] = nullptr;
            } else {
                res["sessionStart"] = station.getSessionStart();
            }
            return crow::response(200, res);
        } else {
            crow::json::wvalue res;
            res["success"] = false;
            res["message"] = "Station not found with ID: " + id;
            return crow::response(404, res);
        }
    });

    // POST /api/stations - Create a new charging station
    CROW_ROUTE(app, "/api/stations").methods(crow::HTTPMethod::POST)
    ([&stationService](const crow::request& req){
        crow::json::wvalue res;
        auto json = crow::json::load(req.body);
        if (!json) {
            res["success"] = false;
            res["message"] = "Invalid JSON body";
            return crow::response(400, res);
        }

        if (!json.has("id") || !json.has("name")) {
            res["success"] = false;
            res["message"] = "id and name are required fields";
            return crow::response(400, res);
        }

        std::string sId = json["id"].s();
        std::string sName = json["name"].s();
        double sPower = 50.0;
        if (json.has("power")) {
            sPower = json["power"].d();
        }

        std::string sConnector = "CCS2";
        if (json.has("connectorType")) {
            sConnector = json["connectorType"].s();
        }

        std::string sStatus = "available";
        if (json.has("status")) {
            sStatus = json["status"].s();
        }

        std::string sLocation = "Bay 1";
        if (json.has("location")) {
            sLocation = json["location"].s();
        }

        ChargingStation newStation(sId, sName, sPower, sConnector, sStatus, "", sLocation);

        std::string errorMsg;
        if (stationService.createStation(newStation, errorMsg)) {
            res["success"] = true;
            res["message"] = "Station created successfully";
            res["station"]["id"] = newStation.getId();
            res["station"]["name"] = newStation.getName();
            res["station"]["power"] = newStation.getPower();
            res["station"]["connectorType"] = newStation.getConnectorType();
            res["station"]["status"] = newStation.getStatus();
            res["station"]["location"] = newStation.getLocation();
            res["station"]["totalPorts"] = newStation.getTotalPorts();
            res["station"]["availablePorts"] = newStation.getAvailablePorts();
            return crow::response(201, res);
        } else {
            res["success"] = false;
            res["message"] = errorMsg;
            if (errorMsg.find("already exists") != std::string::npos) {
                return crow::response(409, res);
            }
            return crow::response(400, res);
        }
    });

    // PUT /api/stations/<string>/status - Update station status with validated state transitions
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
        std::string errorMsg;
        if (stationService.updateStationStatus(id, newStatus, errorMsg)) {
            std::string norm = ChargingStation::normalizeStatus(newStatus);
            res["success"] = true;
            res["message"] = "Station status updated to " + norm;
            res["status"] = norm;
            res["station"]["id"] = id;
            res["station"]["status"] = norm;
            return crow::response(200, res);
        } else {
            res["success"] = false;
            res["message"] = errorMsg;
            if (errorMsg.find("not found") != std::string::npos) {
                return crow::response(404, res);
            }
            return crow::response(400, res);
        }
    });
}
