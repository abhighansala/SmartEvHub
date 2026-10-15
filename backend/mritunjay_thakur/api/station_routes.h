#ifndef STATION_ROUTES_H
#define STATION_ROUTES_H

#include <crow.h>
#include <crow/middlewares/cors.h>
#include "../services/station_service.h"

// Registers Charging Station REST API routes
// Created by: Mritunjay Thakur (Charging Stations Module Owner, ID: 2519425)
void registerStationRoutes(crow::App<crow::CORSHandler>& app, StationService& stationService);

#endif
