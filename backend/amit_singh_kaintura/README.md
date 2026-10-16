# Smart EV Hub — Battery Inventory & Battery Swap Module
**Owner:** Amit Singh Kaintura  
**Academic Role:** B.Tech 2nd Year, Section G (Student ID: 2518188)  
**Assigned Scope:** `backend/amit_singh_kaintura/`  

---

## 1. Academic & Syllabus Context

This module is designed strictly according to the **B.Tech 2nd Year Computer Science & Engineering (Semester III)** university curriculum:

### A. TCS 307: Object Oriented Programming with C++
- **Unit 1 & 2 (Classes & Objects, Encapsulation, Data Hiding):**
  - Private attributes in `Battery` and `SwapRequest` with public getters and setters and boundary validation.
  - Lifecycle management: Default Constructor, Parameterized Constructor, Copy Constructor, and Destructor.
  - Explicit use of the `this` pointer (`this->id = id;`).
  - Arrays of objects (`Battery batteryArray[MAX_BATTERIES]`).
- **Unit 2 (Operator Overloading & Friend Functions):**
  - `operator==` and `operator!=` for comparing battery identities.
  - `friend std::ostream& operator<<` for streaming formatted object representations to `std::cout` and logs.
- **Unit 4 (Polymorphism & Function Overloading):**
  - Overloaded `findReadyBattery(Battery&)` and `findReadyBattery(double capacity, Battery&)`.
  - Overloaded `performSwap(evId, outBat)` and `performSwap(evId, oldBatId, reqCap, outBat)`.

### B. TCS 302: Data Structures with C
- **Unit 1 (Linear Arrays & Complexity Trade-offs):**
  - Direct fixed-size contiguous memory array (`Battery batteryArray[MAX_BATTERIES]`) avoiding complex template libraries.
  - $O(N)$ Linear Search algorithms for item retrieval by identifier.
  - In-place sequential record update and state transitions.

---

## 2. Architecture & Role of Crow API

```
┌─────────────────────────────────┐
│        React Frontend           │
│  (Battery Management & Swap UI) │
└───────────────┬─────────────────┘
                │  HTTP / JSON (REST)
                ▼
┌─────────────────────────────────┐
│       Crow C++ Helper API       │
│  (Routes in api/*_routes.cpp)   │
└───────────────┬─────────────────┘
                │  C++ In-Memory / SQL
                ▼
┌─────────────────────────────────┐
│        BatteryService           │
│  Fixed Array + SQLite Database  │
└─────────────────────────────────┘
```

The **Crow C++ microframework** serves as the API gateway between the C++ backend and the React frontend:
1. Receives HTTP requests (`GET`, `POST`, `PUT`) from the React client.
2. Parses incoming JSON request payloads (`crow::json::load(req.body)`).
3. Invokes the domain methods in `BatteryService`.
4. Packages responses into JSON format (`crow::json::wvalue`) with appropriate HTTP status codes (200, 201, 400, 404).

---

## 3. Directory Layout

```
backend/amit_singh_kaintura/
├── README.md               # Complete academic and architectural documentation
├── models/
│   ├── battery.h           # Battery domain model (OOP constructors, operators, isReady)
│   └── swap_request.h      # SwapRequest domain model
├── services/
│   ├── battery_service.h   # Service interface (fixed-size array, function overloading)
│   └── battery_service.cpp # Swap business logic & SQLite persistence
├── api/
│   ├── battery_routes.h    # Battery routes declaration for Crow
│   ├── battery_routes.cpp  # GET, POST, PUT /api/batteries
│   ├── swap_routes.h       # Swap workflow routes declaration for Crow
│   └── swap_routes.cpp     # POST /api/swap/request, /process, GET /available-count
└── tests/
    └── test_amit.cpp       # Comprehensive 9-suite standalone unit test
```

---

## 4. REST API Specification

### 1. List Battery Inventory
- **Endpoint:** `GET /api/batteries`
- **Response (200 OK):**
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

### 2. Get Battery by ID
- **Endpoint:** `GET /api/batteries/:id`
- **Response (200 OK):**
  ```json
  {
    "id": "BAT-01",
    "capacity": 50.0,
    "currentCharge": 100.0,
    "status": "available",
    "health": 98.5,
    "cycleCount": 42,
    "currentVehicle": "",
    "isReady": true
  }
  ```

### 3. Add Battery to Inventory (React "Add Battery" Dialog)
- **Endpoint:** `POST /api/batteries`
- **Request Body:**
  ```json
  {
    "id": "BAT-08",
    "capacity": 60.0,
    "currentCharge": 100.0,
    "status": "available",
    "health": 100.0,
    "cycleCount": 0
  }
  ```
- **Response (201 Created):**
  ```json
  {
    "success": true,
    "message": "Battery registered successfully: BAT-08",
    "id": "BAT-08"
  }
  ```

### 4. Update Battery Details
- **Endpoint:** `PUT /api/batteries/:id`
- **Request Body:**
  ```json
  {
    "currentCharge": 95.0,
    "status": "available"
  }
  ```
- **Response (200 OK):**
  ```json
  {
    "success": true,
    "message": "Battery updated successfully: BAT-01"
  }
  ```

### 5. Request Battery Swap
- **Endpoint:** `POST /api/swap/request`
- **Request Body:**
  ```json
  {
    "evId": "EV-102",
    "currentBattery": "BAT-OLD",
    "requiredCapacity": 50.0
  }
  ```
- **Response (Swap Successful - 200 OK):**
  ```json
  {
    "success": true,
    "swapped": true,
    "batteryId": "BAT-01",
    "chargeLevel": 100.0,
    "capacity": 50.0,
    "message": "Swap completed! Assigned battery BAT-01"
  }
  ```
- **Response (Inventory Exhausted - Queued - 200 OK):**
  ```json
  {
    "success": true,
    "swapped": false,
    "queued": true,
    "message": "No charged batteries available in inventory. Added to Swap Queue."
  }
  ```

### 6. Process Queued Swap
- **Endpoint:** `POST /api/swap/process`
- **Request Body:**
  ```json
  {
    "evId": "EV-102"
  }
  ```
- **Response (200 OK):**
  ```json
  {
    "success": true,
    "message": "Processed next swap from queue for EV: EV-102",
    "batteryId": "BAT-02",
    "chargeLevel": 100.0
  }
  ```

---

## 5. How to Compile & Run Tests

### Standalone Compilation (Amit Singh Kaintura Module):
```bash
# 1. Compile sqlite3.c into object file (if not already done)
gcc -O2 -c backend/src/sqlite3.c -Ibackend/include -o backend/src/sqlite3.o

# 2. Build and run unit test suite
g++ -std=c++17 \
  backend/amit_singh_kaintura/tests/test_amit.cpp \
  backend/amit_singh_kaintura/services/battery_service.cpp \
  backend/aditya_singh/database/database.cpp \
  backend/src/sqlite3.o \
  -Ibackend/include \
  -Ibackend/amit_singh_kaintura/models \
  -Ibackend/amit_singh_kaintura/services \
  -Ibackend/aditya_singh/database \
  -lpthread \
  -o backend/amit_singh_kaintura/tests/test_amit.exe

# 3. Execute tests
./backend/amit_singh_kaintura/tests/test_amit.exe
```

All 9 test suites validate object construction, operator overloading, array storage, search algorithms, capacity-matched swaps, and queue fallback!
