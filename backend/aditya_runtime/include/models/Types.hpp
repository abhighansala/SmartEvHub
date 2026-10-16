#pragma once

#include <string>
#include <iostream>

namespace evhub {

enum class StationStatus {
    AVAILABLE,
    CHARGING,
    MAINTENANCE,
    OFFLINE
};

inline std::string stationStatusToString(StationStatus status) {
    switch (status) {
        case StationStatus::AVAILABLE:   return "AVAILABLE";
        case StationStatus::CHARGING:    return "CHARGING";
        case StationStatus::MAINTENANCE: return "MAINTENANCE";
        case StationStatus::OFFLINE:     return "OFFLINE";
    }
    return "UNKNOWN";
}

inline StationStatus stringToStationStatus(const std::string& str) {
    if (str == "AVAILABLE")   return StationStatus::AVAILABLE;
    if (str == "CHARGING")    return StationStatus::CHARGING;
    if (str == "MAINTENANCE") return StationStatus::MAINTENANCE;
    return StationStatus::OFFLINE;
}

enum class SessionStatus {
    REQUESTED,
    QUEUED,
    IN_PROGRESS,
    COMPLETED,
    CANCELLED
};

inline std::string sessionStatusToString(SessionStatus status) {
    switch (status) {
        case SessionStatus::REQUESTED:   return "REQUESTED";
        case SessionStatus::QUEUED:      return "QUEUED";
        case SessionStatus::IN_PROGRESS: return "IN_PROGRESS";
        case SessionStatus::COMPLETED:   return "COMPLETED";
        case SessionStatus::CANCELLED:   return "CANCELLED";
    }
    return "UNKNOWN";
}


inline SessionStatus stringToSessionStatus(const std::string& str) {
    if (str == "REQUESTED")   return SessionStatus::REQUESTED;
    if (str == "QUEUED")      return SessionStatus::QUEUED;
    if (str == "IN_PROGRESS") return SessionStatus::IN_PROGRESS;
    if (str == "COMPLETED")   return SessionStatus::COMPLETED;
    if (str == "CANCELLED")   return SessionStatus::CANCELLED;
    return SessionStatus::REQUESTED;
}


enum class PortType {
    CCS2,       // Combined Charging System 2 (DC Fast)
    TYPE_2,     // Mennekes (AC Fast)
    CHADEMO,    // DC Fast (Nissan/Mitsubishi)
    GB_T        // Bharat EV standard / China
};

inline std::string portTypeToString(PortType port) {
    switch (port) {
        case PortType::CCS2:    return "CCS2";
        case PortType::TYPE_2:  return "TYPE_2";
        case PortType::CHADEMO: return "CHADEMO";
        case PortType::GB_T:    return "GB_T";
    }
    return "UNKNOWN";
}

inline PortType stringToPortType(const std::string& str) {
    if (str == "CCS2")    return PortType::CCS2;
    if (str == "TYPE_2")  return PortType::TYPE_2;
    if (str == "CHADEMO") return PortType::CHADEMO;
    return PortType::GB_T;
}

enum class BatteryStatus {
    READY,
    CHARGING,
    FAULTY,
    IN_USE
};

inline std::string batteryStatusToString(BatteryStatus status) {
    switch (status) {
        case BatteryStatus::READY:    return "READY";
        case BatteryStatus::CHARGING: return "CHARGING";
        case BatteryStatus::FAULTY:   return "FAULTY";
        case BatteryStatus::IN_USE:   return "IN_USE";
    }
    return "UNKNOWN";
}

} // namespace evhub
