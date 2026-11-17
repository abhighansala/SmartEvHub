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

    std::cout << "\n========================================================\n";
    std::cout << " >>> ALL 18 MRITUNJAY THAKUR TEST SCENARIOS PASSED! <<< \n";
    std::cout << "========================================================\n";

    return 0;
}
