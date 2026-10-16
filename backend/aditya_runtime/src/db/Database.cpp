#include "db/Database.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace evhub {

std::unique_ptr<Database> Database::instance_ = nullptr;
std::mutex Database::instanceMutex_;

Database& Database::getInstance(const std::string& dbPath) {
    std::lock_guard<std::mutex> lock(instanceMutex_);
    if (!instance_) {
        instance_ = std::make_unique<Database>(dbPath);
        instance_->open();
        instance_->initializeSchema();
    }
    return *instance_;
}

Database::Database(std::string dbPath) : db_(nullptr), dbPath_(std::move(dbPath)) {}

Database::~Database() {
    close();
}

bool Database::open() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (db_) return true;

    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database] Failed to open SQLite database: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    // Enable WAL mode & foreign keys for high reliability and concurrency
    sqlite3_exec(db_, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA busy_timeout = 5000;", nullptr, nullptr, nullptr);

    std::cout << "[Database] SQLite initialized successfully at: " << dbPath_ << "\n";
    return true;
}

void Database::close() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::initializeSchema() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            email TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            role TEXT DEFAULT 'DRIVER',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS evs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            vin TEXT UNIQUE NOT NULL,
            make TEXT NOT NULL,
            model TEXT NOT NULL,
            license_plate TEXT NOT NULL,
            battery_capacity_kwh REAL NOT NULL,
            current_soc_percent REAL NOT NULL,
            supported_port TEXT NOT NULL,
            FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS stations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            port_type TEXT NOT NULL,
            max_power_kw REAL NOT NULL,
            status TEXT NOT NULL DEFAULT 'AVAILABLE',
            current_session_id INTEGER DEFAULT 0,
            assigned_ev_id INTEGER DEFAULT 0,
            total_energy_dispensed REAL DEFAULT 0.0
        );

        CREATE TABLE IF NOT EXISTS sessions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            station_id INTEGER NOT NULL,
            ev_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            start_time TEXT NOT NULL,
            end_time TEXT DEFAULT '',
            initial_soc_percent REAL NOT NULL,
            final_soc_percent REAL NOT NULL,
            energy_delivered_kwh REAL DEFAULT 0.0,
            total_cost REAL DEFAULT 0.0,
            status TEXT NOT NULL,
            FOREIGN KEY (station_id) REFERENCES stations(id),
            FOREIGN KEY (ev_id) REFERENCES evs(id),
            FOREIGN KEY (user_id) REFERENCES users(id)
        );

        CREATE TABLE IF NOT EXISTS transactions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id INTEGER UNIQUE NOT NULL,
            user_id INTEGER NOT NULL,
            ev_id INTEGER NOT NULL,
            station_id INTEGER NOT NULL,
            amount REAL NOT NULL,
            energy_kwh REAL NOT NULL,
            timestamp TEXT NOT NULL,
            payment_status TEXT DEFAULT 'PAID',
            FOREIGN KEY (session_id) REFERENCES sessions(id)
        );

         CREATE TABLE IF NOT EXISTS charging_queue (
         request_id INTEGER PRIMARY KEY,
         user_id INTEGER NOT NULL,
         ev_id INTEGER NOT NULL,
         required_port TEXT NOT NULL,
         request_time TEXT NOT NULL,
         queue_position INTEGER NOT NULL
        );

        CREATE INDEX IF NOT EXISTS idx_sessions_status ON sessions(status);
        CREATE INDEX IF NOT EXISTS idx_stations_status ON stations(status);
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, schema, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database] Schema initialization error: " << (errMsg ? errMsg : "") << "\n";
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

bool Database::beginTransaction() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    return sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool Database::commitTransaction() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    return sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool Database::rollbackTransaction() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    return sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr) == SQLITE_OK;
}

std::string Database::hashPassword(const std::string& plainPassword) {
    // Academic simulated salted hash for B.Tech project viva
    unsigned long hash = 5381;
    std::string salted = "evhub_salt_" + plainPassword;
    for (char c : salted) {
        hash = ((hash << 5) + hash) + static_cast<unsigned char>(c);
    }
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return ss.str();
}

