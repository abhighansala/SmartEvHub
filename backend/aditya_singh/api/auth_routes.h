#ifndef AUTH_ROUTES_H
#define AUTH_ROUTES_H

#include <crow.h>
#include <crow/middlewares/cors.h>
#include "../services/auth_service.h"

// Registers Authentication routes to Crow app
// Created by: Aditya Singh (Lead - Database & Backend Foundation)
void registerAuthRoutes(crow::App<crow::CORSHandler>& app, AuthService& authService);

#endif
