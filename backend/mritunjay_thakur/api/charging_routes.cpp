#include "charging_routes.h"
#include "../../aditya_singh/security/jwt_util.h"
#include <vector>

void registerChargingRoutes(crow::App<crow::CORSHandler>& app,
                            StationService& stationService,
                            ChargingQueue& chargingQueue,
                            TransactionStack* txnStack) {

    // GET /api/sessions - Retrieve charging sessions (filtered by user ownership if role is 'user')
    CROW_ROUTE(app, "/api/sessions").methods(crow::HTTPMethod::GET)
    ([&stationService, &chargingQueue](const crow::request& req){
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

        // Advance live server-side progression and auto-complete any finished sessions
        stationService.advanceActiveSessions(&chargingQueue);

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
<<<<<<< HEAD
            if (list[i].getStatus() == "active") {
                ChargingSession progSession;
                double currentSoC = list[i].getBatteryStart();
                double energyDelivered = 0.0;
                double currentCost = list[i].getEstimatedCost();
                int elapsedSeconds = 0;
                if (stationService.getSessionProgress(list[i].getId(), progSession, currentSoC, energyDelivered, currentCost, elapsedSeconds)) {
                    s["currentCharge"] = currentSoC;
                    s["batteryEnd"] = currentSoC;
                    s["energyConsumed"] = energyDelivered;
                    s["estimatedCost"] = currentCost;
                    s["elapsedSeconds"] = elapsedSeconds;
                } else {
                    s["currentCharge"] = list[i].getBatteryStart();
                    s["batteryEnd"] = list[i].getBatteryStart();
                    s["energyConsumed"] = list[i].getEnergyConsumed();
                    s["estimatedCost"] = list[i].getEstimatedCost();
                }
            } else {
                s["currentCharge"] = list[i].getBatteryEnd();
                s["batteryEnd"] = list[i].getBatteryEnd();
                s["energyConsumed"] = list[i].getEnergyConsumed();
                s["estimatedCost"] = list[i].getEstimatedCost();
=======

            double currentCharge = list[i].getBatteryStart();
            if (list[i].getStatus() == "active") {
                currentCharge = (list[i].getBatteryEnd() > 0.0) ? list[i].getBatteryEnd() : list[i].getBatteryStart();
                s["batteryEnd"] = currentCharge;
            } else {
                currentCharge = (list[i].getBatteryEnd() > 0.0) ? list[i].getBatteryEnd() : list[i].getTargetCharge();
                s["batteryEnd"] = currentCharge;
>>>>>>> origin/main
            }
            s["currentCharge"] = currentCharge;

            // Physical simulated & demo remaining time calculation
            int remainingMins = 0;
            int demoRemainingMins = 0;
            if (list[i].getStatus() == "active" && list[i].getTargetCharge() > currentCharge) {
                double neededKWh = ((list[i].getTargetCharge() - currentCharge) / 100.0) * 50.0;
                double powerKW = 25.0;
                ChargingStation st;
                if (stationService.getStationById(list[i].getStationId(), st) && st.getPower() > 0.0) {
                    powerKW = st.getPower();
                }
                double eff = stationService.getChargingEfficiency();
                double batterySidePower = powerKW * eff;
                double physicalHours = (batterySidePower > 0.0) ? (neededKWh / batterySidePower) : 0.0;
                remainingMins = static_cast<int>(std::ceil(physicalHours * 60.0));
                double mult = stationService.getSimulationSpeedMultiplier();
                demoRemainingMins = static_cast<int>(std::ceil((physicalHours / mult) * 60.0));
            }
            s["remainingMinutes"] = remainingMins;
            s["simulatedRemainingMinutes"] = remainingMins;
            s["demoRemainingMinutes"] = demoRemainingMins;
            s["speedMultiplier"] = stationService.getSimulationSpeedMultiplier();

            s["targetCharge"] = list[i].getTargetCharge();
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
    ([&stationService, &chargingQueue](const crow::request& req){
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

        // Advance live server-side progression and auto-complete any finished sessions
        stationService.advanceActiveSessions(&chargingQueue);

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

<<<<<<< HEAD
            ChargingSession progSession;
            double currentSoC = list[i].getBatteryStart();
            double energyDelivered = 0.0;
            double currentCost = list[i].getEstimatedCost();
            int elapsedSeconds = 0;
            if (stationService.getSessionProgress(list[i].getId(), progSession, currentSoC, energyDelivered, currentCost, elapsedSeconds)) {
                s["currentCharge"] = currentSoC;
                s["batteryEnd"] = currentSoC;
                s["energyConsumed"] = energyDelivered;
                s["estimatedCost"] = currentCost;
                s["elapsedSeconds"] = elapsedSeconds;
            } else {
                s["currentCharge"] = list[i].getBatteryStart();
                s["batteryEnd"] = list[i].getBatteryStart();
                s["energyConsumed"] = list[i].getEnergyConsumed();
                s["estimatedCost"] = list[i].getEstimatedCost();
            }
=======
            double currentCharge = (list[i].getBatteryEnd() > 0.0) ? list[i].getBatteryEnd() : list[i].getBatteryStart();
            s["batteryEnd"] = currentCharge;
            s["currentCharge"] = currentCharge;

            int remainingMins = 0;
            int demoRemainingMins = 0;
            if (list[i].getTargetCharge() > currentCharge) {
                double neededKWh = ((list[i].getTargetCharge() - currentCharge) / 100.0) * 50.0;
                double powerKW = 25.0;
                ChargingStation st;
                if (stationService.getStationById(list[i].getStationId(), st) && st.getPower() > 0.0) {
                    powerKW = st.getPower();
                }
                double eff = stationService.getChargingEfficiency();
                double batterySidePower = powerKW * eff;
                double physicalHours = (batterySidePower > 0.0) ? (neededKWh / batterySidePower) : 0.0;
                remainingMins = static_cast<int>(std::ceil(physicalHours * 60.0));
                double mult = stationService.getSimulationSpeedMultiplier();
                demoRemainingMins = static_cast<int>(std::ceil((physicalHours / mult) * 60.0));
            }
            s["remainingMinutes"] = remainingMins;
            s["simulatedRemainingMinutes"] = remainingMins;
            s["demoRemainingMinutes"] = demoRemainingMins;
            s["speedMultiplier"] = stationService.getSimulationSpeedMultiplier();
>>>>>>> origin/main

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

    // GET /api/sessions/<string> - Detailed session metrics and live simulated progress
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

        // Ownership enforcement: User cannot view another customer's session
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

        crow::json::wvalue wrapper;
        wrapper["session"] = std::move(s);
        return crow::response(200, wrapper);
    });

    // GET /api/charging/requests - Retrieve customer charging requests (filtered by user ownership if role is 'user')
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

        // Fetch all queued requests to compute FIFO queue positions accurately
        ChargingRequest allQueued[MAX_REQUESTS];
        int allQueuedCount = stationService.getAllRequests(allQueued, MAX_REQUESTS, 0);
        std::vector<std::string> queuedFifoIds;
        for (int i = allQueuedCount - 1; i >= 0; i--) { // getAllRequests is ordered by created_at DESC
            if (allQueued[i].getStatus() == "queued") {
                queuedFifoIds.push_back(allQueued[i].getId());
            }
        }

        crow::json::wvalue::list reqList;
        for (int i = 0; i < count; i++) {
            crow::json::wvalue r;
            r["id"] = list[i].getId();
            r["userId"] = list[i].getUserId();
            r["vehicleId"] = list[i].getVehicleId();
            r["stationId"] = list[i].getStationId();
            r["bayId"] = list[i].getBayId();
            r["targetCharge"] = list[i].getTargetCharge();
            r["connectorType"] = list[i].getConnectorType();
            r["status"] = list[i].getStatus();
            r["createdAt"] = list[i].getCreatedAt();

            // Calculate 1-based queue position if queued
            int queuePos = 0;
            if (list[i].getStatus() == "queued") {
                for (size_t p = 0; p < queuedFifoIds.size(); p++) {
                    if (queuedFifoIds[p] == list[i].getId()) {
                        queuePos = static_cast<int>(p + 1);
                        break;
                    }
                }
            }
            r["queuePosition"] = queuePos;

            reqList.push_back(std::move(r));
        }

        crow::json::wvalue finalRes = std::move(reqList);
        return crow::response(200, finalRes);
    });

    // GET /api/charging/request/<string> - Single customer charging request details
    CROW_ROUTE(app, "/api/charging/request/<string>").methods(crow::HTTPMethod::GET)
    ([&stationService](const crow::request& req, const std::string& requestId){
        crow::response res;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, res, claims)) {
            return res;
        }

        ChargingRequest request;
        if (!stationService.getRequestById(requestId, request)) {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Charging request not found with ID: " + requestId;
            return crow::response(404, err);
        }

        if (claims.role == "user" && request.getUserId() > 0 && request.getUserId() != claims.userId) {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Access forbidden: you do not have permission to view this charging request";
            return crow::response(403, err);
        }

        crow::json::wvalue r;
        r["id"] = request.getId();
        r["userId"] = request.getUserId();
        r["vehicleId"] = request.getVehicleId();
        r["stationId"] = request.getStationId();
        r["bayId"] = request.getBayId();
        r["targetCharge"] = request.getTargetCharge();
        r["connectorType"] = request.getConnectorType();
        r["status"] = request.getStatus();
        r["createdAt"] = request.getCreatedAt();

        return crow::response(200, r);
    });

    // POST /api/charging/request - Request a charging bay (assigns bay or enqueues in FIFO ChargingQueue)
    CROW_ROUTE(app, "/api/charging/request").methods(crow::HTTPMethod::POST)
    ([&stationService, &chargingQueue](const crow::request& req){
        crow::response authRes;
        security::JwtClaims claims;
        int callerUserId = 0;
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

        std::string connectorType = "CCS2";
        if (json.has("connectorType")) {
            connectorType = json["connectorType"].s();
        }

        ChargingRequest newRequest;
        ChargingSession newSession;
        bool allocated = false;

        bool ok = stationService.createChargingRequest(callerUserId, evId, requestedStationId, targetCharge,
                                                       newRequest, newSession, allocated, &chargingQueue, connectorType);
        if (!ok) {
            res["success"] = false;
            res["message"] = "Charging request rejected. Ensure EV is owned by you, not already charging/queued, and target SoC (" +
                             std::to_string((int)targetCharge) + "%) is greater than current battery charge.";
            return crow::response(400, res);
        }

        res["success"] = true;
        res["requestId"] = newRequest.getId();
        res["assigned"] = allocated;
        res["allocated"] = allocated;
        res["queued"] = !allocated;
        res["stationId"] = newRequest.getStationId();
        res["bayId"] = newRequest.getBayId();
        res["targetCharge"] = newRequest.getTargetCharge();
        res["connectorType"] = newRequest.getConnectorType();

        if (allocated) {
            res["sessionId"] = newSession.getId();
            res["estimatedCost"] = newSession.getEstimatedCost();
            res["session"]["id"] = newSession.getId();
            res["session"]["stationId"] = newSession.getStationId();
            res["session"]["evId"] = newSession.getVehicleId();
            res["session"]["estimatedCost"] = newSession.getEstimatedCost();
            res["message"] = "Assigned to Station " + newRequest.getStationId() + " (" + newRequest.getBayId() + ")";
        } else {
            res["queueSize"] = chargingQueue.getSize();
            res["message"] = "All bays currently busy. Added vehicle " + evId + " to FIFO Charging Queue.";
        }

        return crow::response(200, res);
    });

    // POST /api/charging/request/<string>/cancel - Cancel a customer charging request
    CROW_ROUTE(app, "/api/charging/request/<string>/cancel").methods(crow::HTTPMethod::POST)
    ([&stationService, &chargingQueue](const crow::request& req, const std::string& requestId){
        crow::response authRes;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, authRes, claims)) {
            return authRes;
        }

        ChargingRequest existingReq;
        if (!stationService.getRequestById(requestId, existingReq)) {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Charging request not found with ID: " + requestId;
            return crow::response(404, err);
        }

        if (claims.role == "user" && claims.userId > 0 && existingReq.getUserId() != claims.userId) {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Access forbidden: you do not have permission to cancel another customer's request";
            return crow::response(403, err);
        }

        if (stationService.cancelChargingRequest(requestId, claims.userId, claims.role, &chargingQueue)) {
            crow::json::wvalue res;
            res["success"] = true;
            res["message"] = "Charging request " + requestId + " cancelled successfully";
            return crow::response(200, res);
        } else {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Could not cancel charging request. Request may already be completed or cancelled.";
            return crow::response(400, err);
        }
    });

    // DELETE /api/charging/requests/<string> - Cancel / delete a charging request
    CROW_ROUTE(app, "/api/charging/requests/<string>").methods(crow::HTTPMethod::Delete)
    ([&stationService, &chargingQueue](const crow::request& req, const std::string& requestId){
        crow::response authRes;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, authRes, claims)) {
            return authRes;
        }

        ChargingRequest existingReq;
        if (!stationService.getRequestById(requestId, existingReq)) {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Charging request not found with ID: " + requestId;
            return crow::response(404, err);
        }

        if (claims.role == "user" && claims.userId > 0 && existingReq.getUserId() != claims.userId) {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Access forbidden: you do not have permission to delete another customer's request";
            return crow::response(403, err);
        }

        if (stationService.cancelChargingRequest(requestId, claims.userId, claims.role, &chargingQueue)) {
            crow::json::wvalue res;
            res["success"] = true;
            res["message"] = "Charging request " + requestId + " removed successfully";
            return crow::response(200, res);
        } else {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Could not remove charging request.";
            return crow::response(400, err);
        }
    });

    // POST /api/charging/allocate - Manually allocate a queued request to an available bay (Operator / Admin only)
    CROW_ROUTE(app, "/api/charging/allocate").methods(crow::HTTPMethod::POST)
    ([&stationService, &chargingQueue](const crow::request& req){
        crow::response authRes;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, authRes, claims)) {
            return authRes;
        }
        if (claims.role != "operator" && claims.role != "admin") {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Access forbidden: Only operators and admins can manually allocate charging bays";
            return crow::response(403, err);
        }

        auto json = crow::json::load(req.body);
        if (!json || !json.has("requestId") || !json.has("stationId")) {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Fields 'requestId' and 'stationId' are required";
            return crow::response(400, err);
        }

        std::string requestId = json["requestId"].s();
        std::string stationId = json["stationId"].s();

        ChargingRequest outReq;
        ChargingSession outSes;
        if (stationService.allocateBayManually(requestId, stationId, outReq, outSes, &chargingQueue, claims.userId, claims.role)) {
            crow::json::wvalue res;
            res["success"] = true;
            res["message"] = "Manually allocated request " + requestId + " to station " + stationId;
            res["requestId"] = outReq.getId();
            res["stationId"] = outReq.getStationId();
            res["sessionId"] = outSes.getId();
            return crow::response(200, res);
        } else {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Could not allocate bay. Ensure request is queued and station is available and compatible.";
            return crow::response(400, err);
        }
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

        // Ownership enforcement: Check if session belongs to caller if caller is a normal user
        ChargingSession activeSession;
        bool sessionFound = false;
        if (identifier.rfind("SES-", 0) == 0) {
            sessionFound = stationService.getSessionById(identifier, activeSession);
        } else {
            sessionFound = stationService.getActiveSessionByStationId(identifier, activeSession);
        }

        if (sessionFound && hasAuth && claims.role == "user" && claims.userId > 0) {
            if (activeSession.getUserId() > 0 && activeSession.getUserId() != claims.userId) {
                res["success"] = false;
                res["message"] = "Access forbidden: you do not have permission to complete another customer's charging session";
                return crow::response(403, res);
            }
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

            res["transaction"]["id"] = outTxn.getId();
            res["transaction"]["amount"] = outTxn.getAmount();
            res["transaction"]["energyDelivered"] = outTxn.getEnergyDelivered();

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

    // GET /api/charging/config - Query simulation configuration
    CROW_ROUTE(app, "/api/charging/config").methods(crow::HTTPMethod::GET)
    ([&stationService](){
        crow::json::wvalue res;
        res["success"] = true;
        res["speedMultiplier"] = stationService.getSimulationSpeedMultiplier();
        res["efficiency"] = stationService.getChargingEfficiency();
        res["tariffRate"] = stationService.getTariffRate();
        return crow::response(200, res);
    });

    // POST /api/charging/config - Update simulation configuration (operator/admin only)
    CROW_ROUTE(app, "/api/charging/config").methods(crow::HTTPMethod::POST)
    ([&stationService](const crow::request& req){
        crow::response authRes;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, authRes, claims, "operator")) {
            return authRes;
        }

        auto json = crow::json::load(req.body);
        if (!json) {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Invalid JSON payload";
            return crow::response(400, err);
        }

        if (json.has("speedMultiplier")) {
            stationService.setSimulationSpeedMultiplier(json["speedMultiplier"].d());
        }
        if (json.has("efficiency")) {
            stationService.setChargingEfficiency(json["efficiency"].d());
        }
        if (json.has("tariffRate")) {
            stationService.setTariffRate(json["tariffRate"].d());
        }

        crow::json::wvalue res;
        res["success"] = true;
        res["speedMultiplier"] = stationService.getSimulationSpeedMultiplier();
        res["efficiency"] = stationService.getChargingEfficiency();
        res["tariffRate"] = stationService.getTariffRate();
        return crow::response(200, res);
    });
}
