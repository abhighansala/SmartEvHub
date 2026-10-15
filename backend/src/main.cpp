#include <crow.h>
#include <crow/middlewares/cors.h>
#include <iostream>

// Member 1: Aditya Singh (Database + Auth)
#include "../aditya_singh/database/database.h"
#include "../aditya_singh/services/auth_service.h"
#include "../aditya_singh/api/auth_routes.h"

// Member 2: Priyanshu Dutt (EV Management)
#include "../priyanshu_dutt/services/ev_service.h"
#include "../priyanshu_dutt/api/ev_routes.h"

// Member 3: Mritunjay Thakur (Charging Stations)
#include "../mritunjay_thakur/services/station_service.h"
#include "../mritunjay_thakur/api/station_routes.h"
#include "../mritunjay_thakur/api/charging_routes.h"

// Member 4: Amit Singh Kaintura (Battery + Battery Swap)
#include "../amit_singh_kaintura/services/battery_service.h"
#include "../amit_singh_kaintura/api/battery_routes.h"
#include "../amit_singh_kaintura/api/swap_routes.h"

// Member 5: Abhishek Ghansala (UI + DSA)
#include "../abhishek_ghansala/dsa/charging_queue.h"
#include "../abhishek_ghansala/dsa/swap_queue.h"
#include "../abhishek_ghansala/dsa/transaction_stack.h"
#include "../abhishek_ghansala/api/dsa_routes.h"

// Member 6: Agreev Singh (Transactions + Analytics)
#include "../agreev_singh/services/transaction_service.h"
#include "../agreev_singh/api/transaction_routes.h"
#include "../agreev_singh/api/analytics_routes.h"

int main()
{
    std::cout << "========================================================\n";
    std::cout << "       Smart EV Hub - Unified C++ Backend Server        \n";
    std::cout << "========================================================\n";

    // 1. Initialize SQLite Database (Aditya Singh)
    Database db("evhub.db");
    if (!db.open()) {
        std::cerr << "[Warning] Could not open evhub.db. Running in fallback mode.\n";
    } else {
        // Initialize schema and seed data if not present
        if (!db.initSchema("../database/schema.sql") &&
            !db.initSchema("backend/database/schema.sql") &&
            !db.initSchema("database/schema.sql")) {
            std::cerr << "[Warning] Could not load schema.sql\n";
        }
    }

    // 2. Initialize Member Services
    AuthService authService(&db);
    EVService evService(&db);
    StationService stationService(&db);
    BatteryService batteryService(&db);
    ChargingQueue chargingQueue;
    SwapQueue swapQueue;
    TransactionStack txnStack;
    TransactionService txnService(&db);

    // 3. Setup Crow Application with CORS for React frontend
    crow::App<crow::CORSHandler> app;
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors
      .global()
        .headers("X-Custom-Header", "Content-Type", "Authorization")
        .methods("POST"_method, "GET"_method, "PUT"_method, "DELETE"_method, "OPTIONS"_method)
        .origin("*"); // Allow all origins for local React development

    // 4. Health Check Endpoint
    CROW_ROUTE(app, "/api/health")
    ([](){
        crow::json::wvalue res;
        res["status"] = "ok";
        res["service"] = "Smart EV Hub Unified Backend";
        res["teamMembers"] = 6;
        return crow::response(200, res);
    });

    // 5. Register Routes from Each Team Member
    registerAuthRoutes(app, authService);
    registerEVRoutes(app, evService);
    registerStationRoutes(app, stationService);
    registerChargingRoutes(app, stationService);
    registerBatteryRoutes(app, batteryService);
    registerSwapRoutes(app, batteryService);
    registerDSARoutes(app, chargingQueue, swapQueue, txnStack);
    registerTransactionRoutes(app, txnService);
    registerAnalyticsRoutes(app, txnService);

    std::cout << "\n[Server Ready] Listening on http://localhost:8080\n";
    std::cout << "Press Ctrl+C to stop the server.\n\n";

    // Run on port 8080 (standard for local backend)
    app.port(8080).run();

    return 0;
}
