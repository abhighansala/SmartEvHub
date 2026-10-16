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

    std::cout << "\n========================================================\n";
    std::cout << " >>> ALL 9 TEST SUITES PASSED FOR AMIT SINGH KAINTURA! <<<\n";
    std::cout << "========================================================\n";
    return 0;
}