bool Database::createUser(const User& user, int& outId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    const char* sql = "INSERT INTO users (name, email, password_hash, role) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, user.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.getEmail().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.getPasswordHash().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, user.getRole().c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    bool ok = (rc == SQLITE_DONE);
    if (ok) {
        outId = static_cast<int>(sqlite3_last_insert_rowid(db_));
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::getUserByEmail(const std::string& email, User& outUser) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    const char* sql = "SELECT id, name, email, password_hash, role, created_at FROM users WHERE email = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string em = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        std::string role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char* dt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        std::string createdAt = dt ? dt : "";

        outUser = User(id, name, em, hash, role, createdAt);
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}


bool Database::getUserById(int id, User& outUser) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql =
        "SELECT id, name, email, password_hash, role, created_at "
        "FROM users WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    bool found = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* nameText = sqlite3_column_text(stmt, 1);
        const auto* emailText = sqlite3_column_text(stmt, 2);
        const auto* hashText = sqlite3_column_text(stmt, 3);
        const auto* roleText = sqlite3_column_text(stmt, 4);
        const auto* createdText = sqlite3_column_text(stmt, 5);

        outUser = User(
            sqlite3_column_int(stmt, 0),
            nameText ? reinterpret_cast<const char*>(nameText) : "",
            emailText ? reinterpret_cast<const char*>(emailText) : "",
            hashText ? reinterpret_cast<const char*>(hashText) : "",
            roleText ? reinterpret_cast<const char*>(roleText) : "DRIVER",
            createdText ? reinterpret_cast<const char*>(createdText) : ""
        );
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}


bool Database::createEV(const EV& ev, int& outId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    const char* sql = "INSERT INTO evs (user_id, vin, make, model, license_plate, battery_capacity_kwh, current_soc_percent, supported_port) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, ev.getUserId());
    sqlite3_bind_text(stmt, 2, ev.getVin().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, ev.getMake().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, ev.getModel().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, ev.getLicensePlate().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 6, ev.getBatteryCapacityKwh());
    sqlite3_bind_double(stmt, 7, ev.getCurrentSocPercent());
    sqlite3_bind_text(stmt, 8, portTypeToString(ev.getSupportedPort()).c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    bool ok = (rc == SQLITE_DONE);
    if (ok) {
        outId = static_cast<int>(sqlite3_last_insert_rowid(db_));
    }
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<EV> Database::getAllEVs() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<EV> list;
    const char* sql = "SELECT id, user_id, vin, make, model, license_plate, battery_capacity_kwh, current_soc_percent, supported_port FROM evs;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int userId = sqlite3_column_int(stmt, 1);
        std::string vin = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string make = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        std::string model = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        std::string plate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        double cap = sqlite3_column_double(stmt, 6);
        double soc = sqlite3_column_double(stmt, 7);
        std::string portStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

        list.emplace_back(id, userId, vin, make, model, plate, cap, soc, stringToPortType(portStr));
    }
    sqlite3_finalize(stmt);
    return list;
}


bool Database::getEVById(int id, EV& outEv) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql =
        "SELECT id, user_id, vin, make, model, license_plate, "
        "battery_capacity_kwh, current_soc_percent, supported_port "
        "FROM evs WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    bool found = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* vinText = sqlite3_column_text(stmt, 2);
        const auto* makeText = sqlite3_column_text(stmt, 3);
        const auto* modelText = sqlite3_column_text(stmt, 4);
        const auto* plateText = sqlite3_column_text(stmt, 5);
        const auto* portText = sqlite3_column_text(stmt, 8);

        outEv = EV(
            sqlite3_column_int(stmt, 0),
            sqlite3_column_int(stmt, 1),
            vinText ? reinterpret_cast<const char*>(vinText) : "",
            makeText ? reinterpret_cast<const char*>(makeText) : "",
            modelText ? reinterpret_cast<const char*>(modelText) : "",
            plateText ? reinterpret_cast<const char*>(plateText) : "",
            sqlite3_column_double(stmt, 6),
            sqlite3_column_double(stmt, 7),
            stringToPortType(
                portText ? reinterpret_cast<const char*>(portText) : "CCS2")
        );
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

