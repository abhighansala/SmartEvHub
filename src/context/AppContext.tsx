import React, { createContext, useContext, useState, useCallback, type ReactNode } from 'react';
import type {
  User, EV, Station, Battery, ChargingSession, Transaction,
  ChargingRequest, SwapRequest, Notification, AppSettings,
} from '@/types';
import { mockEVs } from '@/data/mockEVs';
import { mockStations } from '@/data/mockStations';
import { mockBatteries } from '@/data/mockBatteries';
import { mockSessions } from '@/data/mockSessions';
import { mockTransactions } from '@/data/mockTransactions';
import { mockChargingQueue, mockSwapQueue } from '@/data/mockQueues';
import { mockNotifications } from '@/data/mockNotifications';

interface AppContextType {
  // State
  evs: EV[];
  stations: Station[];
  batteries: Battery[];
  sessions: ChargingSession[];
  transactions: Transaction[];
  chargingQueue: ChargingRequest[];
  swapQueue: SwapRequest[];
  notifications: Notification[];
  settings: AppSettings;

  // EV CRUD
  addEV: (ev: Omit<EV, 'lastActivity' | 'registeredAt'>) => void;
  updateEV: (id: string, data: Partial<EV>) => void;
  deleteEV: (id: string) => void;

  // Station operations
  updateStation: (id: string, data: Partial<Station>) => void;

  // Battery operations
  addBattery: (battery: Omit<Battery, 'lastUpdated'>) => void;
  updateBattery: (id: string, data: Partial<Battery>) => void;

  // Queue operations
  processNextCharging: () => ChargingRequest | null;
  removeChargingRequest: (id: string) => void;
  addChargingRequest: (req: Omit<ChargingRequest, 'id' | 'requestedAt'>) => void;
  processNextSwap: () => SwapRequest | null;
  removeSwapRequest: (id: string) => void;
  addSwapRequest: (req: Omit<SwapRequest, 'id' | 'requestedAt'>) => void;

  // Session operations
  completeSession: (sessionId: string) => Transaction | null;

  // Notification operations
  markNotificationRead: (id: string) => void;
  markAllNotificationsRead: () => void;
  addNotification: (notification: Omit<Notification, 'id' | 'timestamp' | 'read'>) => void;

  // Settings
  updateSettings: (settings: Partial<AppSettings>) => void;
  toggleTheme: () => void;
  // Auth
  isAuthenticated: boolean;
  currentUser: User | null;
  login: (user: User) => void;
  logout: () => void;

  // Snackbar
  snackbar: { open: boolean; message: string; severity: 'success' | 'error' | 'info' | 'warning' };
  showSnackbar: (message: string, severity?: 'success' | 'error' | 'info' | 'warning') => void;
  closeSnackbar: () => void;
}

const AppContext = createContext<AppContextType | undefined>(undefined);

let idCounter = 1000;
const nextId = (prefix: string) => `${prefix}${++idCounter}`;

