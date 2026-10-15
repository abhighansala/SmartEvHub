#ifndef STATION_SERVICE_H
#define STATION_SERVICE_H

#include "../models/charging_station.h"
#include "../../aditya_singh/database/database.h"
#include <string>

const int MAX_STATIONS = 20;

// Simple Service to manage Charging Stations
// Uses simple C++ arrays (no vectors)
// Created by: Mritunjay Thakur (Charging Stations Module Owner, ID: 2519425)
class StationService {
private:
    Database* db;
    ChargingStation stationArray[MAX_STATIONS];
    int stationCount;

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
};

#endif
