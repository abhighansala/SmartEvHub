#include <iostream>
#include <cassert>
#include <cstdio>
#include "../database/database.h"
#include "../models/user.h"
#include "../services/auth_service.h"

// Comprehensive Unit & Persistence Test Suite for Aditya Singh's Module
// Role: Database + Backend Foundation (Team Lead)
// Student ID: 2519272 (ML1)
int main() {
    std::cout << "========================================================\n";
    std::cout << " Running Tests: Aditya Singh (Database & Auth Foundation)\n";
    std::cout << "========================================================\n\n";

    const std::string testDbFile = "test_evhub.db";
    // Remove stale test database file if present
    std::remove(testDbFile.c_str());

    // -----------------------------------------------------------------
    // 1. Test Database connection
    // -----------------------------------------------------------------
    std::cout << "[Step 1] Testing Database Connection...\n";
    Database db(testDbFile);
    assert(db.open() && "Failed to open database connection");
    assert(db.isOpen() && "Database should report isOpen() == true");
    std::cout << "  [PASS] Database successfully opened and connected.\n\n";

    // -----------------------------------------------------------------
    // 2. Test Schema initialization
    // -----------------------------------------------------------------
    std::cout << "[Step 2] Testing Schema Initialization & Seed Data...\n";
    bool loaded = db.initSchema("../../database/schema.sql") ||
                  db.initSchema("../database/schema.sql") ||
                  db.initSchema("backend/database/schema.sql") ||
                  db.initSchema("database/schema.sql");
    assert(loaded && "Failed to load and execute schema.sql");

    // Verify critical tables exist in SQLite
    assert(db.tableExists("Users") && "Users table must exist");
    assert(db.tableExists("EV") && "EV table must exist");
    assert(db.tableExists("ChargingStation") && "ChargingStation table must exist");
    assert(db.tableExists("Battery") && "Battery table must exist");
    assert(db.tableExists("ChargingSession") && "ChargingSession table must exist");
    assert(db.tableExists("TransactionLog") && "TransactionLog table must exist");
    std::cout << "  [PASS] All 6 tables verified in SQLite schema.\n\n";

    // -----------------------------------------------------------------
    // 3. Test Transaction Management Helpers
    // -----------------------------------------------------------------
    std::cout << "[Step 3] Testing Transaction Helpers (Begin / Commit / Rollback)...\n";
    assert(db.beginTransaction() && "beginTransaction failed");
    assert(db.execute("INSERT OR IGNORE INTO Users (name, email, password, role) VALUES ('TxnUser', 'txn@evhub.com', 'pass', 'user');"));
    assert(db.commitTransaction() && "commitTransaction failed");

    // Test Rollback
    assert(db.beginTransaction() && "beginTransaction failed");
    assert(db.execute("INSERT OR IGNORE INTO Users (name, email, password, role) VALUES ('RollbackUser', 'rollback@evhub.com', 'pass', 'user');"));
    assert(db.rollbackTransaction() && "rollbackTransaction failed");
    // Verify rollback user does not exist
    AuthService tempAuth(&db);
    assert(!tempAuth.userExists("rollback@evhub.com") && "Rolled back user must not exist");
    std::cout << "  [PASS] Database transaction helpers (commit & rollback) verified.\n\n";

    // -----------------------------------------------------------------
    // 4. Test AuthService Authentication
    // -----------------------------------------------------------------
    std::cout << "[Step 4] Testing User Authentication Scenarios...\n";
    AuthService auth(&db);
    User loggedInUser;

    // A. Valid Login with Email
    bool loginSuccess = auth.login("admin@evhub.com", "admin123", loggedInUser);
    assert(loginSuccess && "Valid login with email should succeed");
    assert(loggedInUser.getRole() == "admin" && "Admin role should match");
    assert(loggedInUser.getId() == 1 && "Admin ID should be 1");
    std::cout << "  [PASS] Valid email login passed for: " << loggedInUser.getEmail() << "\n";

    // B. Valid Login with Username / Name
    User teamLead;
    bool leadLogin = auth.login("Aditya Singh", "aditya", teamLead);
    assert(leadLogin && "Valid login with username should succeed");
    assert(teamLead.getEmail() == "aditya@evhub.com" && "Team lead email should match");
    std::cout << "  [PASS] Valid username login passed for: " << teamLead.getName() << "\n";

    // C. Wrong Password Rejection
    User failedUser;
    bool wrongPass = auth.login("admin@evhub.com", "incorrect_password", failedUser);
    assert(!wrongPass && "Wrong password must be rejected");
    std::cout << "  [PASS] Wrong password rejection verified.\n";

    // D. Unknown User Rejection
    bool unknownUser = auth.login("nonexistent@evhub.com", "password", failedUser);
    assert(!unknownUser && "Unknown user must be rejected");
    std::cout << "  [PASS] Unknown user rejection verified.\n";

    // E. Empty Credentials Rejection
    bool emptyCreds = auth.login("", "", failedUser);
    assert(!emptyCreds && "Empty credentials must be rejected");
    std::cout << "  [PASS] Empty credentials rejection verified.\n\n";

    // -----------------------------------------------------------------
    // 5. Test Registration & Duplicate Prevention
    // -----------------------------------------------------------------
    std::cout << "[Step 5] Testing User Registration & Duplicate Constraints...\n";
    bool registered = auth.registerUser("Campus Operator", "campus_op@evhub.com", "secure999", "operator");
    assert(registered && "New operator registration should succeed");
    std::cout << "  [PASS] New user successfully registered.\n";

    // Test duplicate registration rejection (UNIQUE email constraint)
    bool duplicate = auth.registerUser("Duplicate Op", "campus_op@evhub.com", "diffpass", "operator");
    assert(!duplicate && "Duplicate email registration must be rejected");
    std::cout << "  [PASS] Duplicate email registration rejected properly.\n\n";

    // -----------------------------------------------------------------
    // 6. Test Persistence Across Database Restart (PRD Requirement)
    // -----------------------------------------------------------------
    std::cout << "[Step 6] Testing Database Persistence (Restart Verification)...\n";
    std::cout << "  -> Closing active SQLite connection...\n";
    db.close();
    assert(!db.isOpen() && "Database should be closed");

    std::cout << "  -> Simulating application restart & reopening SQLite database...\n";
    Database reopenedDb(testDbFile);
    assert(reopenedDb.open() && "Reopening database should succeed");

    AuthService restartAuth(&reopenedDb);
    User persistedUser;
    bool persistedLogin = restartAuth.login("campus_op@evhub.com", "secure999", persistedUser);
    assert(persistedLogin && "Registered user MUST persist and authenticate after DB restart");
    assert(persistedUser.getRole() == "operator" && "Persisted role must match");
    std::cout << "  [PASS] PERSISTENCE VERIFIED: Registered user survived connection restart!\n\n";

    reopenedDb.close();
    // Clean up temporary test database
    std::remove(testDbFile.c_str());

    std::cout << "========================================================\n";
    std::cout << " >>> ALL TESTS PASSED FOR ADITYA SINGH (LEAD) <<<        \n";
    std::cout << "========================================================\n";
    return 0;
}
