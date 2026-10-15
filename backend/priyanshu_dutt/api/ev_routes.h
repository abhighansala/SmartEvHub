#ifndef EV_ROUTES_H
#define EV_ROUTES_H

#include <crow.h>
#include <crow/middlewares/cors.h>
#include "../services/ev_service.h"

// Registers EV Management REST API routes
// Created by: Priyanshu Dutt (EV Management Module Owner, ID: 2519460)
void registerEVRoutes(crow::App<crow::CORSHandler>& app, EVService& evService);

#endif
