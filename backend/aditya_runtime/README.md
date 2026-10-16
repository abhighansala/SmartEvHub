# Smart EV Hub — C++ Backend & Database Build Guide

**Project:** Smart EV Hub (2nd Year B.Tech CSE Project-Based Learning)  
**Team Lead:** Aditya Singh (Database & Backend Foundation)  
**Teammates:** Priyanshu Dutt, Mritunjay Thakur, Amit Singh Kaintura, Abhishek Ghansala, Agreev Singh  

---

## 1. Prerequisites (Windows Development)

- **Compiler:** GCC 9+ (via MinGW-w64 or MSYS2) or Microsoft Visual C++ (MSVC 2019/2022)
- **Build System:** CMake 3.15 or newer
- **Libraries:**
  - SQLite3 (`sqlite3.h` and `sqlite3.lib` / `-lsqlite3`)
  - Crow C++ Framework (`crow_all.h` or Crow repository with standalone Asio)

---

## 2. Compilation Instructions (Windows with MinGW)

Open **PowerShell** or **Command Prompt** in the `backend` directory:

```powershell
# 1. Create and enter build directory
mkdir build
cd build

# 2. Configure with CMake
cmake -G "MinGW Makefiles" ..

# 3. Compile the Server and DSA Tests
cmake --build . --config Release

# 4. Run the DSA Verification Tests
./dsa_test.exe

# 5. Run the Crow REST API Server
./smart_ev_hub.exe
```

---

## 3. Compilation Instructions (Windows with Visual Studio / MSVC)

```powershell
cd backend
mkdir build
cd build
cmake ..
cmake --build . --config Release
.\Release\dsa_test.exe
.\Release\smart_ev_hub.exe
```

---

## 4. SQLite Schema Verification

The SQLite database file `evhub.db` is initialized automatically on startup with WAL mode:
```sql
PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;
```

You can inspect the tables using the `sqlite3` CLI:
```bash
sqlite3 evhub.db ".schema"
sqlite3 evhub.db "SELECT * FROM stations;"
sqlite3 evhub.db "SELECT * FROM transactions;"
```

---

## 5. Architectural Viva Defense Checklist

1. **Encapsulation:** All model fields (e.g. `ChargingStation::status_`, `EV::currentSocPercent_`) are strictly private with invariant-checked mutators.
2. **Manual DSA Ownership:** `ManualQueue<T>` and `ManualStack<T>` use manual raw pointers and custom heap nodes (`Node*`) with proper RAII destructors to demonstrate linked list data structure mastery to evaluators.
3. **Double Assignment Guard:** A station cannot host two sessions (`isAvailable()` check).
4. **Double Completion Guard:** A session cannot be completed twice (`complete()` throws `logic_error` if already completed).
