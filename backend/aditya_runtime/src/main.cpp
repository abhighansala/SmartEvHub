/**
 * @file main.cpp
 * @brief Smart EV Hub - Crow C++ REST API Server
 * Team Lead: Aditya Singh (Database & Backend Foundation)
 * Members: Priyanshu Dutt, Mritunjay Thakur, Amit Singh Kaintura, Abhishek Ghansala, Agreev Singh
 */

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>

#include "models/Types.hpp"
#include "models/ChargingStation.hpp"
#include "models/EV.hpp"
#include "services/ChargingService.hpp"
#include "db/Database.hpp"
#include "models/User.hpp"

// Note: For Windows MinGW/MSVC build, ensure crow_all.h is included
#if __has_include(<crow.h>)
#include <crow.h>
#define HAS_CROW 1
#elif __has_include("crow_all.h")
#include "crow_all.h"
#define HAS_CROW 1
#else
#define HAS_CROW 0
#endif

using namespace evhub;


struct CorsMiddleware {
    struct context {};

    void before_handle(
        crow::request& req,
        crow::response& res,
        context& ctx
    ) {
        res.add_header("Access-Control-Allow-Origin", "http://localhost:3000");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");

        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 204;
            res.end();
        }
    }

    void after_handle(
        crow::request& req,
        crow::response& res,
        context& ctx
    ) {
        res.add_header("Access-Control-Allow-Origin", "http://localhost:3000");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    }
};


