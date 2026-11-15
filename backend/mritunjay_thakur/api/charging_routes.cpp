#include "charging_routes.h"
#include "../../aditya_singh/security/jwt_util.h"
#include <vector>

void registerChargingRoutes(crow::App<crow::CORSHandler>& app,
                            StationService& stationService,
                            ChargingQueue& chargingQueue,
                            TransactionStack* txnStack) {

    // GET /api/sessions - Retrieve charging sessions (filtered by user ownership if role is 'user')
    CROW_ROUTE(app, "/api/sessions").methods(crow::HTTPMethod::GET)
    ([&stationService](const crow::request& req){
        std::string authHeader = req.get_header_value("Authorization");
        int userId = 0;
        if (!authHeader.empty()) {
            crow::response authRes;
            security::JwtClaims claims;
            if (security::JwtUtil::requireAuth(req, authRes, claims)) {
                if (claims.role == "user") {
                    userId = claims.userId;
                }
            } else {
                return authRes;
            }
        }

        std::string statusFilter = "";
        if (req.url_params.get("status") != nullptr) {
            statusFilter = req.url_params.get("status");
        }

        ChargingSession list[MAX_SESSIONS];
        int count = stationService.getAllSessions(list, MAX_SESSIONS, statusFilter, userId);

        crow::json::wvalue::list sessionsJson;
        for (int i = 0; i < count; i++) {
            crow::json::wvalue s;
            s["id"] = list[i].getId();
            s["evId"] = list[i].getVehicleId();
            s["evModel"] = list[i].getEvModel();
            s["stationId"] = list[i].getStationId();
            s["bayId"] = list[i].getBayId();
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
            s["userId"] = list[i].getUserId();
            s["requestId"] = list[i].getRequestId();
            sessionsJson.push_back(std::move(s));
        }

        crow::json::wvalue finalRes = std::move(sessionsJson);
        return crow::response(200, finalRes);
    });

    // GET /api/sessions/active - Convenience route for active charging sessions
    CROW_ROUTE(app, "/api/sessions/active").methods(crow::HTTPMethod::GET)
    ([&stationService](const crow::request& req){
        crow::response res;
        security::JwtClaims claims;
        int userId = 0;
        if (security::JwtUtil::requireAuth(req, res, claims)) {
            if (claims.role == "user") {
                userId = claims.userId;
            }
        } else {
            return res;
        }

        ChargingSession list[MAX_SESSIONS];
        int count = stationService.getAllSessions(list, MAX_SESSIONS, "active", userId);

        crow::json::wvalue::list sessionsJson;
        for (int i = 0; i < count; i++) {
            crow::json::wvalue s;
            s["id"] = list[i].getId();
            s["evId"] = list[i].getVehicleId();
            s["evModel"] = list[i].getEvModel();
            s["stationId"] = list[i].getStationId();
            s["bayId"] = list[i].getBayId();
            s["startTime"] = list[i].getStartTime();
            s["endTime"] = nullptr;
            s["batteryStart"] = list[i].getBatteryStart();
            s["batteryEnd"] = nullptr;
            s["targetCharge"] = list[i].getTargetCharge();
            s["energyConsumed"] = list[i].getEnergyConsumed();
            s["estimatedCost"] = list[i].getEstimatedCost();
            s["status"] = list[i].getStatus();
            s["userId"] = list[i].getUserId();
            s["requestId"] = list[i].getRequestId();
            sessionsJson.push_back(std::move(s));
        }

        crow::json::wvalue finalRes = std::move(sessionsJson);
        return crow::response(200, finalRes);
    });

    // GET /api/sessions/<string> - Detailed session metrics and live simulated progress (Phase 7 & 8)
    CROW_ROUTE(app, "/api/sessions/<string>").methods(crow::HTTPMethod::GET)
    ([&stationService](const crow::request& req, const std::string& sessionId){
        crow::response res;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, res, claims)) {
            return res;
        }

        ChargingSession session;
        double currentSoC = 0.0;
        double energyDelivered = 0.0;
        double currentCost = 0.0;
        int elapsedSeconds = 0;

        if (!stationService.getSessionProgress(sessionId, session, currentSoC, energyDelivered, currentCost, elapsedSeconds)) {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Charging session not found with ID: " + sessionId;
            return crow::response(404, err);
        }

        // Ownership enforcement (Phase 14)
        if (claims.role == "user" && session.getUserId() > 0 && session.getUserId() != claims.userId) {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Access forbidden: you do not have permission to view this charging session";
            return crow::response(403, err);
        }

        crow::json::wvalue s;
        s["id"] = session.getId();
        s["evId"] = session.getVehicleId();
        s["evModel"] = session.getEvModel();
        s["stationId"] = session.getStationId();
        s["bayId"] = session.getBayId();
        s["startTime"] = session.getStartTime();
        if (session.getEndTime().empty()) {
            s["endTime"] = nullptr;
        } else {
            s["endTime"] = session.getEndTime();
        }
        s["batteryStart"] = session.getBatteryStart();
        s["currentCharge"] = currentSoC;
        if (session.getStatus() == "active") {
            s["batteryEnd"] = currentSoC;
        } else {
            s["batteryEnd"] = session.getBatteryEnd();
        }
        s["targetCharge"] = session.getTargetCharge();
        s["energyConsumed"] = energyDelivered;
        s["estimatedCost"] = currentCost;
        s["elapsedSeconds"] = elapsedSeconds;
        s["status"] = session.getStatus();
        s["userId"] = session.getUserId();
        s["requestId"] = session.getRequestId();
        s["power"] = 50.0;
        s["efficiency"] = 0.92;

        return crow::response(200, s);
    });

    // GET /api/charging/requests - Retrieve customer charging requests (Phase 5)
    CROW_ROUTE(app, "/api/charging/requests").methods(crow::HTTPMethod::GET)
    ([&stationService](const crow::request& req){
        crow::response res;
        security::JwtClaims claims;
        int userId = 0;
        if (security::JwtUtil::requireAuth(req, res, claims)) {
            if (claims.role == "user") {
                userId = claims.userId;
            }
        } else {
            return res;
        }

        ChargingRequest list[MAX_REQUESTS];
        int count = stationService.getAllRequests(list, MAX_REQUESTS, userId);

        crow::json::wvalue::list reqList;
        for (int i = 0; i < count; i++) {
            crow::json::wvalue r;
            r["id"] = list[i].getId();
            r["userId"] = list[i].getUserId();
            r["vehicleId"] = list[i].getVehicleId();
            r["stationId"] = list[i].getStationId();
            r["bayId"] = list[i].getBayId();
            r["targetCharge"] = list[i].getTargetCharge();
            r["status"] = list[i].getStatus();
            r["createdAt"] = list[i].getCreatedAt();
            reqList.push_back(std::move(r));
        }

        crow::json::wvalue finalRes = std::move(reqList);
        return crow::response(200, finalRes);
    });

    // POST /api/charging/request - Request a charging bay (assigns bay or enqueues in FIFO ChargingQueue)
    CROW_ROUTE(app, "/api/charging/request").methods(crow::HTTPMethod::POST)
    ([&stationService, &chargingQueue](const crow::request& req){
        crow::response authRes;
        security::JwtClaims claims;
        int callerUserId = 0;
        // Verify JWT authentication (Phase 14)
        if (!security::JwtUtil::requireAuth(req, authRes, claims)) {
            return authRes;
        }
        if (claims.role == "user") {
            callerUserId = claims.userId;
        }

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

        std::string requestedStationId = "";
        if (json.has("stationId")) {
            requestedStationId = json["stationId"].s();
        }

        ChargingRequest newRequest;
        ChargingSession newSession;
        bool allocated = false;

        bool ok = stationService.createChargingRequest(callerUserId, evId, requestedStationId, targetCharge,
                                                       newRequest, newSession, allocated, &chargingQueue);
        if (!ok) {
            res["success"] = false;
            res["message"] = "Charging request rejected. Ensure EV is owned by you, not already charging/queued, and target SoC (" +
                             std::to_string((int)targetCharge) + "%) is greater than current battery charge.";
            return crow::response(400, res);
        }

        res["success"] = true;
        res["requestId"] = newRequest.getId();
        res["assigned"] = allocated;
        res["queued"] = !allocated;
        res["stationId"] = newRequest.getStationId();
        res["bayId"] = newRequest.getBayId();
        res["targetCharge"] = newRequest.getTargetCharge();

        if (allocated) {
            res["sessionId"] = newSession.getId();
            res["estimatedCost"] = newSession.getEstimatedCost();
            res["message"] = "Assigned to Station " + newRequest.getStationId() + " (" + newRequest.getBayId() + ")";
        } else {
            res["queueSize"] = chargingQueue.getSize();
            res["message"] = "All bays currently busy. Added vehicle " + evId + " to FIFO Charging Queue.";
        }

        return crow::response(200, res);
    });

    // POST /api/charging/complete - Complete charging session, record transaction & dispatch FIFO waiting EV
    CROW_ROUTE(app, "/api/charging/complete").methods(crow::HTTPMethod::POST)
    ([&stationService, &chargingQueue, txnStack](const crow::request& req){
        std::string authHeader = req.get_header_value("Authorization");
        security::JwtClaims claims;
        bool hasAuth = false;
        if (!authHeader.empty()) {
            crow::response authRes;
            if (security::JwtUtil::requireAuth(req, authRes, claims)) {
                hasAuth = true;
            } else {
                return authRes;
            }
        }

        crow::json::wvalue res;
        auto json = crow::json::load(req.body);
        if (!json) {
            res["success"] = false;
            res["message"] = "Invalid JSON body";
            return crow::response(400, res);
        }

        std::string identifier = "";
        if (json.has("sessionId")) {
            identifier = json["sessionId"].s();
        } else if (json.has("stationId")) {
            identifier = json["stationId"].s();
        }

        if (identifier.empty()) {
            res["success"] = false;
            res["message"] = "sessionId or stationId is required";
            return crow::response(400, res);
        }

        ChargingSession completedSession;
        Transaction outTxn;
        ChargingSession dispatchedSession;

        if (stationService.completeChargingSession(identifier, completedSession, outTxn, &chargingQueue, &dispatchedSession, claims.userId, claims.role)) {
            // Push completed transaction to Abhishek Ghansala's LIFO stack if available
            if (txnStack != nullptr) {
                txnStack->push(outTxn.getId());
            }

            res["success"] = true;
            res["message"] = "Charging session completed successfully.";
            res["sessionId"] = completedSession.getId();
            res["transactionId"] = outTxn.getId();
            res["amount"] = outTxn.getAmount();
            res["energyDelivered"] = outTxn.getEnergyDelivered();
            res["finalCharge"] = completedSession.getBatteryEnd();

            if (!dispatchedSession.getId().empty()) {
                res["dispatched"] = true;
                res["dispatchedEvId"] = dispatchedSession.getVehicleId();
                res["dispatchedSessionId"] = dispatchedSession.getId();
                res["message"] = "Charging session completed. Auto-dispatched waiting vehicle " + dispatchedSession.getVehicleId() + " from FIFO queue.";
            } else {
                res["dispatched"] = false;
            }
            return crow::response(200, res);
        } else {
            res["success"] = false;
            res["message"] = "Could not complete charging session. Session may not be active, station already released, or permission denied.";
            return crow::response(400, res);
        }
    });
}
