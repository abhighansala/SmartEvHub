#ifndef BATTERY_ROUTES_H
#define BATTERY_ROUTES_H

#include <crow.h>
#include <crow/middlewares/cors.h>
#include "../services/battery_service.h"

// Registers Battery Inventory REST API routes
// Created by: Amit Singh Kaintura (Battery & Swap Module Owner, ID: 2518188)
void registerBatteryRoutes(crow::App<crow::CORSHandler>& app, BatteryService& batteryService);

#endif
