import type { Battery } from '@/types';

export const mockBatteries: Battery[] = [
  { id: 'BAT001', capacity: 40.5, currentCharge: 95, status: 'available', currentVehicle: null, lastUpdated: '2026-08-26T08:00:00', cycleCount: 120 },
  { id: 'BAT002', capacity: 40.5, currentCharge: 88, status: 'available', currentVehicle: null, lastUpdated: '2026-08-26T07:45:00', cycleCount: 85 },
  { id: 'BAT003', capacity: 39.4, currentCharge: 72, status: 'available', currentVehicle: null, lastUpdated: '2026-08-26T06:30:00', cycleCount: 200 },
  { id: 'BAT004', capacity: 50.3, currentCharge: 60, status: 'charging', currentVehicle: null, lastUpdated: '2026-08-26T09:00:00', cycleCount: 150 },
  { id: 'BAT005', capacity: 39.2, currentCharge: 45, status: 'inuse', currentVehicle: 'EV115', lastUpdated: '2026-08-26T09:15:00', cycleCount: 90 },
  { id: 'BAT006', capacity: 26.0, currentCharge: 100, status: 'available', currentVehicle: null, lastUpdated: '2026-08-26T05:00:00', cycleCount: 50 },
  { id: 'BAT007', capacity: 60.5, currentCharge: 80, status: 'available', currentVehicle: null, lastUpdated: '2026-08-26T07:00:00', cycleCount: 110 },
  { id: 'BAT008', capacity: 40.5, currentCharge: 30, status: 'charging', currentVehicle: null, lastUpdated: '2026-08-26T08:30:00', cycleCount: 175 },
  { id: 'BAT009', capacity: 35.0, currentCharge: 92, status: 'available', currentVehicle: null, lastUpdated: '2026-08-26T06:00:00', cycleCount: 65 },
  { id: 'BAT010', capacity: 39.4, currentCharge: 55, status: 'available', currentVehicle: null, lastUpdated: '2026-08-26T07:30:00', cycleCount: 140 },
  { id: 'BAT011', capacity: 50.3, currentCharge: 15, status: 'maintenance', currentVehicle: null, lastUpdated: '2026-08-25T20:00:00', cycleCount: 300 },
  { id: 'BAT012', capacity: 26.0, currentCharge: 85, status: 'available', currentVehicle: null, lastUpdated: '2026-08-26T04:00:00', cycleCount: 78 },
  { id: 'BAT013', capacity: 40.5, currentCharge: 70, status: 'available', currentVehicle: null, lastUpdated: '2026-08-26T07:15:00', cycleCount: 95 },
  { id: 'BAT014', capacity: 39.2, currentCharge: 40, status: 'charging', currentVehicle: null, lastUpdated: '2026-08-26T08:45:00', cycleCount: 160 },
  { id: 'BAT015', capacity: 60.5, currentCharge: 98, status: 'available', currentVehicle: null, lastUpdated: '2026-08-26T03:00:00', cycleCount: 42 },
  { id: 'BAT016', capacity: 35.0, currentCharge: 78, status: 'available', currentVehicle: null, lastUpdated: '2026-08-26T06:45:00', cycleCount: 88 },
  { id: 'BAT017', capacity: 40.5, currentCharge: 65, status: 'available', currentVehicle: null, lastUpdated: '2026-08-26T05:30:00', cycleCount: 130 },
  { id: 'BAT018', capacity: 29.2, currentCharge: 50, status: 'available', currentVehicle: null, lastUpdated: '2026-08-26T07:00:00', cycleCount: 105 },
  { id: 'BAT019', capacity: 39.4, currentCharge: 5, status: 'unavailable', currentVehicle: null, lastUpdated: '2026-08-25T22:00:00', cycleCount: 350 },
  { id: 'BAT020', capacity: 50.3, currentCharge: 82, status: 'available', currentVehicle: null, lastUpdated: '2026-08-26T06:15:00', cycleCount: 72 },
];
