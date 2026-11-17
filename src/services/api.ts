// ============================================================================
// Smart EV Hub - Frontend API Client
// Role: UI + DSA + Final Integration
// Owner: Abhishek Ghansala (Student ID: 2518132, Section H)
// Connects the React UI to the C++ Crow Backend on http://localhost:8080
// Features: Bearer JWT Authorization, robust error handling, full CRUD
// ============================================================================

const API_BASE = (import.meta as any).env?.VITE_API_URL || 'http://localhost:8080/api';

let lastAuthExpiredDispatch = 0;

function notifyAuthExpired() {
  const now = Date.now();
  // Debounce expiration notifications: maximum once every 3 seconds
  if (now - lastAuthExpiredDispatch < 3000) {
    return;
  }
  lastAuthExpiredDispatch = now;
  localStorage.removeItem('authToken');
  localStorage.removeItem('currentUser');
  window.dispatchEvent(new Event('evhub-auth-expired'));
}

// Helper to make fetch requests with auth token, timeout and error handling
async function request(endpoint: string, options: RequestInit = {}) {
  const url = `${API_BASE}${endpoint}`;
  const token = localStorage.getItem('authToken');

  const headers: Record<string, string> = {
    'Content-Type': 'application/json',
    ...(token ? { Authorization: `Bearer ${token}` } : {}),
    ...(options.headers as Record<string, string> || {}),
  };

  const response = await fetch(url, {
    ...options,
    headers,
  });

  if (!response.ok) {
    if (response.status === 401 && endpoint !== '/login' && endpoint !== '/register') {
      const currentToken = localStorage.getItem('authToken');
      // Only treat as session expiration if:
      // 1. This request was actually sent with an active token (not an unauthenticated call)
      // 2. The token sent with this request is STILL the current token in localStorage (not replaced by a new login)
      if (token && currentToken === token) {
        notifyAuthExpired();
      }
    }
    const errorData = await response.json().catch(() => ({}));
    const message = errorData.message || `Request failed with status ${response.status}`;
    throw new Error(message);
  }

  return response.json();
}

