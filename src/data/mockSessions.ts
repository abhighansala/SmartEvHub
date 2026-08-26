import type { ChargingSession } from '@/types';

export const mockSessions: ChargingSession[] = [
  { id: 'SES001', evId: 'EV101', evModel: 'Tata Nexon EV', stationId: 'S02', startTime: '2026-08-26T09:30:00', endTime: null, batteryStart: 20, batteryEnd: null, targetCharge: 80, estimatedCost: 120.50, status: 'active' },
  { id: 'SES002', evId: 'EV104', evModel: 'Hyundai Kona EV', stationId: 'S03', startTime: '2026-08-26T08:45:00', endTime: null, batteryStart: 35, batteryEnd: null, targetCharge: 90, estimatedCost: 95.00, status: 'active' },
  { id: 'SES003', evId: 'EV106', evModel: 'Mahindra XUV400', stationId: 'S04', startTime: '2026-08-26T07:00:00', endTime: null, batteryStart: 10, batteryEnd: null, targetCharge: 85, estimatedCost: 185.75, status: 'active' },
  { id: 'SES004', evId: 'EV111', evModel: 'Citroen eC3', stationId: 'S05', startTime: '2026-08-26T08:00:00', endTime: null, batteryStart: 15, batteryEnd: null, targetCharge: 80, estimatedCost: 78.00, status: 'active' },
];
