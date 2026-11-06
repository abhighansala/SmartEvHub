#include <crow.h>
#include <crow/middlewares/cors.h>
#include <iostream>

// Member 1: Aditya Singh (Database + Auth)
#include "../aditya_singh/database/database.h"
#include "../aditya_singh/services/auth_service.h"
#include "../aditya_singh/api/auth_routes.h"
#include "../aditya_singh/security/jwt_util.h"

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

int main(int argc, char* argv[])
{
    std::cout << "========================================================\n";
    std::cout << "       Smart EV Hub - Unified C++ Backend Server        \n";
    std::cout << "========================================================\n";

    // 0. Enforce JWT Security Configuration on Startup (Requirement 1)
    std::string secretErr;
    if (!security::JwtUtil::validateStartupSecret(secretErr)) {
        std::cerr << "========================================================\n";
        std::cerr << " [FATAL SECURITY ERROR] Startup aborted: " << secretErr << "\n";
        std::cerr << " A securely generated JWT_SECRET environment variable is required.\n";
        std::cerr << " Minimum key length: 32 characters.\n";
        std::cerr << "========================================================\n";
        return 1;
    }

    // 1. Resolve canonical SQLite Database & Schema path
    // Ensures consistent persistence whether started from root, backend, or backend/build
    std::string dbPath = "evhub.db";
    std::string schemaPath = "";

    std::ifstream checkRootSchema("backend/database/schema.sql");
    std::ifstream checkBuildSchema("../database/schema.sql");
    std::ifstream checkBackendSchema("database/schema.sql");

    if (checkRootSchema.good()) {
        dbPath = "backend/evhub.db";
        schemaPath = "backend/database/schema.sql";
    } else if (checkBuildSchema.good()) {
        dbPath = "../evhub.db";
        schemaPath = "../database/schema.sql";
    } else if (checkBackendSchema.good()) {
        dbPath = "evhub.db";
        schemaPath = "database/schema.sql";
    }

    if (argc > 1 && argv[1] != nullptr && std::string(argv[1]).length() > 0) {
        dbPath = argv[1];
    }

    int port = 8080;
    if (argc > 2 && argv[2] != nullptr) {
        try {
            port = std::stoi(argv[2]);
        } catch (...) {
            port = 8080;
        }
    }

    Database db(dbPath);
    if (!db.open()) {
        std::cerr << "[Warning] Could not open " << dbPath << ". Running in fallback mode.\n";
    } else {
        if (!schemaPath.empty() && !db.initSchema(schemaPath)) {
            std::cerr << "[Warning] Could not load schema from: " << schemaPath << "\n";
        }
    }

    // 2. Initialize Member Services
    AuthService authService(&db);
    int migratedPasswords = authService.migratePlaintextPasswords();
    if (migratedPasswords > 0) {
        std::cout << "[Security] Migrated " << migratedPasswords << " legacy user password(s) to secure bcrypt hashes.\n";
    }

    EVService evService(&db);
    StationService stationService(&db);
    BatteryService batteryService(&db);
    ChargingQueue chargingQueue;
    SwapQueue swapQueue;
    TransactionStack txnStack;
    TransactionService txnService(&db);

    // Rehydrate FIFO Charging Queue deterministically from SQLite (waiting EVs ordered by last_activity)
    int rehydratedEVs = stationService.rehydrateChargingQueue(chargingQueue);
    std::cout << "[ChargingQueue] Rehydrated " << rehydratedEVs << " waiting EVs from SQLite into FIFO queue.\n";

    // Rehydrate FIFO Swap Queue deterministically from SQLite (waiting swap EVs ordered by last_activity)
    int rehydratedSwaps = batteryService.rehydrateSwapQueue(swapQueue);
    std::cout << "[SwapQueue] Rehydrated " << rehydratedSwaps << " waiting swap EVs from SQLite into FIFO queue.\n";

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
    registerChargingRoutes(app, stationService, chargingQueue, &txnStack);
    registerBatteryRoutes(app, batteryService, &swapQueue, &txnStack);
    registerSwapRoutes(app, batteryService, swapQueue, &txnStack);
    registerDSARoutes(app, chargingQueue, swapQueue, txnStack);
    registerTransactionRoutes(app, txnService);
    registerAnalyticsRoutes(app, txnService);

    std::cout << "\n[Server Ready] Listening on http://localhost:" << port << "\n";
    std::cout << "Press Ctrl+C to stop the server.\n\n";

    // Run on configured port (standard 8080 for local backend)
    app.port(port).run();

    return 0;
}
