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

    std::cout << "\n>>> ALL TESTS PASSED FOR PRIYANSHU DUTT! <<<\n";
    return 0;
}
