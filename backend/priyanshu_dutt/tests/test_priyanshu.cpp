#include <iostream>
#include <cassert>
#include <cstdio>
#include "../models/ev.h"
#include "../services/ev_service.h"
#include "../../aditya_singh/database/database.h"

// Comprehensive Unit & Persistence Test Suite for Priyanshu Dutt's Module
// Role: EV Management Module Owner
// Student ID: 2519460 (ML3)
int main() {
    std::cout << "========================================================\n";
    std::cout << " Running Tests: Priyanshu Dutt (EV Management Module)\n";
    std::cout << "========================================================\n\n";

    // =========================================================================
    // SECTION 1: IN-MEMORY / ARRAY FALLBACK TESTS (service without database)
    // =========================================================================
    std::cout << "--- [SECTION 1] In-Memory / Array Fallback Unit Tests ---\n";

    // Test 1: Valid EV Model Instantiation
    EV ev1("EV-999", "Priyanshu Dutt", "Tesla Model 3", 75.0, 45.0, "available");
    assert(ev1.getId() == "EV-999");
    assert(ev1.getOwnerName() == "Priyanshu Dutt");
    assert(ev1.getModel() == "Tesla Model 3");
    assert(ev1.getBatteryCapacity() == 75.0);
    assert(ev1.getCurrentCharge() == 45.0);
    assert(ev1.getStatus() == "available");
    assert(ev1.isValid() == true);
    std::cout << "  [PASS] Test 1: EV Model instantiation and validation passed.\n";

    // Test 2: Boundary Valid Charges (0% and 100%)
    EV evZero("EV-ZERO", "Owner Zero", "Nissan Leaf", 40.0, 0.0);
    EV evFull("EV-FULL", "Owner Full", "Porsche Taycan", 93.4, 100.0);
    assert(evZero.isValid() == true);
    assert(evFull.isValid() == true);
    std::cout << "  [PASS] Test 2: Boundary valid charges (0% and 100%) accepted.\n";

    // Test 3: Invalid EV Charges (<0% and >100%)
    EV badNegative("EV-BAD-1", "Nobody", "Faulty Neg", 50.0, -0.1);
    EV badOver("EV-BAD-2", "Nobody", "Faulty Over", 50.0, 100.1);
    assert(badNegative.isValid() == false);
    assert(badOver.isValid() == false);
    std::cout << "  [PASS] Test 3: Invalid battery charge (<0% or >100%) rejected.\n";

    // Test 4: Invalid Capacity (<=0) and Empty Identifiers
    EV badCap("EV-BAD-3", "Nobody", "Faulty Cap", 0.0, 50.0);
    EV badEmptyId("", "Nobody", "Empty ID", 50.0, 50.0);
    EV badEmptyModel("EV-BAD-4", "Nobody", "", 50.0, 50.0);
    assert(badCap.isValid() == false);
    assert(badEmptyId.isValid() == false);
    assert(badEmptyModel.isValid() == false);
    std::cout << "  [PASS] Test 4: Non-positive capacity and empty ID/model rejected.\n";

    // Test 5: In-Memory Creation & Retrieval
    EVService memService(nullptr);
    std::string err;
    assert(memService.createEV(ev1, err) == true);
    EV retrieved;
    assert(memService.getEVById("EV-999", retrieved) == true);
    assert(retrieved.getOwnerName() == "Priyanshu Dutt");
    std::cout << "  [PASS] Test 5: In-memory EV creation and retrieval by ID passed.\n";

    // Test 6: In-Memory Duplicate ID Rejection
    EV dupEV("EV-999", "Different Owner", "Duplicate Car", 60.0, 50.0);
    assert(memService.createEV(dupEV, err) == false);
    assert(!err.empty());
    std::cout << "  [PASS] Test 6: In-memory duplicate EV ID rejected: " << err << "\n";

    // Test 7: In-Memory Update Existing EV
    retrieved.setCurrentCharge(80.0);
    retrieved.setStatus("charging");
    assert(memService.updateEV("EV-999", retrieved, err) == true);
    EV afterUpdate;
    assert(memService.getEVById("EV-999", afterUpdate) == true);
    assert(afterUpdate.getCurrentCharge() == 80.0);
    assert(afterUpdate.getStatus() == "charging");
    std::cout << "  [PASS] Test 7: In-memory EV update passed.\n";

    // Test 8: In-Memory Update Non-Existent EV
    EV ghostEV("EV-GHOST", "Ghost Owner", "Phantom", 50.0, 50.0);
    assert(memService.updateEV("EV-GHOST", ghostEV, err) == false);
    assert(!err.empty());
    std::cout << "  [PASS] Test 8: In-memory update on non-existent EV returned false.\n";

    // Test 9: In-Memory Delete Existing & Non-Existent EV
    std::string delErr;
    assert(memService.deleteEV("EV-999", delErr) == true);
    assert(memService.getEVById("EV-999", retrieved) == false);
    assert(memService.deleteEV("EV-NONEXISTENT", delErr) == false);
    assert(!delErr.empty());
    std::cout << "  [PASS] Test 9: In-memory delete and non-existent delete handling passed.\n";

    // Test 10: Array Traversal Count
    EV list[MAX_EVS];
    int count = memService.getAllEVs(list, MAX_EVS);
    assert(count == 0);
    std::cout << "  [PASS] Test 10: In-memory empty list verified (count = 0).\n\n";

    // =========================================================================
    // SECTION 2: SQLITE PERSISTENCE & DATABASE INTEGRATION TESTS
    // =========================================================================
    std::cout << "--- [SECTION 2] SQLite Persistence & Integration Tests ---\n";

    const std::string testDbFile = "test_priyanshu_temp.db";
    std::remove(testDbFile.c_str());

    Database db(testDbFile);
    assert(db.open() && "Failed to open test database");
    assert(db.isOpen() && "Database should report isOpen() == true");

    bool schemaLoaded = db.initSchema("database/schema.sql") ||
                        db.initSchema("backend/database/schema.sql") ||
                        db.initSchema("../database/schema.sql") ||
                        db.initSchema("../../database/schema.sql");
    assert(schemaLoaded && "Failed to load schema.sql for EV tests");
    assert(db.tableExists("EV") && "EV table must exist in schema");
    std::cout << "  [PASS] Test 11: SQLite test database opened and EV schema initialized.\n";

    EVService dbService(&db);

    // Test 12: Seed Record Count Verification (7 seeded EVs in schema.sql)
    EV seedList[MAX_EVS];
    int seedCount = dbService.getAllEVs(seedList, MAX_EVS);
    assert(seedCount == 7);
    std::cout << "  [PASS] Test 12: SQLite seed records verified (" << seedCount << " EVs found).\n";

    // Test 13: Query Seeded Record by ID
    EV seedEV;
    assert(dbService.getEVById("EV-101", seedEV) == true);
    assert(seedEV.getOwnerName() == "Rahul Sharma");
    assert(seedEV.getModel() == "Tata Nexon EV Max");
    assert(seedEV.getCurrentCharge() == 32.0);
    assert(seedEV.getStatus() == "charging");
    std::cout << "  [PASS] Test 13: Seed EV query by ID (EV-101) verified.\n";

    // Test 14: Create New EV in SQLite
    EV newEV("EV-201", "Priyanshu Dutt", "Lucid Air Grand Touring", 112.0, 95.0, "available");
    std::string dbErr;
    assert(dbService.createEV(newEV, dbErr) == true);
    EV retrievedNew;
    assert(dbService.getEVById("EV-201", retrievedNew) == true);
    assert(retrievedNew.getOwnerName() == "Priyanshu Dutt");
    assert(retrievedNew.getBatteryCapacity() == 112.0);
    std::cout << "  [PASS] Test 14: New EV created and retrieved from SQLite.\n";

    // Test 15: Duplicate EV ID Rejection in SQLite
    EV dupDbEV("EV-201", "Other Owner", "Clone Model", 70.0, 50.0);
    assert(dbService.createEV(dupDbEV, dbErr) == false);
    assert(dbErr.find("already exists") != std::string::npos);
    std::cout << "  [PASS] Test 15: Duplicate EV ID rejected by SQLite service: " << dbErr << "\n";

    // Test 16: Update Existing Record in SQLite
    retrievedNew.setCurrentCharge(42.0);
    retrievedNew.setStatus("waiting");
    assert(dbService.updateEV("EV-201", retrievedNew, dbErr) == true);
    EV afterDbUpdate;
    assert(dbService.getEVById("EV-201", afterDbUpdate) == true);
    assert(afterDbUpdate.getCurrentCharge() == 42.0);
    assert(afterDbUpdate.getStatus() == "waiting");
    std::cout << "  [PASS] Test 16: Existing EV record updated in SQLite.\n";

    // Test 17: REGRESSION - Update Non-Existent EV in SQLite (Must return false!)
    EV ghostDbEV("EV-NONEXISTENT", "Nobody", "Ghost Car", 50.0, 50.0);
    bool ghostUpdate = dbService.updateEV("EV-NONEXISTENT", ghostDbEV, dbErr);
    assert(ghostUpdate == false && "Updating non-existent EV in SQLite must return false");
    assert(dbErr.find("EV not found with ID") != std::string::npos);
    std::cout << "  [PASS] Test 17: [REGRESSION VERIFIED] Update on non-existent EV returned false with: " << dbErr << "\n";

    // Test 18: REGRESSION - Delete Non-Existent EV in SQLite (Must return false!)
    std::string ghostDelErr;
    bool ghostDelete = dbService.deleteEV("EV-NONEXISTENT", ghostDelErr);
    assert(ghostDelete == false && "Deleting non-existent EV in SQLite must return false");
    assert(ghostDelErr.find("EV not found with ID") != std::string::npos);
    std::cout << "  [PASS] Test 18: [REGRESSION VERIFIED] Delete on non-existent EV returned false with: " << ghostDelErr << "\n";

    // Test 19: Persistence Across Connection Restart
    std::cout << "  -> Closing active SQLite connection to simulate backend restart...\n";
    db.close();

    Database restartDb(testDbFile);
    assert(restartDb.open() && "Failed to re-open test database");
    EVService restartedService(&restartDb);

    EV restartedEV;
    assert(restartedService.getEVById("EV-201", restartedEV) == true);
    assert(restartedEV.getCurrentCharge() == 42.0);
    assert(restartedEV.getStatus() == "waiting");
    std::cout << "  [PASS] Test 19: [PERSISTENCE VERIFIED] EV-201 survived connection close and reopen!\n";

    // Test 20: Delete Existing EV from SQLite
    std::string finalDelErr;
    assert(restartedService.deleteEV("EV-201", finalDelErr) == true);
    EV postDeleteEV;
    assert(restartedService.getEVById("EV-201", postDeleteEV) == false);
    std::cout << "  [PASS] Test 20: EV-201 deleted from SQLite and confirmed absent.\n";

    // Cleanup temporary test database file
    restartDb.close();
    std::remove(testDbFile.c_str());
    std::remove((testDbFile + "-wal").c_str());
    std::remove((testDbFile + "-shm").c_str());

    std::cout << "\n========================================================\n";
    std::cout << " >>> ALL 20 TESTS PASSED FOR PRIYANSHU DUTT (EV MODULE)! <<<\n";
    std::cout << "========================================================\n";
    return 0;
}
