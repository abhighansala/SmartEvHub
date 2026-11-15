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

- **Hardened Authentication & Security (Aditya)**: Bcrypt password hashing (10 rounds), RFC 7519 JWT tokens with HMAC-SHA256 signatures, authentication middleware (`requireAuth`), role-based authorization (`admin`, `operator`, `user`), constant-time signature verification, and transparent on-boot bcrypt migration for legacy seed users.
- **Real-Time Station Monitoring & Control**: View live status, power rating, and current vehicle for all charging stations (`CS-1` through `CS-4`).
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
- **Node.js**: Version 18.x, 20.x, or higher, and `npm`.
- **Git**: For version control.

*(Note: Crow and Asio dependencies are automatically fetched via CMake `FetchContent` — no manual library installation is required).*

---

## 5. Environment Configuration

Copy the example environment file to `.env`:
```powershell
cp .env.example .env
```

Ensure your `.env` contains:
```env
# Mandatory >= 32 characters for HMAC-SHA256 security
JWT_SECRET=SmartEvHub_LocalDev_SecretKey_Minimum_32_Characters_2026_Secure!
PORT=8080
VITE_API_URL=http://localhost:8080/api
```

---

## 6. How to Build & Run Locally

### Option A: Simultaneous One-Command Startup (Recommended)

From the project root (`C:\SmartEvHub-Abhishek-Local`), run:
```powershell
npm run dev:full
```
This runs the C++ Crow backend (with automated CMake build guard) and Vite React frontend concurrently with color-coded logging. Press `Ctrl+C` to terminate both services cleanly.

### Option B: Independent Service Startup

**Terminal 1 — C++ Backend:**
```powershell
# Using the Node launcher (loads .env and validates build):
npm run dev:backend

# Or manually via CMake:
cmake -B backend/build -S backend
cmake --build backend/build
backend/build/smart_ev_hub_backend.exe
```
Backend runs on: **`http://localhost:8080`**

**Terminal 2 — Frontend:**
```powershell
npm run dev
```
Frontend runs on: **`http://localhost:3000`**

---

## 7. Testing & Quality Assurance

### A. All 7 C++ Backend Test Suites

Run all 7 C++ member test suites from the project root:
```powershell
node -e "
const { execFileSync } = require('child_process');
const path = require('path');
const tests = [
  'test_security_auth.exe',
  'test_aditya_singh.exe',
  'test_priyanshu_dutt.exe',
  'test_mritunjay_thakur.exe',
  'test_amit_singh_kaintura.exe',
  'test_abhishek_ghansala.exe',
  'test_agreev_singh.exe'
];
tests.forEach(t => {
  console.log('Testing ' + t + '...');
  execFileSync(path.resolve('backend/build', t), { stdio: 'inherit' });
});
console.log('All 7 C++ test suites passed!');
"
```

Individual test executables can also be run directly:
- `backend/build/test_security_auth.exe` — Cryptographic primitives, Bcrypt, JWT HS256, Constant-Time Comparison, Privilege Escalation Guard
- `backend/build/test_aditya_singh.exe` — SQLite database initialization, transactions, user authentication
- `backend/build/test_priyanshu_dutt.exe` — EV fleet CRUD, database persistence, and rollback verification
- `backend/build/test_mritunjay_thakur.exe` — Charging station allocation, status state machine, and session management
- `backend/build/test_amit_singh_kaintura.exe` — Battery inventory management, SoC/SoH checks, and rapid swap execution
- `backend/build/test_abhishek_ghansala.exe` — Custom FIFO ChargingQueue, SwapQueue, and LIFO TransactionStack DSA structures
- `backend/build/test_agreev_singh.exe` — Tariff billing calculation, transaction logging, and operational analytics

### B. End-to-End System Verification (All 6 Scenarios)

With the backend running, execute the automated end-to-end integration test:
```powershell
node scripts/test-e2e-scenarios.js
```
This tests and validates:
1. **Scenario 1**: Authentication, bcrypt hashing, JWT issuance & verification, privilege escalation guard, `/api/me`.
2. **Scenario 2**: EV registration, retrieval, update, delete, and SQLite persistence.
3. **Scenario 3**: Charging bay allocation, FIFO queue enqueueing, session completion, auto-dispatch, and billing.
4. **Scenario 4**: Battery inventory querying, rapid swap processing, and swap queue fallback.
5. **Scenario 5**: Transaction recording, LIFO stack operations, tariff formulas, and KPI analytics.
6. **Scenario 6**: Database cold-restart persistence verification across process termination.

