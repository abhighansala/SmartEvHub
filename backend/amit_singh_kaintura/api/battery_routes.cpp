#include "battery_routes.h"
#include "../../aditya_singh/security/jwt_util.h"

// ============================================================================
// Smart EV Hub - Battery REST API Endpoints
// Module Owner: Amit Singh Kaintura (B.Tech 2nd Year, Section G, Student ID: 2518188)
// Role: Battery Inventory & Battery Swapping Module
//
// Crow Helper API:
// Communicates between C++ backend and React frontend via JSON.
// ============================================================================

void registerBatteryRoutes(crow::App<crow::CORSHandler>& app,
                           BatteryService& batteryService,
                           SwapQueue* swapQueue,
                           TransactionStack* txnStack) {

    // -------------------------------------------------------------------------
    // 1. GET /api/batteries - List all replacement batteries in inventory
    // -------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/batteries").methods(crow::HTTPMethod::GET)
    ([&batteryService, swapQueue, txnStack]() {
        // Automatically advance charging batteries and dispatch any queued swaps
        batteryService.advanceChargingBatteries(swapQueue, txnStack);

        Battery list[MAX_BATTERIES];
        int count = batteryService.getAllBatteries(list, MAX_BATTERIES);

        crow::json::wvalue res;
        for (int i = 0; i < count; i++) {
            res[i]["id"] = list[i].getId();
            res[i]["capacity"] = list[i].getCapacity();
            res[i]["currentCharge"] = list[i].getChargeLevel();
            res[i]["status"] = list[i].getStatus();
            res[i]["health"] = list[i].getHealth();
            res[i]["cycleCount"] = list[i].getCycleCount();
            res[i]["currentVehicle"] = list[i].getCurrentVehicle();
            res[i]["chargingStart"] = list[i].getChargingStart();
            res[i]["initialCharge"] = list[i].getInitialCharge();
            res[i]["isReady"] = list[i].isReady();
        }

        return crow::response(200, res);
    });

    // -------------------------------------------------------------------------
    // 2. GET /api/batteries/<id> - Get details of a single battery by ID
    // -------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/batteries/<string>").methods(crow::HTTPMethod::GET)
    ([&batteryService, swapQueue, txnStack](std::string id) {
        batteryService.advanceChargingBatteries(swapQueue, txnStack);

        Battery battery;
        if (batteryService.getBatteryById(id, battery)) {
            crow::json::wvalue res;
            res["id"] = battery.getId();
            res["capacity"] = battery.getCapacity();
            res["currentCharge"] = battery.getChargeLevel();
            res["status"] = battery.getStatus();
            res["health"] = battery.getHealth();
            res["cycleCount"] = battery.getCycleCount();
            res["currentVehicle"] = battery.getCurrentVehicle();
            res["chargingStart"] = battery.getChargingStart();
            res["initialCharge"] = battery.getInitialCharge();
            res["isReady"] = battery.isReady();
            return crow::response(200, res);
        } else {
            crow::json::wvalue res;
            res["success"] = false;
            res["message"] = "Battery not found with ID: " + id;
            return crow::response(404, res);
        }
    });

    // -------------------------------------------------------------------------
    // 3. POST /api/batteries - Add new battery into inventory (from React UI)
    // -------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/batteries").methods(crow::HTTPMethod::POST)
    ([&batteryService, swapQueue, txnStack](const crow::request& req) {
        crow::response authRes;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, authRes, claims)) {
            return authRes;
        }

        // Only Operator or Admin can register new batteries
        if (claims.role == "user") {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Access forbidden: only operators and administrators can register batteries";
            return crow::response(403, err);
        }

        crow::json::wvalue res;
        auto body = crow::json::load(req.body);
        if (!body) {
            res["success"] = false;
            res["message"] = "Invalid JSON body";
            return crow::response(400, res);
        }

        std::string id = body.has("id") ? std::string(body["id"].s()) : "";
        if (id.empty()) {
            res["success"] = false;
            res["message"] = "Battery ID is required";
            return crow::response(400, res);
        }

        double capacity = body.has("capacity") ? body["capacity"].d() : 50.0;
        double currentCharge = body.has("currentCharge") ? body["currentCharge"].d() : 100.0;
        std::string status = body.has("status") ? std::string(body["status"].s()) : "available";
        double health = body.has("health") ? body["health"].d() : 100.0;
        int cycleCount = body.has("cycleCount") ? body["cycleCount"].i() : 0;
        std::string vehicle = body.has("currentVehicle") ? std::string(body["currentVehicle"].s()) : "";

        Battery newBattery(id, capacity, currentCharge, status, health, cycleCount, vehicle);

        if (batteryService.addBattery(newBattery)) {
            // Process oldest queued request if new battery is ready and vehicles are waiting in swap queue
            if (newBattery.isReady() && swapQueue != nullptr && !swapQueue->isEmpty()) {
                Battery outBat;
                Transaction outTxn;
                batteryService.dispatchNextSwapIfAvailable(*swapQueue, outBat, outTxn, txnStack);
            }

            res["success"] = true;
            res["message"] = "Battery registered successfully: " + id;
            res["id"] = id;
            return crow::response(201, res);
        } else {
            res["success"] = false;
            res["message"] = "Inventory storage limit reached (MAX_BATTERIES)";
            return crow::response(507, res);
        }
    });

    // -------------------------------------------------------------------------
    // 4. PUT /api/batteries/<id> - Update existing battery attributes
    // -------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/batteries/<string>").methods(crow::HTTPMethod::PUT)
    ([&batteryService, swapQueue, txnStack](const crow::request& req, std::string id) {
        crow::response authRes;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, authRes, claims)) {
            return authRes;
        }

        // Only Operator or Admin can modify battery inventory
        if (claims.role == "user") {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Access forbidden: only operators and administrators can update batteries";
            return crow::response(403, err);
        }

        crow::json::wvalue res;
        auto body = crow::json::load(req.body);
        if (!body) {
            res["success"] = false;
            res["message"] = "Invalid JSON payload";
            return crow::response(400, res);
        }

        Battery existing;
        if (!batteryService.getBatteryById(id, existing)) {
            res["success"] = false;
            res["message"] = "Battery not found: " + id;
            return crow::response(404, res);
        }

        std::string targetStatus = existing.getStatus();
        std::string targetVehicle = existing.getCurrentVehicle();
        if (body.has("status")) targetStatus = std::string(body["status"].s());
        if (body.has("currentVehicle")) targetVehicle = std::string(body["currentVehicle"].s());

        // Lifecycle State Transition Validation
        if (!existing.isValidTransition(targetStatus, targetVehicle)) {
            res["success"] = false;
            res["message"] = "Invalid battery state transition: cannot transition status from '" + 
                             existing.getStatus() + "' to '" + targetStatus + "' with vehicle '" + targetVehicle + "'";
            return crow::response(400, res);
        }

        if (body.has("capacity")) existing.setCapacity(body["capacity"].d());
        if (body.has("currentCharge")) existing.setChargeLevel(body["currentCharge"].d());
        existing.setStatus(targetStatus);
        existing.setCurrentVehicle(targetVehicle);
        if (body.has("health")) existing.setHealth(body["health"].d());
        if (body.has("cycleCount")) existing.setCycleCount(body["cycleCount"].i());

        if (targetStatus == "charging") {
            existing.setChargingStart(std::to_string(time(nullptr)));
            existing.setInitialCharge(existing.getChargeLevel());
            existing.setCurrentVehicle("");
        } else if (targetStatus == "available") {
            existing.setChargingStart("");
            existing.setCurrentVehicle("");
        }

        batteryService.updateBattery(existing);

        // Process oldest queued request if battery transitioned to ready and vehicles are waiting in swap queue
        if (existing.isReady() && swapQueue != nullptr && !swapQueue->isEmpty()) {
            Battery outBat;
            Transaction outTxn;
            batteryService.dispatchNextSwapIfAvailable(*swapQueue, outBat, outTxn, txnStack);
        }

        res["success"] = true;
        res["message"] = "Battery updated successfully: " + id;
        return crow::response(200, res);
    });
}
