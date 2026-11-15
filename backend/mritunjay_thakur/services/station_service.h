#ifndef STATION_SERVICE_H
#define STATION_SERVICE_H

#include "../models/charging_station.h"
#include "../models/charging_session.h"
#include "../models/charging_request.h"
#include "../../agreev_singh/models/transaction.h"
#include "../../abhishek_ghansala/dsa/charging_queue.h"
#include "../../aditya_singh/database/database.h"
#include <string>

const int MAX_STATIONS = 20;
const int MAX_SESSIONS = 50;
const int MAX_REQUESTS = 50;

// Simple Service to manage Charging Stations, Sessions and Requests
// Uses simple C++ arrays (no vectors)
// Created by: Mritunjay Thakur (Charging Stations Module Owner, ID: 2519425)
class StationService {
private:
    Database* db;
    ChargingStation stationArray[MAX_STATIONS];
    int stationCount;

    // In-memory fallback for sessions
    ChargingSession sessionArray[MAX_SESSIONS];
    int sessionCount;

    // In-memory fallback for requests
    ChargingRequest requestArray[MAX_REQUESTS];
    int requestCount;

public:
    StationService(Database* database = nullptr);

    // Database schema auto-migration helper
    void ensureSchemaMigrations();

    // ─── Charging Stations Management ──────────────────────────────────
    // Get all stations into outList array, returns count
    int getAllStations(ChargingStation outList[], int maxCapacity);

    // Get station by ID
    bool getStationById(const std::string& id, ChargingStation& outStation);

    // Update station operational status ("available", "charging", "maintenance", "offline")
    bool updateStationStatus(const std::string& id, const std::string& newStatus);

    // Assign an EV to a station and mark it as "charging"
    bool assignVehicle(const std::string& stationId, const std::string& vehicleId);

    // Release station when charging completes, marks as "available"
    bool releaseStation(const std::string& stationId);

    // Find the first available station (returns true if found)
    bool findAvailableStation(ChargingStation& outStation);

    // Add new station to local array
    bool addStation(const ChargingStation& station);

    // ─── Customer Charging Request Management (Intake & Queue) ─────────
    // Retrieves customer charging requests. If userId > 0, strictly filters for that user.
    int getAllRequests(ChargingRequest outList[], int maxCapacity, int userId = 0);

    // Get charging request by ID
    bool getRequestById(const std::string& id, ChargingRequest& outRequest);

    // Validates and processes a customer charging request:
    // If a bay is free -> immediately allocates bay, creates active session, updates station & EV to 'charging'.
    // If all stations busy -> creates queued request, updates EV to 'waiting', enqueues into ChargingQueue.
    bool createChargingRequest(int userId,
                               const std::string& evId,
                               const std::string& stationId,
                               double targetCharge,
                               ChargingRequest& outRequest,
                               ChargingSession& outSession,
                               bool& outAllocated,
                               ChargingQueue* queue = nullptr);

    // ─── Charging Session Management ───────────────────────────────────
    // Get charging sessions with optional filtering by status ("active", "completed", or "") and userId
    int getAllSessions(ChargingSession outList[], int maxCapacity, const std::string& statusFilter = "", int userId = 0);

    // Get a charging session by session ID
    bool getSessionById(const std::string& id, ChargingSession& outSession);

    // Get the active charging session on a given station
    bool getActiveSessionByStationId(const std::string& stationId, ChargingSession& outSession);

    // Create a new charging session and assign EV to station (atomic SQLite transaction)
    bool createChargingSession(const std::string& evId,
                               const std::string& stationId,
                               double targetCharge,
                               ChargingSession& outSession,
                               int userId = 0,
                               const std::string& requestId = "");

    // Computes live simulated session progress using Academic Constant-Power Model (Phase 8):
    // ElapsedHours = elapsedSeconds / 3600.0
    // EnergyStored = PowerKW * ElapsedHours * Efficiency (0.92)
    // CurrentSoC = min(TargetSoC, StartingSoC + (EnergyStored / Capacity) * 100.0)
    // Cost = calculateTariff(GridEnergy, "charging")
    bool getSessionProgress(const std::string& sessionId,
                            ChargingSession& outSession,
                            double& currentSoC,
                            double& energyDelivered,
                            double& currentCost,
                            int& elapsedSeconds);

    // Complete active charging session on station/session, record transaction, and optionally dispatch next waiting EV from queue
    bool completeChargingSession(const std::string& stationOrSessionId,
                                 ChargingSession& outCompletedSession,
                                 Transaction& outTxn,
                                 ChargingQueue* queue = nullptr,
                                 ChargingSession* outDispatchedSession = nullptr,
                                 int callerUserId = 0,
                                 const std::string& callerRole = "admin");

    // Safely enqueue an EV into the FIFO queue and synchronize SQLite status to 'waiting'
    bool enqueueWaitingEV(const std::string& evId, ChargingQueue& queue);

    // Deterministically rehydrate the FIFO ChargingQueue from SQLite based on status='waiting' ordered by last_activity ASC, registered_at ASC, rowid ASC
    int rehydrateChargingQueue(ChargingQueue& queue);
};

#endif // STATION_SERVICE_H
