#include "auth_routes.h"

void registerAuthRoutes(crow::App<crow::CORSHandler>& app, AuthService& authService) {

    // POST /api/login - Authenticate with email or username & password
    CROW_ROUTE(app, "/api/login").methods(crow::HTTPMethod::POST)
    ([&authService](const crow::request& req){
        crow::json::wvalue res;

        auto jsonBody = crow::json::load(req.body);
        if (!jsonBody) {
            res["success"] = false;
            res["message"] = "Invalid JSON body";
            return crow::response(400, res);
        }

        std::string identifier = "";
        if (jsonBody.has("email")) {
            identifier = jsonBody["email"].s();
        } else if (jsonBody.has("username")) {
            identifier = jsonBody["username"].s();
        }

        if (identifier.empty() || !jsonBody.has("password")) {
            res["success"] = false;
            res["message"] = "Email/Username and password are required";
            return crow::response(400, res);
        }

        std::string password = jsonBody["password"].s();

        User user;
        if (authService.login(identifier, password, user)) {
            res["success"] = true;
            res["message"] = "Login successful";
            
            crow::json::wvalue userJson;
            userJson["id"] = user.getId();
            userJson["name"] = user.getName();
            userJson["email"] = user.getEmail();
            userJson["role"] = user.getRole();
            if (!user.getCreatedAt().empty()) {
                userJson["createdAt"] = user.getCreatedAt();
            }
            
            res["user"] = std::move(userJson);
            return crow::response(200, res);
        } else {
            res["success"] = false;
            res["message"] = "Invalid credentials";
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

    // POST /api/register - Register a new user
    CROW_ROUTE(app, "/api/register").methods(crow::HTTPMethod::POST)
    ([&authService](const crow::request& req){
        crow::json::wvalue res;

        auto jsonBody = crow::json::load(req.body);
        if (!jsonBody) {
            res["success"] = false;
            res["message"] = "Invalid JSON body";
            return crow::response(400, res);
        }

        if (!jsonBody.has("name") || !jsonBody.has("email") || !jsonBody.has("password")) {
            res["success"] = false;
            res["message"] = "Name, email, and password are required";
            return crow::response(400, res);
        }

        std::string name = jsonBody["name"].s();
        std::string email = jsonBody["email"].s();
        std::string password = jsonBody["password"].s();
        std::string role = jsonBody.has("role") ? jsonBody["role"].s() : "user";

        if (authService.userExists(email)) {
            res["success"] = false;
            res["message"] = "A user with this email already exists";
            return crow::response(409, res);
        }

        if (authService.registerUser(name, email, password, role)) {
            res["success"] = true;
            res["message"] = "User registered successfully";
            return crow::response(201, res);
        } else {
            res["success"] = false;
            res["message"] = "Failed to register user";
            return crow::response(500, res);
        }
    });

    // GET /api/users - List users for verification/admin
    CROW_ROUTE(app, "/api/users").methods(crow::HTTPMethod::GET)
    ([&authService](){
        crow::json::wvalue res;
        User users[50];
        int count = authService.getAllUsers(users, 50);

        crow::json::wvalue::list userList;
        for (int i = 0; i < count; i++) {
            crow::json::wvalue u;
            u["id"] = users[i].getId();
            u["name"] = users[i].getName();
            u["email"] = users[i].getEmail();
            u["role"] = users[i].getRole();
            u["createdAt"] = users[i].getCreatedAt();
            userList.push_back(std::move(u));
        }

        res["success"] = true;
        res["count"] = count;
        res["users"] = std::move(userList);
        return crow::response(200, res);
    });
}
