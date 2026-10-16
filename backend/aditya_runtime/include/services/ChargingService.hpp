#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <ostream>
#include <iostream>

#include "../models/Types.hpp"
#include "../models/ChargingStation.hpp"
#include "../models/EV.hpp"
#include "../models/ChargingSession.hpp"
#include "../models/Transaction.hpp"
#include "../dsa/ManualQueue.hpp"
#include "../dsa/ManualStack.hpp"
#include "../db/Database.hpp"

namespace evhub {

struct QueuedRequest {
    int requestId;
    int userId;
    int evId;
    PortType requiredPort;
    std::string requestTime;

    friend std::ostream& operator<<(std::ostream& os, const QueuedRequest& req) {
        os << "Req#" << req.requestId << " (EV:" << req.evId << " | "
           << portTypeToString(req.requiredPort) << ")";
        return os;
    }
};

/**
 * Coordinates charging station allocation, FIFO queueing, and LIFO
 * transaction logging. SQLite is updated before in-memory state changes.
 */
class ChargingService {
private:
    ManualQueue<QueuedRequest> chargingQueue_;
    ManualStack<Transaction> recentTransactionsStack_;
    int nextRequestId_;
    int nextSessionId_;
    int nextTxId_;
    mutable std::mutex serviceMutex_;

    const double BASE_FEE = 2.50;
    const double RATE_PER_KWH = 0.28;

public:
    ChargingService()
        : nextRequestId_(101), nextSessionId_(501), nextTxId_(9001) {}

    void setNextRequestId(int nextId) {
        std::lock_guard<std::mutex> lock(serviceMutex_);
        if (nextId > nextRequestId_) nextRequestId_ = nextId;
    }

    void setNextSessionId(int nextId) {
        std::lock_guard<std::mutex> lock(serviceMutex_);
        if (nextId > nextSessionId_) nextSessionId_ = nextId;
    }

    void setNextTxId(int nextId) {
        std::lock_guard<std::mutex> lock(serviceMutex_);
        if (nextId > nextTxId_) nextTxId_ = nextId;
    }

    ManualQueue<QueuedRequest>& getQueue() { return chargingQueue_; }
    const ManualQueue<QueuedRequest>& getQueue() const { return chargingQueue_; }

    ManualStack<Transaction>& getTransactionStack() { return recentTransactionsStack_; }
    const ManualStack<Transaction>& getTransactionStack() const { return recentTransactionsStack_; }

    struct DispatchResult {
        bool immediateStart = false;
        int sessionId = 0;
        int stationId = 0;
        int queuePosition = 0;
        std::string message;
        int requestId = 0;
        std::string requestTime;
        std::string sessionStartTime;
    };

    // Saves the session or queue row before changing the in-memory state.
    DispatchResult requestCharging(
        int userId,
        int evId,
        PortType portType,
        std::vector<ChargingStation>& stations,
        Database& db,
        const std::vector<EV>& evs) {

        std::lock_guard<std::mutex> lock(serviceMutex_);
        DispatchResult result;

        const EV* requestedEV = nullptr;
        for (const auto& ev : evs) {
            if (ev.getId() == evId) {
                requestedEV = &ev;
                break;
            }
        }
        if (!requestedEV) {
            result.message = "EV not found in the database.";
            return result;
        }
        if (requestedEV->getUserId() != userId) {
            result.message = "This EV does not belong to the specified user.";
            return result;
        }
        if (requestedEV->getSupportedPort() != portType) {
            result.message = "Requested connector does not match the EV's supported connector.";
            return result;
        }

        ChargingStation* availableStation = nullptr;
        for (auto& station : stations) {
            if (station.isAvailable() && station.getPortType() == portType) {
                availableStation = &station;
                break;
            }
        }

        const std::string now = getCurrentIsoTimestamp();
        if (availableStation) {
            const int sessionId = nextSessionId_;
            ChargingSession session(
                sessionId,
                availableStation->getId(),
                evId,
                userId,
                now,
                requestedEV->getCurrentSocPercent(),
                SessionStatus::IN_PROGRESS);

            if (!db.beginTransaction()) {
                result.message = "Could not begin database transaction.";
                return result;
            }

            int savedSessionId = 0;
            bool persisted = db.createSession(session, savedSessionId) &&
                db.updateStationStatus(
                    availableStation->getId(),
                    StationStatus::CHARGING,
                    sessionId,
                    evId,
                    availableStation->getTotalEnergyDispensedKwh());

            if (!persisted) {
                db.rollbackTransaction();
                result.message = "Could not persist the charging session and station assignment.";
                return result;
            }
            if (!db.commitTransaction()) {
                db.rollbackTransaction();
                result.message = "Could not commit the charging session to SQLite.";
                return result;
            }

            availableStation->assignSession(sessionId, evId);
            ++nextSessionId_;
            result.immediateStart = true;
            result.sessionId = sessionId;
            result.stationId = availableStation->getId();
            result.sessionStartTime = now;
            result.message = "Eligible station [" + availableStation->getName() + "] assigned immediately.";
            return result;
        }

        QueuedRequest request{
            nextRequestId_, userId, evId, portType, now
        };
        const int position = static_cast<int>(chargingQueue_.size()) + 1;

        if (!db.beginTransaction()) {
            result.message = "Could not begin database transaction.";
            return result;
        }
        if (!db.saveQueuedRequest(
                request.requestId,
                request.userId,
                request.evId,
                portTypeToString(request.requiredPort),
                request.requestTime,
                position)) {
            db.rollbackTransaction();
            result.message = "Could not save the queued request to SQLite.";
            return result;
        }
        if (!db.commitTransaction()) {
            db.rollbackTransaction();
            result.message = "Could not commit the queued request to SQLite.";
            return result;
        }

        chargingQueue_.enqueue(request);
        ++nextRequestId_;
        result.immediateStart = false;
        result.requestId = request.requestId;
        result.requestTime = request.requestTime;
        result.queuePosition = position;
        result.message = "All " + portTypeToString(portType) +
            " stations are occupied. Added to FIFO queue at position #" +
            std::to_string(position) + ".";
        return result;
    }

