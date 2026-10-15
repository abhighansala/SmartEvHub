#include "battery_routes.h"

void registerBatteryRoutes(crow::App<crow::CORSHandler>& app, BatteryService& batteryService) {

    // GET /api/batteries - List all replacement batteries
    CROW_ROUTE(app, "/api/batteries").methods(crow::HTTPMethod::GET)
    ([&batteryService](){
        Battery list[MAX_BATTERIES];
        int count = batteryService.getAllBatteries(list, MAX_BATTERIES);

        crow::json::wvalue res;
        for (int i = 0; i < count; i++) {
            res[i]["id"] = list[i].getId();
            res[i]["capacity"] = list[i].getCapacity();
            res[i]["currentCharge"] = list[i].getChargeLevel();
            res[i]["status"] = list[i].getStatus();
            res[i]["health"] = list[i].getHealth();
            res[i]["cycleCount"] = list[i].getCycleCount();
            res[i]["currentVehicle"] = list[i].getCurrentVehicle();
        }

        return crow::response(200, res);
    });

    // GET /api/batteries/<string> - Single battery
    CROW_ROUTE(app, "/api/batteries/<string>").methods(crow::HTTPMethod::GET)
    ([&batteryService](std::string id){
        Battery battery;
        if (batteryService.getBatteryById(id, battery)) {
            crow::json::wvalue res;
            res["id"] = battery.getId();
            res["capacity"] = battery.getCapacity();
            res["currentCharge"] = battery.getChargeLevel();
            res["status"] = battery.getStatus();
            res["health"] = battery.getHealth();
            res["cycleCount"] = battery.getCycleCount();
            res["currentVehicle"] = battery.getCurrentVehicle();
            return crow::response(200, res);
        } else {
            crow::json::wvalue res;
            res["success"] = false;
            res["message"] = "Battery not found with ID: " + id;
            return crow::response(404, res);
        }
    });
}
