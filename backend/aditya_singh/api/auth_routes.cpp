#include "auth_routes.h"

void registerAuthRoutes(crow::App<crow::CORSHandler>& app, AuthService& authService) {

    // POST /api/login - Authenticate with email & password
    CROW_ROUTE(app, "/api/login").methods(crow::HTTPMethod::POST)
    ([&authService](const crow::request& req){
        crow::json::wvalue res;

        auto jsonBody = crow::json::load(req.body);
        if (!jsonBody) {
            res["success"] = false;
            res["message"] = "Invalid JSON body";
            return crow::response(400, res);
        }

        if (!jsonBody.has("email") || !jsonBody.has("password")) {
            res["success"] = false;
            res["message"] = "Email and password are required";
            return crow::response(400, res);
        }

        std::string email = jsonBody["email"].s();
        std::string password = jsonBody["password"].s();

        User user;
        if (authService.login(email, password, user)) {
            res["success"] = true;
            res["message"] = "Login successful";
            
            crow::json::wvalue userJson;
            userJson["id"] = user.getId();
            userJson["name"] = user.getName();
            userJson["email"] = user.getEmail();
            userJson["role"] = user.getRole();
            
            res["user"] = std::move(userJson);
            return crow::response(200, res);
        } else {
            res["success"] = false;
            res["message"] = "Invalid email or password";
            return crow::response(401, res);
        }
    });

    // POST /api/logout - End session
    CROW_ROUTE(app, "/api/logout").methods(crow::HTTPMethod::POST)
    ([](){
        crow::json::wvalue res;
        res["success"] = true;
        res["message"] = "Logged out successfully";
        return crow::response(200, res);
    });
}
