import type { ChargingRequest, SwapRequest } from '@/types';

export const mockChargingQueue: ChargingRequest[] = [
  { id: 'CQ001', evId: 'EV103', evModel: 'MG ZS EV', owner: 'Rahul Patel', requestedAt: '2026-08-26T09:10:00', currentCharge: 20, priority: 'normal' },
  { id: 'CQ002', evId: 'EV107', evModel: 'Hyundai Kona EV', owner: 'Karthik Nair', requestedAt: '2026-08-26T08:50:00', currentCharge: 30, priority: 'normal' },
  { id: 'CQ003', evId: 'EV109', evModel: 'Mahindra XUV400', owner: 'Arjun Kumar', requestedAt: '2026-08-26T09:00:00', currentCharge: 15, priority: 'urgent' },
  { id: 'CQ004', evId: 'EV112', evModel: 'BYD Atto 3', owner: 'Nisha Das', requestedAt: '2026-08-26T09:20:00', currentCharge: 25, priority: 'normal' },
  { id: 'CQ005', evId: 'EV117', evModel: 'Tata Tigor EV', owner: 'Deepak Tiwari', requestedAt: '2026-08-26T09:25:00', currentCharge: 10, priority: 'urgent' },
];

export const mockSwapQueue: SwapRequest[] = [
  { id: 'SQ001', evId: 'EV201', evModel: 'Tata Nexon EV', owner: 'Gaurav Sinha', currentBattery: 'BAT-OLD-01', requiredCapacity: 40.5, requestedAt: '2026-08-26T09:00:00' },
  { id: 'SQ002', evId: 'EV203', evModel: 'Mahindra XUV400', owner: 'Pallavi Jain', currentBattery: 'BAT-OLD-03', requiredCapacity: 39.4, requestedAt: '2026-08-26T09:10:00' },
  { id: 'SQ003', evId: 'EV205', evModel: 'Tata Tigor EV', owner: 'Manish Dubey', currentBattery: 'BAT-OLD-05', requiredCapacity: 26.0, requestedAt: '2026-08-26T09:20:00' },
];
