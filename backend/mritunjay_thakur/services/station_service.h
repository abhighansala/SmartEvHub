#ifndef STATION_SERVICE_H
#define STATION_SERVICE_H

#include "../models/charging_station.h"
#include "../models/charging_session.h"
#include "../../agreev_singh/models/transaction.h"
#include "../../abhishek_ghansala/dsa/charging_queue.h"
#include "../../aditya_singh/database/database.h"
#include <string>

const int MAX_STATIONS = 50;
const int MAX_SESSIONS = 100;

// Charging Station & Charging Session Service
// Separation of concerns: Handles business logic, state transitions, allocation, completion, SQLite transactions.
// Uses simple C++ arrays (no vectors) adhering to 2nd-year DSA PBL conventions.
// Created by: Mritunjay Thakur (Charging Stations Module Owner, Student ID: 2519425, Section ML-5)
class StationService {
private:
    Database* db;
    ChargingStation stationArray[MAX_STATIONS];
    int stationCount;

    // In-memory fallback for sessions when database is not connected
    ChargingSession sessionArray[MAX_SESSIONS];
    int sessionCount;

public:
    StationService(Database* database = nullptr);

    // ─── Station Management (Phase 2 & Phase 3) ────────────────────────
    // Create and persist a new station in SQLite and in-memory array
    bool createStation(const ChargingStation& station, std::string& outError);

    // Backward-compatible wrapper for addStation
    bool addStation(const ChargingStation& station);

    // Get all stations into outList array, returns count
    int getAllStations(ChargingStation outList[], int maxCapacity);

    // Get station by ID
    bool getStationById(const std::string& id, ChargingStation& outStation);

    // Update station operational status with validated state transitions
    bool updateStationStatus(const std::string& id, const std::string& newStatus, std::string& outError);

    // Backward-compatible wrapper
    bool updateStationStatus(const std::string& id, const std::string& newStatus);

    // Assign an EV to a station and mark it as "charging"
    bool assignVehicle(const std::string& stationId, const std::string& vehicleId);

    // Release station when charging completes, marks as "available"
    bool releaseStation(const std::string& stationId);

    // Find the first available station (returns true if found; skips maintenance & offline)
    bool findAvailableStation(ChargingStation& outStation);

    // ─── Charging Session Management (Phase 4 & Phase 5) ───────────────
    // Get all charging sessions, with optional filtering by status ("active", "completed", or "" for all)
    int getAllSessions(ChargingSession outList[], int maxCapacity, const std::string& statusFilter = "");

    // Get a charging session by session ID
    bool getSessionById(const std::string& id, ChargingSession& outSession);

    // Get the active charging session on a given station
    bool getActiveSessionByStationId(const std::string& stationId, ChargingSession& outSession);

    // Create a new charging session and assign EV to station (Phase 4 atomic SQLite transaction)
    bool createChargingSession(const std::string& evId, const std::string& stationId, double targetCharge, ChargingSession& outSession, std::string& outError);

    // Backward-compatible wrapper
    bool createChargingSession(const std::string& evId, const std::string& stationId, double targetCharge, ChargingSession& outSession);

    // Complete active charging session on station/session, calculate tariff, record transaction, and optionally dispatch next waiting EV from FIFO queue (Phase 5 atomic SQLite transaction)
    bool completeChargingSession(const std::string& stationOrSessionId, ChargingSession& outCompletedSession, Transaction& outTxn, ChargingQueue* queue = nullptr, ChargingSession* outDispatchedSession = nullptr, std::string& outError = *(new std::string()));

    // Safely enqueue an EV into the FIFO queue and synchronize SQLite status to 'waiting'
    bool enqueueWaitingEV(const std::string& evId, ChargingQueue& queue, std::string& outError);

    // Backward-compatible wrapper
    bool enqueueWaitingEV(const std::string& evId, ChargingQueue& queue);

    // Deterministically rehydrate the FIFO ChargingQueue from SQLite based on status='waiting' ordered by last_activity ASC, registered_at ASC, rowid ASC
    int rehydrateChargingQueue(ChargingQueue& queue);
};

#endif // STATION_SERVICE_H
