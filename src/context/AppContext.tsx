import React, { createContext, useContext, useState, useEffect, useCallback, type ReactNode } from 'react';
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
import { api } from '@/services/api';

// ============================================================================
// Smart EV Hub - Central Application Context & State Management
// Module Owner: Abhishek Ghansala (UI + DSA + Final Integration, ID: 2518132)
// Manages operational state and synchronizes with C++ backend & manual DSA
// ============================================================================

export interface DSAMetadata {
  frontIndex: number;
  rearIndex: number;
  size: number;
  capacity: number;
}

export interface StackMetadata {
  topIndex: number;
  size: number;
  capacity: number;
}

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

  // Backend Integration & DSA Metadata (Abhishek Ghansala)
  isBackendOnline: boolean;
  chargingQueueMeta: DSAMetadata;
  swapQueueMeta: DSAMetadata;
  transactionStackMeta: StackMetadata;
  refreshBackendData: () => Promise<void>;

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
  processNextCharging: () => Promise<ChargingRequest | null>;
  removeChargingRequest: (id: string) => void;
  addChargingRequest: (req: Omit<ChargingRequest, 'id' | 'requestedAt'>) => void;
  processNextSwap: () => Promise<SwapRequest | null>;
  removeSwapRequest: (id: string) => void;
  addSwapRequest: (req: Omit<SwapRequest, 'id' | 'requestedAt'>) => void;

  // Session operations
  completeSession: (sessionId: string) => Promise<Transaction | null>;

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

  // Backend connection status and DSA states
  const [isBackendOnline, setIsBackendOnline] = useState<boolean>(false);
  const [chargingQueueMeta, setChargingQueueMeta] = useState<DSAMetadata>({
    frontIndex: 0,
    rearIndex: 1,
    size: 2,
    capacity: 20,
  });
  const [swapQueueMeta, setSwapQueueMeta] = useState<DSAMetadata>({
    frontIndex: 0,
    rearIndex: -1,
    size: 0,
    capacity: 20,
  });
  const [transactionStackMeta, setTransactionStackMeta] = useState<StackMetadata>({
    topIndex: 2,
    size: 3,
    capacity: 50,
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

  const logout = useCallback(async () => {
    try {
      if (isBackendOnline) {
        await api.logout();
      }
    } catch (e) {
      // Ignore network errors on logout
    }
    setCurrentUser(null);
    localStorage.removeItem('currentUser');
    showSnackbar('Logged out successfully', 'info');
  }, [isBackendOnline, showSnackbar]);

  // Synchronize with C++ Crow Backend on Startup
  const refreshBackendData = useCallback(async () => {
    try {
      await api.checkHealth();
      setIsBackendOnline(true);

      // Fetch live DSA Queue and Stack metadata from C++
      const [cQueue, sQueue, tStack] = await Promise.all([
        api.getChargingQueue().catch(() => null),
        api.getSwapQueue().catch(() => null),
        api.getTransactionStack().catch(() => null),
      ]);

      if (cQueue) {
        setChargingQueueMeta({
          frontIndex: cQueue.frontIndex,
          rearIndex: cQueue.rearIndex,
          size: cQueue.size,
          capacity: cQueue.capacity,
        });
      }

      if (sQueue) {
        setSwapQueueMeta({
          frontIndex: sQueue.frontIndex,
          rearIndex: sQueue.rearIndex,
          size: sQueue.size,
          capacity: sQueue.capacity,
        });
      }

      if (tStack) {
        setTransactionStackMeta({
          topIndex: tStack.topIndex,
          size: tStack.size,
          capacity: tStack.capacity,
        });
      }

      // Fetch live entities from SQLite
      const [backendEVs, backendStations, backendBatteries, backendTxns] = await Promise.all([
        api.getEVs().catch(() => null),
        api.getStations().catch(() => null),
        api.getBatteries().catch(() => null),
        api.getTransactions().catch(() => null),
      ]);

      if (backendEVs && Array.isArray(backendEVs) && backendEVs.length > 0) {
        setEVs(backendEVs);
      }
      if (backendStations && Array.isArray(backendStations) && backendStations.length > 0) {
        setStations(backendStations);
      }
      if (backendBatteries && Array.isArray(backendBatteries) && backendBatteries.length > 0) {
        setBatteries(backendBatteries);
      }
      if (backendTxns && Array.isArray(backendTxns) && backendTxns.length > 0) {
        setTransactions(backendTxns);
      }
    } catch (err) {
      // Backend not running, stay in standalone prototype mode
      setIsBackendOnline(false);
    }
  }, []);

  useEffect(() => {
    refreshBackendData();
  }, [refreshBackendData]);

  // ─── EV CRUD ─────────────────────────────────────────────
  const addEV = useCallback(async (ev: Omit<EV, 'lastActivity' | 'registeredAt'>) => {
    const now = new Date().toISOString();
    const newEV = { ...ev, lastActivity: now, registeredAt: now };
    setEVs(prev => [...prev, newEV]);
    showSnackbar(`EV ${ev.id} registered successfully`);

    // Persist to C++ backend if online
    if (isBackendOnline) {
      try {
        await api.createEV(newEV);
      } catch (e: any) {
        console.error('Failed to sync EV to backend:', e.message);
      }
    }
  }, [isBackendOnline, showSnackbar]);

  const updateEV = useCallback(async (id: string, data: Partial<EV>) => {
    setEVs(prev => prev.map(ev => ev.id === id ? { ...ev, ...data } : ev));
    if (isBackendOnline) {
      try {
        await api.updateEV(id, data);
      } catch (e) {
        // silently fallback
      }
    }
  }, [isBackendOnline]);

  const deleteEV = useCallback(async (id: string) => {
    setEVs(prev => prev.filter(ev => ev.id !== id));
    showSnackbar(`EV ${id} removed`);
    if (isBackendOnline) {
      try {
        await api.deleteEV(id);
      } catch (e) {
        // silently fallback
      }
    }
  }, [isBackendOnline, showSnackbar]);

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

  // ─── Charging Queue (FIFO - Abhishek Ghansala DSA) ───────
  const processNextCharging = useCallback(async () => {
    if (chargingQueue.length === 0) return null;
    const [first, ...rest] = chargingQueue;
    setChargingQueue(rest);

    // Call manual C++ ChargingQueue dequeue endpoint if backend online
    if (isBackendOnline) {
      try {
        const dsaRes = await api.dequeueCharging();
        if (dsaRes) {
          setChargingQueueMeta(prev => ({
            ...prev,
            frontIndex: dsaRes.frontIndex ?? prev.frontIndex,
            rearIndex: dsaRes.rearIndex ?? prev.rearIndex,
            size: dsaRes.remainingSize ?? Math.max(0, prev.size - 1),
          }));
        }
      } catch (e) {
        console.error('DSA Dequeue sync failed:', e);
      }
    }

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
      showSnackbar(`${first.evId} dequeued (no station currently available)`, 'warning');
    }
    return first;
  }, [chargingQueue, stations, isBackendOnline, showSnackbar]);

  const removeChargingRequest = useCallback((id: string) => {
    setChargingQueue(prev => prev.filter(r => r.id !== id));
    showSnackbar('Request removed from queue');
  }, [showSnackbar]);

  const addChargingRequest = useCallback(async (req: Omit<ChargingRequest, 'id' | 'requestedAt'>) => {
    const newReq: ChargingRequest = {
      ...req,
      id: nextId('CQ'),
      requestedAt: new Date().toISOString(),
    };
    setChargingQueue(prev => [...prev, newReq]);
    showSnackbar(`${req.evId} added to Charging Queue (REAR)`);

    // Call manual C++ ChargingQueue enqueue endpoint if backend online
    if (isBackendOnline) {
      try {
        const dsaRes = await api.enqueueCharging(req.evId);
        if (dsaRes && dsaRes.success) {
          setChargingQueueMeta(prev => ({
            ...prev,
            frontIndex: dsaRes.frontIndex ?? prev.frontIndex,
            rearIndex: dsaRes.rearIndex ?? prev.rearIndex,
            size: dsaRes.size ?? (prev.size + 1),
          }));
        }
      } catch (e) {
        console.error('DSA Enqueue sync failed:', e);
      }
    }
  }, [isBackendOnline, showSnackbar]);

  // ─── Swap Queue (FIFO - Abhishek Ghansala DSA) ───────────
  const processNextSwap = useCallback(async () => {
    if (swapQueue.length === 0) return null;
    const [first, ...rest] = swapQueue;
    setSwapQueue(rest);

    // Call manual C++ SwapQueue dequeue endpoint
    if (isBackendOnline) {
      try {
        const dsaRes = await api.dequeueSwap();
        if (dsaRes) {
          setSwapQueueMeta(prev => ({
            ...prev,
            frontIndex: dsaRes.frontIndex ?? prev.frontIndex,
            rearIndex: dsaRes.rearIndex ?? prev.rearIndex,
            size: dsaRes.remainingSize ?? Math.max(0, prev.size - 1),
          }));
        }
      } catch (e) {
        console.error('Swap Queue sync failed:', e);
      }
    }

    const txnId = nextId('TXN');
    const txn: Transaction = {
      id: txnId,
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

    // Push completed transaction to C++ LIFO Stack
    if (isBackendOnline) {
      try {
        await api.pushTransactionStack(txnId);
        setTransactionStackMeta(prev => ({
          ...prev,
          topIndex: prev.topIndex + 1,
          size: prev.size + 1,
        }));
      } catch (e) {
        console.error('Stack push sync failed:', e);
      }
    }

    showSnackbar(`Battery swap completed for ${first.evId}`);
    return first;
  }, [swapQueue, isBackendOnline, showSnackbar]);

  const removeSwapRequest = useCallback((id: string) => {
    setSwapQueue(prev => prev.filter(r => r.id !== id));
    showSnackbar('Swap request removed');
  }, [showSnackbar]);

  const addSwapRequest = useCallback(async (req: Omit<SwapRequest, 'id' | 'requestedAt'>) => {
    const newReq: SwapRequest = {
      ...req,
      id: nextId('SQ'),
      requestedAt: new Date().toISOString(),
    };
    setSwapQueue(prev => [...prev, newReq]);
    showSnackbar(`${req.evId} added to Swap Queue (REAR)`);

    // Call manual C++ SwapQueue enqueue
    if (isBackendOnline) {
      try {
        const dsaRes = await api.enqueueSwap(req.evId);
        if (dsaRes && dsaRes.success) {
          setSwapQueueMeta(prev => ({
            ...prev,
            frontIndex: dsaRes.frontIndex ?? prev.frontIndex,
            rearIndex: dsaRes.rearIndex ?? prev.rearIndex,
            size: dsaRes.size ?? (prev.size + 1),
          }));
        }
      } catch (e) {
        console.error('Swap queue enqueue sync failed:', e);
      }
    }
  }, [isBackendOnline, showSnackbar]);

  // ─── Sessions & Transactions (LIFO Stack - Abhishek DSA) ──
  const completeSession = useCallback(async (sessionId: string) => {
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
    const txnId = nextId('TXN');
    const txn: Transaction = {
      id: txnId,
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

    // Push new transaction onto C++ LIFO Stack & SQLite database
    if (isBackendOnline) {
      try {
        await api.completeCharging(session.stationId);
        await api.createTransaction(txn);
        await api.pushTransactionStack(txnId);
        setTransactionStackMeta(prev => ({
          ...prev,
          topIndex: prev.topIndex + 1,
          size: prev.size + 1,
        }));
      } catch (e) {
        console.error('Failed to sync completed session to backend:', e);
      }
    }

    showSnackbar(`Charging session for ${session.evId} completed — ₹${session.estimatedCost.toFixed(2)} (Pushed to Stack)`);
    return txn;
  }, [sessions, isBackendOnline, showSnackbar]);

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
      isBackendOnline, chargingQueueMeta, swapQueueMeta, transactionStackMeta,
      refreshBackendData,
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
