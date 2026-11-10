#ifndef CHARGING_ROUTES_H
#define CHARGING_ROUTES_H

#include <crow.h>
#include <crow/middlewares/cors.h>
#include "../services/station_service.h"
#include "../../abhishek_ghansala/dsa/charging_queue.h"
#include "../../abhishek_ghansala/dsa/transaction_stack.h"

// Registers Charging workflow routes (Integrated with Abhishek Ghansala's ChargingQueue & TransactionStack)
// Created by: Mritunjay Thakur (Charging Stations Module Owner, ID: 2519425)
void registerChargingRoutes(crow::App<crow::CORSHandler>& app,
                            StationService& stationService,
                            ChargingQueue& chargingQueue,
                            TransactionStack* txnStack = nullptr);

#endif
