import type { Transaction } from '@/types';

export const mockTransactions: Transaction[] = [
  { id: 'TXN1001', evId: 'EV101', type: 'charging', stationOrBattery: 'S02', date: '2026-08-26T10:30:00', amount: 120.50, energyDelivered: 15.2, duration: 75, status: 'completed' },
  { id: 'TXN1002', evId: 'EV103', type: 'battery-swap', stationOrBattery: 'BAT003', date: '2026-08-26T09:45:00', amount: 85.00, energyDelivered: 28.5, duration: 10, status: 'completed' },
  { id: 'TXN1003', evId: 'EV108', type: 'charging', stationOrBattery: 'S01', date: '2026-08-25T16:20:00', amount: 210.00, energyDelivered: 30.0, duration: 120, status: 'completed' },
  { id: 'TXN1004', evId: 'EV105', type: 'charging', stationOrBattery: 'S04', date: '2026-08-25T15:00:00', amount: 65.75, energyDelivered: 8.5, duration: 45, status: 'completed' },
  { id: 'TXN1005', evId: 'EV110', type: 'battery-swap', stationOrBattery: 'BAT007', date: '2026-08-25T14:30:00', amount: 95.00, energyDelivered: 35.0, duration: 12, status: 'completed' },
  { id: 'TXN1006', evId: 'EV102', type: 'charging', stationOrBattery: 'S03', date: '2026-08-25T12:00:00', amount: 155.25, energyDelivered: 22.0, duration: 90, status: 'completed' },
  { id: 'TXN1007', evId: 'EV116', type: 'charging', stationOrBattery: 'S05', date: '2026-08-24T18:00:00', amount: 280.00, energyDelivered: 40.0, duration: 150, status: 'completed' },
  { id: 'TXN1008', evId: 'EV114', type: 'battery-swap', stationOrBattery: 'BAT006', date: '2026-08-24T16:00:00', amount: 45.00, energyDelivered: 12.0, duration: 8, status: 'completed' },
  { id: 'TXN1009', evId: 'EV118', type: 'charging', stationOrBattery: 'S01', date: '2026-08-24T14:30:00', amount: 178.50, energyDelivered: 25.0, duration: 100, status: 'completed' },
  { id: 'TXN1010', evId: 'EV121', type: 'charging', stationOrBattery: 'S06', date: '2026-08-24T10:00:00', amount: 92.00, energyDelivered: 12.5, duration: 60, status: 'completed' },
];
