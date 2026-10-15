#ifndef CHARGING_ROUTES_H
#define CHARGING_ROUTES_H

#include <crow.h>
#include <crow/middlewares/cors.h>
#include "../services/station_service.h"

// Registers Charging workflow routes
// Created by: Mritunjay Thakur (Charging Stations Module Owner, ID: 2519425)
void registerChargingRoutes(crow::App<crow::CORSHandler>& app, StationService& stationService);

#endif
