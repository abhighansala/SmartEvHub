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

    std::cout << "\n>>> ALL TESTS PASSED FOR MRITUNJAY THAKUR! <<<\n";
    return 0;
}
