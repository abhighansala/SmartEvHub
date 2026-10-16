#include "analytics_routes.h"

// ============================================================================
// Implementation of Reports & Analytics REST API Routes
// Module Owner: Agreev Singh (Transactions + Reports + Analytics, ID: 2518161)
// Uses Crow C++ Microframework to exchange JSON data with React frontend
// ============================================================================

void registerAnalyticsRoutes(crow::App<crow::CORSHandler>& app, TransactionService& txnService) {

    // ------------------------------------------------------------------------
    // 1. GET /api/reports
    // High-level operational KPI summaries matching the React Reports.tsx cards
    // ------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/reports").methods(crow::HTTPMethod::GET)
    ([&txnService](){
        crow::json::wvalue res;

        // Dynamic KPI calculations from TransactionService
        res["totalRevenue"]          = txnService.getTotalRevenue();
        res["totalEnergyDelivered"]   = txnService.getTotalEnergy();
        res["totalSessions"]         = txnService.getTotalSessions();
        res["chargingSessions"]      = txnService.getChargingSessionCount();
        res["batterySwaps"]          = txnService.getBatterySwapCount();
        res["averageDuration"]       = txnService.getAverageDurationMinutes();
        res["averageWaitingTime"]    = txnService.getAverageWaitingMinutes();
        res["stationUtilization"]   = txnService.getStationUtilization();
        res["activeBays"]            = 3;
        res["totalBays"]             = 4;

        // Check if there's a highest revenue transaction (OOP Lab 4 Best index)
        Transaction topTxn;
        if (txnService.getHighestRevenueTransaction(topTxn)) {
            res["topTransaction"]["id"]     = topTxn.getId();
            res["topTransaction"]["amount"] = topTxn.getAmount();
            res["topTransaction"]["evId"]   = topTxn.getVehicleId();
        }

        return crow::response(200, res);
    });

    // ------------------------------------------------------------------------
    // 2. GET /api/analytics
    // Formatted chart datasets for Recharts visualizations
    // ------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/analytics").methods(crow::HTTPMethod::GET)
    ([&txnService](){
        crow::json::wvalue res;

        // Weekly Trends (matches Reports.tsx weeklyData: { day, sessions, swaps })
        const char* days[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        int weekSessions[7] = {12, 15, 10, 18, 22, 25, 20};
        int weekSwaps[7]    = {4, 6, 3, 7, 5, 9, 8};

        for (int i = 0; i < 7; i++) {
            res["weeklyData"][i]["day"]      = days[i];
            res["weeklyData"][i]["sessions"] = weekSessions[i];
            res["weeklyData"][i]["swaps"]    = weekSwaps[i];
        }

        // Station Utilization (matches Reports.tsx stationUtilization: { station, utilization })
        const char* stations[6] = {"S01", "S02", "S03", "S04", "S05", "S06"};
        int utilPercentages[6]  = {65, 85, 72, 90, 78, 45};

        for (int i = 0; i < 6; i++) {
            res["stationUtilization"][i]["station"]     = stations[i];
            res["stationUtilization"][i]["utilization"] = utilPercentages[i];
        }

        // Hourly Peak Load Distribution
        const char* hours[6] = {"08:00", "10:00", "12:00", "14:00", "16:00", "18:00"};
        int hourSessions[6]  = {3, 7, 12, 10, 15, 8};
        double hourEnergy[6] = {65.0, 150.0, 240.0, 210.0, 310.0, 180.0};

        for (int i = 0; i < 6; i++) {
            res["peakHours"][i]["hour"]     = hours[i];
            res["peakHours"][i]["sessions"] = hourSessions[i];
            res["peakHours"][i]["energy"]   = hourEnergy[i];
        }

        // Revenue Breakdown by Operation Type
        Transaction chargingList[MAX_TRANSACTIONS];
        Transaction swapList[MAX_TRANSACTIONS];
        int chargingCount = txnService.getTransactionsByType("charging", chargingList, MAX_TRANSACTIONS);
        int swapCount     = txnService.getTransactionsByType("battery-swap", swapList, MAX_TRANSACTIONS);

        double chargingRevenue = 0.0;
        for (int i = 0; i < chargingCount; i++) {
            chargingRevenue += chargingList[i].getAmount();
        }

        double swapRevenue = 0.0;
        for (int i = 0; i < swapCount; i++) {
            swapRevenue += swapList[i].getAmount();
        }

        res["revenueSummary"]["chargingRevenue"] = chargingRevenue;
        res["revenueSummary"]["swapRevenue"]     = swapRevenue;
        res["revenueSummary"]["totalRevenue"]    = txnService.getTotalRevenue();

        return crow::response(200, res);
    });
}
