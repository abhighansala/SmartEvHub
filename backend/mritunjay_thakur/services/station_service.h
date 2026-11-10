#ifndef STATION_SERVICE_H
#define STATION_SERVICE_H

#include "../models/charging_station.h"
#include "../models/charging_session.h"
#include "../../agreev_singh/models/transaction.h"
#include "../../abhishek_ghansala/dsa/charging_queue.h"
#include "../../aditya_singh/database/database.h"
#include <string>

const int MAX_STATIONS = 20;
const int MAX_SESSIONS = 50;

// Simple Service to manage Charging Stations and Charging Sessions
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

public:
    StationService(Database* database = nullptr);

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

    // ─── Charging Session Management (Phase B) ─────────────────────────
    // Get all charging sessions, with optional filtering by status ("active", "completed", or "" for all)
    int getAllSessions(ChargingSession outList[], int maxCapacity, const std::string& statusFilter = "");

    // Get a charging session by session ID
    bool getSessionById(const std::string& id, ChargingSession& outSession);

    // Get the active charging session on a given station
    bool getActiveSessionByStationId(const std::string& stationId, ChargingSession& outSession);

    // Create a new charging session and assign EV to station (atomic SQLite transaction)
    bool createChargingSession(const std::string& evId, const std::string& stationId, double targetCharge, ChargingSession& outSession);

    // Complete active charging session on station, record transaction, and optionally dispatch next waiting EV from queue (atomic SQLite transaction)
    bool completeChargingSession(const std::string& stationId, ChargingSession& outCompletedSession, Transaction& outTxn, ChargingQueue* queue = nullptr, ChargingSession* outDispatchedSession = nullptr);

    // Safely enqueue an EV into the FIFO queue and synchronize SQLite status to 'waiting'
    bool enqueueWaitingEV(const std::string& evId, ChargingQueue& queue);

    // Deterministically rehydrate the FIFO ChargingQueue from SQLite based on status='waiting' ordered by last_activity ASC, registered_at ASC, rowid ASC
    int rehydrateChargingQueue(ChargingQueue& queue);
};

#endif
