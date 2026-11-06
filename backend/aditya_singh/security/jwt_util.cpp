#include "jwt_util.h"
#include "sha256.h"
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace security {

bool JwtUtil::validateSecret(const std::string& secret, std::string& outError) {
    if (secret.empty()) {
        outError = "JWT secret is missing (JWT_SECRET environment variable not set)";
        return false;
    }
    if (secret.length() < MIN_SECRET_LENGTH) {
        outError = "JWT secret is insufficiently strong: length " +
                   std::to_string(secret.length()) +
                   " < " + std::to_string(MIN_SECRET_LENGTH) + " characters required";
        return false;
    }
    return true;
}

bool JwtUtil::validateStartupSecret(std::string& outError) {
    const char* envSecret = std::getenv("JWT_SECRET");
    std::string secret = (envSecret != nullptr) ? std::string(envSecret) : "";
    return validateSecret(secret, outError);
}

std::string JwtUtil::getSecret() {
    const char* envSecret = std::getenv("JWT_SECRET");
    if (envSecret == nullptr || std::string(envSecret).empty()) {
        throw std::runtime_error("FATAL SECURITY ERROR: JWT_SECRET environment variable is missing. A cryptographically secure secret (minimum 32 characters) must be configured.");
    }
    std::string secret(envSecret);
    if (secret.length() < MIN_SECRET_LENGTH) {
        throw std::runtime_error("FATAL SECURITY ERROR: JWT_SECRET is insufficiently strong. Minimum length is 32 characters.");
    }
    return secret;
}

bool JwtUtil::constantTimeCompare(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) {
        return false;
    }
    unsigned char result = 0;
    for (size_t i = 0; i < a.length(); ++i) {
        result |= static_cast<unsigned char>(a[i] ^ b[i]);
    }
    return result == 0;
}

std::string JwtUtil::generateToken(const JwtClaims& claims, int expirySeconds, const std::string& secret) {
    std::string key = secret;
    if (key.empty()) {
        key = getSecret();
    } else {
        std::string secErr;
        if (!validateSecret(key, secErr)) {
            throw std::runtime_error("Invalid secret for token generation: " + secErr);
        }
    }

    if (claims.userId <= 0) {
        throw std::invalid_argument("Cannot generate JWT: 'sub' (userId) must be a positive integer");
    }

    // 1. Header: {"alg":"HS256","typ":"JWT"}
    std::string headerJson = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    std::string encHeader = base64UrlEncode(headerJson);

    // 2. Payload
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    int64_t iat = (claims.iat > 0) ? claims.iat : now;
    int64_t exp = (claims.exp > 0) ? claims.exp : (now + expirySeconds);

    crow::json::wvalue payloadW;
    payloadW["sub"] = claims.userId;
    payloadW["email"] = claims.email;
    payloadW["name"] = claims.name;
    payloadW["role"] = claims.role.empty() ? "user" : claims.role;
    payloadW["iat"] = iat;
    payloadW["exp"] = exp;

    std::string payloadJson = payloadW.dump();
    std::string encPayload = base64UrlEncode(payloadJson);

    // 3. Signature: HMAC-SHA256(encHeader + "." + encPayload, secret)
    std::string dataToSign = encHeader + "." + encPayload;
    std::string signatureRaw = hmacSha256Raw(key, dataToSign);
    std::string encSignature = base64UrlEncode(signatureRaw);

    return dataToSign + "." + encSignature;
}

