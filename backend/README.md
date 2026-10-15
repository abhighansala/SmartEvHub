# Smart EV Hub — Backend Master Guide

Welcome to the **Smart EV Hub** backend repository!  
This backend is built for a 2nd-Year B.Tech Project-Based Learning (PBL) project.

---

## 1. Team & Module Division

As specified in the PRD, the backend is organized into **dedicated folders for each team member**:

| Member | Student ID & Section | Primary Role | Folder |
| :--- | :--- | :--- | :--- |
| **Aditya Singh** | 2519272 (ML1) | **Database + Backend Foundation (Team Lead)** | `backend/aditya_singh/` |
| **Priyanshu Dutt** | 2519460 (ML3) | **EV Management** | `backend/priyanshu_dutt/` |
| **Mritunjay Thakur** | 2519425 (ML5) | **Charging Stations & Sessions** | `backend/mritunjay_thakur/` |
| **Amit Singh Kaintura** | 2518188 (G) | **Battery Inventory & Swapping** | `backend/amit_singh_kaintura/` |
| **Abhishek Ghansala** | 2518132 (H) | **UI + DSA (Queues & Stack) + Final Integration** | `backend/abhishek_ghansala/` |
| **Agreev Singh** | 2518161 (F) | **Transactions + Reports + Analytics** | `backend/agreev_singh/` |

---

## 2. Coding Principles (Simple & Beginner-Friendly)

To make learning smooth and prepare for academic viva questions:
- **No Complex Libraries:** Avoid `std::vector`, `std::map`, templates, or complex lambdas.
- **Fixed Arrays & Simple Loops:** Use standard arrays (e.g. `items[50]`) and standard `for (int i = 0; i < count; i++)` loops.
- **Manual Data Structures:** `ChargingQueue` (FIFO) and `TransactionStack` (LIFO) are built manually without `std::queue` or `std::stack`.
- **SQLite Persistence:** Operational data is persisted in a local `evhub.db` using `database/schema.sql`.

---

## 3. Directory Structure

```text
backend/
├── CMakeLists.txt                # Master CMake configuration (builds server + all tests)
├── README.md                     # This master guide
├── database/
│   └── schema.sql                # Complete SQLite tables & seed data
├── include/
│   ├── Database.h                # Global database include helper
│   └── sqlite3.h                 # SQLite3 C header
├── src/
│   ├── main.cpp                  # Main Crow server (registers all routes & starts on port 8080)
│   └── sqlite3.c                 # SQLite3 C source
│
├── aditya_singh/                 # Role 1: Database + Auth (Lead)
│   ├── README.md
│   ├── database/                 # Simple Database class (open, execute, initSchema)
│   ├── models/                   # User class
│   ├── services/                 # AuthService (login, register)
│   ├── api/                      # POST /api/login, POST /api/logout
│   └── tests/test_aditya.cpp     # Standalone test runner
│
├── priyanshu_dutt/               # Role 2: EV Management
│   ├── README.md
│   ├── models/                   # EV class (charge, capacity, status, isValid)
│   ├── services/                 # EVService (CRUD on fixed arrays & DB)
│   ├── api/                      # GET, POST, PUT, DELETE /api/evs
│   └── tests/test_priyanshu.cpp  # Standalone test runner
│
├── mritunjay_thakur/             # Role 3: Charging Stations
│   ├── README.md
│   ├── models/                   # ChargingStation & ChargingSession classes
│   ├── services/                 # StationService (assign, release, available check)
│   ├── api/                      # GET, PUT /api/stations, POST /api/charging/*
│   └── tests/test_mritunjay.cpp  # Standalone test runner
│
├── amit_singh_kaintura/          # Role 4: Battery & Battery Swap
│   ├── README.md
│   ├── models/                   # Battery & SwapRequest classes
│   ├── services/                 # BatteryService (ready check, performSwap)
│   ├── api/                      # GET /api/batteries, POST /api/swap/*
│   └── tests/test_amit.cpp       # Standalone test runner
│
├── abhishek_ghansala/            # Role 5: DSA + UI Integration
│   ├── README.md
│   ├── dsa/                      # ChargingQueue (FIFO), SwapQueue, TransactionStack (LIFO)
│   ├── api/                      # GET /api/charging/queue, GET /api/transactions/stack
│   └── tests/test_abhishek.cpp   # Standalone test runner
│
└── agreev_singh/                 # Role 6: Transactions & Analytics
    ├── README.md
    ├── models/                   # Transaction class
    ├── services/                 # TransactionService (revenue, energy, summaries)
    ├── api/                      # GET, POST /api/transactions, GET /api/reports, /api/analytics
    └── tests/test_agreev.cpp     # Standalone test runner
```

