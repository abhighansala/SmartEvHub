#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <sqlite3.h>

#include "../models/User.hpp"
#include "../models/EV.hpp"
#include "../models/ChargingStation.hpp"
#include "../models/ChargingSession.hpp"
#include "../models/Transaction.hpp"

namespace evhub {

/**
 * @brief Thread-safe SQLite Database Manager.
 * Module Owner: Aditya Singh (Database & Backend Foundation).
 * Handles schema migration, prepared statements, foreign key validation, and ACID transactions.
 */

 struct PersistedQueuedRequest {
    int requestId;
    int userId;
    int evId;
    std::string requiredPort;
    std::string requestTime;
    int queuePosition;
};
class Database {
private:
    sqlite3* db_;
    std::string dbPath_;
    mutable std::mutex dbMutex_;

    static std::unique_ptr<Database> instance_;
    static std::mutex instanceMutex_;

public:
    explicit Database(std::string dbPath = "evhub.db");
    ~Database();

    // Singleton access
    static Database& getInstance(const std::string& dbPath = "evhub.db");

    // Lifecycle
    bool open();
    void close();
    bool initializeSchema();

    // Transaction management
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // User operations
    bool createUser(const User& user, int& outId);
    bool getUserByEmail(const std::string& email, User& outUser);
    bool getUserById(int id, User& outUser);

    // EV operations (Priyanshu Dutt)
    bool createEV(const EV& ev, int& outId);
    bool getEVById(int id, EV& outEv);
    std::vector<EV> getEVsByUserId(int userId);
    std::vector<EV> getAllEVs();
    bool updateEVSoc(int evId, double newSoc);

    // Station operations (Mritunjay Thakur)
    bool createStation(const ChargingStation& station, int& outId);
    std::vector<ChargingStation> getAllStations();
    bool getStationById(int id, ChargingStation& outStation);
    bool updateStationStatus(int stationId, StationStatus status, int currentSessionId, int assignedEvId, double totalEnergyDispensed);

    // Charging queue persistence
bool saveQueuedRequest(
    int requestId,
    int userId,
    int evId,
    const std::string& requiredPort,
    const std::string& requestTime,
    int queuePosition
);

std::vector<PersistedQueuedRequest> getQueuedRequests();

bool deleteQueuedRequest(int requestId);

bool clearChargingQueue();

    // Session operations
    int getMaxSessionId();
    int getMaxTransactionId();
    bool createSession(const ChargingSession& session, int& outId);
    bool getSessionById(int id, ChargingSession& outSession);
    std::vector<ChargingSession> getActiveSessions();
    bool completeSession(int sessionId, const std::string& endTime, double finalSoc, double energyKwh, double cost);

    // Transaction operations (Agreev Singh)
    bool createTransaction(const Transaction& tx, int& outId);
    std::vector<Transaction> getAllTransactions();

    // Password Hashing helper (SHA-256 simulation with salt)
    static std::string hashPassword(const std::string& plainPassword);
};

} // namespace evhub
