import type { ChargingRequest, SwapRequest } from '@/types';

// Dynamic recent timestamps (within last 15 minutes) to ensure clean, realistic wait times
const now = Date.now();

export const mockChargingQueue: ChargingRequest[] = [
  { id: 'CQ001', evId: 'EV103', evModel: 'MG ZS EV (50 kWh)', owner: 'Rahul Patel', requestedAt: new Date(now - 14 * 60000).toISOString(), currentCharge: 20, priority: 'normal' },
  { id: 'CQ002', evId: 'EV107', evModel: 'Hyundai Kona EV (39 kWh)', owner: 'Karthik Nair', requestedAt: new Date(now - 11 * 60000).toISOString(), currentCharge: 30, priority: 'normal' },
  { id: 'CQ003', evId: 'EV109', evModel: 'Mahindra XUV400 (39 kWh)', owner: 'Arjun Kumar', requestedAt: new Date(now - 8 * 60000).toISOString(), currentCharge: 15, priority: 'urgent' },
  { id: 'CQ004', evId: 'EV112', evModel: 'BYD Atto 3 (60 kWh)', owner: 'Nisha Das', requestedAt: new Date(now - 5 * 60000).toISOString(), currentCharge: 25, priority: 'normal' },
  { id: 'CQ005', evId: 'EV125', evModel: 'Volvo EX90 (100 kWh)', owner: 'Vikram Malhotra', requestedAt: new Date(now - 2 * 60000).toISOString(), currentCharge: 0, priority: 'urgent' },
];

export const mockSwapQueue: SwapRequest[] = [
  { id: 'SQ001', evId: 'EV201', evModel: 'Tata Nexon EV', owner: 'Gaurav Sinha', currentBattery: 'BAT001', requiredCapacity: 40.5, requestedAt: new Date(now - 10 * 60000).toISOString() },
  { id: 'SQ002', evId: 'EV203', evModel: 'Mahindra XUV400', owner: 'Pallavi Jain', currentBattery: 'BAT003', requiredCapacity: 39.4, requestedAt: new Date(now - 6 * 60000).toISOString() },
  { id: 'SQ003', evId: 'EV205', evModel: 'Tata Tigor EV', owner: 'Manish Dubey', currentBattery: 'BAT006', requiredCapacity: 26.0, requestedAt: new Date(now - 3 * 60000).toISOString() },
];
