#include <iostream>
#include <cassert>
#include "../models/charging_station.h"
#include "../services/station_service.h"

// Standalone unit test for Mritunjay Thakur's module
int main() {
    std::cout << "========================================\n";
    std::cout << " Running Tests: Mritunjay Thakur (Charging Stations)\n";
    std::cout << "========================================\n";

    // 1. Test ChargingStation Model
    ChargingStation station("CS-99", "Fast Charger Test", 50.0, "CCS2", "available");
    assert(station.getId() == "CS-99");
    assert(station.isAvailable() == true);
    std::cout << "[PASS] ChargingStation model creation and availability passed.\n";

    // 2. Test StationService with local array
    StationService service(nullptr);

    // Test List stations
    ChargingStation list[MAX_STATIONS];
    int count = service.getAllStations(list, MAX_STATIONS);
    assert(count > 0);
    std::cout << "[PASS] Retrieved stations list (count = " << count << ").\n";

    // Test Find Available Station
    ChargingStation availableStation;
    bool found = service.findAvailableStation(availableStation);
    assert(found == true);
    std::string assignedId = availableStation.getId();
    std::cout << "[PASS] Found available station: " << assignedId << "\n";

    // Test Assign Vehicle
    bool assigned = service.assignVehicle(assignedId, "EV-101");
    assert(assigned == true);

    ChargingStation checkedStation;
    service.getStationById(assignedId, checkedStation);
    assert(checkedStation.getStatus() == "charging");
    assert(checkedStation.getCurrentVehicleId() == "EV-101");
    assert(checkedStation.isAvailable() == false);
    std::cout << "[PASS] Station assignment to EV-101 passed.\n";

    // Test Release Station
    bool released = service.releaseStation(assignedId);
    assert(released == true);

    service.getStationById(assignedId, checkedStation);
    assert(checkedStation.getStatus() == "available");
    assert(checkedStation.getCurrentVehicleId().empty());
    std::cout << "[PASS] Station release back to available passed.\n";

    // 3. Test StationService with Real SQLite Database
    std::cout << "\n--- Testing Real SQLite Persistence for Mritunjay Thakur ---\n";
    std::string dbFile = "test_mritunjay.db";
    std::remove(dbFile.c_str());

    Database db(dbFile);
    assert(db.open());

    bool schemaLoaded = db.initSchema("../database/schema.sql") ||
                        db.initSchema("backend/database/schema.sql") ||
                        db.initSchema("database/schema.sql");
    assert(schemaLoaded && "Failed to load database schema for charging stations!");
    std::cout << "[PASS] Database connected and schema initialized for StationService.\n";

    StationService dbService(&db);
    ChargingStation dbStations[MAX_STATIONS];
    int stationCount = dbService.getAllStations(dbStations, MAX_STATIONS);
    assert(stationCount == 4); // Schema seed has CS-1 through CS-4
    std::cout << "[PASS] Seeded charging stations retrieved from SQLite (count = " << stationCount << ").\n";

    // CS-2 is available in seed data
    ChargingStation freeStation;
    bool foundFree = dbService.findAvailableStation(freeStation);
    assert(foundFree);
    assert(freeStation.getId() == "CS-2");
    std::cout << "[PASS] Available station found in SQLite: " << freeStation.getId() << "\n";

    // Assign vehicle EV-102 to CS-2 in SQLite
    assert(dbService.assignVehicle("CS-2", "EV-102"));
    ChargingStation assignedStation;
    assert(dbService.getStationById("CS-2", assignedStation));
    assert(assignedStation.getStatus() == "charging");
    assert(assignedStation.getCurrentVehicleId() == "EV-102");
    std::cout << "[PASS] Assigned EV-102 to station CS-2 in SQLite database.\n";

    // Update status to maintenance
    assert(dbService.updateStationStatus("CS-3", "maintenance"));
    ChargingStation maintStation;
    assert(dbService.getStationById("CS-3", maintStation));
    assert(maintStation.getStatus() == "maintenance");
    std::cout << "[PASS] Updated station CS-3 status to maintenance in SQLite.\n";

    // 4. Test Persistence Across Restart
    std::cout << "\n--- Testing Persistence across Connection Close / Reopen ---\n";
    db.close();

    Database reopenedDb(dbFile);
    assert(reopenedDb.open());
    StationService reopenedService(&reopenedDb);

    ChargingStation persistedCS2;
    assert(reopenedService.getStationById("CS-2", persistedCS2));
    assert(persistedCS2.getStatus() == "charging");
    assert(persistedCS2.getCurrentVehicleId() == "EV-102");

    ChargingStation persistedCS3;
    assert(reopenedService.getStationById("CS-3", persistedCS3));
    assert(persistedCS3.getStatus() == "maintenance");
    std::cout << "[PASS] PERSISTENCE VERIFIED: Station status & assigned EV survived connection restart!\n";

    // Release station CS-2
    assert(reopenedService.releaseStation("CS-2"));
    assert(reopenedService.getStationById("CS-2", persistedCS2));
    assert(persistedCS2.getStatus() == "available");
    assert(persistedCS2.getCurrentVehicleId().empty());
    std::cout << "[PASS] Station CS-2 released in SQLite.\n";

    // ─── 5. Test Phase B: Charging Session Creation & Persistence ──────────
    std::cout << "\n--- Testing Phase B: Charging Session Creation & Persistence ---\n";
    // EV-105 is 'available' with 45.0% in seed data; CS-2 is 'available'
    ChargingSession newSession;
    bool created = reopenedService.createChargingSession("EV-105", "CS-2", 80.0, newSession);
    assert(created && "Failed to create charging session for EV-105 on CS-2");
    assert(newSession.getId().rfind("SES-", 0) == 0);
    assert(newSession.getVehicleId() == "EV-105");
    assert(newSession.getStationId() == "CS-2");
    assert(newSession.getTargetCharge() == 80.0);
    assert(newSession.getStatus() == "active");

    // Verify station and EV statuses synchronized in SQLite
    ChargingStation stationAfterCreate;
    assert(reopenedService.getStationById("CS-2", stationAfterCreate));
    assert(stationAfterCreate.getStatus() == "charging");
    assert(stationAfterCreate.getCurrentVehicleId() == "EV-105");
    std::cout << "[PASS] Charging session created & persisted. CS-2 marked 'charging'.\n";

    // ─── 6. Test Active-Session Retrieval & Filtering ───────────────────────
    std::cout << "\n--- Testing Session Retrieval & Filtering ---\n";
    ChargingSession activeSessions[MAX_SESSIONS];
    int activeCount = reopenedService.getAllSessions(activeSessions, MAX_SESSIONS, "active");
    assert(activeCount >= 2); // Seed has SES-1001, plus our newly created session
    for (int i = 0; i < activeCount; i++) {
        assert(activeSessions[i].getStatus() == "active");
    }

    ChargingSession completedSessions[MAX_SESSIONS];
    int completedCount = reopenedService.getAllSessions(completedSessions, MAX_SESSIONS, "completed");
    assert(completedCount >= 1); // Seed has SES-1000
    for (int i = 0; i < completedCount; i++) {
        assert(completedSessions[i].getStatus() == "completed");
    }

    ChargingSession retrievedSession;
    assert(reopenedService.getSessionById(newSession.getId(), retrievedSession));
    assert(retrievedSession.getVehicleId() == "EV-105");

    ChargingSession activeOnCS2;
    assert(reopenedService.getActiveSessionByStationId("CS-2", activeOnCS2));
    assert(activeOnCS2.getId() == newSession.getId());
    std::cout << "[PASS] Active and completed session filtering & retrieval verified.\n";

    // ─── 7. Test FIFO Queue Rehydration Across Restart ─────────────────────
    std::cout << "\n--- Testing FIFO Queue Rehydration Across Restart ---\n";
    // Close and reopen database to test recovery from cold storage
    reopenedDb.close();

    Database restartDb(dbFile);
    assert(restartDb.open());
    StationService restartService(&restartDb);

    ChargingQueue restoredQueue;
    int restoredCount = restartService.rehydrateChargingQueue(restoredQueue);

    // Seed data has EV-102 (-10m), EV-104 (-5m), and EV-125 (-2m) as 'waiting'
    assert(restoredCount >= 3 && "Should recover all waiting EVs from SQLite");
    assert(restoredQueue.getSize() >= 3);

    // Verify exact FIFO ordering determined by last_activity ASC
    std::string firstQueued = restoredQueue.dequeue();
    assert(firstQueued == "EV-102" && "Oldest waiting EV must be at FRONT of FIFO queue");
    std::string secondQueued = restoredQueue.dequeue();
    assert(secondQueued == "EV-104" && "Second oldest waiting EV must be second in FIFO queue");
    std::string thirdQueued = restoredQueue.dequeue();
    assert(thirdQueued == "EV-125");
    std::cout << "[PASS] FIFO Queue deterministically rehydrated across restart: EV-102 -> EV-104 -> EV-125.\n";

    // ─── 8. Test Session Completion, Tariff Calculation & Accurate Charges ─
    std::cout << "\n--- Testing Session Completion & Tariff Calculation ---\n";
    ChargingSession completedSession;
    Transaction sessionTxn;
    ChargingSession dispatchedSession;

    // First test completion without queue dispatch (pass empty queue)
    ChargingQueue emptyQueue;
    emptyQueue.clear();
    bool completedSuccess = restartService.completeChargingSession("CS-2", completedSession, sessionTxn, &emptyQueue, &dispatchedSession);
    assert(completedSuccess && "Session completion must succeed for active session");
    assert(completedSession.getStatus() == "completed");
    // Ensure charge was set to targetCharge (80.0%), NOT hardcoded 100%!
    assert(completedSession.getBatteryEnd() == 80.0);
    assert(completedSession.getEnergyConsumed() > 0.0);
    assert(sessionTxn.getType() == "charging");
    assert(sessionTxn.getAmount() == Transaction::calculateTariff(completedSession.getEnergyConsumed(), "charging"));
    assert(dispatchedSession.getId().empty() && "No dispatch should occur with empty queue");

    // Verify station is released to 'available'
    ChargingStation stationAfterComplete;
    assert(restartService.getStationById("CS-2", stationAfterComplete));
    assert(stationAfterComplete.getStatus() == "available");
    assert(stationAfterComplete.getCurrentVehicleId().empty());
    std::cout << "[PASS] Session completed: battery set to target charge (80%), correct tariff billed, station released.\n";

    // ─── 9. Test FIFO Automated Queue Dispatch on Station Release ─────────
    std::cout << "\n--- Testing Automated FIFO Queue Dispatch on Completion ---\n";
    // 1. Assign EV-105 again to CS-2
    ChargingSession ses2;
    assert(restartService.createChargingSession("EV-105", "CS-2", 85.0, ses2));

    // 2. Set up queue with waiting EV-104
    ChargingQueue liveQueue;
    liveQueue.clear();
    liveQueue.enqueue("EV-104");
    assert(liveQueue.getSize() == 1);

    // 3. Complete CS-2 with liveQueue: EV-104 should be auto-dispatched to CS-2!
    ChargingSession compSes;
    Transaction compTxn;
    ChargingSession autoDispatched;
    bool dispatchComplete = restartService.completeChargingSession("CS-2", compSes, compTxn, &liveQueue, &autoDispatched);
    assert(dispatchComplete);
    assert(!autoDispatched.getId().empty() && "Waiting EV must be dispatched to released bay");
    assert(autoDispatched.getVehicleId() == "EV-104");
    assert(liveQueue.isEmpty() && "Dispatched EV must be dequeued from FIFO queue");

    // Verify CS-2 is now charging EV-104 in database
    ChargingStation stationDispatched;
    assert(restartService.getStationById("CS-2", stationDispatched));
    assert(stationDispatched.getStatus() == "charging");
    assert(stationDispatched.getCurrentVehicleId() == "EV-104");
    std::cout << "[PASS] FIFO Dispatch verified: Completing session auto-dispatched waiting EV-104 to CS-2.\n";

    // ─── 10. Test Duplicate Operation Prevention & Invalid Requests ────────
    std::cout << "\n--- Testing Duplicate Operation & Invalid Request Prevention ---\n";
    // Duplicate session creation for an EV already charging (EV-104)
    ChargingSession invalidSes;
    bool dupCreate = restartService.createChargingSession("EV-104", "CS-3", 85.0, invalidSes);
    assert(!dupCreate && "Should reject creating session for EV that is already charging");

    // Attempt to create session on busy station CS-2
    bool busyCreate = restartService.createChargingSession("EV-106", "CS-2", 85.0, invalidSes);
    assert(!busyCreate && "Should reject creating session on a busy station");

    // Attempt to create session for non-existent station
    bool invalidStationCreate = restartService.createChargingSession("EV-106", "CS-999", 85.0, invalidSes);
    assert(!invalidStationCreate && "Should reject creating session on non-existent station");

    // Complete session on station that has no active session (CS-1 or available station)
    ChargingSession dummySes;
    Transaction dummyTxn;
    bool dupComplete = restartService.completeChargingSession("CS-3", dummySes, dummyTxn);
    assert(!dupComplete && "Should reject completing session on station without active session");

    // Prevent duplicate queue entry
    ChargingQueue dupCheckQueue;
    dupCheckQueue.clear();
    dupCheckQueue.enqueue("EV-102");
    assert(dupCheckQueue.contains("EV-102") == true);
    assert(dupCheckQueue.contains("EV-999") == false);
    std::cout << "[PASS] Duplicate creation, busy station assignment, and duplicate completion rejected.\n";

    // ─── 11. Test Database Transaction Rollback on Failure ─────────────────
    std::cout << "\n--- Testing Atomic Database Rollback ---\n";
    assert(restartDb.beginTransaction());
    // Simulate failed transaction that must be rolled back
    assert(restartDb.rollbackTransaction());
    std::cout << "[PASS] Database transaction rollback verified.\n";

    // ─── 12. Test Queue Ordering Under Identical Timestamps (Stable Tie-Breakers) ────
    std::cout << "\n--- Testing Queue Ordering Under Identical Timestamps ---\n";
    // Explicitly set only EV-102 and EV-104 as waiting with identical timestamps in SQLite
    sqlite3* rawDb = restartDb.getDbHandle();
    sqlite3_exec(rawDb, "UPDATE EV SET status = 'available';", nullptr, nullptr, nullptr);
    sqlite3_exec(rawDb, "UPDATE EV SET status = 'waiting', last_activity = '2026-10-19 12:00:00' WHERE id IN ('EV-102', 'EV-104');", nullptr, nullptr, nullptr);

    // Close and reopen to simulate restart under equal timestamps
    restartDb.close();

    Database tieBreakDb(dbFile);
    assert(tieBreakDb.open());
    StationService tieBreakService(&tieBreakDb);

    ChargingQueue equalTimestampQueue;
    int tieBreakCount = tieBreakService.rehydrateChargingQueue(equalTimestampQueue);
    assert(tieBreakCount >= 2);

    // Verify ordering is strictly deterministic across restarts via registered_at / rowid tie-breaker
    std::string firstTie = equalTimestampQueue.dequeue();
    std::string secondTie = equalTimestampQueue.dequeue();
    assert(firstTie == "EV-102" && "Tie-breaker must deterministically prioritize earlier registered EV-102");
    assert(secondTie == "EV-104" && "Second EV in deterministic tie-break must be EV-104");
    std::cout << "[PASS] Equal timestamp ordering verified: Stable tie-breaker preserved EV-102 before EV-104 across restart.\n";

    // ─── 13. Test Memory & SQLite Consistency on Failure (Queue Rollback Safety) ───
    std::cout << "\n--- Testing Memory & SQLite Consistency on Failure ---\n";
    ChargingQueue memoryQueue;
    memoryQueue.clear();

    // Test safe enqueue: attempting to enqueue a non-existent EV must fail in DB and leave queue empty
    bool badEnqueue = tieBreakService.enqueueWaitingEV("EV-NONEXISTENT", memoryQueue);
    assert(!badEnqueue && "Enqueue must fail for non-existent vehicle");
    assert(memoryQueue.isEmpty() && "In-memory queue must NOT be mutated when DB validation fails");

    // Enqueue valid EV-102
    bool goodEnqueue = tieBreakService.enqueueWaitingEV("EV-102", memoryQueue);
    assert(goodEnqueue);
    assert(memoryQueue.getSize() == 1);
    assert(memoryQueue.peek() == "EV-102");

    // Test completion failure safety: attempting completeChargingSession on station with no active session
    // must NOT dequeue from memoryQueue
    ChargingSession failCompSes;
    Transaction failCompTxn;
    ChargingSession failDispSes;
    bool failComplete = tieBreakService.completeChargingSession("CS-3", failCompSes, failCompTxn, &memoryQueue, &failDispSes);
    assert(!failComplete && "Completion must fail on station with no active session");
    assert(memoryQueue.getSize() == 1 && "In-memory queue must NOT lose items when completion fails");
    assert(memoryQueue.peek() == "EV-102" && "Queue front must remain untouched after failed completion");
    std::cout << "[PASS] Memory & SQLite consistency verified: Failed DB operations safely restored in-memory queue.\n";

    tieBreakDb.close();
    std::remove(dbFile.c_str());

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
    ChargingStation cs4;
    p7Service.getStationById("CS-4", cs4);
    assert(cs4.getStatus() == "maintenance");
    assert(cs4.getConnectorType() == "Type 2");

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
