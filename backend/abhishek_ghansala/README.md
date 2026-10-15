# Abhishek Ghansala — Module Starter Guide
**Role:** UI + DSA + Final Integration  
**Student ID:** 2518132 (H)  
**Folder:** `backend/abhishek_ghansala/`

---

## 1. Responsibilities Overview
As the UI, DSA & Integration owner:
1. Implement pure manual C++ Data Structures & Algorithms without using standard STL containers (`std::queue` / `std::stack`):
   - `ChargingQueue`: Circular FIFO queue for vehicles waiting for charging bays.
   - `SwapQueue`: FIFO queue for vehicles waiting for replacement batteries.
   - `TransactionStack`: LIFO stack tracking completed transactions.
2. Expose the Queue and Stack data structures to the React UI through Crow endpoints so the UI can visually animate the FRONT/REAR of the queue and TOP of the stack:
   - `GET /api/charging/queue`
   - `GET /api/swap/queue`
   - `GET /api/transactions/stack`
3. Maintain final repository integration, build configuration (`CMakeLists.txt`), and coordinate module compilation.

---

## 2. Directory Structure
```
abhishek_ghansala/
├── README.md               # This guide
├── dsa/
│   ├── charging_queue.h    # FIFO circular queue header (enqueue, dequeue, peek, display)
│   ├── charging_queue.cpp  # Queue implementation
│   ├── swap_queue.h        # Swap queue header
│   ├── swap_queue.cpp      # Swap queue implementation
│   ├── transaction_stack.h # LIFO stack header (push, pop, peek, display)
│   └── transaction_stack.cpp # Stack implementation
├── api/
│   ├── dsa_routes.h        # Crow REST routes header
│   └── dsa_routes.cpp      # Endpoints exposing frontIndex, rearIndex, topIndex
└── tests/
    └── test_abhishek.cpp   # Standalone unit test verifying FIFO & LIFO properties
```

---

## 3. API Endpoints You Own

### A. Charging Queue State
- **Endpoint:** `GET /api/charging/queue`
- **Response (HTTP 200):**
  ```json
  {
    "frontIndex": 0,
    "rearIndex": 1,
    "size": 2,
    "capacity": 20,
    "items": ["EV-102", "EV-104"]
  }
  ```

### B. Transaction Stack State
- **Endpoint:** `GET /api/transactions/stack`
- **Response (HTTP 200):**
  ```json
  {
    "topIndex": 2,
    "size": 3,
    "capacity": 50,
    "items": ["TXN-5003", "TXN-5002", "TXN-5001"]
  }
  ```

---

## 4. How to Test Your Code Standalone

From the `backend/` folder:
```bash
mkdir -p build && cd build
cmake ..
cmake --build . --target test_abhishek_ghansala
./test_abhishek_ghansala
```

Or using `g++` directly:
```bash
cd abhishek_ghansala/tests
g++ -std=c++17 test_abhishek.cpp ../dsa/charging_queue.cpp ../dsa/swap_queue.cpp ../dsa/transaction_stack.cpp -I../dsa -o test_abhishek
./test_abhishek
```
