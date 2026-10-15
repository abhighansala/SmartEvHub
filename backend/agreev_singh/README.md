# Agreev Singh — Module Starter Guide
**Role:** Transactions + Reports + Analytics Owner  
**Student ID:** 2518161 (F)  
**Folder:** `backend/agreev_singh/`

---

## 1. Responsibilities Overview
As the Transactions & Reporting owner:
1. Implement the `Transaction` domain model representing completed charging sessions and battery swaps.
2. Implement `TransactionService` using simple C++ fixed arrays (no vectors) to record transactions and calculate summary statistics:
   - Total Revenue (INR)
   - Total Energy Delivered (kWh)
   - Total Completed Sessions
   - Average Waiting Time (Minutes)
3. Expose REST endpoints for Transactions and Recharts visualizations:
   - `GET /api/transactions` (List transaction history)
   - `POST /api/transactions` (Record completed transaction)
   - `GET /api/reports` (High-level operational KPI summaries)
   - `GET /api/analytics` (Formatted chart datasets for peak hours & station utilization)

---

## 2. Directory Structure
```
agreev_singh/
├── README.md               # This guide
├── models/
│   └── transaction.h       # Transaction class (id, vehicleId, type, amount, energy, timestamp)
├── services/
│   ├── transaction_service.h # Transaction service header (using simple arrays)
│   └── transaction_service.cpp # Revenue, energy, and session calculations
├── api/
│   ├── transaction_routes.h # Transaction routes header
│   ├── transaction_routes.cpp # GET and POST /api/transactions
│   ├── analytics_routes.h   # Reports and Analytics routes header
│   └── analytics_routes.cpp # /api/reports and /api/analytics
└── tests/
    └── test_agreev.cpp     # Standalone unit test
```

---

## 3. API Endpoints You Own

### A. List Transactions
- **Endpoint:** `GET /api/transactions`
- **Response (HTTP 200):**
  ```json
  [
    {
      "id": "TXN-5001",
      "evId": "EV-103",
      "type": "charging",
      "stationOrBattery": "CS-2",
      "description": "Fast charging 20% to 80%",
      "amount": 870.0,
      "energyDelivered": 43.5,
      "duration": 60,
      "date": "2026-10-15 14:30:00",
      "status": "completed"
    }
  ]
  ```

### B. Reports Summary
- **Endpoint:** `GET /api/reports`
- **Response (HTTP 200):**
  ```json
  {
    "totalRevenue": 2000.0,
    "totalEnergyDelivered": 142.9,
    "totalSessions": 3,
    "averageWaitingTime": 8.5,
    "stationUtilization": 84.5
  }
  ```

---

## 4. How to Test Your Code Standalone

From the `backend/` folder:
```bash
mkdir -p build && cd build
cmake ..
cmake --build . --target test_agreev_singh
./test_agreev_singh
```

Or using `g++` directly:
```bash
cd agreev_singh/tests
g++ -std=c++17 test_agreev.cpp ../services/transaction_service.cpp ../../aditya_singh/database/database.cpp ../../src/sqlite3.c -I../../include -I../models -I../services -I../../aditya_singh/database -lpthread -ldl -o test_agreev
./test_agreev
```
