# Amit Singh Kaintura — Module Starter Guide
**Role:** Battery + Battery Swap Module Owner  
**Student ID:** 2518188 (G)  
**Folder:** `backend/amit_singh_kaintura/`

---

## 1. Responsibilities Overview
As the Battery & Swap owner:
1. Model the `Battery` class representing swap inventory:
   - `available`: Battery is fully charged (>=95%) and ready to be swapped into a car.
   - `inuse`: Battery is currently installed in a customer EV.
   - `charging`: Depleted battery is currently charging in a swap bay.
   - `maintenance`: Battery is degraded or being repaired.
2. Implement `BatteryService` using simple fixed-size arrays (no vectors) to check inventory and perform swaps.
3. Expose REST endpoints:
   - `GET /api/batteries` (List all batteries in inventory)
   - `GET /api/batteries/:id` (Get single battery details)
   - `POST /api/swap/request` (Request an immediate swap; queue if inventory is empty)
   - `POST /api/swap/process` (Process next swap from the queue)

---

## 2. Directory Structure
```
amit_singh_kaintura/
├── README.md               # This guide
├── models/
│   ├── battery.h           # Battery domain class (id, capacity, chargeLevel, status, health)
│   └── swap_request.h      # Swap request class
├── services/
│   ├── battery_service.h   # Battery inventory service (using simple arrays)
│   └── battery_service.cpp # Ready check and swap logic
├── api/
│   ├── battery_routes.h    # Battery inventory routes header
│   ├── battery_routes.cpp  # Battery GET routes
│   ├── swap_routes.h       # Swap workflow routes header
│   └── swap_routes.cpp     # POST /api/swap/request and /process
└── tests/
    └── test_amit.cpp       # Standalone unit test
```

---

## 3. API Endpoints You Own

### A. List Battery Inventory
- **Endpoint:** `GET /api/batteries`
- **Response (HTTP 200):**
  ```json
  [
    {
      "id": "BAT-01",
      "capacity": 50.0,
      "currentCharge": 100.0,
      "status": "available",
      "health": 98.5,
      "cycleCount": 42,
      "currentVehicle": ""
    }
  ]
  ```

### B. Request Battery Swap
- **Endpoint:** `POST /api/swap/request`
- **Request Body (JSON):**
  ```json
  {
    "evId": "EV-102"
  }
  ```
- **Response (Success - Swapped):**
  ```json
  {
    "success": true,
    "swapped": true,
    "batteryId": "BAT-01",
    "chargeLevel": 100.0,
    "message": "Swap completed! Assigned battery BAT-01"
  }
  ```
- **Response (If Inventory Exhausted):**
  ```json
  {
    "success": true,
    "swapped": false,
    "queued": true,
    "message": "No charged batteries available in inventory. Added to Swap Queue."
  }
  ```

---

## 4. How to Test Your Code Standalone

From the `backend/` folder:
```bash
mkdir -p build && cd build
cmake ..
cmake --build . --target test_amit_singh_kaintura
./test_amit_singh_kaintura
```

Or using `g++` directly:
```bash
cd amit_singh_kaintura/tests
g++ -std=c++17 test_amit.cpp ../services/battery_service.cpp ../../aditya_singh/database/database.cpp ../../src/sqlite3.c -I../../include -I../models -I../services -I../../aditya_singh/database -lpthread -ldl -o test_amit
./test_amit
```
