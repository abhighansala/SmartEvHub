#include <iostream>
#include <cassert>
#include "../models/ev.h"
#include "../services/ev_service.h"

// Standalone unit test for Priyanshu Dutt's module
int main() {
    std::cout << "========================================\n";
    std::cout << " Running Tests: Priyanshu Dutt (EV Management)\n";
    std::cout << "========================================\n";

    // 1. Test EV Model
    EV ev1("EV-999", "Test Owner", "Tesla Model 3", 75.0, 45.0, "available");
    assert(ev1.getId() == "EV-999");
    assert(ev1.getCurrentCharge() == 45.0);
    assert(ev1.isValid() == true);
    std::cout << "[PASS] EV Model instantiation and validation passed.\n";

    // 2. Test Invalid EV Charge (>100)
    EV badEV("EV-BAD", "Nobody", "Faulty", 50.0, 150.0);
    assert(badEV.isValid() == false);
    std::cout << "[PASS] Invalid charge rejection passed.\n";

    // 3. Test EVService (using local array storage)
    EVService service(nullptr);
    std::string err;

    // Create EV
    bool created = service.createEV(ev1, err);
    assert(created == true);
    std::cout << "[PASS] EV creation passed.\n";

    // Retrieve EV
    EV retrieved;
    bool found = service.getEVById("EV-999", retrieved);
    assert(found == true);
    assert(retrieved.getOwnerName() == "Test Owner");
    std::cout << "[PASS] EV retrieval by ID passed.\n";

    // Update EV charge
    retrieved.setCurrentCharge(80.0);
    bool updated = service.updateEV("EV-999", retrieved, err);
    assert(updated == true);

    EV afterUpdate;
    service.getEVById("EV-999", afterUpdate);
    assert(afterUpdate.getCurrentCharge() == 80.0);
    std::cout << "[PASS] EV update passed.\n";

    // Test List retrieval using simple array
    EV list[MAX_EVS];
    int count = service.getAllEVs(list, MAX_EVS);
    assert(count == 1);
    std::cout << "[PASS] Array list retrieval passed (count = " << count << ").\n";

    // Delete EV
    bool deleted = service.deleteEV("EV-999");
    assert(deleted == true);
    assert(service.getEVById("EV-999", retrieved) == false);
    std::cout << "[PASS] EV deletion passed.\n";

    // 4. Test EVService with real SQLite Database
    std::cout << "\n--- Testing Real SQLite Persistence for Priyanshu Dutt ---\n";
    std::string dbFile = "test_priyanshu.db";
    // Remove stale test db if present
    std::remove(dbFile.c_str());

    Database db(dbFile);
    assert(db.open());

    bool schemaLoaded = db.initSchema("../database/schema.sql") ||
                        db.initSchema("backend/database/schema.sql") ||
                        db.initSchema("database/schema.sql");
    assert(schemaLoaded && "Failed to load database schema!");
    std::cout << "[PASS] Database connected and schema.sql initialized successfully.\n";

    EVService dbService(&db);
    EV dbList[MAX_EVS];
    int seedCount = dbService.getAllEVs(dbList, MAX_EVS);
    assert(seedCount >= 6); // Schema seed has EV-101 through EV-106, EV-125
    std::cout << "[PASS] Seeded EVs retrieved from SQLite (count = " << seedCount << ").\n";

    // Insert new EV into SQLite
    EV realEV("EV-DB-TEST", "Suresh Kumar", "Hyundai Kona Electric", 39.2, 55.0, "available");
    assert(dbService.createEV(realEV, err));
    std::cout << "[PASS] Real EV successfully created in SQLite database.\n";

    // Query back from SQLite
    EV queriedEV;
    assert(dbService.getEVById("EV-DB-TEST", queriedEV));
    assert(queriedEV.getOwnerName() == "Suresh Kumar");
    assert(queriedEV.getModel() == "Hyundai Kona Electric");
    assert(queriedEV.getBatteryCapacity() == 39.2);
    assert(queriedEV.getCurrentCharge() == 55.0);
    assert(queriedEV.getStatus() == "available");
    std::cout << "[PASS] EV retrieved from SQLite by ID with all matching attributes.\n";

    // Update in SQLite
    queriedEV.setCurrentCharge(88.0);
    queriedEV.setStatus("charging");
    assert(dbService.updateEV("EV-DB-TEST", queriedEV, err));

    EV verifiedEV;
    assert(dbService.getEVById("EV-DB-TEST", verifiedEV));
    assert(verifiedEV.getCurrentCharge() == 88.0);
    assert(verifiedEV.getStatus() == "charging");
    std::cout << "[PASS] EV updated in SQLite database.\n";

    // 5. Test Persistence Across Connection Restart
    std::cout << "\n--- Testing Persistence across Connection Close / Reopen ---\n";
    db.close();

    Database reopenedDb(dbFile);
    assert(reopenedDb.open());
    EVService reopenedService(&reopenedDb);

    EV persistedEV;
    assert(reopenedService.getEVById("EV-DB-TEST", persistedEV));
    assert(persistedEV.getCurrentCharge() == 88.0);
    assert(persistedEV.getStatus() == "charging");
    std::cout << "[PASS] PERSISTENCE VERIFIED: Updated EV survived connection restart!\n";

    // Clean up test EV
    assert(reopenedService.deleteEV("EV-DB-TEST"));
    EV notFoundEV;
    assert(!reopenedService.getEVById("EV-DB-TEST", notFoundEV));
    std::cout << "[PASS] Test EV cleaned up from SQLite.\n";
    reopenedDb.close();
    std::remove(dbFile.c_str());

    std::cout << "\n>>> ALL IN-MEMORY AND REAL SQLITE TESTS PASSED FOR PRIYANSHU DUTT! <<<\n";
    return 0;
}
