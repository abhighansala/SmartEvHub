import { Routes, Route, Navigate } from 'react-router-dom';
import { useApp } from '@/context/AppContext';
import AppShell from '@/components/layout/AppShell';
import { Login } from '@/pages/Login';
import Dashboard from '@/pages/Dashboard';
import EVManagement from '@/pages/EVManagement';
import EVDetails from '@/pages/EVDetails';
import ChargingStations from '@/pages/ChargingStations';
import StationDetails from '@/pages/StationDetails';
import BatteryManagement from '@/pages/BatteryManagement';
import BatterySwap from '@/pages/BatterySwap';
import ChargingQueue from '@/pages/ChargingQueue';
import SwapQueue from '@/pages/SwapQueue';
import ActiveSessions from '@/pages/ActiveSessions';
import Transactions from '@/pages/Transactions';
import Reports from '@/pages/Reports';
import Analytics from '@/pages/Analytics';
import Settings from '@/pages/Settings';

export default function AppRoutes() {
  const { isAuthenticated } = useApp();

  return (
    <Routes>
      <Route path="/login" element={isAuthenticated ? <Navigate to="/dashboard" replace /> : <Login />} />
      
      <Route element={isAuthenticated ? <AppShell /> : <Navigate to="/login" replace />}>
        <Route index element={<Navigate to="/dashboard" replace />} />
        <Route path="dashboard" element={<Dashboard />} />
        <Route path="evs" element={<EVManagement />} />
        <Route path="evs/:id" element={<EVDetails />} />
        <Route path="stations" element={<ChargingStations />} />
        <Route path="stations/:id" element={<StationDetails />} />
        <Route path="batteries" element={<BatteryManagement />} />
        <Route path="battery-swap" element={<BatterySwap />} />
        <Route path="charging-queue" element={<ChargingQueue />} />
        <Route path="swap-queue" element={<SwapQueue />} />
        <Route path="sessions" element={<ActiveSessions />} />
        <Route path="transactions" element={<Transactions />} />
        <Route path="reports" element={<Reports />} />
        <Route path="analytics" element={<Analytics />} />
        <Route path="settings" element={<Settings />} />
        <Route path="*" element={<Navigate to="/dashboard" replace />} />
      </Route>
    </Routes>
  );
}
