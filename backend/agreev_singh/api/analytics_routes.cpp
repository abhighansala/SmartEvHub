#include "analytics_routes.h"

void registerAnalyticsRoutes(crow::App<crow::CORSHandler>& app, TransactionService& txnService) {

    // GET /api/reports - High-level operational summaries
    CROW_ROUTE(app, "/api/reports").methods(crow::HTTPMethod::GET)
    ([&txnService](){
        crow::json::wvalue res;
        res["totalRevenue"] = txnService.getTotalRevenue();
        res["totalEnergyDelivered"] = txnService.getTotalEnergy();
        res["totalSessions"] = txnService.getTotalSessions();
        res["averageWaitingTime"] = txnService.getAverageWaitingMinutes();
        res["stationUtilization"] = 84.5; // percentage
        res["activeBays"] = 3;
        res["totalBays"] = 4;
        return crow::response(200, res);
    });

    // GET /api/analytics - Formatted datasets for Recharts
    CROW_ROUTE(app, "/api/analytics").methods(crow::HTTPMethod::GET)
    ([](){
        crow::json::wvalue res;

        // Hourly peak data
        const char* hours[6] = {"08:00", "10:00", "12:00", "14:00", "16:00", "18:00"};
        int sessions[6] = {3, 7, 12, 10, 15, 8};
        double energy[6] = {65.0, 150.0, 240.0, 210.0, 310.0, 180.0};

        for (int i = 0; i < 6; i++) {
            res["peakHours"][i]["hour"] = hours[i];
            res["peakHours"][i]["sessions"] = sessions[i];
            res["peakHours"][i]["energy"] = energy[i];
        }

        // Station utilization
        const char* stations[4] = {"CS-1", "CS-2", "CS-3", "CS-4"};
        int util[4] = {88, 75, 92, 40};
        for (int i = 0; i < 4; i++) {
            res["stationUtilization"][i]["station"] = stations[i];
            res["stationUtilization"][i]["utilization"] = util[i];
        }

        return crow::response(200, res);
    });
}
