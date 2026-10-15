# Priyanshu Dutt — Module Starter Guide
**Role:** EV Management Module Owner  
**Student ID:** 2519460 (ML3)  
**Folder:** `backend/priyanshu_dutt/`

---

## 1. Responsibilities Overview
As the EV Management owner:
1. Implement the `EV` domain model with validation (battery charge 0–100%, non-empty ID).
2. Implement `EVService` to perform CRUD operations (Create, Read, Update, Delete) on EVs using simple arrays and SQLite.
3. Expose REST endpoints via Crow for the React frontend:
   - `GET /api/evs` (List all EVs)
   - `GET /api/evs/:id` (Get single EV)
   - `POST /api/evs` (Register new EV)
   - `PUT /api/evs/:id` (Update EV)
   - `DELETE /api/evs/:id` (Delete EV)

---

## 2. Directory Structure
```
priyanshu_dutt/
├── README.md               # This guide
├── models/
│   └── ev.h                # Simple EV class (id, owner, model, capacity, charge, status)
├── services/
│   ├── ev_service.h        # EV service header (uses simple arrays, NO vectors)
│   └── ev_service.cpp      # EV CRUD operations
├── api/
│   ├── ev_routes.h         # Crow routes header
│   └── ev_routes.cpp       # Endpoint handlers
└── tests/
    └── test_priyanshu.cpp  # Standalone unit test
```

---

## 3. API Endpoints You Own

### A. List All EVs
- **Endpoint:** `GET /api/evs`
- **Response (HTTP 200):**
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

### B. Register New EV
- **Endpoint:** `POST /api/evs`
- **Request Body (JSON):**
  ```json
  {
    "id": "EV-107",
    "owner": "Amit Kumar",
    "model": "MG ZS EV",
    "batteryCapacity": 50.3,
    "currentCharge": 25.0,
    "status": "available"
  }
  ```
- **Response (Success - HTTP 201):**
  ```json
  {
    "success": true,
    "message": "EV created successfully",
    "id": "EV-107"
  }
  ```

---

## 4. How to Test Your Code Standalone

From the `backend/` folder:
```bash
mkdir -p build && cd build
cmake ..
cmake --build . --target test_priyanshu_dutt
./test_priyanshu_dutt
```

Or directly using `g++`:
```bash
cd priyanshu_dutt/tests
g++ -std=c++17 test_priyanshu.cpp ../services/ev_service.cpp ../../aditya_singh/database/database.cpp ../../src/sqlite3.c -I../../include -I../models -I../services -I../../aditya_singh/database -lpthread -ldl -o test_priyanshu
./test_priyanshu
```