export function AppProvider({ children }: { children: ReactNode }) {
  const [evs, setEVs] = useState<EV[]>(mockEVs);
  const [stations, setStations] = useState<Station[]>(mockStations);
  const [batteries, setBatteries] = useState<Battery[]>(mockBatteries);
  const [sessions, setSessions] = useState<ChargingSession[]>(mockSessions);
  const [transactions, setTransactions] = useState<Transaction[]>(mockTransactions);
  const [chargingQueue, setChargingQueue] = useState<ChargingRequest[]>(mockChargingQueue);
  const [swapQueue, setSwapQueue] = useState<SwapRequest[]>(mockSwapQueue);
  const [notifications, setNotifications] = useState<Notification[]>(mockNotifications);
  const [settings, setSettings] = useState<AppSettings>({
    themeMode: (localStorage.getItem('themeMode') as 'light' | 'dark') || 'light',
    density: 'comfortable',
    notifications: { queueAlerts: true, sessionCompletion: true, stationAlerts: true },
  });

  const [snackbar, setSnackbar] = useState<AppContextType['snackbar']>({
    open: false, message: '', severity: 'success',
  });

  const showSnackbar = useCallback((message: string, severity: 'success' | 'error' | 'info' | 'warning' = 'success') => {
    setSnackbar({ open: true, message, severity });
  }, []);

  const closeSnackbar = useCallback(() => {
    setSnackbar(prev => ({ ...prev, open: false }));
  }, []);

  const [currentUser, setCurrentUser] = useState<User | null>(() => {
    const stored = localStorage.getItem('currentUser');
    return stored ? JSON.parse(stored) : null;
  });
  const isAuthenticated = !!currentUser;

  const login = useCallback((user: User) => {
    setCurrentUser(user);
    localStorage.setItem('currentUser', JSON.stringify(user));
    showSnackbar(`Welcome, ${user.name}`, 'success');
  }, [showSnackbar]);

  const logout = useCallback(() => {
    setCurrentUser(null);
    localStorage.removeItem('currentUser');
    showSnackbar('Logged out successfully', 'info');
  }, [showSnackbar]);

  // ─── EV CRUD ─────────────────────────────────────────────
  const addEV = useCallback((ev: Omit<EV, 'lastActivity' | 'registeredAt'>) => {
    const now = new Date().toISOString();
    setEVs(prev => [...prev, { ...ev, lastActivity: now, registeredAt: now }]);
    showSnackbar(`EV ${ev.id} registered successfully`);
  }, [showSnackbar]);

  const updateEV = useCallback((id: string, data: Partial<EV>) => {
    setEVs(prev => prev.map(ev => ev.id === id ? { ...ev, ...data } : ev));
  }, []);

  const deleteEV = useCallback((id: string) => {
    setEVs(prev => prev.filter(ev => ev.id !== id));
    showSnackbar(`EV ${id} removed`);
  }, [showSnackbar]);

  // ─── Station ─────────────────────────────────────────────
  const updateStation = useCallback((id: string, data: Partial<Station>) => {
    setStations(prev => prev.map(s => s.id === id ? { ...s, ...data } : s));
  }, []);

  // ─── Battery ─────────────────────────────────────────────
  const addBattery = useCallback((battery: Omit<Battery, 'lastUpdated'>) => {
    setBatteries(prev => [...prev, { ...battery, lastUpdated: new Date().toISOString() }]);
    showSnackbar(`Battery ${battery.id} added successfully`);
  }, [showSnackbar]);

  const updateBattery = useCallback((id: string, data: Partial<Battery>) => {
    setBatteries(prev => prev.map(b => b.id === id ? { ...b, ...data } : b));
  }, []);

  // ─── Charging Queue ──────────────────────────────────────
  const processNextCharging = useCallback(() => {
    if (chargingQueue.length === 0) return null;
    const [first, ...rest] = chargingQueue;
    setChargingQueue(rest);

    // Create a new session for the processed request
    const availableStation = stations.find(s => s.status === 'available');
    if (availableStation) {
      const sessionId = nextId('SES');
      const newSession: ChargingSession = {
        id: sessionId,
        evId: first.evId,
        evModel: first.evModel,
        stationId: availableStation.id,
        startTime: new Date().toISOString(),
        endTime: null,
        batteryStart: first.currentCharge,
        batteryEnd: null,
        targetCharge: 80,
        estimatedCost: Math.round(Math.random() * 150 + 50),
        status: 'active',
      };
      setSessions(prev => [...prev, newSession]);
      setStations(prev => prev.map(s =>
        s.id === availableStation.id
          ? { ...s, status: 'charging' as const, currentEV: first.evId, sessionStart: new Date().toISOString() }
          : s
      ));
      setEVs(prev => prev.map(ev =>
        ev.id === first.evId ? { ...ev, status: 'charging' as const } : ev
      ));
      showSnackbar(`${first.evId} moved to ${availableStation.name} for charging`);
    } else {
      showSnackbar(`${first.evId} removed from queue (no station available)`, 'warning');
    }
    return first;
  }, [chargingQueue, stations, showSnackbar]);

  const removeChargingRequest = useCallback((id: string) => {
    setChargingQueue(prev => prev.filter(r => r.id !== id));
    showSnackbar('Request removed from queue');
  }, [showSnackbar]);

  const addChargingRequest = useCallback((req: Omit<ChargingRequest, 'id' | 'requestedAt'>) => {
    setChargingQueue(prev => [...prev, {
      ...req,
      id: nextId('CQ'),
      requestedAt: new Date().toISOString(),
    }]);
    showSnackbar(`${req.evId} added to charging queue`);
  }, [showSnackbar]);

  // ─── Swap Queue ──────────────────────────────────────────
  const processNextSwap = useCallback(() => {
    if (swapQueue.length === 0) return null;
    const [first, ...rest] = swapQueue;
    setSwapQueue(rest);
    const txn: Transaction = {
      id: nextId('TXN'),
      evId: first.evId,
      type: 'battery-swap',
      stationOrBattery: 'BAT-SWAP',
      date: new Date().toISOString(),
      amount: Math.round(Math.random() * 60 + 40),
      energyDelivered: first.requiredCapacity * 0.8,
      duration: Math.round(Math.random() * 8 + 5),
      status: 'completed',
    };
    setTransactions(prev => [txn, ...prev]);
    showSnackbar(`Battery swap completed for ${first.evId}`);
    return first;
  }, [swapQueue, showSnackbar]);

  const removeSwapRequest = useCallback((id: string) => {
    setSwapQueue(prev => prev.filter(r => r.id !== id));
    showSnackbar('Swap request removed');
  }, [showSnackbar]);

  const addSwapRequest = useCallback((req: Omit<SwapRequest, 'id' | 'requestedAt'>) => {
    setSwapQueue(prev => [...prev, {
      ...req,
      id: nextId('SQ'),
      requestedAt: new Date().toISOString(),
    }]);
    showSnackbar(`${req.evId} added to swap queue`);
  }, [showSnackbar]);

  // ─── Sessions ────────────────────────────────────────────
  const completeSession = useCallback((sessionId: string) => {
    const session = sessions.find(s => s.id === sessionId);
    if (!session) return null;

    // Remove from active sessions
    setSessions(prev => prev.filter(s => s.id !== sessionId));

    // Free the station
    setStations(prev => prev.map(s =>
      s.id === session.stationId
        ? { ...s, status: 'available' as const, currentEV: null, sessionStart: null }
        : s
    ));

    // Update EV status
    setEVs(prev => prev.map(ev =>
      ev.id === session.evId
        ? { ...ev, status: 'available' as const, currentCharge: session.targetCharge, lastActivity: new Date().toISOString() }
        : ev
    ));

    // Create transaction
    const txn: Transaction = {
      id: nextId('TXN'),
      evId: session.evId,
      type: 'charging',
      stationOrBattery: session.stationId,
      date: new Date().toISOString(),
      amount: session.estimatedCost,
      energyDelivered: Math.round((session.targetCharge - session.batteryStart) * 0.4 * 10) / 10,
      duration: Math.round((Date.now() - new Date(session.startTime).getTime()) / 60000),
      status: 'completed',
    };
    setTransactions(prev => [txn, ...prev]);
    showSnackbar(`Charging session for ${session.evId} completed — ₹${session.estimatedCost.toFixed(2)}`);
    return txn;
  }, [sessions, showSnackbar]);

  // ─── Notifications ───────────────────────────────────────
  const markNotificationRead = useCallback((id: string) => {
    setNotifications(prev => prev.map(n => n.id === id ? { ...n, read: true } : n));
  }, []);

  const markAllNotificationsRead = useCallback(() => {
    setNotifications(prev => prev.map(n => ({ ...n, read: true })));
  }, []);

  const addNotification = useCallback((notification: Omit<Notification, 'id' | 'timestamp' | 'read'>) => {
    setNotifications(prev => [{
      ...notification,
      id: nextId('N'),
      timestamp: new Date().toISOString(),
      read: false,
    }, ...prev]);
  }, []);

  // ─── Settings ────────────────────────────────────────────
  const updateSettings = useCallback((newSettings: Partial<AppSettings>) => {
    setSettings(prev => {
      const updated = { ...prev, ...newSettings };
      if (newSettings.themeMode) {
        localStorage.setItem('themeMode', newSettings.themeMode);
      }
      return updated;
    });
  }, []);

  const toggleTheme = useCallback(() => {
    setSettings(prev => {
      const newMode = prev.themeMode === 'light' ? 'dark' : 'light';
      localStorage.setItem('themeMode', newMode);
      return { ...prev, themeMode: newMode };
    });
  }, []);

  return (
    <AppContext.Provider value={{
      evs, stations, batteries, sessions, transactions,
      chargingQueue, swapQueue, notifications, settings,
      isAuthenticated, currentUser, login, logout,
      addEV, updateEV, deleteEV,
      updateStation,
      addBattery, updateBattery,
      processNextCharging, removeChargingRequest, addChargingRequest,
      processNextSwap, removeSwapRequest, addSwapRequest,
      completeSession,
      markNotificationRead, markAllNotificationsRead, addNotification,
      updateSettings, toggleTheme,
      snackbar, showSnackbar, closeSnackbar,
    }}>
      {children}
    </AppContext.Provider>
  );
}

export function useApp() {
  const context = useContext(AppContext);
  if (!context) throw new Error('useApp must be used within AppProvider');
  return context;
}
