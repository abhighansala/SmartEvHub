#ifndef ANALYTICS_ROUTES_H
#define ANALYTICS_ROUTES_H

#include <crow.h>
#include <crow/middlewares/cors.h>
#include "../services/transaction_service.h"

// ============================================================================
// Smart EV Hub - Reports & Analytics REST API Endpoints
// Module Owner: Agreev Singh (Transactions + Reports + Analytics, ID: 2518161)
// ----------------------------------------------------------------------------
// Exposes:
// 1. GET /api/reports    - High-level KPI operational summaries for Reports cards
// 2. GET /api/analytics  - Recharts-formatted datasets for weekly trends & stations
// ============================================================================

void registerAnalyticsRoutes(crow::App<crow::CORSHandler>& app, TransactionService& txnService);

#endif // ANALYTICS_ROUTES_H
