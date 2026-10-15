#include <iostream>
#include <cassert>
#include "../database/database.h"
#include "../models/user.h"
#include "../services/auth_service.h"

// Standalone unit test for Aditya Singh's module
int main() {
    std::cout << "========================================\n";
    std::cout << " Running Tests: Aditya Singh (Database & Auth)\n";
    std::cout << "========================================\n";

    // 1. Test Database connection
    Database db("test_evhub.db");
    assert(db.open() && "Failed to open database");
    std::cout << "[PASS] Database opened successfully.\n";

    // 2. Test Schema initialization (tries relative paths depending on where test is run)
    bool loaded = db.initSchema("../database/schema.sql") ||
                  db.initSchema("backend/database/schema.sql") ||
                  db.initSchema("database/schema.sql");
    assert(loaded && "Failed to load schema.sql");
    std::cout << "[PASS] Database schema and seed data loaded.\n";

    // 3. Test AuthService
    AuthService auth(&db);
    User loggedInUser;

    // Test Valid Login (from seed data)
    bool loginSuccess = auth.login("admin@evhub.com", "admin123", loggedInUser);
    assert(loginSuccess && "Valid login should succeed");
    assert(loggedInUser.getRole() == "admin" && "Admin role should match");
    std::cout << "[PASS] Valid login test passed for: " << loggedInUser.getEmail() << "\n";

    // Test Wrong Password
    User failedUser;
    bool wrongPass = auth.login("admin@evhub.com", "wrongpass", failedUser);
    assert(!wrongPass && "Wrong password must be rejected");
    std::cout << "[PASS] Wrong password rejection test passed.\n";

    // Test Unknown User
    bool unknownUser = auth.login("nobody@evhub.com", "any", failedUser);
    assert(!unknownUser && "Unknown user must be rejected");
    std::cout << "[PASS] Unknown user rejection test passed.\n";

    // Test Register User
    bool registered = auth.registerUser("Test Member", "member@evhub.com", "test1234", "user");
    assert(registered && "Registration should succeed");
    std::cout << "[PASS] New user registration test passed.\n";

    // Test login of newly registered user
    User newLogin;
    assert(auth.login("member@evhub.com", "test1234", newLogin));
    std::cout << "[PASS] Registered user login test passed.\n";

    db.close();

    std::cout << "\n>>> ALL TESTS PASSED FOR ADITYA SINGH! <<<\n";
    return 0;
}