bool JwtUtil::verifyToken(const std::string& token, JwtClaims& outClaims, std::string& outError, const std::string& secret) {
    if (token.empty()) {
        outError = "Malformed token: token is empty";
        return false;
    }

    // 1. Token format validation: exactly 2 dots separating 3 non-empty segments
    int dotCount = 0;
    size_t firstDot = std::string::npos;
    size_t secondDot = std::string::npos;
    for (size_t i = 0; i < token.length(); ++i) {
        if (token[i] == '.') {
            dotCount++;
            if (dotCount == 1) firstDot = i;
            else if (dotCount == 2) secondDot = i;
        }
    }
    if (dotCount != 2) {
        outError = "Malformed token format: expected exactly 3 segments separated by dots";
        return false;
    }

    std::string encHeader = token.substr(0, firstDot);
    std::string encPayload = token.substr(firstDot + 1, secondDot - firstDot - 1);
    std::string encSignature = token.substr(secondDot + 1);

    if (encHeader.empty() || encPayload.empty() || encSignature.empty()) {
        outError = "Malformed token format: empty segment";
        return false;
    }

    if (!isValidBase64Url(encHeader) || !isValidBase64Url(encPayload) || !isValidBase64Url(encSignature)) {
        outError = "Malformed token format: invalid base64url characters";
        return false;
    }

    // 2. Decode and Validate Header (Requirement 2)
    std::string headerJson = base64UrlDecode(encHeader);
    if (headerJson.empty()) {
        outError = "Malformed token header: invalid base64 encoding";
        return false;
    }
    crow::json::rvalue header;
    try {
        header = crow::json::load(headerJson);
    } catch (...) {
        outError = "Invalid JSON in token header";
        return false;
    }
    if (!header) {
        outError = "Invalid JSON in token header";
        return false;
    }
    if (!header.has("alg")) {
        outError = "Missing 'alg' claim in token header";
        return false;
    }
    std::string alg = std::string(header["alg"].s());
    if (alg != "HS256") {
        outError = "Unsupported or invalid algorithm: expected HS256";
        return false;
    }
    if (header.has("typ")) {
        std::string typ = std::string(header["typ"].s());
        if (typ != "JWT") {
            outError = "Invalid token type: expected JWT";
            return false;
        }
    }

    // 3. Resolve and Validate Secret
    std::string key = secret;
    if (key.empty()) {
        std::string secErr;
        if (!validateStartupSecret(secErr)) {
            outError = "Secret configuration error: " + secErr;
            return false;
        }
        key = getSecret();
    } else {
        std::string secErr;
        if (!validateSecret(key, secErr)) {
            outError = "Secret configuration error: " + secErr;
            return false;
        }
    }

    // 4. Verify Signature using Constant-Time Comparison (Requirement 4)
    std::string dataToSign = encHeader + "." + encPayload;
    std::string expectedRaw = hmacSha256Raw(key, dataToSign);
    std::string expectedSig = base64UrlEncode(expectedRaw);

    if (!constantTimeCompare(expectedSig, encSignature)) {
        outError = "Invalid signature";
        return false;
    }

    // 5. Decode and Validate Payload Claims (Requirement 3)
    std::string payloadJson = base64UrlDecode(encPayload);
    if (payloadJson.empty()) {
        outError = "Malformed token payload: invalid base64 encoding";
        return false;
    }
    crow::json::rvalue payload;
    try {
        payload = crow::json::load(payloadJson);
    } catch (...) {
        outError = "Invalid JSON in token payload";
        return false;
    }
    if (!payload) {
        outError = "Invalid JSON in token payload";
        return false;
    }

    // Validate 'exp' claim: must be present and a positive integer
    if (!payload.has("exp") || payload["exp"].t() != crow::json::type::Number) {
        outError = "Missing or invalid 'exp' claim in token payload";
        return false;
    }
    int64_t exp = payload["exp"].i();
    if (exp <= 0) {
        outError = "Invalid 'exp' claim value";
        return false;
    }
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    if (now > exp) {
        outError = "Token has expired";
        return false;
    }
    outClaims.exp = exp;

    // Validate 'sub' claim: must be present and a positive integer
    if (!payload.has("sub") || payload["sub"].t() != crow::json::type::Number) {
        outError = "Missing or invalid 'sub' claim in token payload";
        return false;
    }
    int64_t sub = payload["sub"].i();
    if (sub <= 0) {
        outError = "Invalid 'sub' claim: user ID must be positive";
        return false;
    }
    outClaims.userId = static_cast<int>(sub);

    // Validate 'role' claim: must be one of allowable roles
    if (!payload.has("role")) {
        outError = "Missing 'role' claim in token payload";
        return false;
    }
    std::string role = std::string(payload["role"].s());
    if (role != "admin" && role != "operator" && role != "user") {
        outError = "Invalid 'role' claim in token payload";
        return false;
    }
    outClaims.role = role;

    if (payload.has("email")) outClaims.email = std::string(payload["email"].s());
    if (payload.has("name")) outClaims.name = std::string(payload["name"].s());
    if (payload.has("iat")) outClaims.iat = payload["iat"].i();

    return true;
}

bool JwtUtil::extractBearerToken(const std::string& authHeader, std::string& outToken) {
    if (authHeader.empty()) {
        return false;
    }
    const std::string prefix = "Bearer ";
    if (authHeader.rfind(prefix, 0) == 0) {
        outToken = authHeader.substr(prefix.length());
        while (!outToken.empty() && outToken[0] == ' ') {
            outToken.erase(0, 1);
        }
        return !outToken.empty();
    }
    if (authHeader.find('.') != std::string::npos) {
        outToken = authHeader;
        return true;
    }
    return false;
}

bool JwtUtil::requireAuth(const crow::request& req,
                          crow::response& res,
                          JwtClaims& outClaims,
                          const std::string& requiredRole) {
    std::string authHeader = req.get_header_value("Authorization");
    if (authHeader.empty()) {
        authHeader = req.get_header_value("authorization");
    }

    std::string token;
    if (!extractBearerToken(authHeader, token)) {
        crow::json::wvalue err;
        err["success"] = false;
        err["message"] = "Authentication required: missing Bearer token";
        res = crow::response(401, err);
        return false;
    }

    std::string verifyErr;
    if (!verifyToken(token, outClaims, verifyErr)) {
        crow::json::wvalue err;
        err["success"] = false;
        err["message"] = "Invalid or expired authorization token (" + verifyErr + ")";
        res = crow::response(401, err);
        return false;
    }

    // Role-based authorization check
    if (!requiredRole.empty()) {
        if (requiredRole == "admin" && outClaims.role != "admin") {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Access forbidden: admin privileges required";
            res = crow::response(403, err);
            return false;
        }
        if (requiredRole == "operator" && outClaims.role != "admin" && outClaims.role != "operator") {
            crow::json::wvalue err;
            err["success"] = false;
            err["message"] = "Access forbidden: operator or admin privileges required";
            res = crow::response(403, err);
            return false;
        }
    }

    return true;
}

} // namespace security
