import type { Station } from '@/types';

export const mockStations: Station[] = [
  { id: 'S01', name: 'Station Alpha', power: 22, connectorType: 'Type 2', status: 'available', currentEV: null, sessionStart: null, location: 'Bay A1' },
  { id: 'S02', name: 'Station Beta', power: 50, connectorType: 'CCS2', status: 'charging', currentEV: 'EV101', sessionStart: '2026-08-26T09:30:00', location: 'Bay A2' },
  { id: 'S03', name: 'Station Gamma', power: 22, connectorType: 'Type 2', status: 'charging', currentEV: 'EV104', sessionStart: '2026-08-26T08:45:00', location: 'Bay B1' },
  { id: 'S04', name: 'Station Delta', power: 150, connectorType: 'CCS2', status: 'charging', currentEV: 'EV106', sessionStart: '2026-08-26T07:00:00', location: 'Bay B2' },
  { id: 'S05', name: 'Station Epsilon', power: 50, connectorType: 'CHAdeMO', status: 'charging', currentEV: 'EV111', sessionStart: '2026-08-26T08:00:00', location: 'Bay C1' },
  { id: 'S06', name: 'Station Zeta', power: 22, connectorType: 'Type 2', status: 'available', currentEV: null, sessionStart: null, location: 'Bay C2' },
];