std::vector<EV> Database::getEVsByUserId(int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<EV> list;
    if (!db_) return list;

    const char* sql =
        "SELECT id, user_id, vin, make, model, license_plate, "
        "battery_capacity_kwh, current_soc_percent, supported_port "
        "FROM evs WHERE user_id = ? ORDER BY id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return list;
    }

    sqlite3_bind_int(stmt, 1, userId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* vinText = sqlite3_column_text(stmt, 2);
        const auto* makeText = sqlite3_column_text(stmt, 3);
        const auto* modelText = sqlite3_column_text(stmt, 4);
        const auto* plateText = sqlite3_column_text(stmt, 5);
        const auto* portText = sqlite3_column_text(stmt, 8);

        list.emplace_back(
            sqlite3_column_int(stmt, 0),
            sqlite3_column_int(stmt, 1),
            vinText ? reinterpret_cast<const char*>(vinText) : "",
            makeText ? reinterpret_cast<const char*>(makeText) : "",
            modelText ? reinterpret_cast<const char*>(modelText) : "",
            plateText ? reinterpret_cast<const char*>(plateText) : "",
            sqlite3_column_double(stmt, 6),
            sqlite3_column_double(stmt, 7),
            stringToPortType(
                portText ? reinterpret_cast<const char*>(portText) : "CCS2")
        );
    }

    sqlite3_finalize(stmt);
    return list;
}

bool Database::updateEVSoc(int evId, double newSoc) {
    if (newSoc < 0.0 || newSoc > 100.0) return false;

    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql =
        "UPDATE evs SET current_soc_percent = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_double(stmt, 1, newSoc);
    sqlite3_bind_int(stmt, 2, evId);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE &&
              sqlite3_changes(db_) > 0;

    if (!ok) {
        std::cerr << "[Database] updateEVSoc failed: "
                  << sqlite3_errmsg(db_) << "\n";
    }

    sqlite3_finalize(stmt);
    return ok;
}


std::vector<ChargingStation> Database::getAllStations() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<ChargingStation> list;
    const char* sql = "SELECT id, name, port_type, max_power_kw, status, current_session_id, assigned_ev_id, total_energy_dispensed FROM stations;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string portStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        double power = sqlite3_column_double(stmt, 3);
        std::string statusStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        int sessId = sqlite3_column_int(stmt, 5);
        int evId = sqlite3_column_int(stmt, 6);
        double energy = sqlite3_column_double(stmt, 7);

        list.emplace_back(id, name, stringToPortType(portStr), power, stringToStationStatus(statusStr), sessId, evId, energy);
    }
    sqlite3_finalize(stmt);
    return list;
}


bool Database::createStation(const ChargingStation& station, int& outId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sqlWithId =
        "INSERT INTO stations "
        "(id, name, port_type, max_power_kw, status, current_session_id, "
        "assigned_ev_id, total_energy_dispensed) VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

    const char* sqlWithoutId =
        "INSERT INTO stations "
        "(name, port_type, max_power_kw, status, current_session_id, "
        "assigned_ev_id, total_energy_dispensed) VALUES (?, ?, ?, ?, ?, ?, ?);";

    bool explicitId = station.getId() > 0;
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            db_, explicitId ? sqlWithId : sqlWithoutId,
            -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    int i = 1;
    if (explicitId) sqlite3_bind_int(stmt, i++, station.getId());

    sqlite3_bind_text(stmt, i++, station.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, portTypeToString(station.getPortType()).c_str(),
                      -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, i++, station.getMaxPowerKw());
    sqlite3_bind_text(stmt, i++, stationStatusToString(station.getStatus()).c_str(),
                      -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, i++, station.getCurrentSessionId());
    sqlite3_bind_int(stmt, i++, station.getAssignedEvId());
    sqlite3_bind_double(stmt, i++, station.getTotalEnergyDispensedKwh());

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    if (ok) {
        outId = explicitId
            ? station.getId()
            : static_cast<int>(sqlite3_last_insert_rowid(db_));
    } else {
        std::cerr << "[Database] createStation failed: "
                  << sqlite3_errmsg(db_) << "\n";
    }

    sqlite3_finalize(stmt);
    return ok;
}

