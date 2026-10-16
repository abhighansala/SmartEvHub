# Aditya's Smart EV Hub backend integration

This folder contains a separate copy of Aditya's C++/Crow/SQLite backend from `C:\EV_HUB\backend`.

It is deliberately isolated under `backend/aditya_runtime` so it does not overwrite the team's existing backend files. It is a standalone backend contribution; it has not been automatically wired into the team's root CMake target or frontend API calls.

## Build (Windows, with CMake and a C++ compiler installed)

From the repository root:

```powershell
cmake -S backend/aditya_runtime -B backend/aditya_runtime/build
cmake --build backend/aditya_runtime/build --config Release
```

For a MinGW Makefiles setup, configure with `-G "MinGW Makefiles"` if needed.

The generated executable and build files should remain untracked. Do not commit local databases, credentials, `.env` files, or build output.

## Integration note

Before connecting the frontend to this backend, verify the endpoint paths and request/response JSON in the source, then update the frontend API client accordingly. This branch preserves the current team backend and frontend unchanged.
