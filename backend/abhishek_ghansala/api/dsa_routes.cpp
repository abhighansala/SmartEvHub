#include "dsa_routes.h"
#include "../../aditya_singh/security/jwt_util.h"
#include "../../aditya_singh/database/database.h"
#include "../../mritunjay_thakur/services/station_service.h"

void registerDSARoutes(crow::App<crow::CORSHandler>& app,
                       ChargingQueue& chargingQueue,
                       SwapQueue& swapQueue,
                       TransactionStack& txnStack,
                       StationService* stationService,
                       Database* db) {

    // GET /api/charging/queue - Visual FIFO Charging Queue state with strict User Data Isolation
    CROW_ROUTE(app, "/api/charging/queue").methods(crow::HTTPMethod::GET)
    ([&chargingQueue, db](const crow::request& req){
        std::string authHeader = req.get_header_value("Authorization");
        security::JwtClaims claims;
        bool isUser = false;
        int userId = 0;
        if (!authHeader.empty()) {
            crow::response authRes;
            if (security::JwtUtil::requireAuth(req, authRes, claims)) {
                if (claims.role == "user") {
                    isUser = true;
                    userId = claims.userId;
                }
            }
        }

        std::string items[CHARGING_QUEUE_CAPACITY];
        int count = chargingQueue.getItems(items, CHARGING_QUEUE_CAPACITY);

        crow::json::wvalue res;
        res["frontIndex"] = chargingQueue.getFrontIndex();
        res["rearIndex"] = chargingQueue.getRearIndex();
        res["size"] = chargingQueue.getSize();
        res["capacity"] = CHARGING_QUEUE_CAPACITY;

        if (isUser && userId > 0 && db != nullptr && db->isOpen()) {
            // Find which items in the queue belong to the authenticated user
            sqlite3* handle = db->getDbHandle();
            int userPos = 0;
            int outIdx = 0;
            for (int i = 0; i < count; i++) {
                std::string evId = items[i];
                std::string sql = "SELECT user_id FROM EV WHERE id = ?;";
                sqlite3_stmt* stmt = nullptr;
                int ownerId = 0;
                if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(stmt, 1, evId.c_str(), -1, SQLITE_STATIC);
                    if (sqlite3_step(stmt) == SQLITE_ROW) {
                        ownerId = sqlite3_column_int(stmt, 0);
                    }
                    sqlite3_finalize(stmt);
                }
                if (ownerId == userId) {
                    res["items"][outIdx++] = evId;
                    if (userPos == 0) {
                        userPos = i + 1; // 1-based queue position
                    }
                }
            }
            if (outIdx == 0) {
                res["items"] = crow::json::wvalue::list();
            }
            res["userQueuePosition"] = userPos;
        } else {
            // Operator / Admin or unauthenticated demo mode: show all operational items
            for (int i = 0; i < count; i++) {
                res["items"][i] = items[i];
            }
            if (count == 0) {
                res["items"] = crow::json::wvalue::list();
            }
        }

        return crow::response(200, res);
    });

    // GET /api/swap/queue - Visual FIFO Swap Queue state
    CROW_ROUTE(app, "/api/swap/queue").methods(crow::HTTPMethod::GET)
    ([&swapQueue](){
        std::string items[SWAP_QUEUE_CAPACITY];
        int count = swapQueue.getItems(items, SWAP_QUEUE_CAPACITY);

        crow::json::wvalue res;
        res["frontIndex"] = swapQueue.getFrontIndex();
        res["rearIndex"] = swapQueue.getRearIndex();
        res["size"] = swapQueue.getSize();
        res["capacity"] = SWAP_QUEUE_CAPACITY;

        for (int i = 0; i < count; i++) {
            res["items"][i] = items[i];
        }
        if (count == 0) {
            res["items"] = crow::json::wvalue::list();
        }

        return crow::response(200, res);
    });

    // GET /api/transactions/stack - Visual LIFO Transaction Stack state
    CROW_ROUTE(app, "/api/transactions/stack").methods(crow::HTTPMethod::GET)
    ([&txnStack](){
        std::string items[TRANSACTION_STACK_CAPACITY];
        int count = txnStack.getItems(items, TRANSACTION_STACK_CAPACITY);

        crow::json::wvalue res;
        res["topIndex"] = txnStack.getTopIndex();
        res["size"] = txnStack.getSize();
        res["capacity"] = TRANSACTION_STACK_CAPACITY;

        for (int i = 0; i < count; i++) {
            res["items"][i] = items[i];
        }
        if (count == 0) {
            res["items"] = crow::json::wvalue::list();
        }

        return crow::response(200, res);
    });

    // POST /api/charging/queue - Enqueue an EV into the FIFO Charging Queue
    CROW_ROUTE(app, "/api/charging/queue").methods(crow::HTTPMethod::POST)
    ([&chargingQueue](const crow::request& req){
        crow::json::wvalue res;
        auto body = crow::json::load(req.body);
        if (!body || !body.has("evId")) {
            res["success"] = false;
            res["message"] = "Field 'evId' is required";
            return crow::response(400, res);
        }

        std::string evId = std::string(body["evId"].s());
        if (chargingQueue.isFull()) {
            res["success"] = false;
            res["message"] = "Charging queue overflow! Queue is full.";
            return crow::response(400, res);
        }

        chargingQueue.enqueue(evId);
        res["success"] = true;
        res["message"] = "Vehicle " + evId + " enqueued at REAR";
        res["size"] = chargingQueue.getSize();
        res["frontIndex"] = chargingQueue.getFrontIndex();
        res["rearIndex"] = chargingQueue.getRearIndex();
        return crow::response(200, res);
    });

    // POST /api/charging/dequeue - Process next EV from FRONT of Charging Queue (FIFO) (Operator / Admin only)
    CROW_ROUTE(app, "/api/charging/dequeue").methods(crow::HTTPMethod::POST)
    ([&chargingQueue, stationService](const crow::request& req){
        crow::response authRes;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, authRes, claims)) {
            return authRes;
        }

        // Role-based authorization: Only operator and admin can dequeue
        if (claims.role != "operator" && claims.role != "admin") {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Access forbidden: Only operators and admins can manually dequeue charging requests";
            return crow::response(403, err);
        }

        crow::json::wvalue res;
        if (chargingQueue.isEmpty()) {
            res["success"] = false;
            res["message"] = "Charging queue is empty! Underflow.";
            return crow::response(400, res);
        }

        std::string vehicleId = chargingQueue.dequeue();

        // Trigger automatic bay allocation if stationService is available
        if (stationService != nullptr) {
            stationService->processQueueForAvailableStations(&chargingQueue);
        }

        res["success"] = true;
        res["message"] = "Dequeued vehicle " + vehicleId + " from FRONT";
        res["evId"] = vehicleId;
        res["remainingSize"] = chargingQueue.getSize();
        res["frontIndex"] = chargingQueue.getFrontIndex();
        res["rearIndex"] = chargingQueue.getRearIndex();
        return crow::response(200, res);
    });

    // POST /api/swap/queue - Enqueue an EV into the FIFO Swap Queue
    CROW_ROUTE(app, "/api/swap/queue").methods(crow::HTTPMethod::POST)
    ([&swapQueue](const crow::request& req){
        crow::json::wvalue res;
        auto body = crow::json::load(req.body);
        if (!body || !body.has("evId")) {
            res["success"] = false;
            res["message"] = "Field 'evId' is required";
            return crow::response(400, res);
        }

        std::string evId = std::string(body["evId"].s());
        if (swapQueue.isFull()) {
            res["success"] = false;
            res["message"] = "Swap queue overflow! Queue is full.";
            return crow::response(400, res);
        }

        swapQueue.enqueue(evId);
        res["success"] = true;
        res["message"] = "Vehicle " + evId + " enqueued into swap queue";
        res["size"] = swapQueue.getSize();
        res["frontIndex"] = swapQueue.getFrontIndex();
        res["rearIndex"] = swapQueue.getRearIndex();
        return crow::response(200, res);
    });

    // POST /api/swap/dequeue - Process next EV from FRONT of Swap Queue (FIFO)
    CROW_ROUTE(app, "/api/swap/dequeue").methods(crow::HTTPMethod::POST)
    ([&swapQueue](){
        crow::json::wvalue res;
        if (swapQueue.isEmpty()) {
            res["success"] = false;
            res["message"] = "Swap queue is empty! Underflow.";
            return crow::response(400, res);
        }

        std::string vehicleId = swapQueue.dequeue();
        res["success"] = true;
        res["message"] = "Dequeued vehicle " + vehicleId + " from FRONT";
        res["evId"] = vehicleId;
        res["remainingSize"] = swapQueue.getSize();
        res["frontIndex"] = swapQueue.getFrontIndex();
        res["rearIndex"] = swapQueue.getRearIndex();
        return crow::response(200, res);
    });

    // POST /api/transactions/stack - Push a transaction ID onto TOP of Stack (LIFO)
    CROW_ROUTE(app, "/api/transactions/stack").methods(crow::HTTPMethod::POST)
    ([&txnStack](const crow::request& req){
        crow::json::wvalue res;
        auto body = crow::json::load(req.body);
        if (!body || !body.has("id")) {
            res["success"] = false;
            res["message"] = "Transaction ID 'id' is required";
            return crow::response(400, res);
        }

        std::string txnId = std::string(body["id"].s());
        if (txnStack.isFull()) {
            res["success"] = false;
            res["message"] = "Transaction stack overflow!";
            return crow::response(400, res);
        }

        txnStack.push(txnId);
        res["success"] = true;
        res["message"] = "Transaction " + txnId + " pushed to TOP of stack";
        res["topIndex"] = txnStack.getTopIndex();
        res["size"] = txnStack.getSize();
        return crow::response(200, res);
    });

    // POST /api/transactions/stack/pop - Pop transaction from TOP of Stack (LIFO)
    CROW_ROUTE(app, "/api/transactions/stack/pop").methods(crow::HTTPMethod::POST)
    ([&txnStack](){
        crow::json::wvalue res;
        if (txnStack.isEmpty()) {
            res["success"] = false;
            res["message"] = "Transaction stack underflow! Stack is empty.";
            return crow::response(400, res);
        }

        std::string popped = txnStack.pop();
        res["success"] = true;
        res["message"] = "Popped transaction " + popped + " from TOP of stack";
        res["id"] = popped;
        res["topIndex"] = txnStack.getTopIndex();
        res["size"] = txnStack.getSize();
        return crow::response(200, res);
    });
}
