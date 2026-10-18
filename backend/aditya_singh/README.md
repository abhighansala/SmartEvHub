# Aditya Singh — Database & Backend Foundation Lead
**Role:** Database + Backend Foundation (Team Lead)  
**Student ID:** 2519272 (ML1)  
**Module Directory:** `backend/aditya_singh/`  
**Shared Project Repository:** [Smart EV Hub](https://github.com/abhighansala/SmartEvHub.git)

---

## 1. Responsibilities & Ownership (Per PRD)
As the **Database & Backend Foundation Lead**:
1. **Database Schema & Initialization:** Designed and maintained SQLite schema (`backend/database/schema.sql`) covering `Users`, `EV`, `ChargingStation`, `Battery`, `ChargingSession`, and `TransactionLog`.
2. **Database Engine & CRUD Helpers:** Implemented the `Database` C++ class providing connection lifecycle management, automated WAL mode, foreign-key enforcement, schema initialization, and transactional control (`beginTransaction`, `commitTransaction`, `rollbackTransaction`).
3. **User Authentication:** Built `User` OOP model and `AuthService` handling secure login validation (supporting email or username), password verification, registration, and duplicate prevention.
4. **REST API Routing:** Exposed authentication endpoints via Crow (`POST /api/login`, `POST /api/logout`, `POST /api/register`, `GET /api/users`) with CORS configuration.
5. **Frontend Authentication Service:** Implemented `src/services/authService.ts` and connected `src/pages/Login.tsx` to authenticate against SQLite with seamless fallback handling.
6. **Cross-Module Foundation:** Ensured database interfaces seamlessly support teammates' services (`EVService`, `StationService`, `BatteryService`, `TransactionService`).

---

## 2. Directory Structure
```text
backend/aditya_singh/
├── README.md               # This module & viva guide
├── database/
│   ├── database.h          # Database wrapper with transaction & query helpers
│   └── database.cpp        # SQLite connection, WAL mode, transaction execution
├── models/
│   └── user.h              # User OOP entity (encapsulation, validation)
├── services/
│   ├── auth_service.h      # AuthService declarations
│   └── auth_service.cpp    # Login, registration, and user retrieval queries
├── api/
│   ├── auth_routes.h       # Crow route declarations
│   └── auth_routes.cpp     # POST /api/login, /api/logout, /api/register, GET /api/users
└── tests/
    └── test_aditya.cpp     # Comprehensive unit & restart persistence test runner
```

---

## 3. Database Architecture & Schema Design
The persistent SQLite database file `evhub.db` is initialized using `backend/database/schema.sql`:

- **Users:** `id` (PK AUTOINCREMENT), `name`, `email` (UNIQUE), `password`, `role` (`admin`, `operator`, `user`), `created_at`.
- **EV:** `id` (PK), `owner_name`, `model`, `battery_capacity`, `current_charge`, `status`, `last_activity`, `registered_at`.
- **ChargingStation:** `id` (PK), `name`, `power`, `connector_type`, `status`, `current_vehicle_id` (FK), `session_start`, `location`, `total_ports`, `available_ports`.
- **Battery:** `id` (PK), `capacity`, `charge_level`, `status`, `health`, `cycle_count`, `current_vehicle` (FK), `last_updated`.
- **ChargingSession:** `id` (PK), `vehicle_id` (FK), `ev_model`, `station_id` (FK), `start_time`, `end_time`, `battery_start`, `battery_end`, `target_charge`, `energy_consumed`, `estimated_cost`, `status`.
- **TransactionLog:** `id` (PK), `vehicle_id` (FK), `type` (`charging` / `battery-swap`), `station_or_battery`, `description`, `amount`, `energy_delivered`, `duration`, `timestamp`, `status`.

### Connection Configuration & Concurrency:
- `PRAGMA foreign_keys = ON;` — guarantees referential integrity.
- `PRAGMA journal_mode = WAL;` — Write-Ahead Logging allows concurrent reads while writing transactions.

---

## 4. API Endpoints Owned by Aditya Singh

### A. Login (`POST /api/login`)
Authenticates users by either email or registered username.
- **Request Body (JSON):**
  ```json
  {
    "email": "admin@evhub.com",
    "password": "admin123"
  }
  ```
- **Response (Success - HTTP 200):**
  ```json
  {
    "success": true,
    "message": "Login successful",
    "user": {
      "id": 1,
      "name": "Admin User",
      "email": "admin@evhub.com",
      "role": "admin"
    }
  }
  ```
- **Response (Failure - HTTP 401):**
  ```json
  {
    "success": false,
    "message": "Invalid credentials"
  }
  ```

### B. Logout (`POST /api/logout`)
Clears active backend/frontend session.
- **Response (HTTP 200):**
  ```json
  {
    "success": true,
    "message": "Logged out successfully"
  }
  ```

### C. User Registration (`POST /api/register`)
- **Request Body (JSON):**
  ```json
  {
    "name": "Campus Operator",
    "email": "operator_b@evhub.com",
    "password": "securepassword",
    "role": "operator"
  }
  ```
- **Response (HTTP 201):**
  ```json
  {
    "success": true,
    "message": "User registered successfully"
  }
  ```

### D. User Verification Listing (`GET /api/users`)
Returns all registered accounts in SQLite for administrative inspection.

---

## 5. Verification & Testing Instructions

### Compiling & Running Standalone Unit and Persistence Tests
From `backend/aditya_singh/tests`:

```powershell
# 1. Compile SQLite (C) and the test runner (C++17)
gcc -c ../../src/sqlite3.c -I../../include -o sqlite3.o
g++ -std=c++17 test_aditya.cpp ../database/database.cpp ../services/auth_service.cpp sqlite3.o -I../../include -I../database -I../models -I../services -o test_aditya.exe

# 2. Execute test suite
.\test_aditya.exe
```

### Verified Test Suite Steps:
1. `[Step 1] Database Connection` — Verified SQLite connection handle and `isOpen()`.
2. `[Step 2] Schema & Seed Initialization` — Verified all 6 database tables created.
3. `[Step 3] Transaction Management` — Verified `beginTransaction()`, `commitTransaction()`, and `rollbackTransaction()`.
4. `[Step 4] User Authentication` — Verified valid email login, valid username login, wrong password rejection, unknown user rejection, and empty field rejection.
5. `[Step 5] Registration & Constraints` — Verified new user creation and UNIQUE email duplicate rejection.
6. `[Step 6] Persistence Restart Verification` — Closed connection, simulated server restart, reopened SQLite file, and verified registered user persisted and logged in successfully.

---

## 6. Academic Viva Defense Preparation

**Q1: Why use SQLite for an embedded EV management system?**  
*Answer:* SQLite is serverless, zero-configuration, and stores the entire database in a single cross-platform disk file (`evhub.db`). With WAL (Write-Ahead Logging) mode and foreign key constraints enabled, it delivers ACID compliance with minimal memory overhead suitable for C++ embedded hubs.

**Q2: How does your module protect passwords?**  
*Answer:* In memory, `outUser.setPassword("")` immediately clears password fields from the `User` object after authentication so credentials are not exposed in memory or serialized in REST API responses.

**Q3: How do transactions prevent partial database corruption during sessions?**  
*Answer:* When a charging session completes, slot release, request status update, and transaction logging must occur together. The `beginTransaction()`, `commitTransaction()`, and `rollbackTransaction()` helpers ensure atomic execution—if any step fails, changes roll back completely.

**Q4: How does the frontend handle backend unavailability?**  
*Answer:* `src/services/authService.ts` encapsulates the REST fetch call. If the C++ Crow server is offline, it gracefully falls back to verified mock credentials while alerting the operator, guaranteeing uninterrupted demonstration flow.