### C. Frontend Production Build & TypeScript Verification

```powershell
npm run build
```
Runs `tsc --noEmit` and `vite build` to guarantee zero type errors and valid production bundles in `dist/`.

---

## 8. REST API Reference

| Method | Endpoint | Description | Subsystem / Owner |
| :--- | :--- | :--- | :--- |
| `GET` | `/api/health` | System health and database connectivity check | Core |
| `POST` | `/api/register` | Register new user with bcrypt hash & JWT issue | Aditya Singh |
| `POST` | `/api/login` | User authentication with bcrypt returning JWT | Aditya Singh |
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
| `POST` | `/api/charging/queue` | Manually enqueue vehicle to FIFO charging queue | Abhishek Ghansala |
| `GET` | `/api/swap/queue` | Query live FIFO swap queue state | Abhishek Ghansala |
| `GET` | `/api/transactions/stack` | Query live LIFO transaction stack state | Abhishek Ghansala |
| `GET` | `/api/transactions` | Query full transaction log | Agreev Singh |
| `POST` | `/api/transactions` | Record a completed transaction | Agreev Singh |
| `GET` | `/api/reports` | High-level KPI operational summaries | Agreev Singh |
| `GET` | `/api/analytics` | Station utilization & energy distribution data | Agreev Singh |

---

## 9. Data Structures & Viva Reference

As required for 2nd-year Data Structures & Algorithms (PBL):

### 1. `ChargingQueue` & `SwapQueue` (FIFO — First In, First Out)
- **Underlying Structure**: Fixed circular/linear array buffer of size 50.
- **Operations**: `enqueue(req)` ($O(1)$), `dequeue()` ($O(1)$), `peek()` ($O(1)$), `isFull()`, `isEmpty()`.
- **Viva Explanation**: Guarantees fair vehicle servicing without starvation. When all charging stations are busy, incoming cars wait in the queue and are assigned to the first station that becomes available in exact arrival sequence.
- **Persistence Rehydration**: On backend boot, waiting vehicles in SQLite (`status = 'waiting'`) are rehydrated into the in-memory FIFO queue preserving order of `last_activity`.

### 2. `TransactionStack` (LIFO — Last In, First Out)
- **Underlying Structure**: Fixed array buffer with a top index pointer (capacity 50).
- **Operations**: `push(txn)` ($O(1)$), `pop()` ($O(1)$), `peek()` ($O(1)$).
- **Viva Explanation**: Used for auditing the most recent payment transactions, enabling instant audit inspection and $O(1)$ rollback/undo functionality for the latest station operations.

### 3. Key Viva FAQ

**Q1: Why use Crow instead of Node.js / Express for the backend?**
*A:* Crow is a modern C++ microframework utilizing Boost.Asio for asynchronous I/O. It provides native C++ execution speeds, direct memory access for custom DSA structures (avoiding serialization overhead between DSA and web server), and thread-safe routing with minimal memory footprint.

**Q2: How are passwords secured?**
*A:* Passwords use the Bcrypt adaptive hashing algorithm with 10 salt rounds (1024 iterations of Blowfish cipher). Bcrypt is intentionally compute-intensive to prevent brute-force and rainbow table attacks. Legacy SHA-256 passwords from seed data are automatically upgraded to bcrypt hashes upon first startup.

**Q3: How does JWT authentication work with role-based authorization?**
*A:* The backend issues RFC 7519 JSON Web Tokens signed using HMAC-SHA256 (`HS256`). When a protected route is requested, `requireAuth()` decodes the payload, verifies expiration (`exp`), verifies constant-time signature equality against the $\ge 32$-character `JWT_SECRET`, and verifies that the `role` matches required authorization (`user`, `operator`, or `admin`).

**Q4: How does optimistic UI work on the frontend?**
*A:* The React frontend updates local UI state immediately upon user action (e.g., adding an EV or changing station status) to ensure a zero-latency user experience. In the background, an asynchronous HTTP request is sent to the Crow backend. If the request fails, the frontend catches the exception, displays an alert, and rolls back the UI state to its previous snapshot.

---

## 10. License & Academic Integrity

Developed for **Project-Based Learning (PBL)** by the Smart EV Hub Team. All rights reserved.
