# Smart EV Hub — Intelligent Electric Vehicle & Battery Station Management System

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/)
[![Crow C++](https://img.shields.io/badge/Crow-v1.2.0-00ADD8.svg)](https://crowcpp.org/)
[![React](https://img.shields.io/badge/React-18.3.1-61DAFB.svg)](https://react.dev/)
[![Material UI](https://img.shields.io/badge/MUI-v6.4.0-007FFF.svg)](https://mui.com/)
[![SQLite](https://img.shields.io/badge/SQLite-3-003B57.svg)](https://www.sqlite.org/)
[![License](https://img.shields.io/badge/Academic-PBL-green.svg)](#)

A high-performance, full-stack Electric Vehicle Charging and Battery Swapping Station Management System designed for 2nd-Year B.Tech Project-Based Learning (PBL). It pairs a modern React + Material-UI dashboard with a high-throughput C++ Crow backend, local SQLite persistence, and custom Data Structures & Algorithms (FIFO Queues and LIFO Stack) for real-time station scheduling, queueing, and live charging simulation.

---

## 1. System Architecture

```text
┌─────────────────────────────────────────────────────────────┐
│                 React + Material UI Frontend                 │
│         (Vite, React Router v7, MUI v6, Recharts)           │
│                    http://localhost:3000                    │
└──────────────────────────────▲──────────────────────────────┘
                               │ REST API (JSON / HTTP)
┌──────────────────────────────▼──────────────────────────────┐
│                  C++ Crow Microframework                    │
│                 (Asio Network Engine, C++20)                │
│                    http://localhost:8080                    │
├─────────────────────────────────────────────────────────────┤
│                     Backend Modular Services                 │
│  • AuthService (Aditya)        • EVService (Priyanshu)      │
│  • StationService (Mritunjay)  • BatteryService (Amit)      │
│  • Custom DSA Engine (Abhishek)• AnalyticsService (Agreev)  │
├─────────────────────────────────────────────────────────────┤
│                 Custom In-Memory Data Structures            │
│  • ChargingQueue (FIFO)        • SwapQueue (FIFO)           │
│  • TransactionStack (LIFO)                                  │
├─────────────────────────────────────────────────────────────┤
│                     SQLite3 Database Engine                 │
│               (evhub.db with schema.sql seeds)              │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. Team & Member Module Breakdown

The project follows a clean separation of concerns, giving each team member an independent module with its own models, services, REST controllers, and dedicated unit test suite:

| Member | Student ID & Section | Primary Role | Backend Subsystem | Test Suite |
| :--- | :--- | :--- | :--- | :--- |
| **Aditya Singh** | 2519272 (ML1) | **Database, Auth & Security (Team Lead)** | `backend/aditya_singh/` | `test_aditya_singh`, `test_security_auth` |
| **Priyanshu Dutt** | 2519460 (ML3) | **EV Fleet Management** | `backend/priyanshu_dutt/` | `test_priyanshu_dutt` |
| **Mritunjay Thakur** | 2519425 (ML5) | **Charging Stations & Sessions** | `backend/mritunjay_thakur/` | `test_mritunjay_thakur` |
| **Amit Singh Kaintura** | 2518188 (G) | **Battery Inventory & Swapping** | `backend/amit_singh_kaintura/` | `test_amit_singh_kaintura` |
| **Abhishek Ghansala** | 2518132 (H) | **Frontend UI + DSA + Final Integration** | `backend/abhishek_ghansala/` + `src/` | `test_abhishek_ghansala` |
| **Agreev Singh** | 2518161 (F) | **Transactions + Reports + Analytics** | `backend/agreev_singh/` | `test_agreev_singh` |

---

## 3. Key Features

- **Hardened Authentication & Security (Aditya)**: Bcrypt password hashing (10 rounds), RFC 7519 JWT tokens with HMAC-SHA256 signatures, authentication middleware (`requireAuth`), role-based authorization (`admin`, `operator`, `user`), and transparent on-boot bcrypt migration for legacy seed users.
- **Real-Time Station Monitoring & Control**: View live status, power rating, and current vehicle for all charging stations (`CS-1` through `CS-6`).
- **Live Charging Simulation**: Progressive SoC (% charge) updates calculated dynamically from charger wattage and battery capacity, advancing vehicle state from charging to completed.
- **Smart FIFO Charging Queue**: When all charging bays are occupied, incoming charging requests automatically enter Abhishek's manual FIFO `ChargingQueue`. Once a bay opens, queued vehicles are dispatched first-come, first-served.
- **Battery Swapping Bay**: Amit's battery inventory manager verifies healthy replacement batteries ($\ge 90\%$ SoC) and performs rapid battery swaps in under 5 minutes.
- **Transaction Audit & Undo Stack**: Every completed session records a transaction, accessible via Agreev's analytics API and visualized in Abhishek's LIFO `TransactionStack` for audit trails and rollback capability.
- **Interactive Analytics & KPIs**: Real-time revenue, total energy dispensed (kWh), active session counts, and station utilization charts powered by Recharts.
- **Optimistic UI with Instant Rollback**: Instant responsive UI state updates with automated error recovery and rollback if backend requests fail.
- **Offline & Mock Fallbacks**: Frontend includes resilient fallback handling, allowing seamless evaluation even if the C++ backend is momentarily offline.

---

## 4. Prerequisites

- **C++ Compiler**: GCC/MinGW-w64 (v11+) or MSVC (Visual Studio 2022) with C++20 support.
- **CMake**: Version 3.16 or higher.
- **Node.js**: Version 18.x or 20.x or higher, and `npm`.
- **Git**: For version control.

*(Note: Crow and Asio dependencies are automatically fetched via CMake `FetchContent` — no manual library installation is required).*

---

## 5. How to Build & Run Locally

### A. Backend Setup & Run

1. Open PowerShell or a terminal and navigate to the `backend` directory:
   ```powershell
   cd backend
   ```
2. Generate build files with CMake:
   ```powershell
   cmake -B build -S .
   ```
3. Compile the unified server and all member test suites:
   ```powershell
   cmake --build build
   ```
4. Run the backend server:
   ```powershell
   cd build
   .\smart_ev_hub_backend.exe
   ```
   The backend will start and listen on **`http://localhost:8080`**.
   Verify backend health:
   ```powershell
   curl http://localhost:8080/api/health
   ```

### B. Running Member Unit Tests

From the `backend/build` folder, you can run individual or all unit tests:

```powershell
# Lead: Database & Auth Foundation
.\test_aditya_singh.exe

# Lead: Cryptography, Bcrypt, JWT & RBAC Security Suite
.\test_security_auth.exe

# Member 2: EV Management
.\test_priyanshu_dutt.exe

# Member 3: Stations & Charging
.\test_mritunjay_thakur.exe

# Member 4: Batteries & Swapping
.\test_amit_singh_kaintura.exe

# Member 5: DSA (FIFO Queues & LIFO Stack)
.\test_abhishek_ghansala.exe

# Member 6: Transactions & Reports
.\test_agreev_singh.exe
```

### C. Frontend Setup & Run

1. In the project root (`c:\SmartEvHub-Abhishek-Local`), install dependencies:
   ```powershell
   npm install
   ```
2. Verify production build & TypeScript validation:
   ```powershell
   npm run build
   ```
3. Start the development server:
   ```powershell
   npm run dev
   ```
   Open **`http://localhost:3000`** in your browser.

---

## 6. REST API Reference

| Method | Endpoint | Description | Subsystem / Owner |
| :--- | :--- | :--- | :--- |
| `GET` | `/api/health` | System health and database connectivity check | Core |
| `POST` | `/api/register` | Register new user with bcrypt hash | Aditya Singh |
| `POST` | `/api/login` | User authentication returning JWT token | Aditya Singh |
| `GET` | `/api/me` | Authenticated user profile (requires Bearer token) | Aditya Singh |
| `GET` | `/api/users` | List users (requires admin or operator token) | Aditya Singh |
| `POST` | `/api/logout` | End user session | Aditya Singh |
| `GET` | `/api/evs` | List all registered electric vehicles | Priyanshu Dutt |
| `POST` | `/api/evs` | Register a new electric vehicle | Priyanshu Dutt |
| `GET` | `/api/evs/:id` | Get details of a single vehicle | Priyanshu Dutt |
| `PUT` | `/api/evs/:id` | Update EV battery charge or metadata | Priyanshu Dutt |
| `DELETE` | `/api/evs/:id` | Remove an EV from registry | Priyanshu Dutt |
| `GET` | `/api/stations` | List all charging stations and statuses | Mritunjay Thakur |
| `PUT` | `/api/stations/:id/status` | Update station status (`available`, `charging`, `offline`) | Mritunjay Thakur |
| `POST` | `/api/charging/request` | Request charging bay; auto-allocates station or enqueues | Mritunjay Thakur |
| `POST` | `/api/charging/complete` | Complete session, calculate bill, and free station | Mritunjay Thakur |
| `GET` | `/api/batteries` | List replacement battery inventory | Amit Singh Kaintura |
| `POST` | `/api/batteries` | Add new battery to inventory | Amit Singh Kaintura |
| `PUT` | `/api/batteries/:id` | Update battery charge/health | Amit Singh Kaintura |
| `POST` | `/api/swap/request` | Submit battery swap request | Amit Singh Kaintura |
| `POST` | `/api/swap/process` | Process next battery swap from queue | Amit Singh Kaintura |
| `GET` | `/api/charging/queue` | Query live FIFO charging queue state | Abhishek Ghansala |
| `GET` | `/api/swap/queue` | Query live FIFO swap queue state | Abhishek Ghansala |
| `GET` | `/api/transactions/stack` | Query live LIFO transaction stack state | Abhishek Ghansala |
| `GET` | `/api/transactions` | Query full transaction log | Agreev Singh |
| `POST` | `/api/transactions` | Record a completed transaction | Agreev Singh |
| `GET` | `/api/reports` | High-level KPI operational summaries | Agreev Singh |
| `GET` | `/api/analytics` | Station utilization & energy distribution data | Agreev Singh |

---

## 7. Data Structures & Viva Reference

As required for 2nd-year Data Structures & Algorithms (PBL):

1. **`ChargingQueue` & `SwapQueue` (FIFO — First In, First Out)**:
   - **Underlying Structure**: Fixed circular/linear array buffer of size 50.
   - **Operations**: `enqueue(req)` ($O(1)$), `dequeue()` ($O(1)$), `peek()` ($O(1)$), `isFull()`, `isEmpty()`.
   - **Viva Explanation**: Ensures fair vehicle servicing without starvation. When all 6 charging stations are busy, incoming cars wait in the queue and are assigned to the first station that becomes available.

2. **`TransactionStack` (LIFO — Last In, First Out)**:
   - **Underlying Structure**: Fixed array buffer with a top index pointer.
   - **Operations**: `push(txn)` ($O(1)$), `pop()` ($O(1)$), `peek()` ($O(1)$).
   - **Viva Explanation**: Used for auditing the most recent payment transactions, enabling instant audit inspection and $O(1)$ rollback/undo functionality for the latest station operations.

---

## 8. License & Academic Integrity

Developed for **Project-Based Learning (PBL)** by the Smart EV Hub Team. All rights reserved.
