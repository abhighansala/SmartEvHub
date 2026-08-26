// ─── Login Types ───────────────────────────────────────────────────────────

export interface User {
  id: string;
  name: string;
  role: 'admin' | 'operator' | 'user';
  password: string;
}

// ─── EV Types ────────────────────────────────────────────────────────────────

export type EVStatus = 'available' | 'charging' | 'waiting' | 'swapping';

export interface EV {
  id: string;
  owner: string;
  model: string;
  batteryCapacity: number;  // kWh
  currentCharge: number;    // percentage 0–100
  status: EVStatus;
  lastActivity: string;     // ISO date string
  registeredAt: string;     // ISO date string
}

// ─── Station Types ───────────────────────────────────────────────────────────

export type StationStatus = 'available' | 'charging' | 'maintenance' | 'offline';
export type ConnectorType = 'Type 2' | 'CCS2' | 'CHAdeMO' | 'GB/T';

export interface Station {
  id: string;
  name: string;
  power: number;           // kW
  connectorType: ConnectorType;
  status: StationStatus;
  currentEV: string | null;
  sessionStart: string | null;   // ISO date string
  location: string;
}

// ─── Battery Types ───────────────────────────────────────────────────────────

export type BatteryStatus = 'available' | 'inuse' | 'charging' | 'maintenance' | 'unavailable';

export interface Battery {
  id: string;
  capacity: number;        // kWh
  currentCharge: number;   // percentage 0–100
  status: BatteryStatus;
  currentVehicle: string | null;
  lastUpdated: string;     // ISO date string
  cycleCount: number;
}

// ─── Session Types ───────────────────────────────────────────────────────────

export type SessionStatus = 'active' | 'completed';

export interface ChargingSession {
  id: string;
  evId: string;
  evModel: string;
  stationId: string;
  startTime: string;        // ISO date string
  endTime: string | null;
  batteryStart: number;     // percentage
  batteryEnd: number | null;
  targetCharge: number;     // percentage
  estimatedCost: number;    // INR
  status: SessionStatus;
}

// ─── Transaction Types ───────────────────────────────────────────────────────

export type TransactionType = 'charging' | 'battery-swap';
export type TransactionStatus = 'completed' | 'pending' | 'failed';

export interface Transaction {
  id: string;
  evId: string;
  type: TransactionType;
  stationOrBattery: string;
  date: string;             // ISO date string
  amount: number;           // INR
  energyDelivered: number;  // kWh
  duration: number;         // minutes
  status: TransactionStatus;
}

// ─── Queue Types ─────────────────────────────────────────────────────────────

export interface ChargingRequest {
  id: string;
  evId: string;
  evModel: string;
  owner: string;
  requestedAt: string;      // ISO date string
  currentCharge: number;    // percentage
  priority: 'normal' | 'urgent';
}

export interface SwapRequest {
  id: string;
  evId: string;
  evModel: string;
  owner: string;
  currentBattery: string;
  requiredCapacity: number; // kWh
  requestedAt: string;      // ISO date string
}

// ─── Notification Types ──────────────────────────────────────────────────────

export type NotificationType = 'info' | 'warning' | 'success' | 'error';

export interface Notification {
  id: string;
  message: string;
  type: NotificationType;
  timestamp: string;        // ISO date string
  read: boolean;
  link?: string;            // Route to navigate to
}

// ─── Settings Types ──────────────────────────────────────────────────────────

export interface AppSettings {
  themeMode: 'light' | 'dark';
  density: 'comfortable' | 'compact';
  notifications: {
    queueAlerts: boolean;
    sessionCompletion: boolean;
    stationAlerts: boolean;
  };
}
