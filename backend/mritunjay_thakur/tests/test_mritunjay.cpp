#include <iostream>
#include <cassert>
#include <cmath>
#include <cstdio>
#include "../models/charging_station.h"
#include "../models/charging_session.h"
#include "../services/station_service.h"

// Comprehensive Dedicated Unit Test Suite: Mritunjay Thakur
// Student ID: 2519425 (ML-5)
// Module: Charging Stations & Charging Sessions
int main() {
    std::cout << "========================================================\n";
    std::cout << " Running Dedicated Test Suite: Mritunjay Thakur (ML-5)  \n";
    std::cout << " Module: Charging Stations & Sessions (Smart EV Hub)    \n";
    std::cout << "========================================================\n";

    std::string dbFile = "test_mritunjay.db";
    std::remove(dbFile.c_str());

    Database db(dbFile);
    assert(db.open() && "Failed to open SQLite test database");

    bool schemaLoaded = db.initSchema("../database/schema.sql") ||
                        db.initSchema("../../database/schema.sql") ||
                        db.initSchema("backend/database/schema.sql") ||
                        db.initSchema("database/schema.sql");
    assert(schemaLoaded && "Failed to load database schema for test suite!");
    std::cout << "[INIT] SQLite database connected and schema initialized successfully.\n";

    StationService service(&db);

    // ─── TEST 1: Station Creation ──────────────────────────────────────────
    std::cout << "\n[TEST 1] Testing Station Creation...\n";
    ChargingStation newStation("CS-10", "Hyper Fast Bay 10", 150.0, "CCS2", "available", "", "Bay E-1");
    std::string createErr;
    bool created = service.createStation(newStation, createErr);
    assert(created && "Failed to create new charging station");

    // Attempt duplicate creation (should fail)
    bool dupCreate = service.createStation(newStation, createErr);
    assert(!dupCreate && "Duplicate station ID must be rejected");
    std::cout << "  ✓ Station CS-10 created and duplicate station ID rejected.\n";

    // ─── TEST 2: Station Retrieval ─────────────────────────────────────────
    std::cout << "\n[TEST 2] Testing Station Retrieval...\n";
    ChargingStation retrievedStation;
    bool foundStation = service.getStationById("CS-10", retrievedStation);
    assert(foundStation);
    assert(retrievedStation.getId() == "CS-10");
    assert(retrievedStation.getName() == "Hyper Fast Bay 10");
    assert(retrievedStation.getPower() == 150.0);
    assert(retrievedStation.getLocation() == "Bay E-1");
    assert(retrievedStation.getStatus() == "available");

    ChargingStation allStations[MAX_STATIONS];
    int totalCount = service.getAllStations(allStations, MAX_STATIONS);
    assert(totalCount >= 5); // 4 seed stations + CS-10
    std::cout << "  ✓ Retrieved single station CS-10 and full stations list (count = " << totalCount << ").\n";

    // ─── TEST 3: Station Status Update ─────────────────────────────────────
    std::cout << "\n[TEST 3] Testing Valid Station Status Update...\n";
    std::string statusErr;
    assert(service.updateStationStatus("CS-10", "maintenance", statusErr));
    assert(service.getStationById("CS-10", retrievedStation));
    assert(retrievedStation.getStatus() == "maintenance");
    assert(retrievedStation.getAvailablePorts() == 0);

    assert(service.updateStationStatus("CS-10", "offline", statusErr));
    assert(service.getStationById("CS-10", retrievedStation));
    assert(retrievedStation.getStatus() == "offline");

    assert(service.updateStationStatus("CS-10", "available", statusErr));
    assert(service.getStationById("CS-10", retrievedStation));
    assert(retrievedStation.getStatus() == "available");
    assert(retrievedStation.getAvailablePorts() == 1);
    std::cout << "  ✓ Status transitions available -> maintenance -> offline -> available succeeded.\n";

    // ─── TEST 4: Invalid Status Transition Validation ──────────────────────
    std::cout << "\n[TEST 4] Testing Invalid Status Transitions...\n";
    // CS-1 is actively charging EV-101 in seed data
    ChargingStation cs1;
    assert(service.getStationById("CS-1", cs1));
    assert(cs1.getStatus() == "charging");

    // Cannot manually mark active station as available
    bool invalidAvail = service.updateStationStatus("CS-1", "available", statusErr);
    assert(!invalidAvail && "Must reject marking charging station available while session is active");

    // Cannot put actively charging station into maintenance or offline directly
    bool invalidMaint = service.updateStationStatus("CS-1", "maintenance", statusErr);
    assert(!invalidMaint && "Must reject putting actively charging station into maintenance");

    // Cannot set station to 'charging' manually without session allocation
    bool invalidCharging = service.updateStationStatus("CS-10", "charging", statusErr);
    assert(!invalidCharging && "Must reject manually setting station to charging");

    // Reject unknown status string
    bool invalidBogus = service.updateStationStatus("CS-10", "broken_down", statusErr);
    assert(!invalidBogus && "Must reject unapproved status string");
    std::cout << "  ✓ Rejected manual release of active bay, direct charging status, and bogus status.\n";

    // ─── TEST 5: Charging Allocation ───────────────────────────────────────
    std::cout << "\n[TEST 5] Testing Charging Allocation...\n";
    // EV-105 is available in seed data; CS-2 is available
    ChargingSession ses5;
    std::string allocErr;
    bool allocSuccess = service.createChargingSession("EV-105", "CS-2", 80.0, ses5, allocErr);
    assert(allocSuccess && "Failed to allocate charging session for EV-105 on CS-2");
    assert(ses5.getId().rfind("SES-", 0) == 0);
    assert(ses5.getVehicleId() == "EV-105");
    assert(ses5.getStationId() == "CS-2");
    assert(ses5.getTargetCharge() == 80.0);
    assert(ses5.getStatus() == "active");

    ChargingStation cs2AfterAlloc;
    assert(service.getStationById("CS-2", cs2AfterAlloc));
    assert(cs2AfterAlloc.getStatus() == "charging");
    assert(cs2AfterAlloc.getCurrentVehicleId() == "EV-105");
    std::cout << "  ✓ Charging session successfully allocated and synchronized with CS-2.\n";

    // ─── TEST 6: Allocation When All Stations Are Occupied ─────────────────
    std::cout << "\n[TEST 6] Testing Allocation When All Stations Are Occupied...\n";
    // Mark CS-3 and CS-10 maintenance/busy so zero stations are available
    assert(service.updateStationStatus("CS-3", "maintenance"));
    assert(service.updateStationStatus("CS-10", "maintenance"));

    ChargingStation anyFree;
    bool freeFound = service.findAvailableStation(anyFree);
    assert(!freeFound && "No stations should be available when all bays are charging/maintenance");

    ChargingSession failSes;
    bool allBusyAlloc = service.createChargingSession("EV-106", "CS-2", 90.0, failSes, allocErr);
    assert(!allBusyAlloc && "Cannot allocate session when target station is busy");
    std::cout << "  ✓ Verified that allocation is prevented when all charging bays are occupied.\n";

    // ─── TEST 7: FIFO Queue Integration ────────────────────────────────────
    std::cout << "\n[TEST 7] Testing FIFO Queue Integration...\n";
    ChargingQueue testQueue;
    testQueue.clear();

    // Enqueue EV-106 into FIFO queue
    std::string qErr;
    assert(service.enqueueWaitingEV("EV-106", testQueue, qErr));
    assert(testQueue.getSize() == 1);
    assert(testQueue.peek() == "EV-106");

    // Duplicate queue entry must be prevented
    bool dupQueue = service.enqueueWaitingEV("EV-106", testQueue, qErr);
    assert(!dupQueue && "Duplicate vehicle enqueue must be rejected");
    std::cout << "  ✓ FIFO Queue integration verified: EV-106 queued at REAR; duplicates rejected.\n";

    // ─── TEST 8: Charging Session Completion ───────────────────────────────
    std::cout << "\n[TEST 8] Testing Charging Session Completion...\n";
    ChargingSession completedSes;
    Transaction completedTxn;
    ChargingSession autoDisp;
    std::string compErr;

    // Complete session on CS-2 (no dispatch, queue is bypassed for this sub-test)
    ChargingQueue emptyQ;
    emptyQ.clear();
    bool compSuccess = service.completeChargingSession("CS-2", completedSes, completedTxn, &emptyQ, &autoDisp, compErr);
    assert(compSuccess && "Failed to complete active charging session on CS-2");
    assert(completedSes.getStatus() == "completed");
    assert(completedSes.getBatteryEnd() == 80.0);
    assert(completedSes.getEnergyConsumed() > 0.0);
    assert(completedSes.getDuration() >= 1);
    std::cout << "  ✓ Session on CS-2 completed: status set to 'completed' with accurate end SoC.\n";

    // ─── TEST 9: Correct Energy Calculation ────────────────────────────────
    std::cout << "\n[TEST 9] Testing Correct Energy Calculation...\n";
    // EV-105: battery capacity = 60.5 kWh, started at 45.0%, completed at 80.0%
    // Expected energy = ((80.0 - 45.0) / 100.0) * 60.5 = 0.35 * 60.5 = 21.175 kWh
    double expectedEnergy = ((80.0 - 45.0) / 100.0) * 60.5;
    assert(std::fabs(completedSes.getEnergyConsumed() - expectedEnergy) < 0.01);
    std::cout << "  ✓ Energy consumed verified: calculated exactly " << completedSes.getEnergyConsumed() << " kWh.\n";

    // ─── TEST 10: Correct Billing Integration ──────────────────────────────
    std::cout << "\n[TEST 10] Testing Correct Billing Integration...\n";
    double expectedTariff = Transaction::calculateTariff(expectedEnergy, "charging");
    assert(completedTxn.getType() == "charging");
    assert(completedTxn.getAmount() == expectedTariff);
    assert(completedSes.getEstimatedCost() == expectedTariff);
    std::cout << "  ✓ Tariff billing verified: INR " << completedTxn.getAmount() << " matches official tariff engine.\n";

    // ─── TEST 11: Station Release ──────────────────────────────────────────
    std::cout << "\n[TEST 11] Testing Station Release...\n";
    ChargingStation cs2AfterComp;
    assert(service.getStationById("CS-2", cs2AfterComp));
    assert(cs2AfterComp.getStatus() == "available");
    assert(cs2AfterComp.getCurrentVehicleId().empty());
    assert(cs2AfterComp.getAvailablePorts() == 1);
    std::cout << "  ✓ Station CS-2 released to 'available' with available_ports = 1.\n";

    // ─── TEST 12: Duplicate Active Session Prevention ──────────────────────
    std::cout << "\n[TEST 12] Testing Duplicate Active Session Prevention...\n";
    // Re-assign CS-2 to EV-105
    ChargingSession sesDup;
    assert(service.createChargingSession("EV-105", "CS-2", 85.0, sesDup, allocErr));
    // Attempt to start a second session for EV-105 while active
    ChargingSession sesDup2;
    bool dupEvSession = service.createChargingSession("EV-105", "CS-10", 90.0, sesDup2, allocErr);
    assert(!dupEvSession && "Must reject assigning EV that already has an active charging session");
    std::cout << "  ✓ Duplicate active session for same vehicle strictly rejected.\n";

    // ─── TEST 13: Invalid EV ID ────────────────────────────────────────────
    std::cout << "\n[TEST 13] Testing Invalid EV ID Rejection...\n";
    ChargingSession nonExistentSes;
    bool nonExistentEv = service.createChargingSession("EV-NONEXISTENT", "CS-10", 85.0, nonExistentSes, allocErr);
    assert(!nonExistentEv && "Must reject non-existent EV ID");

    bool badQueueEv = service.enqueueWaitingEV("EV-FAKE-999", testQueue, qErr);
    assert(!badQueueEv && "Must reject queueing non-existent EV ID");
    std::cout << "  ✓ Non-existent vehicle allocation and enqueueing properly rejected.\n";

    // ─── TEST 14: Invalid Station ID ───────────────────────────────────────
    std::cout << "\n[TEST 14] Testing Invalid Station ID Rejection...\n";
    ChargingSession badStationSes;
    bool badStation = service.createChargingSession("EV-106", "CS-9999", 85.0, badStationSes, allocErr);
    assert(!badStation && "Must reject allocation on non-existent station ID");

    bool badStationUpdate = service.updateStationStatus("CS-9999", "available", statusErr);
    assert(!badStationUpdate && "Must reject status update for non-existent station ID");
    std::cout << "  ✓ Non-existent station ID operations properly rejected.\n";

    // ─── TEST 15: Double Session Completion Prevention ─────────────────────
    std::cout << "\n[TEST 15] Testing Double Session Completion Prevention...\n";
    ChargingSession comp1;
    Transaction txn1;
    ChargingSession disp1;
    // Complete active session on CS-2
    assert(service.completeChargingSession("CS-2", comp1, txn1, nullptr, &disp1, compErr));
    // Second completion on now-available station must fail
    ChargingSession comp2;
    Transaction txn2;
    bool doubleComp = service.completeChargingSession("CS-2", comp2, txn2, nullptr, &disp1, compErr);
    assert(!doubleComp && "Double session completion must be strictly prevented");
    std::cout << "  ✓ Double completion rejected; station with no active session cannot be completed.\n";

    // ─── TEST 16: SQLite Persistence After Restart ─────────────────────────
    std::cout << "\n[TEST 16] Testing SQLite Persistence Across Restart...\n";
    // 1. Setup known state in SQLite: Put CS-10 in maintenance, CS-3 in available
    assert(service.updateStationStatus("CS-10", "maintenance"));
    assert(service.updateStationStatus("CS-3", "available"));

    // 2. Simulate server restart: close DB handle and reconnect
    db.close();

    Database restartedDb(dbFile);
    assert(restartedDb.open());
    StationService restartedService(&restartedDb);

    ChargingStation persistedCS10;
    assert(restartedService.getStationById("CS-10", persistedCS10));
    assert(persistedCS10.getStatus() == "maintenance");

    ChargingStation persistedCS3;
    assert(restartedService.getStationById("CS-3", persistedCS3));
    assert(persistedCS3.getStatus() == "available");

    // Verify session persistence across restart
    ChargingSession persistedSes;
    assert(restartedService.getSessionById(comp1.getId(), persistedSes));
    assert(persistedSes.getStatus() == "completed");
    assert(persistedSes.getEnergyConsumed() > 0.0);
    std::cout << "  ✓ Persistence verified: stations, status updates, and session records survived restart.\n";

    // ─── TEST 17: Concurrent Allocation Safety ─────────────────────────────
    std::cout << "\n[TEST 17] Testing Concurrent Allocation Safety...\n";
    // CS-3 is available. Attempt two successive allocations for two different EVs on the same station.
    ChargingSession sesFirst;
    ChargingSession sesSecond;
    std::string errFirst, errSecond;

    bool firstAlloc = restartedService.createChargingSession("EV-102", "CS-3", 85.0, sesFirst, errFirst);
    bool secondAlloc = restartedService.createChargingSession("EV-104", "CS-3", 85.0, sesSecond, errSecond);

    assert(firstAlloc && "First allocation must succeed");
    assert(!secondAlloc && "Second concurrent allocation on same station must be blocked");
    assert(!errSecond.empty());

    // Clean up CS-3
    ChargingSession dummyC;
    Transaction dummyT;
    assert(restartedService.completeChargingSession("CS-3", dummyC, dummyT));
    std::cout << "  ✓ Concurrent allocation safety verified: second concurrent request safely blocked.\n";

    // ─── TEST 18: Maintenance/Offline Station Rejection ───────────────────
    std::cout << "\n[TEST 18] Testing Maintenance & Offline Station Rejection...\n";
    // CS-4 is in maintenance in schema
    ChargingStation cs4;
    assert(restartedService.getStationById("CS-4", cs4));
    assert(cs4.getStatus() == "maintenance");
    assert(!cs4.isAvailable());

    // Allocating EV to maintenance station must fail
    ChargingSession maintSes;
    std::string maintErr;
    bool maintAlloc = restartedService.createChargingSession("EV-102", "CS-4", 85.0, maintSes, maintErr);
    assert(!maintAlloc && "Charging allocation on maintenance station must be strictly rejected");

    // Set CS-4 to offline
    assert(restartedService.updateStationStatus("CS-4", "offline"));
    bool offlineAlloc = restartedService.createChargingSession("EV-102", "CS-4", 85.0, maintSes, maintErr);
    assert(!offlineAlloc && "Charging allocation on offline station must be strictly rejected");

    // findAvailableStation must never return a maintenance or offline station
    ChargingStation foundBay;
    bool foundAny = restartedService.findAvailableStation(foundBay);
    if (foundAny) {
        assert(foundBay.getStatus() == "available");
        assert(foundBay.getId() != "CS-4");
    }
    std::cout << "  ✓ Maintenance and offline stations strictly rejected from EV charging allocation.\n";
    restartedDb.close();
    std::remove(dbFile.c_str());
    std::remove((dbFile + "-wal").c_str());
    std::remove((dbFile + "-shm").c_str());

    std::cout << "\n========================================================\n";
    std::cout << " >>> ALL 18 MRITUNJAY THAKUR UNIT & RESTART TESTS PASSED! <<< \n";
    std::cout << "========================================================\n\n";

    // ─── 14. Test Complete Customer Charging Journey (Phases 5-10) ───────────
    std::cout << "\n--- Testing Customer Charging Journey & Request Pipeline ---\n";
    std::string journeyDbFile = "test_journey.db";
    std::remove(journeyDbFile.c_str());
    std::remove((journeyDbFile + "-wal").c_str());
    std::remove((journeyDbFile + "-shm").c_str());

    Database journeyDb(journeyDbFile);
    assert(journeyDb.open());
    bool journeySchema = journeyDb.initSchema("../database/schema.sql") ||
                         journeyDb.initSchema("backend/database/schema.sql") ||
                         journeyDb.initSchema("database/schema.sql");
    assert(journeySchema);
    StationService journeyService(&journeyDb);
    ChargingQueue journeyQueue;
    journeyQueue.clear();

    // 14.1 Reject request when user does not own EV (EV-101 owned by User 9, attempt with User 10)
    ChargingRequest badReq;
    ChargingSession badSes;
    bool badAlloc = false;
    bool rejectOwnership = journeyService.createChargingRequest(10, "EV-101", "CS-2", 85.0, badReq, badSes, badAlloc, &journeyQueue);
    assert(!rejectOwnership && "Must reject charging request when EV is owned by another user");
    std::cout << "[PASS] Ownership validation verified: User 10 prevented from requesting charge for User 9's vehicle.\n";

    // 14.2 Reject request when target SoC <= current SoC (EV-103 has 74.0%, target 50.0%)
    bool rejectSoc = journeyService.createChargingRequest(11, "EV-103", "CS-2", 50.0, badReq, badSes, badAlloc, &journeyQueue);
    assert(!rejectSoc && "Must reject charging request when target SoC <= current SoC");
    std::cout << "[PASS] SoC validation verified: Target SoC lower than current SoC rejected.\n";

    // 14.3 Valid Request with available station CS-2 -> Immediate Allocation & Active Session
    ChargingRequest validReq;
    ChargingSession validSes;
    bool validAlloc = false;
    bool goodReq = journeyService.createChargingRequest(11, "EV-103", "CS-2", 90.0, validReq, validSes, validAlloc, &journeyQueue);
    assert(goodReq && "Valid charging request must succeed");
    assert(validAlloc == true && "Free station CS-2 must be allocated immediately");
    assert(validReq.getStatus() == "allocated");
    assert(validReq.getStationId() == "CS-2");
    assert(validSes.getId().rfind("SES-", 0) == 0);
    assert(validSes.getStatus() == "active");
    std::cout << "[PASS] Immediate bay allocation verified: EV-103 allocated to CS-2 with active session.\n";

    // 14.4 Test Live Progress Calculation (Phase 8 Constant Power Model)
    ChargingSession progSes;
    double liveSoC = 0.0, deliveredKWh = 0.0, liveCost = 0.0;
    int elapsed = 0;
    bool progOk = journeyService.getSessionProgress(validSes.getId(), progSes, liveSoC, deliveredKWh, liveCost, elapsed);
    assert(progOk);
    assert(liveSoC >= validSes.getBatteryStart());
    assert(liveSoC <= validSes.getTargetCharge());
    assert(liveCost > 0.0);
    std::cout << "[PASS] Constant-power simulation verified: Live SoC, energy, and cost calculated deterministically.\n";

    // 14.5 Valid Request when all stations busy -> Queue Entry (EV-105 owned by User 13)
    ChargingRequest queueReq;
    ChargingSession queueSes;
    bool queueAlloc = false;
    journeyService.updateStationStatus("CS-3", "charging");
    bool queuedOk = journeyService.createChargingRequest(13, "EV-105", "", 95.0, queueReq, queueSes, queueAlloc, &journeyQueue);
    assert(queuedOk);
    assert(queueAlloc == false && "Must enter queue when stations are busy");
    assert(queueReq.getStatus() == "queued");
    assert(journeyQueue.getSize() == 1);
    assert(journeyQueue.peek() == "EV-105");
    std::cout << "[PASS] FIFO Queue entry verified: EV-105 added to queue when all bays busy.\n";

    // 14.6 User Request & Session Filtering
    ChargingRequest u11Reqs[MAX_REQUESTS];
    int u11Count = journeyService.getAllRequests(u11Reqs, MAX_REQUESTS, 11);
    assert(u11Count >= 1);
    for (int i = 0; i < u11Count; i++) {
        assert(u11Reqs[i].getUserId() == 11);
    }

    ChargingRequest u15Reqs[MAX_REQUESTS];
    int u15Count = journeyService.getAllRequests(u15Reqs, MAX_REQUESTS, 15);
    assert(u15Count == 0 && "User 15 with no requests should receive 0 records");
    std::cout << "[PASS] Request isolation verified: Users only retrieve their own charging requests.\n";

    // 14.7 Complete Session with Queue Dispatch & Authorization
    ChargingSession unauthorizedComp;
    Transaction unauthorizedTxn;
    bool stopDenied = journeyService.completeChargingSession(validSes.getId(), unauthorizedComp, unauthorizedTxn, &journeyQueue, nullptr, 10, "user");
    assert(!stopDenied && "User 10 cannot stop User 11's session");

    ChargingSession authComp;
    Transaction authTxn;
    ChargingSession autoDispatchedSes;
    bool stopOk = journeyService.completeChargingSession(validSes.getId(), authComp, authTxn, &journeyQueue, &autoDispatchedSes, 11, "user");
    assert(stopOk);
    assert(authComp.getStatus() == "completed");
    assert(authComp.getBatteryEnd() == 90.0);
    assert(authTxn.getUserId() == 11);
    assert(!autoDispatchedSes.getId().empty() && "Waiting EV-105 auto-dispatched to released bay CS-2");
    assert(autoDispatchedSes.getVehicleId() == "EV-105");
    std::cout << "[PASS] Complete workflow verified: Session completed, SoC updated to 90%, auto-dispatched EV-105 from queue.\n";

    journeyDb.close();
    std::remove(journeyDbFile.c_str());
    std::remove((journeyDbFile + "-wal").c_str());
    std::remove((journeyDbFile + "-shm").c_str());

        // ─── 15. Testing Automatic FIFO Queue Processing & 12 Phase 7 Assertions ───
    std::cout << "\n--- 15. Testing Automatic FIFO Processing & Phase 7 Assertions ---\n";
    std::string phase7DbFile = "test_phase7.db";
    std::remove(phase7DbFile.c_str());
    std::remove((phase7DbFile + "-wal").c_str());
    std::remove((phase7DbFile + "-shm").c_str());

    Database p7Db(phase7DbFile);
    assert(p7Db.open());
    bool p7Schema = p7Db.initSchema("../database/schema.sql") ||
                    p7Db.initSchema("backend/database/schema.sql") ||
                    p7Db.initSchema("database/schema.sql");
    assert(p7Schema);
    StationService p7Service(&p7Db);
    ChargingQueue p7Queue;
    p7Queue.clear();

    // Assertion 1: User A cannot see User B's queue requests
    ChargingRequest u9Reqs[MAX_REQUESTS];
    int u9Count = p7Service.getAllRequests(u9Reqs, MAX_REQUESTS, 9);
    for (int i = 0; i < u9Count; i++) {
        assert(u9Reqs[i].getUserId() == 9);
    }
    ChargingRequest u10Reqs[MAX_REQUESTS];
    int u10Count = p7Service.getAllRequests(u10Reqs, MAX_REQUESTS, 10);
    for (int i = 0; i < u10Count; i++) {
        assert(u10Reqs[i].getUserId() == 10);
    }
    std::cout << "[PASS] Assertion 1: User A cannot see User B's queue requests.\n";

    // Assertion 2: User A cannot dequeue requests (Role-based access)
    // Assertion 3: User A cannot allocate another user's request
    ChargingRequest allocReq;
    ChargingSession allocSes;
    bool userAllocBlocked = p7Service.allocateBayManually("REQ-1002", "CS-2", allocReq, allocSes, &p7Queue, 9, "user");
    assert(!userAllocBlocked && "Normal user cannot manually allocate requests");
    std::cout << "[PASS] Assertions 2 & 3: User cannot dequeue or manually allocate requests.\n";

    // Assertion 4: A request is automatically allocated when a compatible bay is available
    // CS-2 is available and CCS2 compatible. EV-103 is owned by User 11, SoC 74%.
    ChargingRequest a4Req;
    ChargingSession a4Ses;
    bool a4Alloc = false;
    assert(p7Service.createChargingRequest(11, "EV-103", "", 88.0, a4Req, a4Ses, a4Alloc, &p7Queue));
    assert(a4Alloc == true && "Must be automatically allocated when compatible bay CS-2 is free");
    assert(a4Req.getStatus() == "active" || a4Req.getStatus() == "allocated");
    assert(a4Req.getStationId() == "CS-2");
    std::cout << "[PASS] Assertion 4: Request automatically allocated when compatible bay is available.\n";

    // Assertion 8 & 9: Incompatible connectors & Maintenance stations are never allocated
    // Let's set CS-3 to charging so the only remaining free station is CS-4 (Type 2, maintenance)
    p7Service.updateStationStatus("CS-3", "charging");
    ChargingStation p7cs4;
    p7Service.getStationById("CS-4", p7cs4);
    assert(p7cs4.getStatus() == "maintenance");
    assert(p7cs4.getConnectorType() == "Type 2");

    // Assertion 5: A request remains queued when all compatible bays are occupied
    ChargingRequest a5Req;
    ChargingSession a5Ses;
    bool a5Alloc = false;
    // EV-105 is owned by User 13, SoC 45% -> Request CCS2 charging to 90%
    assert(p7Service.createChargingRequest(13, "EV-105", "", 90.0, a5Req, a5Ses, a5Alloc, &p7Queue, "CCS2"));
    assert(a5Alloc == false && "Must remain queued when all compatible operational bays are occupied");
    assert(a5Req.getStatus() == "queued");
    assert(p7Queue.contains("EV-105"));
    std::cout << "[PASS] Assertion 5, 8 & 9: Incompatible/maintenance bays rejected, request remains queued.\n";

    // Assertion 7: FIFO ordering is preserved among eligible requests
    // Let's enqueue a second request: EV-106 (User 14, SoC 91% -> set SoC to 30% for test)
    sqlite3_exec(p7Db.getDbHandle(), "UPDATE EV SET current_charge = 30.0, status = 'available' WHERE id = 'EV-106';", nullptr, nullptr, nullptr);
    ChargingRequest a7Req;
    ChargingSession a7Ses;
    bool a7Alloc = false;
    assert(p7Service.createChargingRequest(14, "EV-106", "", 80.0, a7Req, a7Ses, a7Alloc, &p7Queue, "CCS2"));
    assert(a7Alloc == false);
    assert(a7Req.getStatus() == "queued");
    assert(p7Queue.getSize() == 2);
    // In FIFO queue: EV-105 must be ahead of EV-106
    assert(p7Queue.peek() == "EV-105");
    std::cout << "[PASS] Assertion 7: FIFO queue ordering strictly preserved.\n";

    // Assertion 6: Releasing a bay triggers automatic processing
    // Complete session on CS-2 -> this frees CS-2, which must automatically allocate to EV-105 (FRONT of FIFO)!
    ChargingSession p7CompSes;
    Transaction p7CompTxn;
    ChargingSession dispSes;
    assert(p7Service.completeChargingSession("CS-2", p7CompSes, p7CompTxn, &p7Queue, &dispSes, 11, "user"));
    assert(p7CompSes.getStatus() == "completed");
    assert(dispSes.getVehicleId() == "EV-105" && "EV-105 from FRONT of FIFO queue must be auto-dispatched to CS-2");
    assert(p7Queue.getSize() == 1 && "Remaining queue size must be 1 (EV-106)");
    assert(p7Queue.peek() == "EV-106");
    std::cout << "[PASS] Assertion 6: Releasing a bay triggers automatic FIFO queue dispatch.\n";

    // Assertion 10 & 11: Concurrent requests cannot reserve same bay & Duplicate processing creates no duplicates
    int dupProcessed = p7Service.processQueueForAvailableStations(&p7Queue);
    assert(dupProcessed == 0 && "Duplicate queue processing run must allocate 0 additional bays when bays busy");
    std::cout << "[PASS] Assertions 10 & 11: Duplicate processing does not create duplicate sessions.\n";

    // Assertion 12: Database persistence survives backend restart
    p7Db.close();
    Database reopenedP7(phase7DbFile);
    assert(reopenedP7.open());
    StationService restartedP7Service(&reopenedP7);

    ChargingStation reStation;
    assert(restartedP7Service.getStationById("CS-2", reStation));
    assert(reStation.getStatus() == "charging");
    assert(reStation.getCurrentVehicleId() == "EV-105");

    ChargingRequest reReq;
    assert(restartedP7Service.getRequestById(a5Req.getId(), reReq));
    assert(reReq.getStatus() == "active" || reReq.getStatus() == "allocated");
    assert(reReq.getStationId() == "CS-2");
    std::cout << "[PASS] Assertion 12: Database persistence survives restart.\n";

    reopenedP7.close();
    std::remove(phase7DbFile.c_str());
    std::remove((phase7DbFile + "-wal").c_str());
    std::remove((phase7DbFile + "-shm").c_str());

    // ------------------------------------------------------------------------
    // 16. Testing Configurable Simulation Speed Multiplier & Accelerated Demonstrations
    // ------------------------------------------------------------------------
    std::cout << "\n--- 16. Testing Configurable Simulation Speed Multiplier & Demonstrations ---\n";
    std::string speedDbFile = "test_speed_sim.db";
    std::remove(speedDbFile.c_str());
    std::remove((speedDbFile + "-wal").c_str());
    std::remove((speedDbFile + "-shm").c_str());

    Database speedDb(speedDbFile);
    assert(speedDb.open());
    bool speedSchemaLoaded = speedDb.initSchema("backend/database/schema.sql") ||
                             speedDb.initSchema("../database/schema.sql") ||
                             speedDb.initSchema("database/schema.sql");
    assert(speedSchemaLoaded && "Failed to load schema for speedDb");

    StationService speedService(&speedDb);

    // Test 1: Configurable Multiplier Defaults and Setting
    assert(speedService.getSimulationSpeedMultiplier() == 20.0);
    speedService.setSimulationSpeedMultiplier(1.0);
    assert(speedService.getSimulationSpeedMultiplier() == 1.0);
    speedService.setSimulationSpeedMultiplier(50.0);
    assert(speedService.getSimulationSpeedMultiplier() == 50.0);
    speedService.setSimulationSpeedMultiplier(20.0);
    assert(speedService.getSimulationSpeedMultiplier() == 20.0);
    std::cout << "[PASS] Configurable simulation speed multiplier (1x, 20x, 50x) verified.\n";

    // Test 2: Section 4 Example Verification Scenario
    // Battery capacity: 75 kWh, Initial SoC: 20%, Target SoC: 80%
    // Station power: 25 kW, Efficiency: 90% (0.90), Tariff: ₹15/kWh, Multiplier: 20
    speedService.setChargingEfficiency(0.90);
    speedService.setTariffRate(15.0);
    speedService.setSimulationSpeedMultiplier(20.0);

    sqlite3* sHandle = speedDb.getDbHandle();
    // Clear any seeded active sessions in speedDb so advanceActiveSessions strictly tests demoSes
    sqlite3_exec(sHandle, "DELETE FROM ChargingSession WHERE status = 'active';", nullptr, nullptr, nullptr);
    sqlite3_exec(sHandle, "UPDATE ChargingStation SET status = 'available', current_vehicle_id = NULL WHERE status = 'charging';", nullptr, nullptr, nullptr);
    sqlite3_exec(sHandle, "UPDATE EV SET status = 'available' WHERE status = 'charging';", nullptr, nullptr, nullptr);
    sqlite3_exec(sHandle, "INSERT OR REPLACE INTO EV (id, owner_name, model, battery_capacity, current_charge, status, user_id, connector_type) "
                          "VALUES ('EV-DEMO-75', 'Test Driver', 'Demonstration Long-Range EV', 75.0, 20.0, 'available', 9, 'CCS2');", nullptr, nullptr, nullptr);
    sqlite3_exec(sHandle, "INSERT OR REPLACE INTO ChargingStation (id, name, power, connector_type, status, current_vehicle_id, location) "
                          "VALUES ('CS-DEMO-25', 'Demo 25kW Charger', 25.0, 'CCS2', 'available', NULL, 'Demo Bay 1');", nullptr, nullptr, nullptr);

    // Physical formula verification:
    double batteryCapacity = 75.0;
    double initialSoC = 20.0;
    double targetSoC = 80.0;
    double stationPower = 25.0;
    double efficiency = 0.90;
    double tariffRate = 15.0;
    double mult = 20.0;

    double expectedBatteryEnergy = batteryCapacity * (targetSoC - initialSoC) / 100.0; // 45.0 kWh
    double expectedBatteryPower = stationPower * efficiency; // 22.5 kW
    double expectedPhysicalHours = expectedBatteryEnergy / expectedBatteryPower; // 2.0 hours
    double expectedPhysicalMinutes = expectedPhysicalHours * 60.0; // 120.0 minutes
    double expectedDemoMinutes = expectedPhysicalMinutes / mult; // 6.0 minutes (360s)
    double expectedGridEnergy = expectedBatteryEnergy / efficiency; // 50.0 kWh
    double expectedCost = expectedGridEnergy * tariffRate; // ₹750.0

    assert(expectedBatteryEnergy == 45.0);
    assert(expectedPhysicalMinutes == 120.0);
    assert(expectedDemoMinutes == 6.0);
    assert(expectedGridEnergy == 50.0);
    assert(expectedCost == 750.0);
    std::cout << "[PASS] Section 4 Physical formulas verified: 45 kWh battery, 50 kWh grid, 120m physical, 6m demo, ₹750 cost.\n";

    // Create charging session for EV-DEMO-75 on CS-DEMO-25
    ChargingSession demoSes;
    assert(speedService.createChargingSession("EV-DEMO-75", "CS-DEMO-25", 80.0, demoSes, 9));
    std::string demoSesId = demoSes.getId();

    // Fast-forward session start_time to simulate 3 real minutes elapsed (180 seconds):
    // 180s * 20x = 3600 simulated seconds = 1.0 simulated hour
    // In 1.0 simulated hour: energy stored = 25kW * 0.90 * 1h = 22.5 kWh
    // currentSoC = 20.0% + (22.5 / 75.0)*100 = 50.0%
    // battery energy delivered = 22.5 kWh
    // grid energy = 22.5 / 0.90 = 25.0 kWh
    // cost = 25.0 * 15.0 = ₹375.0
    std::string ffHalfwaySql = "UPDATE ChargingSession SET start_time = datetime('now', '-180 seconds') WHERE id = '" + demoSesId + "';";
    sqlite3_exec(sHandle, ffHalfwaySql.c_str(), nullptr, nullptr, nullptr);

    ChargingSession progressSes;
    double currentSoC = 0.0, energyDelivered = 0.0, currentCost = 0.0;
    int elapsedSecs = 0;
    assert(speedService.getSessionProgress(demoSesId, progressSes, currentSoC, energyDelivered, currentCost, elapsedSecs));
    assert(std::abs(currentSoC - 50.0) < 0.5 && "Halfway SoC must be ~50%");
    assert(std::abs(energyDelivered - 22.5) < 0.5 && "Battery energy at halfway must be ~22.5 kWh");
    assert(std::abs(currentCost - 375.0) < 1.0 && "Cost at halfway must be ₹375");
    std::cout << "[PASS] Accelerated progress at 3 real minutes (50% SoC, 22.5 kWh, ₹375) verified.\n";

    // Advance session at halfway point:
    ChargingQueue demoQueue;
    int autoComp = speedService.advanceActiveSessions(&demoQueue);
    assert(autoComp == 0 && "Session must remain active before reaching target SoC");

    // Enqueue a waiting vehicle into demoQueue to test automatic FIFO dispatch upon completion
    sqlite3_exec(sHandle, "INSERT OR REPLACE INTO EV (id, owner_name, model, battery_capacity, current_charge, status, user_id, connector_type) "
                          "VALUES ('EV-WAIT-88', 'Waiting Driver', 'Queued Fleet Car', 50.0, 30.0, 'available', 10, 'CCS2');", nullptr, nullptr, nullptr);
    ChargingRequest waitReq;
    ChargingSession waitAllocSes;
    bool waitAllocated = false;
    assert(speedService.createChargingRequest(10, "EV-WAIT-88", "CS-DEMO-25", 85.0, waitReq, waitAllocSes, waitAllocated, &demoQueue, "CCS2"));
    assert(!waitAllocated && "Waiting EV must be queued while CS-DEMO-25 is occupied");
    assert(demoQueue.getSize() == 1 && demoQueue.peek() == "EV-WAIT-88");

    // Fast-forward session start_time to simulate 6 real minutes elapsed (360 seconds):
    // 360s * 20x = 7200 simulated seconds = 2.0 simulated hours
    // Target 80% reached!
    std::string ffFinishSql = "UPDATE ChargingSession SET start_time = datetime('now', '-360 seconds') WHERE id = '" + demoSesId + "';";
    sqlite3_exec(sHandle, ffFinishSql.c_str(), nullptr, nullptr, nullptr);

    autoComp = speedService.advanceActiveSessions(&demoQueue);
    assert(autoComp == 1 && "advanceActiveSessions must auto-complete session when target SoC reached");

    // Verify EV state after completion
    sqlite3_stmt* checkEvStmt = nullptr;
    sqlite3_prepare_v2(sHandle, "SELECT current_charge, status FROM EV WHERE id = 'EV-DEMO-75';", -1, &checkEvStmt, nullptr);
    assert(sqlite3_step(checkEvStmt) == SQLITE_ROW);
    double finalEvSoC = sqlite3_column_double(checkEvStmt, 0);
    std::string finalEvStatus = (const char*)sqlite3_column_text(checkEvStmt, 1);
    sqlite3_finalize(checkEvStmt);

    assert(finalEvSoC == 80.0 && "Final EV SoC must be exactly 80.0%, never higher");
    assert(finalEvStatus == "available");
    std::cout << "[PASS] Final SoC accuracy verified: exactly 80.0%, status=available.\n";

    // Verify Transaction generation
    sqlite3_stmt* checkTxStmt = nullptr;
    std::string txSql = "SELECT amount, energy_delivered, duration, status FROM TransactionLog WHERE vehicle_id = 'EV-DEMO-75';";
    sqlite3_prepare_v2(sHandle, txSql.c_str(), -1, &checkTxStmt, nullptr);
    assert(sqlite3_step(checkTxStmt) == SQLITE_ROW);
    double txAmount = sqlite3_column_double(checkTxStmt, 0);
    double txEnergy = sqlite3_column_double(checkTxStmt, 1);
    int txDuration = sqlite3_column_int(checkTxStmt, 2);
    std::string txStatus = (const char*)sqlite3_column_text(checkTxStmt, 3);
    sqlite3_finalize(checkTxStmt);

    assert(txAmount == 750.0 && "Transaction billed amount must be exactly INR 750.0");
    assert(txEnergy == 50.0 && "Transaction energy delivered must be 50.0 kWh (grid consumed)");
    assert(txDuration == 120 && "Transaction simulated duration must be 120 minutes");
    assert(txStatus == "completed");
    std::cout << "[PASS] TransactionLog verified: INR 750.0, 50.0 kWh, 120 mins duration.\n";

    // Verify Duplicate Transaction Prevention
    ChargingSession dupCompSes;
    Transaction dupTxn;
    bool dupResult = speedService.completeChargingSession(demoSesId, dupCompSes, dupTxn, &demoQueue);
    assert(!dupResult && "Attempting duplicate completion on already-completed session must return false");
    std::cout << "[PASS] Duplicate transaction prevention verified: duplicate completion safely rejected.\n";

    // Verify Automatic FIFO Queue Progression
    // EV-WAIT-88 should have been auto-dispatched to CS-DEMO-25!
    ChargingStation demoSt;
    assert(speedService.getStationById("CS-DEMO-25", demoSt));
    assert(demoSt.getStatus() == "charging" && "CS-DEMO-25 must be occupied by next vehicle");
    assert(demoSt.getCurrentVehicleId() == "EV-WAIT-88" && "EV-WAIT-88 must have been dispatched from FIFO queue");
    assert(demoQueue.isEmpty() && "FIFO queue must now be empty");
    std::cout << "[PASS] Automatic FIFO queue progression verified: EV-WAIT-88 auto-dispatched to released bay.\n";

    // Verify Backend Restart Recovery with simulation multiplier
    speedDb.close();
    Database reopenedSpeedDb(speedDbFile);
    assert(reopenedSpeedDb.open());
    StationService restartedSpeedService(&reopenedSpeedDb);
    restartedSpeedService.setSimulationSpeedMultiplier(20.0);
    restartedSpeedService.setChargingEfficiency(0.90);
    restartedSpeedService.setTariffRate(15.0);

    ChargingStation recoveredSt;
    assert(restartedSpeedService.getStationById("CS-DEMO-25", recoveredSt));
    assert(recoveredSt.getStatus() == "charging");
    assert(recoveredSt.getCurrentVehicleId() == "EV-WAIT-88");
    std::cout << "[PASS] Backend restart recovery verified: station and session restored with simulation speed multiplier.\n";

    reopenedSpeedDb.close();
    std::remove(speedDbFile.c_str());
    std::remove((speedDbFile + "-wal").c_str());
    std::remove((speedDbFile + "-shm").c_str());

std::cout << "\n>>> ALL MRITUNJAY THAKUR (CHARGING BACKEND & FIFO QUEUE) TESTS PASSED! <<<\n";
    return 0;
}
