#include "auth_routes.h"
#include "../security/jwt_util.h"

void registerAuthRoutes(crow::App<crow::CORSHandler>& app, AuthService& authService) {

    // POST /api/login - Authenticate with email or username & password with bcrypt + JWT issue
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
            // Generate standard JWT token with user claims
            try {
                security::JwtClaims claims;
                claims.userId = user.getId();
                claims.email = user.getEmail();
                claims.name = user.getName();
                claims.role = user.getRole();
                std::string token = security::JwtUtil::generateToken(claims);

                res["success"] = true;
                res["message"] = "Login successful";
                res["token"] = token;

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
            } catch (const std::exception& e) {
                res["success"] = false;
                res["message"] = "Authentication service token generation error";
                return crow::response(500, res);
            }
        } else {
            // Generic safe error response to protect against user enumeration
            res["success"] = false;
            res["message"] = "Invalid email or password";
            return crow::response(401, res);
        }
    });

    // POST /api/register - Register a new user with input validation, bcrypt hashing, and auto-login JWT
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

        // Requirement 6: Public registration must always create a 'user' role.
        // Never trust any 'role' supplied in registration JSON.
        const std::string role = "user";

        // Input validation
        if (name.length() < 2) {
            res["success"] = false;
            res["message"] = "Name must be at least 2 characters long";
            return crow::response(400, res);
        }
        if (email.find('@') == std::string::npos || email.find('.') == std::string::npos) {
            res["success"] = false;
            res["message"] = "Invalid email address format";
            return crow::response(400, res);
        }
        if (password.length() < 6) {
            res["success"] = false;
            res["message"] = "Password must be at least 6 characters long";
            return crow::response(400, res);
        }

        if (authService.userExists(email)) {
            res["success"] = false;
            res["message"] = "A user with this email already exists";
            return crow::response(409, res);
        }

        if (authService.registerUser(name, email, password, role)) {
            User registeredUser;
            // Requirement 7: Ensure registration only issues a JWT after successful
            // authentication of the newly registered account.
            if (!authService.login(email, password, registeredUser)) {
                res["success"] = false;
                res["message"] = "Account registered, but initial authentication failed. Please sign in.";
                return crow::response(500, res);
            }

            try {
                security::JwtClaims claims;
                claims.userId = registeredUser.getId();
                claims.email = registeredUser.getEmail();
                claims.name = registeredUser.getName();
                claims.role = registeredUser.getRole();
                std::string token = security::JwtUtil::generateToken(claims);

                res["success"] = true;
                res["message"] = "User registered successfully";
                res["token"] = token;

                crow::json::wvalue userJson;
                userJson["id"] = registeredUser.getId();
                userJson["name"] = registeredUser.getName();
                userJson["email"] = registeredUser.getEmail();
                userJson["role"] = registeredUser.getRole();
                res["user"] = std::move(userJson);

                return crow::response(201, res);
            } catch (const std::exception& e) {
                res["success"] = false;
                res["message"] = "Account registered, but token generation failed.";
                return crow::response(500, res);
            }
        } else {
            res["success"] = false;
            res["message"] = "Failed to register user. Please verify your details.";
            return crow::response(500, res);
        }
    });

    // GET /api/me - Retrieve current authenticated user profile via JWT Bearer token
    CROW_ROUTE(app, "/api/me").methods(crow::HTTPMethod::GET)
    ([](const crow::request& req){
        crow::response res;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, res, claims)) {
            return res;
        }

        crow::json::wvalue body;
        body["success"] = true;
        body["user"]["id"] = claims.userId;
        body["user"]["email"] = claims.email;
        body["user"]["name"] = claims.name;
        body["user"]["role"] = claims.role;
        return crow::response(200, body);
    });

    // POST /api/logout - End session
    CROW_ROUTE(app, "/api/logout").methods(crow::HTTPMethod::POST)
    ([](){
        crow::json::wvalue res;
        res["success"] = true;
        res["message"] = "Logged out successfully";
        return crow::response(200, res);
    });

    // GET /api/users - List users for verification (Protected: Operator or Admin)
    CROW_ROUTE(app, "/api/users").methods(crow::HTTPMethod::GET)
    ([&authService](const crow::request& req){
        crow::response authRes;
        security::JwtClaims claims;
        if (!security::JwtUtil::requireAuth(req, authRes, claims, "operator")) {
            return authRes;
        }

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
