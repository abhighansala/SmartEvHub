import type { Station } from '@/types';

// Stations with realistic power ratings between 25 kW and 29 kW
// Rate: 25-29 kW delivers 25-29 kWh in 1 hour
export const mockStations: Station[] = [
  { id: 'S01', name: 'Station Alpha', power: 25, connectorType: 'Type 2', status: 'available', currentEV: null, sessionStart: null, location: 'Bay A1' },
  { id: 'S02', name: 'Station Beta', power: 29, connectorType: 'CCS2', status: 'charging', currentEV: 'EV101', sessionStart: new Date(Date.now() - 35 * 60000).toISOString(), location: 'Bay A2' },
  { id: 'S03', name: 'Station Gamma', power: 26, connectorType: 'Type 2', status: 'charging', currentEV: 'EV104', sessionStart: new Date(Date.now() - 50 * 60000).toISOString(), location: 'Bay B1' },
  { id: 'S04', name: 'Station Delta', power: 28, connectorType: 'CCS2', status: 'charging', currentEV: 'EV106', sessionStart: new Date(Date.now() - 75 * 60000).toISOString(), location: 'Bay B2' },
  { id: 'S05', name: 'Station Epsilon', power: 25, connectorType: 'CHAdeMO', status: 'charging', currentEV: 'EV111', sessionStart: new Date(Date.now() - 20 * 60000).toISOString(), location: 'Bay C1' },
  { id: 'S06', name: 'Station Zeta', power: 27, connectorType: 'Type 2', status: 'available', currentEV: null, sessionStart: null, location: 'Bay C2' },
];