    struct CompletionResult {
        bool success = false;
        Transaction transaction;
        bool nextRequestAssigned = false;
        int nextEvIdAssigned = 0;
        std::string message;
    };

    // Persists completion + transaction + station update + any FIFO dispatch
    // in one SQLite transaction. In-memory state changes only after commit.
    CompletionResult completeSession(
        int stationId,
        double energyDeliveredKwh,
        std::vector<ChargingStation>& stations,
        Database& db,
        std::vector<EV>& evs) {

        std::lock_guard<std::mutex> lock(serviceMutex_);
        CompletionResult result;

        if (!std::isfinite(energyDeliveredKwh) || energyDeliveredKwh < 0.0) {
            result.message = "Energy delivered must be a finite, non-negative number.";
            return result;
        }

        ChargingStation* targetStation = nullptr;
        for (auto& station : stations) {
            if (station.getId() == stationId) {
                targetStation = &station;
                break;
            }
        }
        if (!targetStation || targetStation->getStatus() != StationStatus::CHARGING) {
            result.message = "Invalid station or station is not currently charging.";
            return result;
        }

        const int completedSessionId = targetStation->getCurrentSessionId();
        const int completedEvId = targetStation->getAssignedEvId();
        ChargingSession savedSession;
        if (!db.getSessionById(completedSessionId, savedSession) ||
            savedSession.getStatus() != SessionStatus::IN_PROGRESS) {
            result.message = "The active charging session could not be found in SQLite.";
            return result;
        }

        EV* completedEV = nullptr;
        for (auto& ev : evs) {
            if (ev.getId() == completedEvId) {
                completedEV = &ev;
                break;
            }
        }
        if (!completedEV || completedEV->getBatteryCapacityKwh() <= 0.0) {
            result.message = "The assigned EV or its battery capacity could not be found.";
            return result;
        }

        double finalSoc = completedEV->getCurrentSocPercent() +
            (energyDeliveredKwh / completedEV->getBatteryCapacityKwh()) * 100.0;
        finalSoc = std::max(0.0, std::min(100.0, finalSoc));
        const double totalCost = BASE_FEE + (energyDeliveredKwh * RATE_PER_KWH);
        const std::string endTime = getCurrentIsoTimestamp();
        const double newTotalEnergy =
            targetStation->getTotalEnergyDispensedKwh() + energyDeliveredKwh;

        QueuedRequest nextRequest{};
        bool dispatchNext = chargingQueue_.peek(nextRequest) &&
            nextRequest.requiredPort == targetStation->getPortType();
        EV* nextEV = nullptr;
        int newSessionId = 0;
        std::string newSessionStartTime;
        ChargingSession nextSession;

        if (dispatchNext) {
            for (auto& ev : evs) {
                if (ev.getId() == nextRequest.evId) {
                    nextEV = &ev;
                    break;
                }
            }
            if (!nextEV || nextEV->getUserId() != nextRequest.userId) {
                result.message = "The next FIFO request references a missing or mismatched EV.";
                return result;
            }
            newSessionId = nextSessionId_;
            newSessionStartTime = getCurrentIsoTimestamp();
            nextSession = ChargingSession(
                newSessionId,
                stationId,
                nextRequest.evId,
                nextRequest.userId,
                newSessionStartTime,
                nextEV->getCurrentSocPercent(),
                SessionStatus::IN_PROGRESS);
        }

        Transaction tx(
            nextTxId_,
            completedSessionId,
            savedSession.getUserId(),
            completedEvId,
            stationId,
            totalCost,
            energyDeliveredKwh,
            endTime,
            "PAID");

        if (!db.beginTransaction()) {
            result.message = "Could not begin database transaction.";
            return result;
        }

        bool persisted = db.completeSession(
                completedSessionId, endTime, finalSoc, energyDeliveredKwh, totalCost) &&
            db.updateEVSoc(completedEvId, finalSoc);

        int transactionId = 0;
        if (persisted) persisted = db.createTransaction(tx, transactionId);

        if (persisted && dispatchNext) {
            int savedNextSessionId = 0;
            persisted = db.createSession(nextSession, savedNextSessionId) &&
                db.deleteQueuedRequest(nextRequest.requestId);
        }

        if (persisted) {
            persisted = db.updateStationStatus(
                stationId,
                dispatchNext ? StationStatus::CHARGING : StationStatus::AVAILABLE,
                dispatchNext ? newSessionId : 0,
                dispatchNext ? nextRequest.evId : 0,
                newTotalEnergy);
        }

        // Re-number the remaining FIFO entries after removing the dispatched head.
        if (persisted && dispatchNext) {
            const auto pending = chargingQueue_.toVector();
            bool skippedDispatched = false;
            int position = 1;
            for (const auto& request : pending) {
                if (!skippedDispatched && request.requestId == nextRequest.requestId) {
                    skippedDispatched = true;
                    continue;
                }
                if (!db.saveQueuedRequest(
                        request.requestId,
                        request.userId,
                        request.evId,
                        portTypeToString(request.requiredPort),
                        request.requestTime,
                        position++)) {
                    persisted = false;
                    break;
                }
            }
        }

        if (!persisted) {
            db.rollbackTransaction();
            result.message = "SQLite rejected a charging completion update; no in-memory state was changed.";
            return result;
        }
        if (!db.commitTransaction()) {
            db.rollbackTransaction();
            result.message = "Could not commit charging completion to SQLite; no in-memory state was changed.";
            return result;
        }

        // Database commit succeeded. Mirror the committed state in memory.
        targetStation->releaseSession(energyDeliveredKwh);
        if (dispatchNext) {
            QueuedRequest removedRequest{};
            if (!chargingQueue_.dequeue(removedRequest) ||
                removedRequest.requestId != nextRequest.requestId) {
                // DB is already committed; warn loudly rather than hiding divergence.
                std::cerr << "[CRITICAL] FIFO queue diverged after database commit. Restart to recover from SQLite.\n";
            }
            targetStation->assignSession(newSessionId, nextRequest.evId);
            ++nextSessionId_;
            result.nextRequestAssigned = true;
            result.nextEvIdAssigned = nextRequest.evId;
        }

        recentTransactionsStack_.push(tx);
        ++nextTxId_;
        evs = db.getAllEVs();
        result.success = true;
        result.transaction = tx;
        result.message = dispatchNext
            ? "Session completed and station assigned to the next queued EV #" + std::to_string(nextRequest.evId) + "."
            : "Session completed successfully. Station is now available.";
        return result;
    }

private:
    std::string getCurrentIsoTimestamp() const {
        auto now = std::chrono::system_clock::now();
        std::time_t timestamp = std::chrono::system_clock::to_time_t(now);
        // All callers hold serviceMutex_, so the standard-library UTC buffer
        // is not concurrently used by another ChargingService operation.
        const std::tm* utc = std::gmtime(&timestamp);
        if (!utc) return {};
        std::stringstream ss;
        ss << std::put_time(utc, "%Y-%m-%dT%H:%M:%SZ");
        return ss.str();
    }
};

} // namespace evhub
