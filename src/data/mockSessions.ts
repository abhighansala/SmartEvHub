import type { ChargingSession } from '@/types';

export const mockSessions: ChargingSession[] = [
  { id: 'SES-1001', evId: 'EV-101', evModel: 'Tata Nexon EV Max', stationId: 'CS-1', startTime: '2026-08-26T09:30:00', endTime: null, batteryStart: 32, batteryEnd: 55, targetCharge: 80, estimatedCost: 145.00, status: 'active' },
  { id: 'SES-1002', evId: 'EV-106', evModel: 'Kia EV6', stationId: 'CS-5', startTime: '2026-08-26T08:00:00', endTime: null, batteryStart: 25, batteryEnd: 60, targetCharge: 90, estimatedCost: 210.00, status: 'active' },
];
