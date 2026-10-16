# Agreev Singh — Transactions & Analytics Module Guide

**Student Name:** Agreev Singh  
**Student ID:** 2518161  
**Section:** F  
**Role:** Role 6 — Transactions, Reports & Analytics Owner  
**Module Directory:** `backend/agreev_singh/`

---

## 1. Role & Responsibilities Overview

As the owner of the **Transactions & Operational Analytics** module in the **Smart EV Hub** project:

1. **Transaction Domain Model (`models/transaction.h`):**
   - Represents completed charging sessions and battery swaps.
   - Encapsulates transaction attributes (`id`, `vehicleId`, `type`, `stationOrBattery`, `description`, `amount`, `energyDelivered`, `duration`, `timestamp`, `status`).
   - Implements input validation (`isValid()`), electricity tariff calculation (`calculateTariff()`), operator overloading (`==`, `!=`), and receipt printing.

2. **Transaction Service (`services/transaction_service.h` / `.cpp`):**
   - Stores transactions using fixed-size C++ arrays (`Transaction transactionArray[MAX_TRANSACTIONS]`) without relying on `std::vector` (adhering to project guidelines for second-year DSA simplicity).
   - Integrates with SQLite database (`evhub.db` via `Database* db`) with automatic in-memory fallback.
   - Provides linear search by transaction ID, vehicle ID, and operation type.
   - Computes operational Key Performance Indicators (KPIs): Total Revenue (₹), Total Energy Delivered (kWh), Total Sessions, Charging vs. Swap Session Counts, Average Duration, and Hub Utilization.
   - Computes peak/extremum metrics (e.g., highest revenue transaction).

3. **REST API via Crow C++ Microframework (`api/`):**
   - Serializes data into JSON format to communicate with the React frontend.
   - `GET /api/transactions`: Lists transactions with optional query filters (`?type=` or `?evId=`).
   - `GET /api/transactions/<id>`: Fetches individual transaction details by ID.
   - `POST /api/transactions`: Parses JSON payload, auto-generates ID if needed, applies tariff rates, and records new transactions.
   - `GET /api/reports`: Delivers real-time KPI data consumed by the 6 StatCards on the React Reports page (`Reports.tsx`).
   - `GET /api/analytics`: Formats operational data for Recharts components (`weeklyData`, `stationUtilization`, `peakHours`, and `revenueSummary`).

4. **Standalone Unit Testing (`tests/test_agreev.cpp`):**
   - Self-contained test suite validating model construction, array bounds, linear search, filters, and KPI calculations.

---

## 2. Academic Syllabus Mapping

This module directly applies concepts from the **Data Structures** and **Object-Oriented Programming with C++ (PCS307)** lab syllabi:

| Syllabus Reference | Concept Applied | Where in Agreev's Code |
| :--- | :--- | :--- |
| **OOP Lab 5 & 7** | Data Abstraction & Encapsulation | Private data members with protected access through public getters/setters in `Transaction`. |
| **OOP Lab 1 & 10** | Tariff & Electricity Billing Calculation | `Transaction::calculateTariff(energy, type)` implementing tiered energy rates and swap fees. |
| **OOP Lab 14** | Operator Overloading | Overloaded `operator==` and `operator!=` comparing transactions by unique identifier. |
| **OOP Lab 6 & 7** | Formatted Output & Receipts | `displayReceipt()` displaying formatted transaction slips. |
| **OOP Lab 4 & DS Week 1** | Best Index / Extremum Finding | `getHighestRevenueTransaction()` performing linear scan to identify peak transactions. |
| **DS Week 1 & 2** | Fixed Arrays & Sequential Traversal | Fixed-size array `transactionArray[MAX_TRANSACTIONS]` managed without standard vectors. |
| **DS Week 1 & 2** | Linear Search Algorithm | `getTransactionById()` and filtering queries iterating linearly through arrays ($O(N)$). |

---

## 3. Directory Layout

```text
backend/agreev_singh/
├── README.md                     # Comprehensive module documentation (this file)
├── models/
│   └── transaction.h             # Transaction class (encapsulation, validation, tariff, operators)
├── services/
│   ├── transaction_service.h     # Service header (fixed array, database pointer, analytics)
│   └── transaction_service.cpp   # Implementation of queries, linear searches, and KPI stats
├── api/
│   ├── transaction_routes.h      # Crow REST routes header for transactions
│   ├── transaction_routes.cpp    # GET & POST /api/transactions endpoints
│   ├── analytics_routes.h        # Crow REST routes header for reports & analytics
│   └── analytics_routes.cpp      # GET /api/reports and GET /api/analytics endpoints
└── tests/
    └── test_agreev.cpp           # Complete standalone unit test suite
```

