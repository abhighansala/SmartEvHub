#include "transaction_routes.h"
#include <iostream>

// ============================================================================
// Implementation of Transaction REST API Routes
// Module Owner: Agreev Singh (Transactions + Reports + Analytics, ID: 2518161)
// Uses Crow C++ Microframework to exchange JSON data with React frontend
// ============================================================================

void registerTransactionRoutes(crow::App<crow::CORSHandler>& app, TransactionService& txnService) {

    // ------------------------------------------------------------------------
    // 1. GET /api/transactions
    // Returns transaction history. Optional query params:
    // - ?type=charging or ?type=battery-swap
    // - ?evId=EV-103
    // ------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/transactions").methods(crow::HTTPMethod::GET)
    ([&txnService](const crow::request& req){
        Transaction list[MAX_TRANSACTIONS];
        int count = 0;

        char* typeParam = req.url_params.get("type");
        char* evIdParam = req.url_params.get("evId");

        if (typeParam != nullptr) {
            count = txnService.getTransactionsByType(std::string(typeParam), list, MAX_TRANSACTIONS);
        } else if (evIdParam != nullptr) {
            count = txnService.getTransactionsByVehicleId(std::string(evIdParam), list, MAX_TRANSACTIONS);
        } else {
            count = txnService.getAllTransactions(list, MAX_TRANSACTIONS);
        }

        crow::json::wvalue res;
        for (int i = 0; i < count; i++) {
            res[i]["id"] = list[i].getId();
            res[i]["evId"] = list[i].getVehicleId();
            res[i]["type"] = list[i].getType();
            res[i]["stationOrBattery"] = list[i].getStationOrBattery();
            res[i]["description"] = list[i].getDescription();
            res[i]["amount"] = list[i].getAmount();
            res[i]["energyDelivered"] = list[i].getEnergyDelivered();
            res[i]["duration"] = list[i].getDuration();
            res[i]["date"] = list[i].getTimestamp();
            res[i]["status"] = list[i].getStatus();
        }

        return crow::response(200, res);
    });

    // ------------------------------------------------------------------------
    // 2. GET /api/transactions/<string>
    // Looks up a single transaction by ID
    // ------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/transactions/<string>").methods(crow::HTTPMethod::GET)
    ([&txnService](const std::string& id){
        Transaction txn;
        if (txnService.getTransactionById(id, txn)) {
            crow::json::wvalue res;
            res["id"] = txn.getId();
            res["evId"] = txn.getVehicleId();
            res["type"] = txn.getType();
            res["stationOrBattery"] = txn.getStationOrBattery();
            res["description"] = txn.getDescription();
            res["amount"] = txn.getAmount();
            res["energyDelivered"] = txn.getEnergyDelivered();
            res["duration"] = txn.getDuration();
            res["date"] = txn.getTimestamp();
            res["status"] = txn.getStatus();
            return crow::response(200, res);
        } else {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Transaction with ID " + id + " not found";
            return crow::response(404, err);
        }
    });

    // ------------------------------------------------------------------------
    // 3. POST /api/transactions
    // Records a completed charging or swapping transaction
    // ------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/transactions").methods(crow::HTTPMethod::POST)
    ([&txnService](const crow::request& req){
        crow::json::wvalue res;
        auto json = crow::json::load(req.body);

        if (!json) {
            res["success"] = false;
            res["message"] = "Invalid JSON body";
            return crow::response(400, res);
        }

        // Extract fields with fallbacks
        std::string id = txnService.generateNextId();
        if (json.has("id")) {
            id = std::string(json["id"].s());
        }

        std::string evId = "";
        if (json.has("evId")) {
            evId = std::string(json["evId"].s());
        } else if (json.has("vehicleId")) {
            evId = std::string(json["vehicleId"].s());
        }

        std::string type = "charging";
        if (json.has("type")) {
            type = std::string(json["type"].s());
        }

        std::string target = "";
        if (json.has("stationOrBattery")) {
            target = std::string(json["stationOrBattery"].s());
        }

        std::string desc = "";
        if (json.has("description")) {
            desc = std::string(json["description"].s());
        }

        double energy = 0.0;
        if (json.has("energyDelivered")) {
            energy = json["energyDelivered"].d();
        }

        double amount = 0.0;
        if (json.has("amount")) {
            amount = json["amount"].d();
        } else {
            // Auto-calculate bill using OOP syllabus tariff logic
            amount = Transaction::calculateTariff(energy, type);
        }

        int duration = 0;
        if (json.has("duration")) {
            duration = json["duration"].i();
        }

        std::string timestamp = "";
        if (json.has("date")) {
            timestamp = std::string(json["date"].s());
        } else if (json.has("timestamp")) {
            timestamp = std::string(json["timestamp"].s());
        }

        std::string status = "completed";
        if (json.has("status")) {
            status = std::string(json["status"].s());
        }

        Transaction txn(id, evId, type, target, desc, amount, energy, duration, timestamp, status);

        if (txnService.addTransaction(txn)) {
            res["success"] = true;
            res["message"] = "Transaction recorded successfully";
            res["id"] = txn.getId();
            res["amount"] = txn.getAmount();
            return crow::response(201, res);
        } else {
            res["success"] = false;
            res["message"] = "Failed to record transaction. Ensure ID and Vehicle ID are provided.";
            return crow::response(400, res);
        }
    });
}
