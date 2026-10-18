import type { User } from '@/types';
import { mockUsers } from '@/data/mockUsers';

// Smart EV Hub - Centralized Authentication Service
// Module: Aditya Singh (Database + Backend Foundation - Team Lead)
// Integrates React Frontend with C++/Crow Backend & SQLite persistence

const CANDIDATE_URLS: string[] = [
  (import.meta as any).env?.VITE_API_BASE_URL,
  'http://localhost:8080',
  'http://localhost:18080',
].filter((u): u is string => Boolean(u)).map(u => u.replace(/\/+$/, ''));

export interface LoginResult {
  success: boolean;
  user: User;
  message: string;
  source: 'backend_sqlite' | 'offline_fallback';
}

/**
 * Authenticates the user with the C++ backend (Crow + SQLite).
 * Automatically probes both port 8080 and 18080. If neither is running,
 * falls back gracefully to local credentials.
 */
export async function loginUser(identifier: string, password: string): Promise<LoginResult> {
  const trimmedId = identifier.trim();
  const trimmedPass = password.trim();

  if (!trimmedId || !trimmedPass) {
    throw new Error('Email/Username and password are required.');
  }

  // Try each candidate backend endpoint
  for (const baseUrl of CANDIDATE_URLS) {
    try {
      const response = await fetch(`${baseUrl}/api/login`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          email: trimmedId,
          username: trimmedId,
          password: trimmedPass,
        }),
      });

      const data = await response.json();

      if (response.ok && data.success && data.user) {
        return {
          success: true,
          user: {
            id: String(data.user.id),
            name: data.user.name,
            email: data.user.email,
            role: data.user.role,
          },
          message: data.message || `Login successful (SQLite at ${baseUrl})`,
          source: 'backend_sqlite',
        };
      } else if (response.status === 401 || response.status === 400) {
        // Backend actively responded that password or username was wrong
        throw new Error(data.message || 'Invalid credentials');
      }
    } catch (err: any) {
      if (err.message && err.message !== 'Failed to fetch' && !err.message.includes('NetworkError') && !err.message.includes('fetch')) {
        throw err;
      }
      // Connection refused on this port, continue loop to try next candidate
    }
  }

  // If backend is offline on all ports, use local development fallback
  console.warn('[Smart EV Hub] C++ backend offline. Using verified fallback credentials.');
  const user = mockUsers.find(
    u => (u.name.toLowerCase() === trimmedId.toLowerCase() || (u as any).email?.toLowerCase() === trimmedId.toLowerCase()) &&
         u.password === trimmedPass
  );

  if (user) {
    return {
      success: true,
      user,
      message: 'Logged in (Local Fallback - C++ backend offline)',
      source: 'offline_fallback',
    };
  }

  throw new Error('Invalid email/username or password.');
}

/**
 * Logs out the current session and notifies the backend.
 */
export async function logoutUser(): Promise<void> {
  for (const baseUrl of CANDIDATE_URLS) {
    try {
      await fetch(`${baseUrl}/api/logout`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
      });
      break;
    } catch {
      // ignore offline errors on logout
    }
  }
}