bool Database::getStationById(int id, ChargingStation& outStation) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql =
        "SELECT id, name, port_type, max_power_kw, status, "
        "current_session_id, assigned_ev_id, total_energy_dispensed "
        "FROM stations WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    bool found = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* nameText = sqlite3_column_text(stmt, 1);
        const auto* portText = sqlite3_column_text(stmt, 2);
        const auto* statusText = sqlite3_column_text(stmt, 4);

        std::string name = nameText
            ? reinterpret_cast<const char*>(nameText) : "";
        std::string port = portText
            ? reinterpret_cast<const char*>(portText) : "CCS2";
        std::string status = statusText
            ? reinterpret_cast<const char*>(statusText) : "OFFLINE";

        outStation = ChargingStation(
            sqlite3_column_int(stmt, 0),
            name,
            stringToPortType(port),
            sqlite3_column_double(stmt, 3),
            stringToStationStatus(status),
            sqlite3_column_int(stmt, 5),
            sqlite3_column_int(stmt, 6),
            sqlite3_column_double(stmt, 7)
        );
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool Database::updateStationStatus(
    int stationId,
    StationStatus status,
    int currentSessionId,
    int assignedEvId,
    double totalEnergyDispensed) {

    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql =
        "UPDATE stations SET status = ?, current_session_id = ?, "
        "assigned_ev_id = ?, total_energy_dispensed = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, stationStatusToString(status).c_str(),
                      -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, currentSessionId);
    sqlite3_bind_int(stmt, 3, assignedEvId);
    sqlite3_bind_double(stmt, 4, totalEnergyDispensed);
    sqlite3_bind_int(stmt, 5, stationId);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE &&
              sqlite3_changes(db_) > 0;

    if (!ok) {
        std::cerr << "[Database] updateStationStatus failed: "
                  << sqlite3_errmsg(db_) << "\n";
    }

    sqlite3_finalize(stmt);
    return ok;
}

bool Database::createTransaction(const Transaction& tx, int& outId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sqlWithId =
        "INSERT INTO transactions "
        "(id, session_id, user_id, ev_id, station_id, amount, energy_kwh, "
        "timestamp, payment_status) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

    const char* sqlWithoutId =
        "INSERT INTO transactions "
        "(session_id, user_id, ev_id, station_id, amount, energy_kwh, "
        "timestamp, payment_status) VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

    bool explicitId = tx.getId() > 0;
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            db_, explicitId ? sqlWithId : sqlWithoutId,
            -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    int i = 1;
    if (explicitId) sqlite3_bind_int(stmt, i++, tx.getId());

    sqlite3_bind_int(stmt, i++, tx.getSessionId());
    sqlite3_bind_int(stmt, i++, tx.getUserId());
    sqlite3_bind_int(stmt, i++, tx.getEvId());
    sqlite3_bind_int(stmt, i++, tx.getStationId());
    sqlite3_bind_double(stmt, i++, tx.getAmount());
    sqlite3_bind_double(stmt, i++, tx.getEnergyKwh());
    sqlite3_bind_text(stmt, i++, tx.getTimestamp().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, tx.getPaymentStatus().c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    if (ok) {
        outId = explicitId
            ? tx.getId()
            : static_cast<int>(sqlite3_last_insert_rowid(db_));
    } else {
        std::cerr << "[Database] createTransaction failed: "
                  << sqlite3_errmsg(db_) << "\n";
    }

    sqlite3_finalize(stmt);
    return ok;
}

std::vector<Transaction> Database::getAllTransactions() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<Transaction> list;
    if (!db_) return list;

    const char* sql =
        "SELECT id, session_id, user_id, ev_id, station_id, amount, "
        "energy_kwh, timestamp, payment_status "
        "FROM transactions ORDER BY id DESC;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return list;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* timestampText = sqlite3_column_text(stmt, 7);
        const auto* statusText = sqlite3_column_text(stmt, 8);

        std::string timestamp = timestampText
            ? reinterpret_cast<const char*>(timestampText) : "";
        std::string paymentStatus = statusText
            ? reinterpret_cast<const char*>(statusText) : "PENDING";

        list.emplace_back(
            sqlite3_column_int(stmt, 0),
            sqlite3_column_int(stmt, 1),
            sqlite3_column_int(stmt, 2),
            sqlite3_column_int(stmt, 3),
            sqlite3_column_int(stmt, 4),
            sqlite3_column_double(stmt, 5),
            sqlite3_column_double(stmt, 6),
            timestamp,
            paymentStatus
        );
    }

    sqlite3_finalize(stmt);
    return list;
}