int main() {
    std::cout << "====================================================\n";
    std::cout << "  SMART EV HUB - C++ OOP & STATION MANAGEMENT SYSTEM\n";
    std::cout << "  B.Tech CSE Project-Based Learning\n";
    std::cout << "  Lead: Aditya Singh (Database & Backend)\n";
    std::cout << "====================================================\n";

    // 1. Initialize SQLite Database
    Database& db = Database::getInstance("evhub.db");
    
    // Seed sample users only when they do not already exist.
    User existingUser;
    int sampleUserId = 0;

    if (!db.getUserByEmail("driver1@evhub.local", existingUser)) {
        User user1(
            1, "Sample Driver 1", "driver1@evhub.local",
            Database::hashPassword("DemoPass123!"), "DRIVER"
        );

        if (!db.createUser(user1, sampleUserId)) {
            std::cerr << "[INIT] Failed to create sample user 1.\n";
            return 1;
        }
    }

    if (!db.getUserByEmail("driver2@evhub.local", existingUser)) {
        User user2(
            2, "Sample Driver 2", "driver2@evhub.local",
            Database::hashPassword("DemoPass123!"), "DRIVER"
        );

        if (!db.createUser(user2, sampleUserId)) {
            std::cerr << "[INIT] Failed to create sample user 2.\n";
            return 1;
        }
    }

    std::cout << "[INIT] Database initialized & schema verified.\n";

    // 2. Setup Charging Service & In-Memory Stations
    ChargingService chargingService;
    std::vector<ChargingStation> stations = {
        ChargingStation(1, "HyperCharger Hub Alpha", PortType::CCS2, 150.0, StationStatus::AVAILABLE),
        ChargingStation(2, "Urban DC Express Beta", PortType::CCS2, 60.0, StationStatus::AVAILABLE),
        ChargingStation(3, "AC EcoCharge Gamma", PortType::TYPE_2, 22.0, StationStatus::AVAILABLE),
        ChargingStation(4, "Delta CHAdeMO QuickBay", PortType::CHADEMO, 50.0, StationStatus::AVAILABLE)
    };

    
    // Seed sample stations only when the database has no stations.
    std::vector<ChargingStation> storedStations = db.getAllStations();

  
    if (storedStations.empty()) {
        for (const auto& station : stations) {
            int insertedId = 0;

            if (!db.createStation(station, insertedId)) {
                std::cerr << "[INIT] Failed to seed station: "
                          << station.getName() << '\n';
                return 1;
            }
        }

        storedStations = db.getAllStations();
    } else {
        std::cout << "[INIT] Existing station records found: "
                  << storedStations.size() << '\n';
    }

    // Use persisted station records as the runtime source of truth.
    stations = std::move(storedStations);



    // 3. Register Sample EVs
    std::vector<EV> evs = {
        EV(1, 1, "VIN10924X78901", "Tata", "Nexon EV Max", "DL-01-EV-4412", 40.5, 34.0, PortType::CCS2),
        EV(2, 1, "VIN98765A12345", "MG", "ZS EV Pro", "HR-26-EV-9901", 50.3, 18.0, PortType::CCS2),
        EV(3, 2, "VIN45678B54321", "Hyundai", "Ioniq 5 AWD", "UP-16-EV-1002", 72.6, 62.0, PortType::CCS2),
        EV(4, 2, "VIN33221C99887", "Mahindra", "XUV400 EL", "DL-04-EV-7721", 39.4, 45.0, PortType::TYPE_2)
    };
   
    // Seed sample EVs only when the database has no EV records.
    std::vector<EV> storedEVs = db.getAllEVs();

    if (storedEVs.empty()) {
        for (const auto& ev : evs) {
            int insertedId = 0;

            if (!db.createEV(ev, insertedId)) {
                std::cerr << "[INIT] Failed to seed EV: "
                          << ev.getVin() << '\n';
                return 1;
            }
        }

        storedEVs = db.getAllEVs();
    } else {
        std::cout << "[INIT] Existing EV records found: "
                  << storedEVs.size() << '\n';
    }

    // Use persisted EV records as the runtime source of truth.
    evs = std::move(storedEVs);

    // Recover station assignment from active session records if a previous run
    // stopped between saving the session and saving the station row.
    const auto activeSessions = db.getActiveSessions();
    for (const auto& activeSession : activeSessions) {
        auto stationIt = std::find_if(stations.begin(), stations.end(),
            [&activeSession](const ChargingStation& station) {
                return station.getId() == activeSession.getStationId();
            });
        if (stationIt == stations.end()) {
            std::cerr << "[RECOVERY] Active session " << activeSession.getId()
                      << " refers to missing station " << activeSession.getStationId() << ".\n";
            continue;
        }
        if (stationIt->getStatus() == StationStatus::CHARGING &&
            stationIt->getCurrentSessionId() != 0 &&
            stationIt->getCurrentSessionId() != activeSession.getId()) {
            std::cerr << "[RECOVERY] Conflicting active sessions for station "
                      << stationIt->getId() << "; preserving stored station state.\n";
            continue;
        }
        if (stationIt->getStatus() != StationStatus::CHARGING ||
            stationIt->getCurrentSessionId() != activeSession.getId() ||
            stationIt->getAssignedEvId() != activeSession.getEvId()) {
            if (!db.beginTransaction()) {
                std::cerr << "[RECOVERY] Could not begin transaction for station "
                          << stationIt->getId() << ".\n";
                continue;
            }
            bool repaired = db.updateStationStatus(
                stationIt->getId(), StationStatus::CHARGING,
                activeSession.getId(), activeSession.getEvId(),
                stationIt->getTotalEnergyDispensedKwh());
            if (!repaired || !db.commitTransaction()) {
                db.rollbackTransaction();
                std::cerr << "[RECOVERY] Could not persist recovery for station "
                          << stationIt->getId() << ".\n";
                continue;
            }
            stationIt->assignSession(activeSession.getId(), activeSession.getEvId());
            std::cout << "[RECOVERY] Restored active session " << activeSession.getId()
                      << " on station " << stationIt->getId() << ".\n";
        }
    }

    // Restore persisted charging requests in FIFO order.
    const auto persistedRequests = db.getQueuedRequests();
    int nextRequestId = 101;

    for (const auto& saved : persistedRequests) {
        QueuedRequest request{
            saved.requestId,
            saved.userId,
            saved.evId,
            stringToPortType(saved.requiredPort),
            saved.requestTime
        };

        chargingService.getQueue().enqueue(request);

        if (saved.requestId >= nextRequestId) {
            nextRequestId = saved.requestId + 1;
        }
    }

    chargingService.setNextRequestId(nextRequestId);

    chargingService.setNextSessionId(db.getMaxSessionId() + 1);
    chargingService.setNextTxId(db.getMaxTransactionId() + 1);

    // Restore transaction history into the in-memory LIFO stack. The DB query
    // returns newest-first, so push oldest-first to leave the newest at the top.
    const auto persistedTransactions = db.getAllTransactions();
    for (auto it = persistedTransactions.rbegin(); it != persistedTransactions.rend(); ++it) {
        chargingService.getTransactionStack().push(*it);
    }

    std::cout << "[INIT] Restored "
              << persistedRequests.size()
              << " queued charging requests.\n";




    std::cout << "[INIT] Initialized " << stations.size() << " charging stations.\n";
    std::cout << "[INIT] Initialized " << evs.size() << " registered electric vehicles.\n";

#if HAS_CROW
    crow::App<CorsMiddleware> app;

    // Health check
    CROW_ROUTE(app, "/api/health")([]() {
        crow::json::wvalue res;
        res["status"] = "HEALTHY";
        res["backend"] = "C++ Crow Framework (C++17)";
        res["database"] = "SQLite 3 (WAL Mode)";
        return res;
    });

    
    // Station list, including assigned EV details
    CROW_ROUTE(app, "/api/stations")([&stations, &evs]() {
        crow::json::wvalue res;

        for (size_t i = 0; i < stations.size(); ++i) {
            res[i]["id"] = stations[i].getId();
            res[i]["name"] = stations[i].getName();
            res[i]["portType"] =
                portTypeToString(stations[i].getPortType());
            res[i]["maxPowerKw"] = stations[i].getMaxPowerKw();
            res[i]["status"] =
                stationStatusToString(stations[i].getStatus());
            res[i]["currentSessionId"] =
                stations[i].getCurrentSessionId();
            res[i]["assignedEvId"] = stations[i].getAssignedEvId();
            res[i]["totalEnergyDispensedKwh"] =
                stations[i].getTotalEnergyDispensedKwh();

            int assignedEvId = stations[i].getAssignedEvId();

            // Include full details when a station has an assigned EV.
            for (const auto& ev : evs) {
                if (ev.getId() == assignedEvId && assignedEvId != 0) {
                    res[i]["assignedEV"]["id"] = ev.getId();
                    res[i]["assignedEV"]["userId"] = ev.getUserId();
                    res[i]["assignedEV"]["make"] = ev.getMake();
                    res[i]["assignedEV"]["model"] = ev.getModel();
                    res[i]["assignedEV"]["vin"] = ev.getVin();
                    res[i]["assignedEV"]["licensePlate"] =
                        ev.getLicensePlate();
                    res[i]["assignedEV"]["batteryCapacityKwh"] =
                        ev.getBatteryCapacityKwh();
                    res[i]["assignedEV"]["currentSocPercent"] =
                        ev.getCurrentSocPercent();
                    res[i]["assignedEV"]["supportedPort"] =
                        portTypeToString(ev.getSupportedPort());

                    break;
                }
            }
        }

        return res;
    });


    // Request Charging endpoint (assigns station or enqueues in manual C++ Queue)
    CROW_ROUTE(app, "/api/charging/request").methods("POST"_method)([&chargingService, &stations, &db, &evs](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("userId") || !body.has("evId") || !body.has("portType")) {
            return crow::response(400, "{\"error\": \"Missing required fields: userId, evId, portType\"}");
        }

        int userId = body["userId"].i();
        int evId = body["evId"].i();
        PortType port = stringToPortType(body["portType"].s());

        
        const EV* requestedEV = nullptr;

        for (const auto& ev : evs) {
            if (ev.getId() == evId) {
                requestedEV = &ev;
                break;
            }
        }

        if (requestedEV == nullptr) {
            return crow::response(
                404,
                "{\"error\":\"EV not found in the database\"}"
            );
        }

        if (requestedEV->getUserId() != userId) {
            return crow::response(
                403,
                "{\"error\":\"This EV does not belong to the specified user\"}"
            );
        }



        auto result = chargingService.requestCharging(userId, evId, port, stations, db, evs);
        if (!result.immediateStart && result.queuePosition == 0) {
            crow::json::wvalue error;
            error["error"] = result.message;
            return crow::response(500, error);
        }

        crow::json::wvalue res;
        res["immediateStart"] = result.immediateStart;
        res["sessionId"] = result.sessionId;
        res["stationId"] = result.stationId;
        res["queuePosition"] = result.queuePosition;
        res["requestId"] = result.requestId;
        res["message"] = result.message;
        return crow::response(200, res);
    });

    // Complete Charging endpoint (releases station, pushes to LIFO Stack, auto-assigns next queued EV)
    CROW_ROUTE(app, "/api/charging/complete").methods("POST"_method)([&chargingService, &stations, &db, &evs](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("stationId") || !body.has("energyDeliveredKwh")) {
            return crow::response(400, "{\"error\": \"Missing required fields: stationId, energyDeliveredKwh\"}");
        }

        int stationId = body["stationId"].i();
        double energyKwh = body["energyDeliveredKwh"].d();

        auto result = chargingService.completeSession(stationId, energyKwh, stations, db, evs);
        if (!result.success) {
            return crow::response(400, "{\"error\": \"" + result.message + "\"}");
        }

        crow::json::wvalue res;
        res["success"] = true;
        res["transactionId"] = result.transaction.getId();
        res["amount"] = result.transaction.getAmount();
        res["nextRequestAssigned"] = result.nextRequestAssigned;
        res["nextEvIdAssigned"] = result.nextEvIdAssigned;
        res["message"] = result.message;
        return crow::response(200, res);
    });

    std::cout << "[SERVER] Starting Crow C++ REST server on port 18080...\n";
    app.port(18080).multithreaded().run();
#else
    std::cout << "[INFO] Compiled in Standalone C++ mode. Crow header not linked in host environment.\n";
    std::cout << "[INFO] Ready for Windows MinGW/MSVC or Linux build via CMake.\n";
#endif

    return 0;
}
