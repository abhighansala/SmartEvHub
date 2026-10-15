#include <iostream>
#include <cassert>
#include "../models/battery.h"
#include "../services/battery_service.h"

// Standalone unit test for Amit Singh Kaintura's module
int main() {
    std::cout << "========================================\n";
    std::cout << " Running Tests: Amit Singh Kaintura (Battery & Swap)\n";
    std::cout << "========================================\n";

    // 1. Test Battery Model
    Battery b1("BAT-TEST", 50.0, 100.0, "available", 99.0);
    assert(b1.getId() == "BAT-TEST");
    assert(b1.isReady() == true);
    std::cout << "[PASS] Battery model creation and isReady() check passed.\n";

    // Test battery with low charge is not ready
    Battery bLow("BAT-LOW", 50.0, 20.0, "available", 99.0);
    assert(bLow.isReady() == false);
    std::cout << "[PASS] Depleted battery isReady() rejection passed.\n";

    // 2. Test BatteryService (using local array)
    BatteryService service(nullptr);

    // Test list batteries
    Battery list[MAX_BATTERIES];
    int count = service.getAllBatteries(list, MAX_BATTERIES);
    assert(count > 0);
    std::cout << "[PASS] Retrieved battery inventory (count = " << count << ").\n";

    // Test find ready battery
    Battery readyBat;
    bool found = service.findReadyBattery(readyBat);
    assert(found == true);
    assert(readyBat.isReady() == true);
    std::cout << "[PASS] Found ready battery for swap: " << readyBat.getId() << "\n";

    // Test perform swap
    Battery assignedBattery;
    bool swapDone = service.performSwap("EV-102", assignedBattery);
    assert(swapDone == true);
    assert(assignedBattery.getStatus() == "inuse");
    assert(assignedBattery.getCurrentVehicle() == "EV-102");
    std::cout << "[PASS] Performed battery swap for EV-102 with " << assignedBattery.getId() << "\n";

    std::cout << "\n>>> ALL TESTS PASSED FOR AMIT SINGH KAINTURA! <<<\n";
    return 0;
}
