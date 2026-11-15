#include "ev_routes.h"
#include "../../aditya_singh/security/jwt_util.h"

void registerEVRoutes(crow::App<crow::CORSHandler>& app, EVService& evService) {

    // GET /api/evs - List EVs with strict ownership filtering for normal users
    CROW_ROUTE(app, "/api/evs").methods(crow::HTTPMethod::GET)
    ([&evService](const crow::request& req){
        crow::response res;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, res, claims)) {
            return res;
        }

        EV list[MAX_EVS];
        int count = 0;
        // Role-based data isolation:
        // Admin and Operator see all EVs in the system.
        // Normal USER accounts only see EVs associated with their authenticated userId.
        if (claims.role == "admin" || claims.role == "operator") {
            count = evService.getAllEVs(list, MAX_EVS, 0);
        } else {
            count = evService.getAllEVs(list, MAX_EVS, claims.userId);
        }

        crow::json::wvalue::list evList;
        for (int i = 0; i < count; i++) {
            crow::json::wvalue item;
            item["id"] = list[i].getId();
            item["owner"] = list[i].getOwnerName();
            item["model"] = list[i].getModel();
            item["batteryCapacity"] = list[i].getBatteryCapacity();
            item["currentCharge"] = list[i].getCurrentCharge();
            item["status"] = list[i].getStatus();
            item["lastActivity"] = list[i].getLastActivity();
            item["registeredAt"] = list[i].getRegisteredAt();
            item["userId"] = list[i].getUserId();
            evList.push_back(std::move(item));
        }

        crow::json::wvalue jsonRes = std::move(evList);
        return crow::response(200, jsonRes);
    });

    // GET /api/evs/<string> - Get single EV with ownership check
    CROW_ROUTE(app, "/api/evs/<string>").methods(crow::HTTPMethod::GET)
    ([&evService](const crow::request& req, std::string id){
        crow::response res;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, res, claims)) {
            return res;
        }

        EV ev;
        if (!evService.getEVById(id, ev)) {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "EV not found with ID: " + id;
            return crow::response(404, err);
        }

        // Ownership enforcement: USER can only view their own EV
        if (claims.role == "user" && ev.getUserId() != claims.userId) {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Access forbidden: you do not have permission to view this vehicle";
            return crow::response(403, err);
        }

        crow::json::wvalue jsonRes;
        jsonRes["id"] = ev.getId();
        jsonRes["owner"] = ev.getOwnerName();
        jsonRes["model"] = ev.getModel();
        jsonRes["batteryCapacity"] = ev.getBatteryCapacity();
        jsonRes["currentCharge"] = ev.getCurrentCharge();
        jsonRes["status"] = ev.getStatus();
        jsonRes["lastActivity"] = ev.getLastActivity();
        jsonRes["registeredAt"] = ev.getRegisteredAt();
        jsonRes["userId"] = ev.getUserId();
        return crow::response(200, jsonRes);
    });

    // POST /api/evs - Register a new EV (auto-bind authenticated user ID)
    CROW_ROUTE(app, "/api/evs").methods(crow::HTTPMethod::POST)
    ([&evService](const crow::request& req){
        crow::response res;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, res, claims)) {
            return res;
        }

        crow::json::wvalue jsonRes;
        auto json = crow::json::load(req.body);
        if (!json) {
            jsonRes["success"] = false;
            jsonRes["message"] = "Invalid JSON body";
            return crow::response(400, jsonRes);
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

        // Requirements 6 & 8: Verify ownership using authenticated JWT identity, never client-supplied.
        int evUserId = claims.userId;
        if (claims.role == "user") {
            if (owner.empty()) {
                owner = claims.name;
            }
        } else if (claims.role == "admin" || claims.role == "operator") {
            // Operator/Admin can optionally assign a specific userId if passed in JSON
            if (json.has("userId")) {
                evUserId = json["userId"].i();
            }
        }

        EV newEV(id, owner, model, cap, charge, status, "", "", evUserId);
        std::string err;

        if (evService.createEV(newEV, err)) {
            jsonRes["success"] = true;
            jsonRes["message"] = "EV created successfully";
            jsonRes["id"] = id;
            jsonRes["userId"] = evUserId;
            return crow::response(201, jsonRes);
        } else {
            jsonRes["success"] = false;
            jsonRes["message"] = err;
            return crow::response(400, jsonRes);
        }
    });

    // PUT /api/evs/<string> - Update EV (enforcing ownership)
    CROW_ROUTE(app, "/api/evs/<string>").methods(crow::HTTPMethod::PUT)
    ([&evService](const crow::request& req, std::string id){
        crow::response res;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, res, claims)) {
            return res;
        }

        crow::json::wvalue jsonRes;
        auto json = crow::json::load(req.body);
        if (!json) {
            jsonRes["success"] = false;
            jsonRes["message"] = "Invalid JSON";
            return crow::response(400, jsonRes);
        }

        EV existing;
        if (!evService.getEVById(id, existing)) {
            jsonRes["success"] = false;
            jsonRes["message"] = "EV not found";
            return crow::response(404, jsonRes);
        }

        // Ownership enforcement: USER can only update their own EV
        if (claims.role == "user" && existing.getUserId() != claims.userId) {
            jsonRes["success"] = false;
            jsonRes["message"] = "Access forbidden: you do not have permission to modify this vehicle";
            return crow::response(403, jsonRes);
        }

        if (json.has("owner")) existing.setOwnerName(std::string(json["owner"].s()));
        if (json.has("model")) existing.setModel(std::string(json["model"].s()));
        if (json.has("batteryCapacity")) existing.setBatteryCapacity(json["batteryCapacity"].d());
        if (json.has("currentCharge")) existing.setCurrentCharge(json["currentCharge"].d());
        if (json.has("status")) existing.setStatus(std::string(json["status"].s()));

        // Admin/Operator can reassign userId if specified
        if ((claims.role == "admin" || claims.role == "operator") && json.has("userId")) {
            existing.setUserId(json["userId"].i());
        }

        std::string err;
        if (evService.updateEV(id, existing, err)) {
            jsonRes["success"] = true;
            jsonRes["message"] = "EV updated successfully";
            return crow::response(200, jsonRes);
        } else {
            jsonRes["success"] = false;
            jsonRes["message"] = err;
            return crow::response(400, jsonRes);
        }
    });

    // DELETE /api/evs/<string> - Delete EV (enforcing ownership)
    CROW_ROUTE(app, "/api/evs/<string>").methods(crow::HTTPMethod::Delete)
    ([&evService](const crow::request& req, std::string id){
        crow::response res;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, res, claims)) {
            return res;
        }

        crow::json::wvalue jsonRes;
        EV existing;
        if (!evService.getEVById(id, existing)) {
            jsonRes["success"] = false;
            jsonRes["message"] = "EV not found with ID: " + id;
            return crow::response(404, jsonRes);
        }

        // Ownership enforcement: USER can only delete their own EV
        if (claims.role == "user" && existing.getUserId() != claims.userId) {
            jsonRes["success"] = false;
            jsonRes["message"] = "Access forbidden: you do not have permission to delete this vehicle";
            return crow::response(403, jsonRes);
        }

        std::string err;
        if (evService.deleteEV(id, err)) {
            jsonRes["success"] = true;
            jsonRes["message"] = "EV deleted successfully";
            return crow::response(200, jsonRes);
        } else {
            jsonRes["success"] = false;
            jsonRes["message"] = err.empty() ? ("EV not found with ID: " + id) : err;
            return crow::response(400, jsonRes);
        }
    });
}
