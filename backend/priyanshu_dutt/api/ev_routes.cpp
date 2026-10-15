#include "ev_routes.h"

void registerEVRoutes(crow::App<crow::CORSHandler>& app, EVService& evService) {

    // GET /api/evs - List all EVs
    CROW_ROUTE(app, "/api/evs").methods(crow::HTTPMethod::GET)
    ([&evService](){
        EV list[MAX_EVS];
        int count = evService.getAllEVs(list, MAX_EVS);

        crow::json::wvalue res;
        for (int i = 0; i < count; i++) {
            res[i]["id"] = list[i].getId();
            res[i]["owner"] = list[i].getOwnerName();
            res[i]["model"] = list[i].getModel();
            res[i]["batteryCapacity"] = list[i].getBatteryCapacity();
            res[i]["currentCharge"] = list[i].getCurrentCharge();
            res[i]["status"] = list[i].getStatus();
            res[i]["lastActivity"] = list[i].getLastActivity();
            res[i]["registeredAt"] = list[i].getRegisteredAt();
        }

        return crow::response(200, res);
    });

    // GET /api/evs/<string> - Get single EV
    CROW_ROUTE(app, "/api/evs/<string>").methods(crow::HTTPMethod::GET)
    ([&evService](std::string id){
        EV ev;
        if (evService.getEVById(id, ev)) {
            crow::json::wvalue res;
            res["id"] = ev.getId();
            res["owner"] = ev.getOwnerName();
            res["model"] = ev.getModel();
            res["batteryCapacity"] = ev.getBatteryCapacity();
            res["currentCharge"] = ev.getCurrentCharge();
            res["status"] = ev.getStatus();
            res["lastActivity"] = ev.getLastActivity();
            res["registeredAt"] = ev.getRegisteredAt();
            return crow::response(200, res);
        } else {
            crow::json::wvalue res;
            res["success"] = false;
            res["message"] = "EV not found with ID: " + id;
            return crow::response(404, res);
        }
    });

    // POST /api/evs - Register a new EV
    CROW_ROUTE(app, "/api/evs").methods(crow::HTTPMethod::POST)
    ([&evService](const crow::request& req){
        crow::json::wvalue res;
        auto json = crow::json::load(req.body);
        if (!json) {
            res["success"] = false;
            res["message"] = "Invalid JSON body";
            return crow::response(400, res);
        }

        std::string id = "";
        std::string owner = "";
        std::string model = "";
        double cap = 50.0;
        double charge = 20.0;
        std::string status = "available";

        if (json.has("id")) id = std::string(json["id"].s());
        if (json.has("owner")) owner = std::string(json["owner"].s());
        if (json.has("model")) model = std::string(json["model"].s());
        if (json.has("batteryCapacity")) cap = json["batteryCapacity"].d();
        if (json.has("currentCharge")) charge = json["currentCharge"].d();
        if (json.has("status")) status = std::string(json["status"].s());

        EV newEV(id, owner, model, cap, charge, status);
        std::string err;

        if (evService.createEV(newEV, err)) {
            res["success"] = true;
            res["message"] = "EV created successfully";
            res["id"] = id;
            return crow::response(201, res);
        } else {
            res["success"] = false;
            res["message"] = err;
            return crow::response(400, res);
        }
    });

    // PUT /api/evs/<string> - Update EV
    CROW_ROUTE(app, "/api/evs/<string>").methods(crow::HTTPMethod::PUT)
    ([&evService](const crow::request& req, std::string id){
        crow::json::wvalue res;
        auto json = crow::json::load(req.body);
        if (!json) {
            res["success"] = false;
            res["message"] = "Invalid JSON";
            return crow::response(400, res);
        }

        EV existing;
        if (!evService.getEVById(id, existing)) {
            res["success"] = false;
            res["message"] = "EV not found";
            return crow::response(404, res);
        }

        if (json.has("owner")) existing.setOwnerName(std::string(json["owner"].s()));
        if (json.has("model")) existing.setModel(std::string(json["model"].s()));
        if (json.has("batteryCapacity")) existing.setBatteryCapacity(json["batteryCapacity"].d());
        if (json.has("currentCharge")) existing.setCurrentCharge(json["currentCharge"].d());
        if (json.has("status")) existing.setStatus(std::string(json["status"].s()));

        std::string err;
        if (evService.updateEV(id, existing, err)) {
            res["success"] = true;
            res["message"] = "EV updated successfully";
            return crow::response(200, res);
        } else {
            res["success"] = false;
            res["message"] = err;
            return crow::response(400, res);
        }
    });

    // DELETE /api/evs/<string> - Delete EV
    CROW_ROUTE(app, "/api/evs/<string>").methods(crow::HTTPMethod::DELETE)
    ([&evService](std::string id){
        crow::json::wvalue res;
        if (evService.deleteEV(id)) {
            res["success"] = true;
            res["message"] = "EV deleted successfully";
            return crow::response(200, res);
        } else {
            res["success"] = false;
            res["message"] = "EV not found with ID: " + id;
            return crow::response(404, res);
        }
    });
}