---

## 4. REST API Reference (Crow & React Integration)

Crow parses incoming HTTP requests and serializes outgoing data as JSON using `crow::json::wvalue`.

### A. List Transactions
- **Endpoint:** `GET /api/transactions`
- **Query Parameters:**
  - `type`: Optional filter (`charging` or `battery-swap`)
  - `evId`: Optional vehicle filter (e.g., `EV-103`)
- **Response (HTTP 200 OK):**
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
  },
  {
    "id": "TXN-5002",
    "evId": "EV-105",
    "type": "battery-swap",
    "stationOrBattery": "BAT-03",
    "description": "Swapped depleted battery with BAT-03 (100%)",
    "amount": 450.0,
    "energyDelivered": 60.0,
    "duration": 5,
    "date": "2026-10-15 13:00:00",
    "status": "completed"
  }
]
```

---

### B. Get Single Transaction
- **Endpoint:** `GET /api/transactions/:id`
- **Example:** `GET /api/transactions/TXN-5001`
- **Response (HTTP 200 OK):**
```json
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
```

---

### C. Record New Transaction
- **Endpoint:** `POST /api/transactions`
- **Request Body (JSON):**
```json
{
  "evId": "EV-101",
  "type": "charging",
  "stationOrBattery": "CS-1",
  "description": "Standard fast charge 20% to 80%",
  "amount": 540.0,
  "energyDelivered": 30.0,
  "duration": 45
}
```
- **Response (HTTP 201 Created):**
```json
{
  "success": true,
  "message": "Transaction recorded successfully",
  "id": "TXN-5005",
  "amount": 540.0
}
```

---

### D. Operational Reports KPI Summary
- **Endpoint:** `GET /api/reports`
- **Response (HTTP 200 OK):**
```json
{
  "totalRevenue": 2610.0,
  "totalEnergyDelivered": 170.9,
  "totalSessions": 4,
  "chargingSessions": 3,
  "batterySwaps": 1,
  "averageDuration": 36.25,
  "averageWaitingTime": 8.5,
  "stationUtilization": 84.5,
  "activeBays": 3,
  "totalBays": 4,
  "topTransaction": {
    "id": "TXN-5001",
    "amount": 870.0,
    "evId": "EV-103"
  }
}
```

---

### E. Recharts Visual Analytics
- **Endpoint:** `GET /api/analytics`
- **Response (HTTP 200 OK):**
```json
{
  "weeklyData": [
    { "day": "Mon", "sessions": 12, "swaps": 4 },
    { "day": "Tue", "sessions": 15, "swaps": 6 },
    { "day": "Wed", "sessions": 10, "swaps": 3 },
    { "day": "Thu", "sessions": 18, "swaps": 7 },
    { "day": "Fri", "sessions": 22, "swaps": 5 },
    { "day": "Sat", "sessions": 25, "swaps": 9 },
    { "day": "Sun", "sessions": 20, "swaps": 8 }
  ],
  "stationUtilization": [
    { "station": "S01", "utilization": 65 },
    { "station": "S02", "utilization": 85 },
    { "station": "S03", "utilization": 72 },
    { "station": "S04", "utilization": 90 },
    { "station": "S05", "utilization": 78 },
    { "station": "S06", "utilization": 45 }
  ],
  "peakHours": [
    { "hour": "08:00", "sessions": 3, "energy": 65.0 },
    { "hour": "10:00", "sessions": 7, "energy": 150.0 },
    { "hour": "12:00", "sessions": 12, "energy": 240.0 },
    { "hour": "14:00", "sessions": 10, "energy": 210.0 },
    { "hour": "16:00", "sessions": 15, "energy": 310.0 },
    { "hour": "18:00", "sessions": 8, "energy": 180.0 }
  ],
  "revenueSummary": {
    "chargingRevenue": 2070.0,
    "swapRevenue": 450.0,
    "totalRevenue": 2520.0
  }
}
```

---

## 5. How to Compile and Run

### Run Standalone Unit Tests Directly
From `backend/agreev_singh/tests/`:
```bash
cd backend/agreev_singh/tests

# 1. Compile SQLite C object
gcc -c ../../src/sqlite3.c -I../../include -o sqlite3.o

# 2. Compile and link test runner
g++ -std=c++17 test_agreev.cpp ../services/transaction_service.cpp ../../aditya_singh/database/database.cpp sqlite3.o -I../../include -I../models -I../services -I../../aditya_singh/database -lpthread -ldl -o test_agreev

# 3. Execute
./test_agreev
```

### Build with CMake
From `backend/`:
```bash
cd backend
mkdir -p build && cd build
cmake ..
cmake --build . --target test_agreev_singh
./test_agreev_singh
```
