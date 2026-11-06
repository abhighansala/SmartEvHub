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
// Features: Optimistic updates with automatic rollback, JWT token storage, live simulation
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

  // EV CRUD (Optimistic with Rollback)
  addEV: (ev: Omit<EV, 'lastActivity' | 'registeredAt'>) => Promise<void>;
  updateEV: (id: string, data: Partial<EV>) => Promise<void>;
  deleteEV: (id: string) => Promise<void>;

  // Station operations (Optimistic with Rollback)
  updateStation: (id: string, data: Partial<Station>) => Promise<void>;

  // Battery operations (Optimistic with Rollback)
  addBattery: (battery: Omit<Battery, 'lastUpdated'>) => Promise<void>;
  updateBattery: (id: string, data: Partial<Battery>) => Promise<void>;

  // Queue operations
  processNextCharging: () => Promise<ChargingRequest | null>;
  removeChargingRequest: (id: string) => void;
  addChargingRequest: (req: Omit<ChargingRequest, 'id' | 'requestedAt'>) => Promise<void>;
  processNextSwap: () => Promise<SwapRequest | null>;
  removeSwapRequest: (id: string) => void;
  addSwapRequest: (req: Omit<SwapRequest, 'id' | 'requestedAt'>) => Promise<void>;

  // Session operations
  completeSession: (sessionId: string) => Promise<any>;

  // Notification operations
  markNotificationRead: (id: string) => void;
  markAllNotificationsRead: () => void;
  addNotification: (notification: Omit<Notification, 'id' | 'timestamp' | 'read'>) => void;

  // Settings
  updateSettings: (settings: Partial<AppSettings>) => void;
  toggleTheme: () => void;

  // Auth & Token Management
  isAuthenticated: boolean;
  currentUser: User | null;
  login: (user: User, token?: string) => void;
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

  const login = useCallback((user: User, token?: string) => {
    setCurrentUser(user);
    localStorage.setItem('currentUser', JSON.stringify(user));
    if (token) {
      localStorage.setItem('authToken', token);
    }
    showSnackbar(`Welcome back, ${user.name}!`, 'success');
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
    localStorage.removeItem('authToken');
    showSnackbar('Logged out successfully', 'info');
  }, [isBackendOnline, showSnackbar]);

  // Handle Token Expiration events from API Client
  useEffect(() => {
    const handleExpired = () => {
      setCurrentUser(null);
      localStorage.removeItem('currentUser');
      localStorage.removeItem('authToken');
      showSnackbar('Your session has expired. Please sign in again.', 'warning');
    };
    window.addEventListener('evhub-auth-expired', handleExpired);
    return () => window.removeEventListener('evhub-auth-expired', handleExpired);
  }, [showSnackbar]);

  // Validate stored JWT on initial boot
  useEffect(() => {
    const token = localStorage.getItem('authToken');
    if (token) {
      api.getMe()
        .then(res => {
          if (res && res.success && res.user) {
            setCurrentUser(res.user);
            localStorage.setItem('currentUser', JSON.stringify(res.user));
          }
        })
        .catch(() => {
          // Token expired or invalid, clear stored creds
          localStorage.removeItem('authToken');
          localStorage.removeItem('currentUser');
          setCurrentUser(null);
        });
    }
  }, []);

  // Synchronize with C++ Crow Backend
  const refreshBackendData = useCallback(async () => {
    try {
      await api.checkHealth();
      setIsBackendOnline(true);

      // Fetch live DSA Queues, Stack, entities, sessions & transactions in parallel
      const [cQueue, sQueue, tStack, backendEVs, backendStations, backendBatteries, backendTxns, backendSessions] = await Promise.all([
        api.getChargingQueue().catch(() => null),
        api.getSwapQueue().catch(() => null),
        api.getTransactionStack().catch(() => null),
        api.getEVs().catch(() => null),
        api.getStations().catch(() => null),
        api.getBatteries().catch(() => null),
        api.getTransactions().catch(() => null),
        api.getSessions().catch(() => null),
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

      // Update SQLite entity states
      let effectiveEVs = evs;
      if (backendEVs && Array.isArray(backendEVs)) {
        setEVs(backendEVs);
        effectiveEVs = backendEVs;
      }
      if (backendStations && Array.isArray(backendStations)) {
        setStations(backendStations);
      }
      if (backendBatteries && Array.isArray(backendBatteries)) {
        setBatteries(backendBatteries);
      }
      if (backendTxns && Array.isArray(backendTxns)) {
        setTransactions(backendTxns);
      }
      if (backendSessions && Array.isArray(backendSessions)) {
        setSessions(backendSessions);
      }

      // Hydrate visual queues from C++ array buffers
      if (cQueue && Array.isArray(cQueue.items)) {
        const queueRequests: ChargingRequest[] = cQueue.items.map((evId: string, index: number) => {
          const ev = effectiveEVs.find((e: any) => e.id === evId);
          return {
            id: `CQ-${evId}-${index}`,
            evId: evId,
            evModel: ev?.model || 'Electric Vehicle',
            owner: ev?.owner || 'Fleet Operator',
            currentCharge: ev?.currentCharge ?? 20,
            requestedAt: ev?.lastActivity || new Date().toISOString(),
            priority: 'normal',
          };
        });
        setChargingQueue(queueRequests);
      }

      if (sQueue && Array.isArray(sQueue.items)) {
        const swapRequests: SwapRequest[] = sQueue.items.map((evId: string, index: number) => {
          const ev = effectiveEVs.find((e: any) => e.id === evId);
          return {
            id: `SQ-${evId}-${index}`,
            evId: evId,
            evModel: ev?.model || 'Electric Vehicle',
            owner: ev?.owner || 'Fleet Operator',
            currentBattery: 'Current',
            requiredCapacity: ev?.batteryCapacity || 50.0,
            requestedAt: ev?.lastActivity || new Date().toISOString(),
          };
        });
        setSwapQueue(swapRequests);
      }
    } catch (err) {
      setIsBackendOnline(false);
    }
  }, [evs]);

  useEffect(() => {
    refreshBackendData();
  }, [refreshBackendData]);

  // ─── Live Charging Simulation Engine ─────────────────────
  useEffect(() => {
    const hasActiveSessions = sessions.some(s => s.status === 'active');
    if (!hasActiveSessions) return;

    const interval = setInterval(() => {
      setSessions(prevSessions => {
        let changed = false;
        const updated = prevSessions.map(session => {
          if (session.status !== 'active') return session;

          const currentSoC = session.batteryEnd ?? session.batteryStart;
          if (currentSoC < session.targetCharge) {
            changed = true;
            const station = stations.find(s => s.id === session.stationId);
            const power = station?.power || 25;
            const increment = Math.max(1, Math.round((power / 25) * 1.5));
            const nextSoC = Math.min(session.targetCharge, currentSoC + increment);

            // Synchronize with EV state
            setEVs(prevEVs => prevEVs.map(ev =>
              ev.id === session.evId ? { ...ev, currentCharge: nextSoC } : ev
            ));

            return {
              ...session,
              batteryEnd: nextSoC,
              energyConsumed: Math.round(((nextSoC - session.batteryStart) * 0.45) * 10) / 10,
            };
          }
          return session;
        });

        return changed ? updated : prevSessions;
      });
    }, 3000);

    return () => clearInterval(interval);
  }, [sessions, stations]);

  // ─── EV CRUD with Optimistic UI & Automatic Rollback ─────
  const addEV = useCallback(async (ev: Omit<EV, 'lastActivity' | 'registeredAt'>) => {
    const now = new Date().toISOString();
    const newEV = { ...ev, lastActivity: now, registeredAt: now };
    const prevEVs = evs;

    // Optimistic UI update
    setEVs(prev => [...prev, newEV]);

    if (isBackendOnline) {
      try {
        await api.createEV(newEV);
        showSnackbar(`EV ${ev.id} registered in SQLite database`, 'success');
      } catch (e: any) {
        // Rollback state on error
        setEVs(prevEVs);
        showSnackbar(`Failed to register EV on server: ${e.message}. Rolled back changes.`, 'error');
      }
    } else {
      showSnackbar(`EV ${ev.id} registered locally (offline mode)`, 'info');
    }
  }, [evs, isBackendOnline, showSnackbar]);

  const updateEV = useCallback(async (id: string, data: Partial<EV>) => {
    const prevEVs = evs;
    setEVs(prev => prev.map(ev => ev.id === id ? { ...ev, ...data } : ev));

    if (isBackendOnline) {
      try {
        await api.updateEV(id, data);
        showSnackbar(`EV ${id} updated successfully`, 'success');
      } catch (e: any) {
        // Rollback state on error
        setEVs(prevEVs);
        showSnackbar(`Failed to update EV: ${e.message}. Rolled back.`, 'error');
      }
    }
  }, [evs, isBackendOnline, showSnackbar]);

  const deleteEV = useCallback(async (id: string) => {
    const prevEVs = evs;
    setEVs(prev => prev.filter(ev => ev.id !== id));

    if (isBackendOnline) {
      try {
        await api.deleteEV(id);
        showSnackbar(`EV ${id} removed from SQLite registry`, 'info');
      } catch (e: any) {
        // Rollback state on error
        setEVs(prevEVs);
        showSnackbar(`Failed to delete EV: ${e.message}. Rolled back.`, 'error');
      }
    } else {
      showSnackbar(`EV ${id} removed`, 'info');
    }
  }, [evs, isBackendOnline, showSnackbar]);

  // ─── Station with Optimistic UI & Automatic Rollback ──────
  const updateStation = useCallback(async (id: string, data: Partial<Station>) => {
    const prevStations = stations;
    setStations(prev => prev.map(s => s.id === id ? { ...s, ...data } : s));

    if (isBackendOnline && data.status) {
      try {
        await api.updateStationStatus(id, data.status);
        showSnackbar(`Station ${id} status set to ${data.status}`, 'success');
      } catch (e: any) {
        // Rollback on error
        setStations(prevStations);
        showSnackbar(`Failed to sync station status: ${e.message}. Rolled back.`, 'error');
      }
    }
  }, [stations, isBackendOnline, showSnackbar]);

  // ─── Battery with Optimistic UI & Automatic Rollback ──────
  const addBattery = useCallback(async (battery: Omit<Battery, 'lastUpdated'>) => {
    const newBattery = { ...battery, lastUpdated: new Date().toISOString() };
    const prevBatteries = batteries;
    setBatteries(prev => [...prev, newBattery]);

    if (isBackendOnline) {
      try {
        await api.createBattery(newBattery);
        showSnackbar(`Battery ${battery.id} added to inventory`, 'success');
      } catch (e: any) {
        setBatteries(prevBatteries);
        showSnackbar(`Failed to add battery to server: ${e.message}. Rolled back.`, 'error');
      }
    } else {
      showSnackbar(`Battery ${battery.id} added locally (offline mode)`);
    }
  }, [batteries, isBackendOnline, showSnackbar]);

  const updateBattery = useCallback(async (id: string, data: Partial<Battery>) => {
    const prevBatteries = batteries;
    setBatteries(prev => prev.map(b => b.id === id ? { ...b, ...data } : b));

    if (isBackendOnline) {
      try {
        await api.updateBattery(id, data);
        showSnackbar(`Battery ${id} updated successfully`, 'success');
      } catch (e: any) {
        setBatteries(prevBatteries);
        showSnackbar(`Failed to update battery: ${e.message}. Rolled back.`, 'error');
      }
    }
  }, [batteries, isBackendOnline, showSnackbar]);

  // ─── Charging Queue (FIFO - Abhishek Ghansala DSA) ───────
  const processNextCharging = useCallback(async () => {
    if (isBackendOnline) {
      try {
        const dsaRes = await api.dequeueCharging();
        const availableStation = stations.find(s => s.status === 'available');
        if (availableStation && dsaRes && dsaRes.evId) {
          await api.requestCharging(dsaRes.evId);
        }
        await refreshBackendData();
        showSnackbar(`Dispatched vehicle ${dsaRes?.evId || 'from queue'} to bay`, 'success');
        return dsaRes;
      } catch (e: any) {
        showSnackbar(e.message || 'Failed to process charging queue', 'error');
        return null;
      }
    }

    if (chargingQueue.length === 0) return null;
    const [first, ...rest] = chargingQueue;
    setChargingQueue(rest);

    // Fallback mode
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
        batteryEnd: first.currentCharge,
        targetCharge: 85,
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
  }, [chargingQueue, stations, isBackendOnline, refreshBackendData, showSnackbar]);

  const removeChargingRequest = useCallback((id: string) => {
    setChargingQueue(prev => prev.filter(r => r.id !== id));
    showSnackbar('Request removed from queue');
  }, [showSnackbar]);

  const addChargingRequest = useCallback(async (req: Omit<ChargingRequest, 'id' | 'requestedAt'>) => {
    if (isBackendOnline) {
      try {
        const res = await api.requestCharging(req.evId, 85.0);
        if (res && res.success) {
          if (res.assigned && res.stationId) {
            showSnackbar(`Vehicle ${req.evId} assigned to Station ${res.stationId}`, 'success');
          } else if (res.queued) {
            showSnackbar(`All stations busy. Added vehicle ${req.evId} to FIFO Charging Queue (REAR)`, 'info');
          } else {
            showSnackbar(res.message || `Charging request processed for ${req.evId}`, 'success');
          }
          await refreshBackendData();
          return;
        } else {
          showSnackbar(res?.message || 'Failed to submit charging request', 'error');
          return;
        }
      } catch (e: any) {
        showSnackbar(e.message || 'Failed to request charging from backend', 'error');
        return;
      }
    }

    // Fallback mode (mock)
    const newReq: ChargingRequest = {
      ...req,
      id: nextId('CQ'),
      requestedAt: new Date().toISOString(),
    };
    const freeStation = stations.find(s => s.status === 'available');
    if (freeStation) {
      const sessionId = nextId('SES');
      const newSession: ChargingSession = {
        id: sessionId,
        evId: req.evId,
        evModel: req.evModel,
        stationId: freeStation.id,
        startTime: new Date().toISOString(),
        endTime: null,
        batteryStart: req.currentCharge,
        batteryEnd: req.currentCharge,
        targetCharge: 85,
        estimatedCost: Math.round(req.currentCharge * 8.5),
        status: 'active',
      };
      setSessions(prev => [...prev, newSession]);
      setStations(prev => prev.map(s =>
        s.id === freeStation.id
          ? { ...s, status: 'charging' as const, currentEV: req.evId, sessionStart: new Date().toISOString() }
          : s
      ));
      setEVs(prev => prev.map(ev =>
        ev.id === req.evId ? { ...ev, status: 'charging' as const } : ev
      ));
      showSnackbar(`Assigned ${req.evId} to ${freeStation.name}`, 'success');
    } else {
      setChargingQueue(prev => [...prev, newReq]);
      setEVs(prev => prev.map(ev =>
        ev.id === req.evId ? { ...ev, status: 'waiting' as const } : ev
      ));
      showSnackbar(`${req.evId} added to Charging Queue (REAR)`, 'info');
    }
  }, [isBackendOnline, stations, refreshBackendData, showSnackbar]);

  // ─── Swap Queue (FIFO - Abhishek Ghansala DSA) ───────────
  const processNextSwap = useCallback(async () => {
    if (isBackendOnline) {
      try {
        const res = await api.processSwap();
        if (res && res.success) {
          showSnackbar(res.message || `Battery swap completed for ${res.evId}`, 'success');
          await refreshBackendData();
          return res;
        } else {
          showSnackbar(res?.message || 'Inventory exhausted. Cannot process swap.', 'error');
          return null;
        }
      } catch (err: any) {
        showSnackbar(err.message || 'Swap processing failed', 'error');
        return null;
      }
    }

    if (swapQueue.length === 0) return null;
    const [first, ...rest] = swapQueue;
    setSwapQueue(rest);

    const availableBat = batteries.find(b => b.status === 'available');
    const batId = availableBat ? availableBat.id : 'BAT-01';

    const txnId = nextId('TXN');
    const swapAmount = 300 + (first.requiredCapacity * 5);
    const txn: Transaction = {
      id: txnId,
      evId: first.evId,
      type: 'battery-swap',
      stationOrBattery: batId,
      date: new Date().toISOString(),
      amount: swapAmount,
      energyDelivered: first.requiredCapacity * 0.8,
      duration: 5,
      status: 'completed',
    };
    setTransactions(prev => [txn, ...prev]);
    setEVs(prev => prev.map(ev => ev.id === first.evId ? { ...ev, currentCharge: 100, status: 'available' } : ev));
    if (availableBat) {
      setBatteries(prev => prev.map(b => b.id === availableBat.id ? { ...b, status: 'inuse', currentVehicle: first.evId } : b));
    }

    showSnackbar(`Battery swap completed for ${first.evId}`);
    return first;
  }, [isBackendOnline, swapQueue, batteries, refreshBackendData, showSnackbar]);

  const removeSwapRequest = useCallback((id: string) => {
    setSwapQueue(prev => prev.filter(r => r.id !== id));
    showSnackbar('Swap request removed');
  }, [showSnackbar]);

  const addSwapRequest = useCallback(async (req: Omit<SwapRequest, 'id' | 'requestedAt'>) => {
    if (isBackendOnline) {
      try {
        const res = await api.requestSwap(req.evId, req.currentBattery, req.requiredCapacity);
        if (res && res.success) {
          if (res.swapped) {
            showSnackbar(res.message || `Swap complete! Assigned battery ${res.batteryId} to ${req.evId}`, 'success');
          } else if (res.queued) {
            showSnackbar(res.message || `No ready batteries. Added ${req.evId} to FIFO Swap Queue (REAR)`, 'info');
          }
          await refreshBackendData();
          return;
        } else {
          showSnackbar(res?.message || 'Failed to submit swap request', 'error');
          return;
        }
      } catch (e: any) {
        showSnackbar(e.message || 'Swap request failed', 'error');
        return;
      }
    }

    // Fallback mode if backend offline
    const newReq: SwapRequest = {
      ...req,
      id: nextId('SQ'),
      requestedAt: new Date().toISOString(),
    };
    setSwapQueue(prev => [...prev, newReq]);
    showSnackbar(`${req.evId} added to Swap Queue (REAR)`);
  }, [isBackendOnline, refreshBackendData, showSnackbar]);

  // ─── Sessions & Transactions (LIFO Stack - Abhishek DSA) ──
  const completeSession = useCallback(async (sessionId: string) => {
    const session = sessions.find(s => s.id === sessionId);
    if (!session) return null;

    if (isBackendOnline) {
      try {
        const res = await api.completeCharging(session.stationId);
        if (res && res.success) {
          showSnackbar(res.message || `Charging session for ${session.evId} completed — ₹${res.amount?.toFixed(2)}`, 'success');
          await refreshBackendData();
          return res;
        } else {
          showSnackbar(res?.message || 'Failed to complete charging session on backend', 'error');
          return null;
        }
      } catch (e: any) {
        showSnackbar(e.message || 'Failed to complete charging session on backend', 'error');
        return null;
      }
    }

    // Fallback mode if backend offline
    setSessions(prev => prev.map(s => s.id === sessionId ? { ...s, status: 'completed' as const, endTime: new Date().toISOString() } : s));
    setStations(prev => prev.map(s =>
      s.id === session.stationId
        ? { ...s, status: 'available' as const, currentEV: null, sessionStart: null }
        : s
    ));
    setEVs(prev => prev.map(ev =>
      ev.id === session.evId
        ? { ...ev, status: 'available' as const, currentCharge: session.targetCharge, lastActivity: new Date().toISOString() }
        : ev
    ));

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
    showSnackbar(`Charging session for ${session.evId} completed — ₹${session.estimatedCost.toFixed(2)}`);
    return txn;
  }, [sessions, isBackendOnline, refreshBackendData, showSnackbar]);

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
