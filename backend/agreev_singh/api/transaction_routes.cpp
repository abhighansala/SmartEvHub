#include "transaction_routes.h"

void registerTransactionRoutes(crow::App<crow::CORSHandler>& app, TransactionService& txnService) {

    // GET /api/transactions - List all transactions
    CROW_ROUTE(app, "/api/transactions").methods(crow::HTTPMethod::GET)
    ([&txnService](){
        Transaction list[MAX_TRANSACTIONS];
        int count = txnService.getAllTransactions(list, MAX_TRANSACTIONS);

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

    // POST /api/transactions - Record a new completed transaction
    CROW_ROUTE(app, "/api/transactions").methods(crow::HTTPMethod::POST)
    ([&txnService](const crow::request& req){
        crow::json::wvalue res;
        auto json = crow::json::load(req.body);
        if (!json) {
            res["success"] = false;
            res["message"] = "Invalid JSON body";
            return crow::response(400, res);
        }

        std::string id = "TXN-AUTO";
        std::string evId = "";
        std::string type = "charging";
        std::string target = "";
        std::string desc = "";
        double amount = 0.0;
        double energy = 0.0;
        int duration = 0;

        if (json.has("id")) id = std::string(json["id"].s());
        if (json.has("evId")) evId = std::string(json["evId"].s());
        if (json.has("type")) type = std::string(json["type"].s());
        if (json.has("stationOrBattery")) target = std::string(json["stationOrBattery"].s());
        if (json.has("description")) desc = std::string(json["description"].s());
        if (json.has("amount")) amount = json["amount"].d();
        if (json.has("energyDelivered")) energy = json["energyDelivered"].d();
        if (json.has("duration")) duration = json["duration"].i();

        Transaction txn(id, evId, type, target, desc, amount, energy, duration);
        if (txnService.addTransaction(txn)) {
            res["success"] = true;
            res["message"] = "Transaction recorded successfully";
            res["id"] = id;
            return crow::response(201, res);
        } else {
            res["success"] = false;
            res["message"] = "Failed to record transaction";
            return crow::response(500, res);
        }
    });
}
