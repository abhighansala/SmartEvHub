#ifndef ANALYTICS_ROUTES_H
#define ANALYTICS_ROUTES_H

#include <crow.h>
#include <crow/middlewares/cors.h>
#include "../services/transaction_service.h"

// Registers Operational Reports and Recharts Analytics routes
// Created by: Agreev Singh (Transactions + Reports + Analytics Owner, ID: 2518161)
void registerAnalyticsRoutes(crow::App<crow::CORSHandler>& app, TransactionService& txnService);

#endif
