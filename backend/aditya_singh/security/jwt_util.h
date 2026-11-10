#ifndef JWT_UTIL_H
#define JWT_UTIL_H

#include <string>
#include <cstdint>
#include <crow.h>

namespace security {

struct JwtClaims {
    int userId = 0;
    std::string email;
    std::string name;
    std::string role; // "admin", "operator", "user"
    int64_t iat = 0;
    int64_t exp = 0;
};

class JwtUtil {
public:
    static constexpr size_t MIN_SECRET_LENGTH = 32;

    // Validates a secret string; returns true if length >= MIN_SECRET_LENGTH
    static bool validateSecret(const std::string& secret, std::string& outError);

    // Validates that JWT_SECRET environment variable is set and >= MIN_SECRET_LENGTH
    static bool validateStartupSecret(std::string& outError);

    // Retrieves JWT secret from environment (JWT_SECRET).
    // Throws std::runtime_error if missing or length < MIN_SECRET_LENGTH (No fallback).
    static std::string getSecret();

    // Constant-time string comparison to defend against timing attacks
    static bool constantTimeCompare(const std::string& a, const std::string& b);

    // Generates a signed HS256 JWT token with expiration (default 24 hours)
    static std::string generateToken(const JwtClaims& claims, int expirySeconds = 86400, const std::string& secret = "");

    // Verifies signature (constant-time), header (HS256 only), claims (exp, sub, role), and extracts claims.
    static bool verifyToken(const std::string& token, JwtClaims& outClaims, std::string& outError, const std::string& secret = "");

    // Extracts token string from "Bearer <token>" header format
    static bool extractBearerToken(const std::string& authHeader, std::string& outToken);

    // Authentication & Role-Based Authorization Guard for Crow routes
    // Returns true if request is authorized; otherwise sets res with 401/403 JSON error and returns false
    static bool requireAuth(const crow::request& req,
                            crow::response& res,
                            JwtClaims& outClaims,
                            const std::string& requiredRole = "");
};

} // namespace security

#endif // JWT_UTIL_H
