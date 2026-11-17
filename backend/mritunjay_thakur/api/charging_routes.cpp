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
            s["duration"] = list[i].getDuration();
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
            s["duration"] = list[i].getDuration();
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
            res["queuePosition"] = chargingQueue.getSize();
            res["queueSize"] = chargingQueue.getSize();
            res["message"] = "Vehicle " + evId + " is already in the Charging Queue.";
            return crow::response(400, res);
        }

        // Check if any station is available
        ChargingStation freeStation;
        if (stationService.findAvailableStation(freeStation)) {
            ChargingSession newSession;
            std::string createError;
            if (stationService.createChargingSession(evId, freeStation.getId(), targetCharge, newSession, createError)) {
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
                res["message"] = "Failed to create charging session: " + createError;
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
            std::string enqueueError;
            if (stationService.enqueueWaitingEV(evId, chargingQueue, enqueueError)) {
                int position = chargingQueue.getSize();
                res["success"] = true;
                res["assigned"] = false;
                res["queued"] = true;
                res["queuePosition"] = position;
                res["queueSize"] = position;
                res["frontIndex"] = chargingQueue.getFrontIndex();
                res["rearIndex"] = chargingQueue.getRearIndex();
                res["message"] = "All stations currently busy. Added vehicle " + evId + " to Charging Queue (Position: " + std::to_string(position) + ").";
                return crow::response(200, res);
            } else {
                res["success"] = false;
                res["assigned"] = false;
                res["queued"] = false;
                res["message"] = "Failed to enqueue vehicle: " + enqueueError;
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

        std::string targetIdentifier = "";
        if (json.has("stationId")) {
            targetIdentifier = json["stationId"].s();
        } else if (json.has("sessionId")) {
            targetIdentifier = json["sessionId"].s();
        }

        if (targetIdentifier.empty()) {
            res["success"] = false;
            res["message"] = "stationId or sessionId is required";
            return crow::response(400, res);
        }

        ChargingSession completedSession;
        Transaction outTxn;
        ChargingSession dispatchedSession;
        std::string compError;

        if (stationService.completeChargingSession(targetIdentifier, completedSession, outTxn, &chargingQueue, &dispatchedSession, compError)) {
            // Push completed transaction to Abhishek Ghansala's LIFO stack if available
            if (txnStack != nullptr) {
                txnStack->push(outTxn.getId());
            }

            res["success"] = true;
            res["message"] = "Station " + completedSession.getStationId() + " session completed.";
            res["sessionId"] = completedSession.getId();
            res["stationId"] = completedSession.getStationId();
            res["evId"] = completedSession.getVehicleId();
            res["transactionId"] = outTxn.getId();
            res["amount"] = outTxn.getAmount();
            res["energyDelivered"] = outTxn.getEnergyDelivered();
            res["duration"] = completedSession.getDuration();
            res["finalCharge"] = completedSession.getBatteryEnd();

            if (!dispatchedSession.getId().empty()) {
                res["dispatched"] = true;
                res["dispatchedEvId"] = dispatchedSession.getVehicleId();
                res["dispatchedSessionId"] = dispatchedSession.getId();
                res["message"] = "Station " + completedSession.getStationId() + " completed. Auto-dispatched waiting vehicle " + dispatchedSession.getVehicleId() + " from FIFO queue.";
            } else {
                res["dispatched"] = false;
            }
            return crow::response(200, res);
        } else {
            res["success"] = false;
            res["message"] = compError.empty() ? ("Could not complete charging session for " + targetIdentifier) : compError;
            return crow::response(400, res);
        }
    });
}
