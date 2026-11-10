# Smart EV Hub — EV Management Module Guide
**Assigned Developer:** Priyanshu Dutt
**Role:** EV Management Module Owner
**Student ID:** 2519460 (ML3)
**Module Directory:** `backend/priyanshu_dutt/`

---

## 1. Responsibilities Overview
As the EV Management Module owner for the Smart EV Hub 2nd-Year B.Tech PBL project:
1. **Domain Model (`EV`):** Implement OOP encapsulation, data members, constructors, getters, setters, and strict validation (`0 <= currentCharge <= 100`, `batteryCapacity > 0`, non-empty ID and model).
2. **Persistence & Service Layer (`EVService`):** Manage SQLite CRUD operations against the `EV` table in `evhub.db`, using parameterized queries (`sqlite3_prepare_v2`, `sqlite3_bind_*`) and row modification verification (`sqlite3_changes`). Provide an in-memory fixed array fallback if the database connection is offline.
3. **Crow REST API Handlers (`registerEVRoutes`):** Expose 5 REST endpoints:
   - `GET /api/evs` — List all registered electric vehicles.
   - `GET /api/evs/:id` — Retrieve details for a single vehicle.
   - `POST /api/evs` — Register a new EV with duplicate-ID prevention.
   - `PUT /api/evs/:id` — Update EV specifications and battery state.
   - `DELETE /api/evs/:id` — Delete an EV with row-affected confirmation.
4. **Frontend Integration:** Connect `EVManagement.tsx` and `EVDetails.tsx` to the backend through `src/services/api.ts` and `src/context/AppContext.tsx`, maintaining Material 3 UI design and robust error handling.
5. **Quality Assurance:** Maintain a standalone unit and regression test suite verifying validation, CRUD operations, duplicate rejection, non-existent ID handling, and persistence across database restarts.

---

## 2. Directory Structure
```text
backend/priyanshu_dutt/
├── README.md               # This module documentation and viva reference
├── models/
│   └── ev.h                # Encapsulated EV class (ID, owner, model, capacity, charge, status)
├── services/
│   ├── ev_service.h        # EV service header (CRUD methods, MAX_EVS = 50 fixed array)
│   └── ev_service.cpp      # EV CRUD implementation with SQLite & row-affected checking
├── api/
│   ├── ev_routes.h         # Crow routes declaration header
│   └── ev_routes.cpp       # Crow REST endpoint handlers with HTTP status codes
└── tests/
    └── test_priyanshu.cpp  # Standalone 20-test suite (in-memory + SQLite persistence)
```

---

## 3. Database Schema & Mapping

