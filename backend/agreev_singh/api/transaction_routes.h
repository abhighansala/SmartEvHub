#ifndef TRANSACTION_ROUTES_H
#define TRANSACTION_ROUTES_H

#include <crow.h>
#include <crow/middlewares/cors.h>
#include "../services/transaction_service.h"

// ============================================================================
// Smart EV Hub - Transaction REST API Endpoints
// Module Owner: Agreev Singh (Transactions + Reports + Analytics, ID: 2518161)
// ----------------------------------------------------------------------------
// Exposes:
// 1. GET  /api/transactions          - List all transactions (supports ?type= and ?evId=)
// 2. GET  /api/transactions/<id>     - Retrieve transaction details by unique ID
// 3. POST /api/transactions          - Record a completed transaction
// ============================================================================

void registerTransactionRoutes(crow::App<crow::CORSHandler>& app, TransactionService& txnService);

#endif // TRANSACTION_ROUTES_H
