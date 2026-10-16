// ============================================================================
// Smart EV Hub - Frontend API Client
// Role: UI + DSA + Final Integration
// Owner: Abhishek Ghansala (Student ID: 2518132, Section H)
// Connects the React UI to the C++ Crow Backend on http://localhost:8080
// ============================================================================

const API_BASE = 'http://localhost:8080/api';

// Helper to make fetch requests with timeout and error handling
async function request(endpoint: string, options: RequestInit = {}) {
  const url = `${API_BASE}${endpoint}`;
  const headers = {
    'Content-Type': 'application/json',
    ...(options.headers || {}),
  };

  const response = await fetch(url, {
    ...options,
    headers,
  });

  if (!response.ok) {
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
  async login(email: string, password: string) {
    return request('/login', {
      method: 'POST',
      body: JSON.stringify({ email, password }),
    });
  },

  async logout() {
    return request('/logout', {
      method: 'POST',
    });
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

  // 6. Charging Stations (Mritunjay Thakur module)
  async getStations() {
    return request('/stations');
  },

  async completeCharging(stationId: string) {
    return request('/charging/complete', {
      method: 'POST',
      body: JSON.stringify({ stationId }),
    });
  },

  // 7. Batteries & Swaps (Amit Singh Kaintura module)
  async getBatteries() {
    return request('/batteries');
  },

  async requestSwap(evId: string, currentBattery?: string, requiredCapacity?: number) {
    return request('/swap/request', {
      method: 'POST',
      body: JSON.stringify({ evId, currentBattery, requiredCapacity }),
    });
  },

  // 8. Transactions & Reports (Agreev Singh module)
  async getTransactions() {
    return request('/transactions');
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
