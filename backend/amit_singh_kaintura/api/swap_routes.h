#ifndef SWAP_ROUTES_H
#define SWAP_ROUTES_H

#include <crow.h>
#include <crow/middlewares/cors.h>
#include "../services/battery_service.h"

// Registers Battery Swap workflow routes
// Created by: Amit Singh Kaintura (Battery & Swap Module Owner, ID: 2518188)
void registerSwapRoutes(crow::App<crow::CORSHandler>& app, BatteryService& batteryService);

#endif
