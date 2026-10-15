# Aditya Singh — Module Starter Guide
**Role:** Database + Backend Foundation (Team Lead)  
**Student ID:** 2519272 (ML1)  
**Folder:** `backend/aditya_singh/`

---

## 1. Responsibilities Overview
As the Database & Backend Foundation Lead:
1. Maintain SQLite schema (`database/schema.sql`) and ensure persistent data storage in `evhub.db`.
2. Provide the simple, clean `Database` class for connecting and running SQL queries.
3. Implement simple Email/Password Authentication using the `Users` table.
4. Expose authentication endpoints for the React Login screen (`POST /api/login`, `POST /api/logout`).
5. Support other team members with database tables, connection helpers, and SQL queries.

---

## 2. Directory Structure
```
aditya_singh/
├── README.md               # This guide
├── database/
│   ├── database.h          # Simple SQLite wrapper class
│   └── database.cpp        # Open/close/execute implementation
├── models/
│   └── user.h              # User C++ class (id, name, email, password, role)
├── services/
│   ├── auth_service.h      # Authentication logic
│   └── auth_service.cpp    # Login & Register queries
├── api/
│   ├── auth_routes.h       # Crow REST API route declarations
│   └── auth_routes.cpp     # POST /api/login and /api/logout handlers
└── tests/
    └── test_aditya.cpp     # Standalone unit test for your module
```

---

## 3. API Endpoints You Own

### A. Login
- **Endpoint:** `POST /api/login`
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
    "message": "Invalid email or password"
  }
  ```

### B. Logout
- **Endpoint:** `POST /api/logout`
- **Response (HTTP 200):**
  ```json
  {
    "success": true,
    "message": "Logged out successfully"
  }
  ```

---

## 4. How to Test Your Code Standalone

From the `backend/` folder, you can build and run your standalone test target:

```bash
mkdir -p build && cd build
cmake ..
cmake --build . --target test_aditya_singh
./test_aditya_singh
```

Or compile with `g++` directly from `aditya_singh/tests`:
```bash
g++ -std=c++17 test_aditya.cpp ../database/database.cpp ../services/auth_service.cpp ../../src/sqlite3.c -I../../include -I../database -I../models -I../services -lpthread -ldl -o test_aditya
./test_aditya
```

---

## 5. Next Steps for Aditya
1. Fork the GitHub repository and clone your fork.
2. Checkout your branch: `git checkout -b feature/database-auth-aditya`
3. Test your `test_aditya.cpp` to make sure it passes on your machine.
4. If you wish, add password hashing or extra user fields in `schema.sql`.
5. Coordinate with Abhishek to integrate your routes into the main server.