SQLite Table `EV` in `backend/database/schema.sql`:
```sql
CREATE TABLE IF NOT EXISTS EV (
    id TEXT PRIMARY KEY,
    owner_name TEXT NOT NULL,
    model TEXT NOT NULL,
    battery_capacity REAL NOT NULL DEFAULT 50.0,
    current_charge REAL NOT NULL DEFAULT 20.0 CHECK(current_charge >= 0.0 AND current_charge <= 100.0),
    status TEXT NOT NULL DEFAULT 'available' CHECK(status IN ('available', 'charging', 'waiting', 'swapping')),
    last_activity DATETIME DEFAULT CURRENT_TIMESTAMP,
    registered_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

| SQLite Column | C++ Member in `EV` | TypeScript Interface (`EV`) | Notes |
| :--- | :--- | :--- | :--- |
| `id` | `std::string id` | `id: string` | Vehicle identifier (e.g., `EV-101`) |
| `owner_name` | `std::string ownerName` | `owner: string` | Mapped in Crow JSON serializer |
| `model` | `std::string model` | `model: string` | Vehicle make and model name |
| `battery_capacity` | `double batteryCapacity` | `batteryCapacity: number` | Total pack capacity in kWh |
| `current_charge` | `double currentCharge` | `currentCharge: number` | Percentage state-of-charge (0.0 to 100.0) |
| `status` | `std::string status` | `status: EVStatus` | 'available', 'charging', 'waiting', 'swapping' |
| `last_activity` | `std::string lastActivity` | `lastActivity: string` | Timestamp of last interaction |
| `registered_at` | `std::string registeredAt` | `registeredAt: string` | Registration timestamp |

---

## 4. REST API Contract

### A. List All EVs
- **Endpoint:** `GET /api/evs`
- **Response (HTTP 200 OK):**
  ```json
  [
    {
      "id": "EV-101",
      "owner": "Rahul Sharma",
      "model": "Tata Nexon EV Max",
      "batteryCapacity": 40.5,
      "currentCharge": 32.0,
      "status": "charging",
      "lastActivity": "2026-10-15 15:00:00",
      "registeredAt": "2026-10-15 12:00:00"
    }
  ]
  ```

### B. Get Single EV
- **Endpoint:** `GET /api/evs/:id`
- **Response (HTTP 200 OK):**
  ```json
  {
    "id": "EV-101",
    "owner": "Rahul Sharma",
    "model": "Tata Nexon EV Max",
    "batteryCapacity": 40.5,
    "currentCharge": 32.0,
    "status": "charging",
    "lastActivity": "2026-10-15 15:00:00",
    "registeredAt": "2026-10-15 12:00:00"
  }
  ```
- **Error Response (HTTP 404 Not Found):**
  ```json
  {
    "success": false,
    "message": "EV not found with ID: EV-999"
  }
  ```

### C. Register New EV
- **Endpoint:** `POST /api/evs`
- **Request Body (JSON):**
  ```json
  {
    "id": "EV-201",
    "owner": "Priyanshu Dutt",
    "model": "Lucid Air Grand Touring",
    "batteryCapacity": 112.0,
    "currentCharge": 95.0,
    "status": "available"
  }
  ```
- **Response (HTTP 201 Created):**
  ```json
  {
    "success": true,
    "message": "EV created successfully",
    "id": "EV-201"
  }
  ```
- **Error Response (HTTP 400 Bad Request):**
  ```json
  {
    "success": false,
    "message": "EV with ID EV-201 already exists."
  }
  ```

### D. Update EV
- **Endpoint:** `PUT /api/evs/:id`
- **Request Body (JSON):**
  ```json
  {
    "owner": "Priyanshu Dutt",
    "model": "Lucid Air Grand Touring",
    "batteryCapacity": 112.0,
    "currentCharge": 42.0,
    "status": "waiting"
  }
  ```
- **Response (HTTP 200 OK):**
  ```json
  {
    "success": true,
    "message": "EV updated successfully"
  }
  ```
- **Error Response (HTTP 404 Not Found):**
  ```json
  {
    "success": false,
    "message": "EV not found"
  }
  ```

### E. Delete EV
- **Endpoint:** `DELETE /api/evs/:id`
- **Response (HTTP 200 OK):**
  ```json
  {
    "success": true,
    "message": "EV deleted successfully"
  }
  ```
- **Error Response (HTTP 404 Not Found):**
  ```json
  {
    "success": false,
    "message": "EV not found with ID: EV-201"
  }
  ```

---

## 5. Standalone Regression & Persistence Testing

The standalone test file `tests/test_priyanshu.cpp` verifies 20 independent test scenarios:

### Section 1: In-Memory / Array Fallback Unit Tests
- **Test 1:** EV model instantiation, getters, setters, and `isValid()` verification.
- **Test 2:** Valid boundary charges (`0.0%` and `100.0%`) accepted.
- **Test 3:** Invalid charges (`-0.1%` and `100.1%`) rejected.
- **Test 4:** Non-positive capacity (`0.0`) and empty ID/model rejected.
- **Test 5:** In-memory EV creation and retrieval by ID.
- **Test 6:** Duplicate EV ID rejection in fallback array.
- **Test 7:** In-memory EV update of charge and status.
- **Test 8:** Updating a non-existent ID returns `false` with error message.
- **Test 9:** Deleting an existing EV and deleting a non-existent ID handling.
- **Test 10:** Array traversal count verification.

### Section 2: SQLite Persistence & Database Integration Tests
- **Test 11:** SQLite connection opening and schema DDL table creation.
- **Test 12:** Seed data count verification (7 seeded EVs from `schema.sql`).
- **Test 13:** Querying seeded EV (`EV-101`) fields from SQLite.
- **Test 14:** Inserting a new EV (`EV-201`) into SQLite.
- **Test 15:** Duplicate ID rejection by service query and SQLite primary key.
- **Test 16:** Updating existing EV record in SQLite.
- **Test 17:** **[Regression Check]** Updating non-existent EV in SQLite returns `false` (`sqlite3_changes == 0`).
- **Test 18:** **[Regression Check]** Deleting non-existent EV in SQLite returns `false` (`sqlite3_changes == 0`).
- **Test 19:** **[Persistence Check]** Simulating server restart by closing and reopening the database handle; verifying that `EV-201` persists.
- **Test 20:** Deleting EV from SQLite and verifying absence.

### How to Build and Run the Tests
1. **Compile SQLite amalgamation object:**
   ```bash
   clang -c backend/src/sqlite3.c -I backend/include -o sqlite3.o
   ```
2. **Compile and link test runner:**
   ```bash
   clang++ -std=c++17 backend/priyanshu_dutt/tests/test_priyanshu.cpp \
     backend/priyanshu_dutt/services/ev_service.cpp \
     backend/aditya_singh/database/database.cpp \
     sqlite3.o \
     -I backend/priyanshu_dutt \
     -I backend/aditya_singh \
     -I backend/include \
     -o test_priyanshu.exe
   ```
3. **Execute:**
   ```bash
   ./test_priyanshu.exe
   ```

---

## 6. Academic PBL Viva Talking Points

When presenting this module during the evaluation:
1. **Why use fixed arrays instead of `std::vector`?**
   - Fixed arrays (`EV evArray[MAX_EVS]`) demonstrate fundamental data structures, memory limits, and explicit boundary checking (`count < maxCapacity`), matching 2nd-year DSA coursework.
2. **How does parameter binding prevent SQL injection?**
   - Using `sqlite3_prepare_v2` and `sqlite3_bind_text` separates executable SQL statements from user data, preventing malicious SQL strings from altering query structure.
3. **Why is `sqlite3_changes()` essential for `updateEV` and `deleteEV`?**
   - SQLite returns `SQLITE_DONE` when a statement executes without syntax or disk errors, even if 0 rows matched the `WHERE id = ?` clause. Checking `sqlite3_changes(handle) > 0` ensures the operation actually affected a record before reporting success.
4. **How does EV Management connect to the rest of Smart EV Hub?**
   - Electric Vehicles are the core entities that compete for charging stations (`ChargingStation.current_vehicle_id`), enter the FIFO waiting list (`ChargingQueue`), request battery swaps (`BatterySwap`), and record completed operations on the LIFO stack (`TransactionLog`).