bool Database::createSession(const ChargingSession& session, int& outId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql =
        "INSERT INTO sessions "
        "(id, station_id, ev_id, user_id, start_time, end_time, "
        "initial_soc_percent, final_soc_percent, energy_delivered_kwh, "
        "total_cost, status) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Database] createSession prepare failed: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, session.getId());
    sqlite3_bind_int(stmt, 2, session.getStationId());
    sqlite3_bind_int(stmt, 3, session.getEvId());
    sqlite3_bind_int(stmt, 4, session.getUserId());
    sqlite3_bind_text(stmt, 5, session.getStartTime().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, session.getEndTime().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 7, session.getInitialSocPercent());
    sqlite3_bind_double(stmt, 8, session.getFinalSocPercent());
    sqlite3_bind_double(stmt, 9, session.getEnergyDeliveredKwh());
    sqlite3_bind_double(stmt, 10, session.getTotalCost());

    std::string status = sessionStatusToString(session.getStatus());
    sqlite3_bind_text(stmt, 11, status.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    if (ok) {
        outId = session.getId();
    } else {
        std::cerr << "[Database] createSession failed: "
                  << sqlite3_errmsg(db_) << "\n";
    }

    sqlite3_finalize(stmt);
    return ok;
}

bool Database::getSessionById(int id, ChargingSession& outSession) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql =
        "SELECT id, station_id, ev_id, user_id, start_time, end_time, "
        "initial_soc_percent, final_soc_percent, energy_delivered_kwh, "
        "total_cost, status FROM sessions WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    bool found = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* startText = sqlite3_column_text(stmt, 4);
        const auto* endText = sqlite3_column_text(stmt, 5);
        const auto* statusText = sqlite3_column_text(stmt, 10);

        std::string start = startText
            ? reinterpret_cast<const char*>(startText) : "";
        std::string end = endText
            ? reinterpret_cast<const char*>(endText) : "";
        std::string status = statusText
            ? reinterpret_cast<const char*>(statusText) : "REQUESTED";

        outSession = ChargingSession(
            sqlite3_column_int(stmt, 0),
            sqlite3_column_int(stmt, 1),
            sqlite3_column_int(stmt, 2),
            sqlite3_column_int(stmt, 3),
            start,
            sqlite3_column_double(stmt, 6),
            stringToSessionStatus(status)
        );

        // Restore fields not covered by the constructor.
        outSession.setStatus(stringToSessionStatus(status));

        // Completed-session details are read from SQLite, but the current
        // model has no setters for end time, final SOC, energy, or cost.
        // Those fields will be restored after we extend the model safely.
        (void)end;
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

std::vector<ChargingSession> Database::getActiveSessions() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<ChargingSession> list;
    if (!db_) return list;

    const char* sql =
        "SELECT id, station_id, ev_id, user_id, start_time, "
        "initial_soc_percent, status FROM sessions "
        "WHERE status = 'IN_PROGRESS' ORDER BY id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return list;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* startText = sqlite3_column_text(stmt, 4);
        const auto* statusText = sqlite3_column_text(stmt, 6);

        std::string start = startText
            ? reinterpret_cast<const char*>(startText) : "";
        std::string status = statusText
            ? reinterpret_cast<const char*>(statusText) : "IN_PROGRESS";

        list.emplace_back(
            sqlite3_column_int(stmt, 0),
            sqlite3_column_int(stmt, 1),
            sqlite3_column_int(stmt, 2),
            sqlite3_column_int(stmt, 3),
            start,
            sqlite3_column_double(stmt, 5),
            stringToSessionStatus(status)
        );
    }

    sqlite3_finalize(stmt);
    return list;
}


