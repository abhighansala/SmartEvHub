#include "charging_routes.h"
#include <vector>

void registerChargingRoutes(crow::App<crow::CORSHandler>& app,
                            StationService& stationService,
                            ChargingQueue& chargingQueue,
                            TransactionStack* txnStack) {

    // GET /api/sessions - Retrieve all charging sessions with optional status filter (?status=active)
    CROW_ROUTE(app, "/api/sessions").methods(crow::HTTPMethod::GET)
    ([&stationService](const crow::request& req){
        std::string statusFilter = "";
        if (req.url_params.get("status") != nullptr) {
            statusFilter = req.url_params.get("status");
        }

        ChargingSession list[MAX_SESSIONS];
        int count = stationService.getAllSessions(list, MAX_SESSIONS, statusFilter);

        std::vector<crow::json::wvalue> sessionsJson;
        for (int i = 0; i < count; i++) {
            crow::json::wvalue s;
            s["id"] = list[i].getId();
            s["evId"] = list[i].getVehicleId();
            s["evModel"] = list[i].getEvModel();
            s["stationId"] = list[i].getStationId();
            s["startTime"] = list[i].getStartTime();
            if (list[i].getEndTime().empty()) {
                s["endTime"] = nullptr;
            } else {
                s["endTime"] = list[i].getEndTime();
            }
            s["batteryStart"] = list[i].getBatteryStart();
            if (list[i].getStatus() == "active" && list[i].getBatteryEnd() <= 0.0) {
                s["batteryEnd"] = nullptr;
            } else {
                s["batteryEnd"] = list[i].getBatteryEnd();
            }
            s["targetCharge"] = list[i].getTargetCharge();
            s["energyConsumed"] = list[i].getEnergyConsumed();
            s["estimatedCost"] = list[i].getEstimatedCost();
            s["status"] = list[i].getStatus();
            sessionsJson.push_back(std::move(s));
        }

        crow::json::wvalue res = std::move(sessionsJson);
        return crow::response(200, res);
    });

    // GET /api/sessions/active - Convenience route for active charging sessions
    CROW_ROUTE(app, "/api/sessions/active").methods(crow::HTTPMethod::GET)
    ([&stationService](){
        ChargingSession list[MAX_SESSIONS];
        int count = stationService.getAllSessions(list, MAX_SESSIONS, "active");

        std::vector<crow::json::wvalue> sessionsJson;
        for (int i = 0; i < count; i++) {
            crow::json::wvalue s;
            s["id"] = list[i].getId();
            s["evId"] = list[i].getVehicleId();
            s["evModel"] = list[i].getEvModel();
            s["stationId"] = list[i].getStationId();
            s["startTime"] = list[i].getStartTime();
            s["endTime"] = nullptr;
            s["batteryStart"] = list[i].getBatteryStart();
            s["batteryEnd"] = nullptr;
            s["targetCharge"] = list[i].getTargetCharge();
            s["energyConsumed"] = list[i].getEnergyConsumed();
            s["estimatedCost"] = list[i].getEstimatedCost();
            s["status"] = list[i].getStatus();
            sessionsJson.push_back(std::move(s));
        }

        crow::json::wvalue res = std::move(sessionsJson);
        return crow::response(200, res);
    });

    // POST /api/charging/request - Request a charging bay (assigns bay or enqueues in FIFO ChargingQueue)
    CROW_ROUTE(app, "/api/charging/request").methods(crow::HTTPMethod::POST)
    ([&stationService, &chargingQueue](const crow::request& req){
        crow::json::wvalue res;
        auto json = crow::json::load(req.body);
        if (!json || !json.has("evId")) {
            res["success"] = false;
            res["message"] = "evId is required";
            return crow::response(400, res);
        }

        std::string evId = json["evId"].s();
        double targetCharge = 85.0;
        if (json.has("targetCharge")) {
            targetCharge = json["targetCharge"].d();
        }

        // Prevent duplicate queue entry
        if (chargingQueue.contains(evId)) {
            res["success"] = false;
            res["assigned"] = false;
            res["queued"] = true;
            res["message"] = "Vehicle " + evId + " is already in the Charging Queue.";
            return crow::response(400, res);
        }

        // Check if any station is available
        ChargingStation freeStation;
        if (stationService.findAvailableStation(freeStation)) {
            ChargingSession newSession;
            if (stationService.createChargingSession(evId, freeStation.getId(), targetCharge, newSession)) {
                res["success"] = true;
                res["assigned"] = true;
                res["stationId"] = freeStation.getId();
                res["sessionId"] = newSession.getId();
                res["targetCharge"] = newSession.getTargetCharge();
                res["estimatedCost"] = newSession.getEstimatedCost();
                res["message"] = "Assigned to Station " + freeStation.getId();
                return crow::response(200, res);
            } else {
                res["success"] = false;
                res["assigned"] = false;
                res["message"] = "Failed to create charging session for vehicle " + evId + " (vehicle may already be charging or unavailable)";
                return crow::response(400, res);
            }
        } else {
            // All stations busy: EV must wait in queue
            if (chargingQueue.isFull()) {
                res["success"] = false;
                res["assigned"] = false;
                res["queued"] = false;
                res["message"] = "All stations busy and charging queue is full!";
                return crow::response(400, res);
            }

            // Atomically synchronize SQLite EV status to 'waiting' and enqueue into FIFO queue
            if (stationService.enqueueWaitingEV(evId, chargingQueue)) {
                res["success"] = true;
                res["assigned"] = false;
                res["queued"] = true;
                res["message"] = "All stations currently busy. Added vehicle " + evId + " to Charging Queue.";
                res["queueSize"] = chargingQueue.getSize();
                res["frontIndex"] = chargingQueue.getFrontIndex();
                res["rearIndex"] = chargingQueue.getRearIndex();
                return crow::response(200, res);
            } else {
                res["success"] = false;
                res["assigned"] = false;
                res["queued"] = false;
                res["message"] = "Failed to enqueue vehicle " + evId + " (vehicle may not exist, is already in queue, or is currently charging)";
                return crow::response(400, res);
            }
        }
    });

    // POST /api/charging/complete - Complete charging session, record transaction & dispatch FIFO waiting EV
    CROW_ROUTE(app, "/api/charging/complete").methods(crow::HTTPMethod::POST)
    ([&stationService, &chargingQueue, txnStack](const crow::request& req){
        crow::json::wvalue res;
        auto json = crow::json::load(req.body);
        if (!json) {
            res["success"] = false;
            res["message"] = "Invalid JSON body";
            return crow::response(400, res);
        }

        std::string stationId = "";
        if (json.has("stationId")) {
            stationId = json["stationId"].s();
        } else if (json.has("sessionId")) {
            std::string sessionId = json["sessionId"].s();
            ChargingSession ses;
            if (stationService.getSessionById(sessionId, ses)) {
                stationId = ses.getStationId();
            }
        }

        if (stationId.empty()) {
            res["success"] = false;
            res["message"] = "stationId or sessionId is required";
            return crow::response(400, res);
        }

        ChargingSession completedSession;
        Transaction outTxn;
        ChargingSession dispatchedSession;

        if (stationService.completeChargingSession(stationId, completedSession, outTxn, &chargingQueue, &dispatchedSession)) {
            // Push completed transaction to Abhishek Ghansala's LIFO stack if available
            if (txnStack != nullptr) {
                txnStack->push(outTxn.getId());
            }

            res["success"] = true;
            res["message"] = "Station " + stationId + " session completed.";
            res["sessionId"] = completedSession.getId();
            res["transactionId"] = outTxn.getId();
            res["amount"] = outTxn.getAmount();
            res["energyDelivered"] = outTxn.getEnergyDelivered();
            res["finalCharge"] = completedSession.getBatteryEnd();

            if (!dispatchedSession.getId().empty()) {
                res["dispatched"] = true;
                res["dispatchedEvId"] = dispatchedSession.getVehicleId();
                res["dispatchedSessionId"] = dispatchedSession.getId();
                res["message"] = "Station " + stationId + " completed. Auto-dispatched waiting vehicle " + dispatchedSession.getVehicleId() + " from FIFO queue.";
            } else {
                res["dispatched"] = false;
            }
            return crow::response(200, res);
        } else {
            res["success"] = false;
            res["message"] = "Could not complete charging session on station " + stationId + ". Station may not have an active session or is already available.";
            return crow::response(400, res);
        }
    });
}
