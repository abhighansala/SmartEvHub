#include <iostream>
#include <cassert>
#include <sstream>
#include "../models/battery.h"
#include "../models/swap_request.h"
#include "../services/battery_service.h"

// ============================================================================
// Smart EV Hub - Comprehensive Unit Test Suite
// Module Owner: Amit Singh Kaintura (B.Tech 2nd Year, Section G, Student ID: 2518188)
// Role: Battery Inventory & Battery Swapping Module
//
// Syllabus References Demonstrated:
// - TCS 307: Classes & Objects, Encapsulation, Default/Parameterized/Copy Constructors,
//   Destructors, 'this' pointer, Function Overloading, Operator Overloading (==, <<).
// - TCS 302: Linear Array traversal, Linear Search, In-place state modifications.
// ============================================================================

int main() {
    std::cout << "========================================================\n";
    std::cout << " Smart EV Hub - Unit Tests: Amit Singh Kaintura\n";
    std::cout << " Role: Battery Inventory & Battery Swapping Module\n";
    std::cout << "========================================================\n\n";

    // -------------------------------------------------------------------------
    // TEST 1: Battery Constructors & Encapsulation (TCS 307 Unit 1 & 2)
    // -------------------------------------------------------------------------
    std::cout << "[TEST 1] Testing Battery Constructors & Encapsulation...\n";
    
    // Default constructor
    Battery bDefault;
    assert(bDefault.getCapacity() == 50.0);
    assert(bDefault.getChargeLevel() == 100.0);
    assert(bDefault.getStatus() == "available");

    // Parameterized constructor
    Battery bParam("BAT-100", 65.0, 98.0, "available", 95.5, 12, "EV-101");
    assert(bParam.getId() == "BAT-100");
    assert(bParam.getCapacity() == 65.0);
    assert(bParam.getChargeLevel() == 98.0);
    assert(bParam.getStatus() == "available");
    assert(bParam.getHealth() == 95.5);
    assert(bParam.getCycleCount() == 12);
    assert(bParam.getCurrentVehicle() == "EV-101");

    // Copy constructor (TCS 307 Unit 2)
    Battery bCopy(bParam);
    assert(bCopy.getId() == bParam.getId());
    assert(bCopy.getCapacity() == bParam.getCapacity());
    assert(bCopy.getChargeLevel() == bParam.getChargeLevel());
    std::cout << "  -> Default, Parameterized, and Copy constructors passed.\n";

    // -------------------------------------------------------------------------
    // TEST 2: Battery Business Logic & Readiness Check (isReady, isDepleted)
    // -------------------------------------------------------------------------
    std::cout << "[TEST 2] Testing Readiness Logic & Health Thresholds...\n";

    Battery bReady("BAT-READY", 50.0, 96.0, "available", 85.0);
    assert(bReady.isReady() == true);

    // Charge < 95% -> not ready
    Battery bLowCharge("BAT-LOW", 50.0, 94.0, "available", 90.0);
    assert(bLowCharge.isReady() == false);

    // Health < 80% -> not ready
    Battery bDegraded("BAT-DEG", 50.0, 100.0, "available", 79.0);
    assert(bDegraded.isReady() == false);

    // Status not available -> not ready
    Battery bInUse("BAT-USE", 50.0, 100.0, "inuse", 95.0);
    assert(bInUse.isReady() == false);

    // Depleted check
    Battery bDepleted("BAT-DEP", 50.0, 12.0, "charging", 95.0);
    assert(bDepleted.isDepleted() == true);
    std::cout << "  -> Readiness rules (Charge >= 95%, SoH >= 80%, status == 'available') verified.\n";

    // -------------------------------------------------------------------------
    // TEST 3: Operator Overloading (TCS 307 Unit 2 & CO3)
    // -------------------------------------------------------------------------
    std::cout << "[TEST 3] Testing Operator Overloading (==, !=, <<)...\n";
    Battery bA("BAT-01", 50.0, 100.0);
    Battery bB("BAT-01", 60.0, 80.0);
    Battery bC("BAT-02", 50.0, 100.0);

    assert(bA == bB); // Same ID
    assert(bA != bC); // Different ID

    // Friend operator<< stream insertion test
    std::stringstream ss;
    ss << bA;
    assert(ss.str().find("BAT-01") != std::string::npos);
    std::cout << "  -> Operator==, Operator!=, and Friend Operator<< successfully verified: " << ss.str() << "\n";

    // -------------------------------------------------------------------------
    // TEST 4: SwapRequest Domain Model Tests
    // -------------------------------------------------------------------------
    std::cout << "[TEST 4] Testing SwapRequest Domain Model...\n";
    SwapRequest req("SWP-001", "EV-102", "BAT-OLD", "BAT-01", 50.0, "completed");
    assert(req.getId() == "SWP-001");
    assert(req.getVehicleId() == "EV-102");
    assert(req.getOldBatteryId() == "BAT-OLD");
    assert(req.getNewBatteryId() == "BAT-01");
    assert(req.isCompleted() == true);

    std::stringstream reqStream;
    reqStream << req;
    assert(reqStream.str().find("SWP-001") != std::string::npos);
    std::cout << "  -> SwapRequest model and stream output passed.\n";

    // -------------------------------------------------------------------------
    // TEST 5: BatteryService - Array Storage & Linear Search (TCS 302)
    // -------------------------------------------------------------------------
    std::cout << "[TEST 5] Testing BatteryService Retrieval & Search Algorithms...\n";
    BatteryService service(nullptr); // Using in-memory array fallback

    Battery list[MAX_BATTERIES];
    int count = service.getAllBatteries(list, MAX_BATTERIES);
    assert(count >= 5);
    std::cout << "  -> Retrieved " << count << " batteries from inventory array.\n";

    // Search by ID
    Battery foundBat;
    bool found = service.getBatteryById("BAT-01", foundBat);
    assert(found == true);
    assert(foundBat.getId() == "BAT-01");

    bool notFound = service.getBatteryById("NON-EXISTENT", foundBat);
    assert(notFound == false);
    std::cout << "  -> Linear search for existing and non-existent IDs passed.\n";

    // Ready count
    int readyCount = service.getReadyCount();
    assert(readyCount > 0);
    std::cout << "  -> Current ready batteries count: " << readyCount << ".\n";

    // -------------------------------------------------------------------------
    // TEST 6: Function Overloading in BatteryService (TCS 307 Unit 4 / CO5)
    // -------------------------------------------------------------------------
    std::cout << "[TEST 6] Testing Function Overloading (findReadyBattery)...\n";
    Battery ready1;
    bool foundBasic = service.findReadyBattery(ready1);
    assert(foundBasic == true);
    assert(ready1.isReady() == true);

    Battery readyCapacity;
    bool foundCap = service.findReadyBattery(60.0, readyCapacity);
    assert(foundCap == true);
    assert(readyCapacity.isReady() == true);
    std::cout << "  -> Overloaded findReadyBattery(double) selected: " << readyCapacity.getId() 
              << " (" << readyCapacity.getCapacity() << "kWh).\n";

    // -------------------------------------------------------------------------
    // TEST 7: Swap Operations & State Transitions
    // -------------------------------------------------------------------------
    std::cout << "[TEST 7] Testing Battery Swap Workflow...\n";
    Battery assigned;
    bool swapSuccess = service.performSwap("EV-102", "BAT-OLD", 50.0, assigned);
    assert(swapSuccess == true);
    assert(assigned.getStatus() == "inuse");
    assert(assigned.getCurrentVehicle() == "EV-102");
    std::cout << "  -> Swap executed successfully: Assigned " << assigned.getId() << " to EV-102.\n";

    // -------------------------------------------------------------------------
    // TEST 8: Add New Battery to Inventory
    // -------------------------------------------------------------------------
    std::cout << "[TEST 8] Testing Add Battery to Inventory...\n";
    Battery bNew("BAT-99", 55.0, 100.0, "available", 99.0, 0);
    bool added = service.addBattery(bNew);
    assert(added == true);

    Battery retrievedNew;
    assert(service.getBatteryById("BAT-99", retrievedNew) == true);
    assert(retrievedNew.getCapacity() == 55.0);
    std::cout << "  -> Added BAT-99 to inventory and retrieved successfully.\n";

    // -------------------------------------------------------------------------
    // TEST 9: Exhaustion & Queue Fallback Check
    // -------------------------------------------------------------------------
    std::cout << "[TEST 9] Testing Inventory Exhaustion...\n";
    // Swap all remaining ready batteries
    Battery dummy;
    while (service.findReadyBattery(dummy)) {
        service.performSwap("EV-TEST", dummy);
    }
    // Now no ready batteries should remain
    assert(service.getReadyCount() == 0);
    bool failSwap = service.performSwap("EV-EXHAUSTED", dummy);
    assert(failSwap == false); // Should cleanly signal exhaustion so frontend can queue
    std::cout << "  -> Exhaustion handled gracefully: returned false for queue delegation.\n";

    // -------------------------------------------------------------------------
    // TEST 10: Real SQLite Database Persistence & Atomic Swap Transaction
    // -------------------------------------------------------------------------
    std::cout << "\n[TEST 10] Testing Real SQLite Database Persistence & Atomic Swap...\n";
    std::string dbFile = "test_amit.db";
    std::remove(dbFile.c_str());

    Database db(dbFile);
    assert(db.open());

    bool schemaLoaded = db.initSchema("../database/schema.sql") ||
                        db.initSchema("backend/database/schema.sql") ||
                        db.initSchema("database/schema.sql");
    assert(schemaLoaded && "Failed to load database schema for BatteryService!");
    std::cout << "  -> SQLite database connected and schema initialized.\n";

    BatteryService dbService(&db);
    Battery dbBatteries[MAX_BATTERIES];
    int batCount = dbService.getAllBatteries(dbBatteries, MAX_BATTERIES);
    assert(batCount == 7); // Schema seed has BAT-01 through BAT-07
    std::cout << "  -> Loaded " << batCount << " batteries from SQLite Battery table.\n";

    int dbReadyCount = dbService.getReadyCount();
    assert(dbReadyCount == 3); // BAT-01, BAT-02, BAT-03 are available at 100%
    std::cout << "  -> Ready battery count in SQLite: " << dbReadyCount << ".\n";

    // Execute atomic battery swap: EV-102 (swapping) receives BAT-01, deposits BAT-04
    Battery swappedBattery;
    bool swapOk = dbService.performSwap("EV-102", "BAT-04", 50.0, swappedBattery);
    assert(swapOk == true);
    assert(swappedBattery.getId() == "BAT-01");
    std::cout << "  -> Atomic swap executed in SQLite: assigned " << swappedBattery.getId() << " to EV-102.\n";

    // Verify database state directly
    Battery verifyNew;
    assert(dbService.getBatteryById("BAT-01", verifyNew));
    assert(verifyNew.getStatus() == "inuse");
    assert(verifyNew.getCurrentVehicle() == "EV-102");

    Battery verifyOld;
    assert(dbService.getBatteryById("BAT-04", verifyOld));
    assert(verifyOld.getStatus() == "charging");
    std::cout << "  -> SQLite state verified: BAT-01 is inuse, BAT-04 is charging.\n";

    // -------------------------------------------------------------------------
    // TEST 11: Persistence across Connection Close / Reopen
    // -------------------------------------------------------------------------
    std::cout << "\n[TEST 11] Testing Persistence across Connection Close / Reopen...\n";
    db.close();

    Database reopenedDb(dbFile);
    assert(reopenedDb.open());
    BatteryService reopenedService(&reopenedDb);

    Battery persistedBat;
    assert(reopenedService.getBatteryById("BAT-01", persistedBat));
    assert(persistedBat.getStatus() == "inuse");
    assert(persistedBat.getCurrentVehicle() == "EV-102");
    std::cout << "  -> PERSISTENCE VERIFIED: Swapped battery status survived connection restart!\n";

    // -------------------------------------------------------------------------
    // TEST 12: Swap Queue Enqueue on Inventory Exhaustion & FIFO Ordering
    // -------------------------------------------------------------------------
    std::cout << "\n[TEST 12] Testing Swap Queue Enqueue on Inventory Exhaustion...\n";
    // Mark remaining available batteries (BAT-02, BAT-03) as inuse to simulate exhausted inventory
    sqlite3* handle = reopenedDb.getDbHandle();
    sqlite3_exec(handle, "UPDATE Battery SET status = 'inuse' WHERE status = 'available';", nullptr, nullptr, nullptr);
    assert(reopenedService.getReadyCount() == 0);
    std::cout << "  -> Inventory exhausted (ready batteries = 0).\n";

    Battery exhaustBat;
    assert(reopenedService.performSwap("EV-104", exhaustBat) == false);

    SwapQueue swapQ;
    assert(swapQ.isEmpty());
    assert(reopenedService.enqueueWaitingSwap("EV-104", swapQ) == true);
    assert(swapQ.getSize() == 1);
    assert(swapQ.contains("EV-104") == true);
    assert(swapQ.peek() == "EV-104");

    // Verify EV-104 status in SQLite transitioned to 'swapping'
    sqlite3_stmt* chkStmt = nullptr;
    sqlite3_prepare_v2(handle, "SELECT status FROM EV WHERE id = 'EV-104';", -1, &chkStmt, nullptr);
    assert(sqlite3_step(chkStmt) == SQLITE_ROW);
    std::string ev104Status = (const char*)sqlite3_column_text(chkStmt, 0);
    sqlite3_finalize(chkStmt);
    assert(ev104Status == "swapping");
    std::cout << "  -> EV-104 enqueued: SQLite status successfully set to 'swapping'.\n";

    // Duplicate queue entry prevention
    assert(reopenedService.enqueueWaitingSwap("EV-104", swapQ) == false);
    assert(swapQ.getSize() == 1);
    std::cout << "  -> Duplicate swap request rejected correctly.\n";

    // Enqueue second waiting EV (EV-105)
    assert(reopenedService.enqueueWaitingSwap("EV-105", swapQ) == true);
    assert(swapQ.getSize() == 2);
    assert(swapQ.peek() == "EV-104"); // FIFO: EV-104 remains at FRONT
    std::cout << "  -> EV-105 enqueued: FIFO queue size is 2, FRONT is EV-104.\n";

    // -------------------------------------------------------------------------
    // TEST 13: Deterministic Swap Queue Rehydration after Restart
    // -------------------------------------------------------------------------
    std::cout << "\n[TEST 13] Testing Deterministic Swap Queue Rehydration...\n";
    reopenedDb.close();

    Database restartDb(dbFile);
    assert(restartDb.open());
    BatteryService restartService(&restartDb);

    SwapQueue restartedQ;
    assert(restartedQ.isEmpty());
    int rehydrated = restartService.rehydrateSwapQueue(restartedQ);
    assert(rehydrated == 2);
    assert(restartedQ.getSize() == 2);
    assert(restartedQ.peek() == "EV-104");
    std::cout << "  -> Deterministic rehydration verified: 2 waiting EVs restored with identical FIFO order!\n";

    // -------------------------------------------------------------------------
    // TEST 14: Process Queued Swap, Transaction Logging & Rollback Safety
    // -------------------------------------------------------------------------
    std::cout << "\n[TEST 14] Testing Process Queued Swap, Transaction Logging & Rollback Safety...\n";
    // Replenish inventory: Mark BAT-02 as available
    Battery b2;
    assert(restartService.getBatteryById("BAT-02", b2));
    b2.setStatus("available");
    b2.setChargeLevel(100.0);
    assert(restartService.updateBattery(b2));
    assert(restartService.getReadyCount() == 1);
    std::cout << "  -> Inventory replenished: BAT-02 is ready for swap.\n";

    Battery allocatedBat;
    Transaction swapTxn;
    bool procOk = restartService.processNextSwap(restartedQ, allocatedBat, swapTxn);
    assert(procOk == true);
    assert(allocatedBat.getId() == "BAT-02");
    assert(restartedQ.getSize() == 1);
    assert(restartedQ.peek() == "EV-105"); // EV-105 is now FRONT
    std::cout << "  -> Dequeued EV-104, assigned BAT-02. EV-105 moved to FRONT.\n";

    // Verify TransactionLog record in SQLite
    sqlite3_stmt* txnStmt = nullptr;
    sqlite3* restartHandle = restartDb.getDbHandle();
    sqlite3_prepare_v2(restartHandle, "SELECT type, station_or_battery, amount, status FROM TransactionLog WHERE vehicle_id = 'EV-104';", -1, &txnStmt, nullptr);
    assert(sqlite3_step(txnStmt) == SQLITE_ROW);
    std::string txType = (const char*)sqlite3_column_text(txnStmt, 0);
    std::string txBat = (const char*)sqlite3_column_text(txnStmt, 1);
    double txAmt = sqlite3_column_double(txnStmt, 2);
    std::string txStat = (const char*)sqlite3_column_text(txnStmt, 3);
    sqlite3_finalize(txnStmt);

    assert(txType == "battery-swap");
    assert(txBat == "BAT-02");
    assert(txAmt == swapTxn.getAmount());
    assert(txStat == "completed");
    std::cout << "  -> TransactionLog in SQLite verified: ID=" << swapTxn.getId() << ", Amount=₹" << txAmt << ".\n";

    // Verify EV-104 status in SQLite is now available with full charge
    sqlite3_stmt* evChk = nullptr;
    sqlite3_prepare_v2(restartHandle, "SELECT status, current_charge FROM EV WHERE id = 'EV-104';", -1, &evChk, nullptr);
    assert(sqlite3_step(evChk) == SQLITE_ROW);
    std::string statAfter = (const char*)sqlite3_column_text(evChk, 0);
    double chgAfter = sqlite3_column_double(evChk, 1);
    sqlite3_finalize(evChk);
    assert(statAfter == "available");
    assert(chgAfter == 100.0);
    std::cout << "  -> EV-104 updated to 'available' with 100% charge.\n";

    // Rollback / Exhaustion Safety: attempt to process EV-105 when inventory is exhausted again
    assert(restartService.getReadyCount() == 0);
    bool exhaustProc = restartService.processNextSwap(restartedQ, allocatedBat, swapTxn);
    assert(exhaustProc == false);
    assert(restartedQ.getSize() == 1);
    assert(restartedQ.peek() == "EV-105"); // EV-105 remains safely at FRONT
    std::cout << "  -> Rollback safety verified: EV-105 preserved in queue when inventory is exhausted.\n";

    restartDb.close();
    std::remove(dbFile.c_str());

    // -------------------------------------------------------------------------
    // TEST 15: Battery Swap Lifecycle, Ownership, Charging Simulation & Anomaly Reconciliation
    // -------------------------------------------------------------------------
    std::cout << "\n[TEST 15] Testing Battery Swap Lifecycle, Ownership & Simulation...\n";
    std::string lifecycleDbFile = "test_lifecycle.db";
    std::remove(lifecycleDbFile.c_str());
    Database lifeDb(lifecycleDbFile);
    assert(lifeDb.open());
    assert(lifeDb.initSchema("backend/database/schema.sql") || lifeDb.initSchema("../database/schema.sql") || lifeDb.initSchema("database/schema.sql"));

    BatteryService lifeService(&lifeDb);
    lifeService.setSimulationSpeedMultiplier(20.0);
    lifeService.setChargingPower(25.0);

    sqlite3* lHandle = lifeDb.getDbHandle();

    // 15.1 Successful battery swap with installed battery detection & old battery recharging
    // Setup EV-USER-9 (User 9, 18% SoC, current battery BAT-01)
    sqlite3_exec(lHandle, "INSERT OR REPLACE INTO EV (id, owner_name, model, battery_capacity, current_charge, status, user_id) "
                          "VALUES ('EV-USER-9', 'Rahul Sharma', 'Nexon EV', 50.0, 18.0, 'available', 9);", nullptr, nullptr, nullptr);
    sqlite3_exec(lHandle, "UPDATE Battery SET status = 'inuse', current_vehicle = 'EV-USER-9', charge_level = 18.0 WHERE id = 'BAT-01';", nullptr, nullptr, nullptr);

    // Swap requested by User 9 (owner)
    Battery freshBat;
    Transaction swapTx;
    bool swapRes = lifeService.performSwap("EV-USER-9", "", 50.0, freshBat, &swapTx, 9, "user");
    assert(swapRes == true && "Owner must be allowed to swap their vehicle");
    assert(freshBat.getId() == "BAT-02" && "Should allocate next available 50kWh battery");

    // Verify BAT-02 is inuse by EV-USER-9
    Battery chkFresh;
    assert(lifeService.getBatteryById("BAT-02", chkFresh));
    assert(chkFresh.getStatus() == "inuse" && chkFresh.getCurrentVehicle() == "EV-USER-9");

    // Verify BAT-01 was detached and transitioned to charging
    Battery chkOld;
    assert(lifeService.getBatteryById("BAT-01", chkOld));
    assert(chkOld.getStatus() == "charging" && chkOld.getCurrentVehicle().empty());
    assert(chkOld.getChargeLevel() == 18.0);
    std::cout << "  -> 15.1 Successful swap verified: fresh BAT-02 assigned, old BAT-01 detached and placed in charging dock.\n";

    // 15.2 Unauthorized user attempting another user's swap
    Battery unauthBat;
    Transaction unauthTx;
    bool unauthRes = lifeService.performSwap("EV-USER-9", "", 50.0, unauthBat, &unauthTx, 10, "user");
    assert(!unauthRes && "User 10 must NOT be allowed to swap User 9's vehicle");
    std::cout << "  -> 15.2 Ownership authorization guard verified: unauthorized user swap rejected.\n";

    // Operator and Admin can swap any vehicle
    bool adminSwap = lifeService.performSwap("EV-USER-9", "", 50.0, unauthBat, &unauthTx, 2, "operator");
    assert(adminSwap == true && "Operator must be allowed to swap customer vehicle");
    std::cout << "  -> 15.3 Operator privilege verified: operator allowed to execute vehicle swap.\n";

    // 15.4 Battery already assigned to another vehicle cannot be assigned
    Battery assignedBat;
    assert(lifeService.getBatteryById(unauthBat.getId(), assignedBat));
    assert(!assignedBat.isReady() && "In-use battery must not be reported as ready for swap");
    assert(assignedBat.getStatus() == "inuse" && assignedBat.getCurrentVehicle() == "EV-USER-9");
    std::cout << "  -> 15.4 Busy battery protection verified: already-assigned battery excluded from swap.\n";

    // 15.5 Insufficient charge (< 95%) and degraded health (< 80%) rejected
    sqlite3_exec(lHandle, "UPDATE Battery SET status = 'available', charge_level = 85.0 WHERE id = 'BAT-03';", nullptr, nullptr, nullptr);
    Battery bLow;
    assert(lifeService.getBatteryById("BAT-03", bLow));
    assert(!bLow.isReady() && "Battery with 85% charge must not be ready");

    sqlite3_exec(lHandle, "UPDATE Battery SET status = 'available', charge_level = 100.0, health = 75.0 WHERE id = 'BAT-03';", nullptr, nullptr, nullptr);
    Battery bDeg;
    assert(lifeService.getBatteryById("BAT-03", bDeg));
    assert(!bDeg.isReady() && "Battery with 75% SoH must not be ready");
    std::cout << "  -> 15.5 Health & charge threshold verified: low SoC and degraded SoH rejected from swap.\n";

    // 15.6 Multi-battery anomaly reconciliation
    // Deliberately simulate anomaly: assign both BAT-03 and BAT-07 to EV-MULTI
    sqlite3_exec(lHandle, "INSERT OR REPLACE INTO EV (id, owner_name, model, battery_capacity, current_charge, status, user_id) "
                          "VALUES ('EV-MULTI', 'Multi Driver', 'Fleet Van', 50.0, 50.0, 'available', 15);", nullptr, nullptr, nullptr);
    sqlite3_exec(lHandle, "UPDATE Battery SET current_vehicle = 'EV-MULTI', status = 'inuse', charge_level = 100.0, last_updated = datetime('now', '-10 minutes') WHERE id = 'BAT-03';", nullptr, nullptr, nullptr);
    sqlite3_exec(lHandle, "UPDATE Battery SET current_vehicle = 'EV-MULTI', status = 'inuse', charge_level = 100.0, last_updated = datetime('now') WHERE id = 'BAT-07';", nullptr, nullptr, nullptr);
    lifeService.reconcileInventory();

    Battery multi1, multi2;
    assert(lifeService.getBatteryById("BAT-07", multi1));
    assert(lifeService.getBatteryById("BAT-03", multi2));
    assert(multi1.getCurrentVehicle() == "EV-MULTI" && "Newer battery remains assigned to EV-MULTI");
    assert(multi2.getCurrentVehicle().empty() && "Older duplicate battery detached from vehicle");
    std::cout << "  -> 15.6 Multi-battery anomaly reconciliation verified: duplicate vehicle assignment resolved.\n";

    // 15.7 Automatic transition from Charging to Available and FIFO swap queue progression
    // Place BAT-01 in charging dock at 90% charge with start_time 10 minutes ago
    // At 20x multiplier: 10 mins * 20x = 200 mins = 3.33 hours -> charges to 100%
    sqlite3_exec(lHandle, "UPDATE Battery SET status = 'charging', charge_level = 90.0, initial_charge = 90.0, "
                          "charging_start = datetime('now', '-600 seconds'), current_vehicle = NULL WHERE id = 'BAT-01';", nullptr, nullptr, nullptr);

    // Enqueue waiting vehicle in SwapQueue
    SwapQueue autoQueue;
    sqlite3_exec(lHandle, "INSERT OR REPLACE INTO EV (id, owner_name, model, battery_capacity, current_charge, status, user_id) "
                          "VALUES ('EV-WAIT-SWP', 'Queued Driver', 'Ioniq 5', 50.0, 10.0, 'swapping', 11);", nullptr, nullptr, nullptr);
    autoQueue.enqueue("EV-WAIT-SWP");

    TransactionStack lifeStack;
    int advanced = lifeService.advanceChargingBatteries(&autoQueue, &lifeStack);
    assert(advanced >= 1 && "At least 1 battery must transition from charging to available");
    
    // EV-WAIT-SWP should have been auto-dispatched
    assert(autoQueue.isEmpty() && "Auto-queue should be dispatched when battery becomes available");
    sqlite3_stmt* checkWaitEv = nullptr;
    sqlite3_prepare_v2(lHandle, "SELECT status, current_charge FROM EV WHERE id = 'EV-WAIT-SWP';", -1, &checkWaitEv, nullptr);
    assert(sqlite3_step(checkWaitEv) == SQLITE_ROW);
    std::string waitEvStat = (const char*)sqlite3_column_text(checkWaitEv, 0);
    double waitEvChg = sqlite3_column_double(checkWaitEv, 1);
    sqlite3_finalize(checkWaitEv);
    assert(waitEvStat == "available" && "Waiting EV must be set to available after auto-swap");
    assert(waitEvChg == 100.0 && "Waiting EV must have 100% charge after auto-swap");
    std::cout << "  -> 15.7 Charging simulation & auto-dispatch verified: battery charged and auto-swapped to queued vehicle.\n";

    // 15.8 Database persistence across restart
    lifeDb.close();
    Database reopenedLifeDb(lifecycleDbFile);
    assert(reopenedLifeDb.open());
    BatteryService restartedLifeService(&reopenedLifeDb);

    // Verify the auto-swapped vehicle has its newly assigned battery persisted in SQLite
    sqlite3_stmt* wStmt = nullptr;
    std::string assignedBatId = "";
    sqlite3_prepare_v2(reopenedLifeDb.getDbHandle(), "SELECT id FROM Battery WHERE current_vehicle = 'EV-WAIT-SWP';", -1, &wStmt, nullptr);
    if (sqlite3_step(wStmt) == SQLITE_ROW) {
        const char* t = (const char*)sqlite3_column_text(wStmt, 0);
        if (t) assignedBatId = t;
    }
    sqlite3_finalize(wStmt);
    assert(!assignedBatId.empty() && "A fresh battery must be assigned to EV-WAIT-SWP");

    Battery persistedBatAfter;
    assert(restartedLifeService.getBatteryById(assignedBatId, persistedBatAfter));
    assert(persistedBatAfter.getCurrentVehicle() == "EV-WAIT-SWP" && "Auto-swapped battery survived DB close & reopen");
    assert(persistedBatAfter.getStatus() == "inuse");

    reopenedLifeDb.close();
    std::remove(lifecycleDbFile.c_str());
    std::remove((lifecycleDbFile + "-wal").c_str());
    std::remove((lifecycleDbFile + "-shm").c_str());
    std::cout << "  -> 15.8 Restart persistence verified: battery and vehicle states survived restart.\n";

    std::cout << "\n========================================================\n";
    std::cout << " >>> ALL 15 TEST SUITES PASSED FOR AMIT SINGH KAINTURA! <<<\n";
    std::cout << "========================================================\n";
    return 0;
}
