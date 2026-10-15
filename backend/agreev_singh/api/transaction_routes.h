#ifndef TRANSACTION_ROUTES_H
#define TRANSACTION_ROUTES_H

#include <crow.h>
#include <crow/middlewares/cors.h>
#include "../services/transaction_service.h"

// Registers Transaction REST API routes
// Created by: Agreev Singh (Transactions + Reports + Analytics Owner, ID: 2518161)
void registerTransactionRoutes(crow::App<crow::CORSHandler>& app, TransactionService& txnService);

#endif
