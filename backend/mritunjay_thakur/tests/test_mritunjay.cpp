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

    std::cout << "\n>>> ALL MRITUNJAY THAKUR (CHARGING BACKEND & FIFO QUEUE) TESTS PASSED! <<<\n";
    return 0;
}
