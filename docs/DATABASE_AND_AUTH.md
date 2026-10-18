# Smart EV Hub — Database Architecture & Authentication Specification
**Module Owner:** Aditya Singh (Team Lead — Database & Backend Foundation)  
**Student ID:** 2519272 (Section ML1)  
**Course:** 2nd-Year B.Tech CSE Project-Based Learning  

---

## 1. Overview
Smart EV Hub requires robust, ACID-compliant persistence for electric vehicle records, charging bays, battery swap inventory, active sessions, and financial transaction records. 

Aditya Singh designed and implemented the unified database layer and backend authentication foundation:
- SQLite embedded database (`evhub.db`) with Write-Ahead Logging (`WAL`) mode and foreign key constraints.
- Object-Oriented C++ `Database` wrapper class with connection management, schema initialization, and transaction management (`BEGIN`, `COMMIT`, `ROLLBACK`).
- User authentication and role-based access control (`admin`, `operator`, `user`).
- REST APIs implemented with Crow C++ (`POST /api/login`, `POST /api/logout`, `POST /api/register`, `GET /api/users`).
- Frontend authentication service (`src/services/authService.ts`) and updated `src/pages/Login.tsx` with Material 3 styling and quick demo accounts.

---

## 2. Entity-Relationship & Schema Model

```
       +-----------------------+
       |         Users         |
       +-----------------------+
       | id (PK)               |
       | name                  |
       | email (UNIQUE)        |
       | password              |
       | role                  |
       | created_at            |
       +-----------------------+

       +-----------------------+          +-----------------------+
       |          EV           |          |    ChargingStation    |
       +-----------------------+          +-----------------------+
       | id (PK)               |<---------| current_vehicle_id(FK)|
       | owner_name            |          | id (PK)               |
       | model                 |          | power                 |
       | battery_capacity      |          | connector_type        |
       | current_charge        |          | status                |
       | status                |          +-----------------------+
       +-----------------------+                      |
           ^               ^                          |
           |               |                          |
           | 1:N           | 1:N                      | 1:N
           |               |                          |
+---------------------+ +----------------------+      |
|       Battery       | |   ChargingSession    |<-----+
+---------------------+ +----------------------+
| id (PK)             | | id (PK)              |
| capacity            | | vehicle_id (FK)      |
| charge_level        | | station_id (FK)      |
| status              | | battery_start / end  |
| health              | | target_charge        |
| current_vehicle(FK) | | energy_consumed      |
+---------------------+ | estimated_cost       |
                        | status               |
                        +----------------------+
                                   |
                                   | 1:1
                                   v
                        +----------------------+
                        |    TransactionLog    |
                        +----------------------+
                        | id (PK)              |
                        | vehicle_id (FK)      |
                        | type                 |
                        | amount               |
                        | timestamp            |
                        +----------------------+
```

---

## 3. SQLite Concurrency & Reliability
To ensure the multi-threaded Crow server operates reliably:
1. **Foreign Keys:** `PRAGMA foreign_keys = ON;` is executed on every connection to prevent dangling references.
2. **Write-Ahead Logging:** `PRAGMA journal_mode = WAL;` enables concurrent read operations without blocking write transactions.
3. **Transaction Safety:** Charging completion and battery swaps involve multi-table modifications executed within atomic transactions:
   ```cpp
   db.beginTransaction();
   // 1. Release station
   // 2. Complete session
   // 3. Log transaction
   db.commitTransaction();
   ```

---

## 4. REST API Specification

### 1. `POST /api/login`
- **Request:**
  ```json
  {
    "email": "aditya@evhub.com",
    "password": "aditya"
  }
  ```
- **Response (200 OK):**
  ```json
  {
    "success": true,
    "message": "Login successful",
    "user": {
      "id": 4,
      "name": "Aditya Singh",
      "email": "aditya@evhub.com",
      "role": "admin"
    }
  }
  ```

### 2. `POST /api/logout`
- **Response (200 OK):**
  ```json
  {
    "success": true,
    "message": "Logged out successfully"
  }
  ```

### 3. `POST /api/register`
- **Request:**
  ```json
  {
    "name": "New Operator",
    "email": "operator2@evhub.com",
    "password": "pass",
    "role": "operator"
  }
  ```
- **Response (201 Created):**
  ```json
  {
    "success": true,
    "message": "User registered successfully"
  }
  ```

---

## 5. Persistence Test Verification
A crucial academic requirement in the PRD is that data survives backend and database restarts. The standalone test suite `backend/aditya_singh/tests/test_aditya.cpp` explicitly verifies:
1. Connect to SQLite database.
2. Initialize tables from `schema.sql`.
3. Register a new user (`campus_op@evhub.com`).
4. **Close connection** (`db.close()`).
5. **Re-open connection** to same database file (`reopenedDb.open()`).
6. Query and authenticate the registered user (`restartAuth.login(...)`).
7. **Result:** Verified 100% data persistence across restarts with zero data corruption.