bool Database::completeSession(
    int sessionId,
    const std::string& endTime,
    double finalSoc,
    double energyKwh,
    double cost) {

    if (finalSoc < 0.0 || finalSoc > 100.0 ||
        energyKwh < 0.0 || cost < 0.0) {
        std::cerr << "[Database] Invalid session completion values.\n";
        return false;
    }

    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql =
        "UPDATE sessions "
        "SET end_time = ?, final_soc_percent = ?, "
        "energy_delivered_kwh = ?, total_cost = ?, status = 'COMPLETED' "
        "WHERE id = ? AND status = 'IN_PROGRESS';";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Database] completeSession prepare failed: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_text(
        stmt, 1, endTime.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, finalSoc);
    sqlite3_bind_double(stmt, 3, energyKwh);
    sqlite3_bind_double(stmt, 4, cost);
    sqlite3_bind_int(stmt, 5, sessionId);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE &&
              sqlite3_changes(db_) == 1;

    if (!ok) {
        std::cerr << "[Database] completeSession failed: "
                  << sqlite3_errmsg(db_) << "\n";
    }

    sqlite3_finalize(stmt);
    return ok;
}


bool Database::saveQueuedRequest(
    int requestId,
    int userId,
    int evId,
    const std::string& requiredPort,
    const std::string& requestTime,
    int queuePosition) {

    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql =
        "INSERT INTO charging_queue "
        "(request_id, user_id, ev_id, required_port, request_time, queue_position) "
        "VALUES (?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(request_id) DO UPDATE SET "
        "user_id = excluded.user_id, "
        "ev_id = excluded.ev_id, "
        "required_port = excluded.required_port, "
        "request_time = excluded.request_time, "
        "queue_position = excluded.queue_position;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Database] saveQueuedRequest prepare failed: "
                  << sqlite3_errmsg(db_) << '\n';
        return false;
    }

    sqlite3_bind_int(stmt, 1, requestId);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_int(stmt, 3, evId);
    sqlite3_bind_text(stmt, 4, requiredPort.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, requestTime.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, queuePosition);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;

    if (!ok) {
        std::cerr << "[Database] saveQueuedRequest failed: "
                  << sqlite3_errmsg(db_) << '\n';
    }

    sqlite3_finalize(stmt);
    return ok;
}

std::vector<PersistedQueuedRequest> Database::getQueuedRequests() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<PersistedQueuedRequest> requests;
    if (!db_) return requests;

    const char* sql =
        "SELECT request_id, user_id, ev_id, required_port, "
        "request_time, queue_position "
        "FROM charging_queue ORDER BY queue_position ASC, request_id ASC;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Database] getQueuedRequests prepare failed: "
                  << sqlite3_errmsg(db_) << '\n';
        return requests;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* portText = sqlite3_column_text(stmt, 3);
        const auto* timeText = sqlite3_column_text(stmt, 4);

        requests.push_back({
            sqlite3_column_int(stmt, 0),
            sqlite3_column_int(stmt, 1),
            sqlite3_column_int(stmt, 2),
            portText ? reinterpret_cast<const char*>(portText) : "",
            timeText ? reinterpret_cast<const char*>(timeText) : "",
            sqlite3_column_int(stmt, 5)
        });
    }

    sqlite3_finalize(stmt);
    return requests;
}

bool Database::deleteQueuedRequest(int requestId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql =
        "DELETE FROM charging_queue WHERE request_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Database] deleteQueuedRequest prepare failed: "
                  << sqlite3_errmsg(db_) << '\n';
        return false;
    }

    sqlite3_bind_int(stmt, 1, requestId);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;

    if (!ok) {
        std::cerr << "[Database] deleteQueuedRequest failed: "
                  << sqlite3_errmsg(db_) << '\n';
    }

    sqlite3_finalize(stmt);
    return ok;
}

bool Database::clearChargingQueue() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return false;

    const char* sql = "DELETE FROM charging_queue;";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "[Database] clearChargingQueue failed: "
                  << (errMsg ? errMsg : "") << '\n';
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}


int Database::getMaxSessionId() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return 0;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT COALESCE(MAX(id), 0) FROM sessions;";

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Database] getMaxSessionId failed: "
                  << sqlite3_errmsg(db_) << '\n';
        return 0;
    }

    int maxId = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        maxId = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return maxId;
}

int Database::getMaxTransactionId() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return 0;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT COALESCE(MAX(id), 0) FROM transactions;";

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Database] getMaxTransactionId failed: "
                  << sqlite3_errmsg(db_) << '\n';
        return 0;
    }

    int maxId = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        maxId = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return maxId;
}

} // namespace evhub