export const api = {
  // Check backend server status
  async checkHealth() {
    return request('/health');
  },

  // 1. Authentication (Aditya Singh backend module)
  async login(emailOrUsername: string, password: string) {
    return request('/login', {
      method: 'POST',
      body: JSON.stringify({ email: emailOrUsername, password }),
    });
  },

  async register(name: string, email: string, password: string, role: string = 'user') {
    return request('/register', {
      method: 'POST',
      body: JSON.stringify({ name, email, password, role }),
    });
  },

  async getMe() {
    return request('/me');
  },

  async logout() {
    return request('/logout', {
      method: 'POST',
    });
  },

  async getUsers() {
    return request('/users');
  },

  // 2. DSA Charging Queue (Abhishek Ghansala module)
  async getChargingQueue() {
    return request('/charging/queue');
  },

  async enqueueCharging(evId: string) {
    return request('/charging/queue', {
      method: 'POST',
      body: JSON.stringify({ evId }),
    });
  },

  async dequeueCharging() {
    return request('/charging/dequeue', {
      method: 'POST',
    });
  },

  // 3. DSA Swap Queue (Abhishek Ghansala module)
  async getSwapQueue() {
    return request('/swap/queue');
  },

  async enqueueSwap(evId: string) {
    return request('/swap/queue', {
      method: 'POST',
      body: JSON.stringify({ evId }),
    });
  },

  async dequeueSwap() {
    return request('/swap/dequeue', {
      method: 'POST',
    });
  },

  // 4. DSA Transaction Stack (Abhishek Ghansala module)
  async getTransactionStack() {
    return request('/transactions/stack');
  },

  async pushTransactionStack(id: string) {
    return request('/transactions/stack', {
      method: 'POST',
      body: JSON.stringify({ id }),
    });
  },

  async popTransactionStack() {
    return request('/transactions/stack/pop', {
      method: 'POST',
    });
  },

  // 5. Electric Vehicles (Priyanshu Dutt module)
  async getEVs() {
    return request('/evs');
  },

  async getEVById(id: string) {
    return request(`/evs/${id}`);
  },

  async createEV(ev: any) {
    return request('/evs', {
      method: 'POST',
      body: JSON.stringify(ev),
    });
  },

  async updateEV(id: string, ev: any) {
    return request(`/evs/${id}`, {
      method: 'PUT',
      body: JSON.stringify(ev),
    });
  },

  async deleteEV(id: string) {
    return request(`/evs/${id}`, {
      method: 'DELETE',
    });
  },

  // 6. Charging Stations & Customer Requests (Mritunjay Thakur module)
  async getStations() {
    return request('/stations');
  },

  async getStationById(id: string) {
    return request(`/stations/${id}`);
  },

  async createStation(station: any) {
    return request('/stations', {
      method: 'POST',
      body: JSON.stringify(station),
    });
  },

  async getSessions(status?: string) {
    const query = status ? `?status=${encodeURIComponent(status)}` : '';
    return request(`/sessions${query}`);
  },

  async getSessionDetails(sessionId: string) {
    return request(`/sessions/${sessionId}`);
  },

  async getChargingRequests() {
    return request('/charging/requests');
  },

  async requestCharging(data: { evId: string; stationId?: string; targetCharge?: number } | string, targetCharge: number = 85.0) {
    let payload: any = {};
    if (typeof data === 'string') {
      payload = { evId: data, targetCharge };
    } else {
      payload = {
        evId: data.evId,
        stationId: data.stationId,
        targetCharge: data.targetCharge || 85.0,
      };
    }
    return request('/charging/request', {
      method: 'POST',
      body: JSON.stringify(payload),
    });
  },

  async cancelChargingRequest(requestId: string) {
    return request('/charging/request/' + encodeURIComponent(requestId) + '/cancel', {
      method: 'POST',
    });
  },

  async allocateBayManually(requestId: string, stationId: string) {
    return request('/charging/allocate', {
      method: 'POST',
      body: JSON.stringify({ requestId, stationId }),
    });
  },

  async updateStationStatus(id: string, status: string) {
    return request(`/stations/${id}/status`, {
      method: 'PUT',
      body: JSON.stringify({ status }),
    });
  },

  async completeCharging(stationOrSession: { sessionId?: string; stationId?: string } | string) {
    let body = {};
    if (typeof stationOrSession === 'string') {
      if (stationOrSession.startsWith('SES-')) {
        body = { sessionId: stationOrSession };
      } else {
        body = { stationId: stationOrSession };
      }
    } else {
      body = stationOrSession;
    }
    return request('/charging/complete', {
      method: 'POST',
      body: JSON.stringify(body),
    });
  },

  // 7. Batteries & Swaps (Amit Singh Kaintura module)
  async getBatteries() {
    return request('/batteries');
  },

  async createBattery(battery: any) {
    return request('/batteries', {
      method: 'POST',
      body: JSON.stringify(battery),
    });
  },

  async updateBattery(id: string, battery: any) {
    return request(`/batteries/${id}`, {
      method: 'PUT',
      body: JSON.stringify(battery),
    });
  },

  async requestSwap(evId: string, currentBattery?: string, requiredCapacity?: number) {
    return request('/swap/request', {
      method: 'POST',
      body: JSON.stringify({ evId, currentBattery, requiredCapacity }),
    });
  },

  async processSwap(evId?: string) {
    return request('/swap/process', {
      method: 'POST',
      body: JSON.stringify({ evId }),
    });
  },

  async getSwapAvailableCount() {
    return request('/swap/available-count');
  },

  // 8. Transactions & Reports (Agreev Singh module)
  async getTransactions(type?: string) {
    const query = type ? `?type=${encodeURIComponent(type)}` : '';
    return request(`/transactions${query}`);
  },

  async getTransactionById(id: string) {
    return request(`/transactions/${id}`);
  },

  async createTransaction(txn: any) {
    return request('/transactions', {
      method: 'POST',
      body: JSON.stringify(txn),
    });
  },

  async getReports() {
    return request('/reports');
  },

  async getAnalytics() {
    return request('/analytics');
  },
};