---

## 4. How to Fork and Start Working

Each member can work independently without conflicts:

1. **Fork the Repository** on GitHub:  
   Click **Fork** on `https://github.com/abhighansala/SmartEvHub.git`.
2. **Clone your fork**:
   ```bash
   git clone https://github.com/<YOUR_GITHUB_USERNAME>/SmartEvHub.git
   cd SmartEvHub
   ```
3. **Create your feature branch**:
   ```bash
   git checkout -b feature/<your_name>-module
   ```
4. **Open your folder** inside `backend/<your_name>/`:
   - Read your dedicated `README.md`.
   - Modify or expand your classes in `models/`, `services/`, or `api/`.
5. **Run your own test target**:
   ```bash
   cd backend
   mkdir -p build && cd build
   cmake ..
   cmake --build . --target test_<your_name>
   ./test_<your_name>
   ```
6. **Commit and Push**:
   ```bash
   git add backend/<your_name>/
   git commit -m "Implement <your_feature> logic"
   git push origin feature/<your_name>-module
   ```
7. **Open a Pull Request** against `main` for Abhishek to review and integrate.

---

## 5. Building the Complete Unified Server

From the `backend/` directory:

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
```

To run the main server:
```bash
./smart_ev_hub_backend
```
The server will start at: `http://localhost:8080`

To test if it's running:
```bash
curl http://localhost:8080/api/health
```

---

## 6. Running All Member Tests at Once

```bash
cd backend/build
./test_aditya_singh
./test_priyanshu_dutt
./test_mritunjay_thakur
./test_amit_singh_kaintura
./test_abhishek_ghansala
./test_agreev_singh
```

---

## 7. API Reference Summary

| Method | Endpoint | Description | Owner |
| :--- | :--- | :--- | :--- |
| `POST` | `/api/login` | Simple email/password authentication | Aditya Singh |
| `POST` | `/api/logout` | End session | Aditya Singh |
| `GET` | `/api/evs` | List all registered EVs | Priyanshu Dutt |
| `POST` | `/api/evs` | Register a new EV | Priyanshu Dutt |
| `GET` | `/api/evs/:id` | Get details of a single EV | Priyanshu Dutt |
| `PUT` | `/api/evs/:id` | Update EV charge or details | Priyanshu Dutt |
| `DELETE` | `/api/evs/:id` | Delete an EV | Priyanshu Dutt |
| `GET` | `/api/stations` | List all charging stations | Mritunjay Thakur |
| `PUT` | `/api/stations/:id/status` | Update station state | Mritunjay Thakur |
| `POST` | `/api/charging/request` | Request charging bay (or queue) | Mritunjay Thakur |
| `POST` | `/api/charging/complete` | Complete charging session & release station | Mritunjay Thakur |
| `GET` | `/api/batteries` | List replacement battery inventory | Amit Singh Kaintura |
| `POST` | `/api/swap/request` | Request a battery swap (or queue) | Amit Singh Kaintura |
| `POST` | `/api/swap/process` | Process next swap from queue | Amit Singh Kaintura |
| `GET` | `/api/charging/queue` | Visual FIFO charging queue state | Abhishek Ghansala |
| `GET` | `/api/swap/queue` | Visual FIFO swap queue state | Abhishek Ghansala |
| `GET` | `/api/transactions/stack` | Visual LIFO transaction stack state | Abhishek Ghansala |
| `GET` | `/api/transactions` | List transaction history | Agreev Singh |
| `POST` | `/api/transactions` | Record new transaction | Agreev Singh |
| `GET` | `/api/reports` | High-level operational summaries | Agreev Singh |
| `GET` | `/api/analytics` | Recharts-formatted analytics data | Agreev Singh |
