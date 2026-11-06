import type { Transaction } from '@/types';

export const mockTransactions: Transaction[] = [
  { id: 'TXN-5001', evId: 'EV-103', type: 'charging', stationOrBattery: 'CS-2', date: '2026-10-18T10:30:00', amount: 870.00, energyDelivered: 43.5, duration: 60, status: 'completed' },
  { id: 'TXN-5002', evId: 'EV-105', type: 'battery-swap', stationOrBattery: 'BAT-03', date: '2026-10-18T09:45:00', amount: 450.00, energyDelivered: 60.0, duration: 5, status: 'completed' },
  { id: 'TXN-5003', evId: 'EV-106', type: 'charging', stationOrBattery: 'CS-3', date: '2026-10-18T08:20:00', amount: 680.00, energyDelivered: 39.4, duration: 35, status: 'completed' },
  { id: 'TXN-5004', evId: 'EV-101', type: 'charging', stationOrBattery: 'CS-1', date: '2026-10-17T15:00:00', amount: 540.00, energyDelivered: 30.0, duration: 45, status: 'completed' },
  { id: 'TXN-5005', evId: 'EV-102', type: 'battery-swap', stationOrBattery: 'BAT-01', date: '2026-10-17T14:30:00', amount: 450.00, energyDelivered: 50.0, duration: 4, status: 'completed' },
  { id: 'TXN-5006', evId: 'EV-104', type: 'charging', stationOrBattery: 'CS-2', date: '2026-10-17T12:00:00', amount: 620.00, energyDelivered: 35.0, duration: 50, status: 'completed' },
  { id: 'TXN-5007', evId: 'EV-103', type: 'charging', stationOrBattery: 'CS-5', date: '2026-10-16T18:00:00', amount: 750.00, energyDelivered: 45.0, duration: 55, status: 'completed' },
  { id: 'TXN-5008', evId: 'EV-105', type: 'battery-swap', stationOrBattery: 'BAT-02', date: '2026-10-16T16:00:00', amount: 450.00, energyDelivered: 50.0, duration: 5, status: 'completed' },
  { id: 'TXN-5009', evId: 'EV-106', type: 'charging', stationOrBattery: 'CS-1', date: '2026-10-16T14:30:00', amount: 580.00, energyDelivered: 32.0, duration: 40, status: 'completed' },
  { id: 'TXN-5010', evId: 'EV-101', type: 'charging', stationOrBattery: 'CS-6', date: '2026-10-16T10:00:00', amount: 420.00, energyDelivered: 24.0, duration: 35, status: 'completed' },
];
