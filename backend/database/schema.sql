-- ============================================================================
-- Smart EV Hub - Comprehensive Database Schema & Seed Data
-- Database Engine: SQLite (evhub.db)
-- Module Owner: Aditya Singh (Database + Backend Foundation, Student ID: 2519272)
-- ============================================================================

PRAGMA foreign_keys = ON;

-- ----------------------------------------------------------------------------
-- 1. Users Table (Authentication & Access Control)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS Users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    email TEXT UNIQUE NOT NULL,
    password TEXT NOT NULL,
    role TEXT NOT NULL DEFAULT 'user' CHECK(role IN ('admin', 'operator', 'user')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- ----------------------------------------------------------------------------
-- 2. EV Table (Electric Vehicles)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS EV (
    id TEXT PRIMARY KEY,
    owner_name TEXT NOT NULL,
    model TEXT NOT NULL,
    battery_capacity REAL NOT NULL DEFAULT 50.0,
    current_charge REAL NOT NULL DEFAULT 20.0 CHECK(current_charge >= 0.0 AND current_charge <= 100.0),
    status TEXT NOT NULL DEFAULT 'available' CHECK(status IN ('available', 'charging', 'waiting', 'swapping')),
    last_activity DATETIME DEFAULT CURRENT_TIMESTAMP,
    registered_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- ----------------------------------------------------------------------------
-- 3. ChargingStation Table (Charging Bays & Hardware)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS ChargingStation (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    power REAL NOT NULL DEFAULT 50.0,
    connector_type TEXT NOT NULL DEFAULT 'CCS2' CHECK(connector_type IN ('Type 2', 'CCS2', 'CHAdeMO', 'GB/T')),
    status TEXT NOT NULL DEFAULT 'available' CHECK(status IN ('available', 'charging', 'maintenance', 'offline')),
    current_vehicle_id TEXT DEFAULT NULL,
    session_start DATETIME DEFAULT NULL,
    location TEXT NOT NULL DEFAULT 'Bay 1',
    total_ports INTEGER DEFAULT 1,
    available_ports INTEGER DEFAULT 1,
    FOREIGN KEY(current_vehicle_id) REFERENCES EV(id) ON DELETE SET NULL
);

-- ----------------------------------------------------------------------------
-- 4. Battery Table (Battery Swap Inventory)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS Battery (
    id TEXT PRIMARY KEY,
    capacity REAL NOT NULL DEFAULT 60.0,
    charge_level REAL NOT NULL DEFAULT 100.0 CHECK(charge_level >= 0.0 AND charge_level <= 100.0),
    status TEXT NOT NULL DEFAULT 'available' CHECK(status IN ('available', 'inuse', 'charging', 'maintenance', 'unavailable')),
    health REAL NOT NULL DEFAULT 100.0 CHECK(health >= 0.0 AND health <= 100.0),
    cycle_count INTEGER DEFAULT 0,
    current_vehicle TEXT DEFAULT NULL,
    last_updated DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(current_vehicle) REFERENCES EV(id) ON DELETE SET NULL
);

-- ----------------------------------------------------------------------------
-- 5. ChargingSession Table (Active & Completed Sessions)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS ChargingSession (
    id TEXT PRIMARY KEY,
    vehicle_id TEXT NOT NULL,
    ev_model TEXT NOT NULL,
    station_id TEXT NOT NULL,
    start_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    end_time DATETIME DEFAULT NULL,
    battery_start REAL NOT NULL,
    battery_end REAL DEFAULT NULL,
    target_charge REAL NOT NULL DEFAULT 100.0,
    energy_consumed REAL DEFAULT 0.0,
    estimated_cost REAL DEFAULT 0.0,
    status TEXT NOT NULL DEFAULT 'active' CHECK(status IN ('active', 'completed')),
    FOREIGN KEY(vehicle_id) REFERENCES EV(id) ON DELETE CASCADE,
    FOREIGN KEY(station_id) REFERENCES ChargingStation(id) ON DELETE CASCADE
);

-- ----------------------------------------------------------------------------
-- 6. TransactionLog Table (Completed Operations / Stack History)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS TransactionLog (
    id TEXT PRIMARY KEY,
    vehicle_id TEXT NOT NULL,
    type TEXT NOT NULL CHECK(type IN ('charging', 'battery-swap')),
    station_or_battery TEXT NOT NULL,
    description TEXT DEFAULT '',
    amount REAL NOT NULL,
    energy_delivered REAL DEFAULT 0.0,
    duration INTEGER DEFAULT 0,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    status TEXT NOT NULL DEFAULT 'completed' CHECK(status IN ('completed', 'pending', 'failed')),
    FOREIGN KEY(vehicle_id) REFERENCES EV(id) ON DELETE CASCADE
);

-- ============================================================================
-- Seed Data for Development & Testing
-- ============================================================================

-- Users (default passwords provided for testing: admin123, operator123, user123)
INSERT OR IGNORE INTO Users (id, name, email, password, role) VALUES
(1, 'Admin User', 'admin@evhub.com', 'admin123', 'admin'),
(2, 'Site Operator', 'operator@evhub.com', 'operator123', 'operator'),
(3, 'Abhishek Ghansala', 'abhishek@evhub.com', 'abhishek', 'admin'),
(4, 'Aditya Singh', 'aditya@evhub.com', 'aditya', 'admin'),
(5, 'Priyanshu Dutt', 'priyanshu@evhub.com', 'priyanshu', 'admin'),
(6, 'Mritunjay Thakur', 'mritunjay@evhub.com', 'mritunjay', 'admin'),
(7, 'Amit Singh Kaintura', 'amit@evhub.com', 'amit', 'admin'),
(8, 'Agreev Singh', 'agreev@evhub.com', 'agreev', 'admin');

-- Electric Vehicles
INSERT OR IGNORE INTO EV (id, owner_name, model, battery_capacity, current_charge, status, last_activity) VALUES
('EV-101', 'Rahul Sharma', 'Tata Nexon EV Max', 40.5, 32.0, 'charging', datetime('now', '-25 minutes')),
('EV-102', 'Priya Patel', 'MG ZS EV', 50.3, 18.0, 'waiting', datetime('now', '-10 minutes')),
('EV-103', 'Vikram Malhotra', 'Hyundai Ioniq 5', 72.6, 74.0, 'available', datetime('now', '-1 hour')),
('EV-104', 'Ananya Iyer', 'Mahindra XUV400', 39.4, 12.0, 'waiting', datetime('now', '-5 minutes')),
('EV-105', 'Karan Verma', 'BYD Atto 3', 60.5, 45.0, 'available', datetime('now', '-2 hours')),
('EV-106', 'Sneha Reddy', 'Kia EV6', 77.4, 91.0, 'available', datetime('now', '-30 minutes')),
('EV-125', 'Vikram Malhotra', 'Volvo EX90 (100 kWh)', 100.0, 0.0, 'waiting', datetime('now', '-2 minutes'));

-- Charging Stations (Rated between 20 kW and 30 kW; 25 kW = 25 kWh/hr standard)
INSERT OR IGNORE INTO ChargingStation (id, name, power, connector_type, status, current_vehicle_id, session_start, location, total_ports, available_ports) VALUES
('CS-1', 'Standard Bay 1', 25.0, 'CCS2', 'charging', 'EV-101', datetime('now', '-25 minutes'), 'Bay A-1', 1, 0),
('CS-2', 'Fast Bay 2', 30.0, 'CCS2', 'available', NULL, NULL, 'Bay A-2', 1, 1),
('CS-3', 'Flex Bay 3', 28.0, 'CCS2', 'available', NULL, NULL, 'Bay B-1', 1, 1),
('CS-4', 'AC Type 2 Charger 4', 22.0, 'Type 2', 'maintenance', NULL, NULL, 'Bay B-2', 1, 0);

-- Batteries for Swap Inventory
INSERT OR IGNORE INTO Battery (id, capacity, charge_level, status, health, cycle_count, current_vehicle, last_updated) VALUES
('BAT-01', 50.0, 100.0, 'available', 98.5, 42, NULL, datetime('now')),
('BAT-02', 50.0, 100.0, 'available', 97.0, 58, NULL, datetime('now')),
('BAT-03', 60.0, 100.0, 'available', 99.2, 18, NULL, datetime('now')),
('BAT-04', 50.0, 44.0, 'charging', 95.0, 89, NULL, datetime('now', '-1 hour')),
('BAT-05', 40.0, 15.0, 'charging', 92.4, 120, NULL, datetime('now', '-45 minutes')),
('BAT-06', 70.0, 88.0, 'inuse', 96.8, 65, 'EV-103', datetime('now', '-2 hours')),
('BAT-07', 50.0, 10.0, 'maintenance', 81.0, 240, NULL, datetime('now', '-1 day'));

-- Charging Sessions
INSERT OR IGNORE INTO ChargingSession (id, vehicle_id, ev_model, station_id, start_time, end_time, battery_start, battery_end, target_charge, energy_consumed, estimated_cost, status) VALUES
('SES-1001', 'EV-101', 'Tata Nexon EV Max', 'CS-1', datetime('now', '-25 minutes'), NULL, 12.0, NULL, 90.0, 14.5, 290.0, 'active'),
('SES-1000', 'EV-103', 'Hyundai Ioniq 5', 'CS-2', datetime('now', '-90 minutes'), datetime('now', '-30 minutes'), 20.0, 80.0, 80.0, 43.5, 870.0, 'completed');

-- Transactions Log (Seed for Stack)
INSERT OR IGNORE INTO TransactionLog (id, vehicle_id, type, station_or_battery, description, amount, energy_delivered, duration, timestamp, status) VALUES
('TXN-5001', 'EV-103', 'charging', 'CS-2', 'Fast charging 20% to 80%', 870.0, 43.5, 60, datetime('now', '-30 minutes'), 'completed'),
('TXN-5002', 'EV-105', 'battery-swap', 'BAT-03', 'Swapped depleted battery with BAT-03 (100%)', 450.0, 60.0, 5, datetime('now', '-2 hours'), 'completed'),
('TXN-5003', 'EV-106', 'charging', 'CS-3', 'Ultra-fast charge 40% to 91%', 680.0, 39.4, 35, datetime('now', '-4 hours'), 'completed');