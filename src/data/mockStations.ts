import type { Station } from '@/types';

export const mockStations: Station[] = [
  { id: 'CS-1', name: 'Standard Bay 1', power: 25, connectorType: 'CCS2', status: 'charging', currentEV: 'EV-101', sessionStart: '2026-08-26T09:30:00', location: 'Bay A-1' },
  { id: 'CS-2', name: 'Fast Bay 2', power: 29, connectorType: 'CCS2', status: 'available', currentEV: null, sessionStart: null, location: 'Bay A-2' },
  { id: 'CS-3', name: 'Flex Bay 3', power: 28, connectorType: 'CCS2', status: 'available', currentEV: null, sessionStart: null, location: 'Bay B-1' },
  { id: 'CS-4', name: 'AC Type 2 Charger 4', power: 26, connectorType: 'Type 2', status: 'maintenance', currentEV: null, sessionStart: null, location: 'Bay B-2' },
  { id: 'CS-5', name: 'High-Power Bay 5', power: 50, connectorType: 'CCS2', status: 'available', currentEV: null, sessionStart: null, location: 'Bay C-1' },
  { id: 'CS-6', name: 'Standard Bay 6', power: 22, connectorType: 'Type 2', status: 'available', currentEV: null, sessionStart: null, location: 'Bay C-2' },
];
