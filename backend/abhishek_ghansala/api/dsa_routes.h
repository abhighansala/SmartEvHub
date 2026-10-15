#ifndef DSA_ROUTES_H
#define DSA_ROUTES_H

#include <crow.h>
#include <crow/middlewares/cors.h>
#include "../dsa/charging_queue.h"
#include "../dsa/swap_queue.h"
#include "../dsa/transaction_stack.h"

// Registers Data Structures & Algorithms REST API routes
// Exposes the manual Queue and Stack state to the React frontend
// Created by: Abhishek Ghansala (UI + DSA + Final Integration, ID: 2518132)
void registerDSARoutes(crow::App<crow::CORSHandler>& app,
                       ChargingQueue& chargingQueue,
                       SwapQueue& swapQueue,
                       TransactionStack& txnStack);

#endif
