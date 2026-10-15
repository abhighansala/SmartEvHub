# Mritunjay Thakur — Module Starter Guide
**Role:** Charging Stations Module Owner  
**Student ID:** 2519425 (ML5)  
**Folder:** `backend/mritunjay_thakur/`

---

## 1. Responsibilities Overview
As the Charging Stations owner:
1. Model the `ChargingStation` domain class with operational states:
   - `available`: Bay is open and ready for an EV.
   - `charging`: Bay is actively charging an assigned EV.
   - `maintenance`: Bay is undergoing servicing.
   - `offline`: Bay is powered off or disconnected.
2. Model the `ChargingSession` class to track active and completed charging events.
3. Implement `StationService` using simple C++ arrays (no vectors) to assign, release, and check bays.
4. Expose REST endpoints:
   - `GET /api/stations` (List all stations)
   - `GET /api/stations/:id` (Get single station details)
   - `PUT /api/stations/:id/status` (Update station status)
   - `POST /api/charging/request` (Request charging slot)
   - `POST /api/charging/complete` (Complete charge & free up station)

---

## 2. Directory Structure
```
mritunjay_thakur/
├── README.md               # This guide
├── models/
│   ├── charging_station.h  # Charging station domain class
│   └── charging_session.h  # Charging session domain class
├── services/
│   ├── station_service.h   # Station service header (using simple arrays)
│   └── station_service.cpp # Availability, assignment, release logic
├── api/
│   ├── station_routes.h    # Station REST API routes
│   ├── station_routes.cpp  # Station endpoint handlers
│   ├── charging_routes.h   # Charging flow routes
│   └── charging_routes.cpp # Request and complete handlers
└── tests/
    └── test_mritunjay.cpp  # Standalone unit test
```

---

## 3. API Endpoints You Own

### A. List Stations
- **Endpoint:** `GET /api/stations`
- **Response (HTTP 200):**
  ```json
  [
    {
      "id": "CS-1",
      "name": "DC Fast Charger 1",
      "power": 60.0,
      "connectorType": "CCS2",
      "status": "available",
      "currentEV": "",
      "location": "Bay A-1"
    }
  ]
  ```

### B. Request Charging Bay
- **Endpoint:** `POST /api/charging/request`
- **Request Body (JSON):**
  ```json
  {
    "evId": "EV-102"
  }
  ```
- **Response (Success - Assigned):**
  ```json
  {
    "success": true,
    "assigned": true,
    "stationId": "CS-2",
    "message": "Assigned to Station CS-2"
  }
  ```

---

## 4. How to Test Your Code Standalone

From the `backend/` folder:
```bash
mkdir -p build && cd build
cmake ..
cmake --build . --target test_mritunjay_thakur
./test_mritunjay_thakur
```

Or using `g++` directly:
```bash
cd mritunjay_thakur/tests
g++ -std=c++17 test_mritunjay.cpp ../services/station_service.cpp ../../aditya_singh/database/database.cpp ../../src/sqlite3.c -I../../include -I../models -I../services -I../../aditya_singh/database -lpthread -ldl -o test_mritunjay
./test_mritunjay
```
