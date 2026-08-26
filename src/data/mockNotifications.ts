import type { Notification } from '@/types';

export const mockNotifications: Notification[] = [
  { id: 'N001', message: 'EV103 has been waiting for 35 minutes.', type: 'warning', timestamp: '2026-08-26T09:45:00', read: false, link: '/charging-queue' },
  { id: 'N002', message: 'Station S01 is now available.', type: 'success', timestamp: '2026-08-26T09:30:00', read: false, link: '/stations' },
  { id: 'N003', message: 'Charging session for EV108 completed.', type: 'success', timestamp: '2026-08-26T09:15:00', read: false, link: '/sessions' },
  { id: 'N004', message: 'Battery BAT011 requires maintenance.', type: 'error', timestamp: '2026-08-26T08:30:00', read: true, link: '/batteries' },
  { id: 'N005', message: 'New EV registered: EV124 — Tata Punch EV.', type: 'info', timestamp: '2026-08-26T08:00:00', read: true, link: '/evs' },
  { id: 'N006', message: 'EV117 battery critically low at 10%.', type: 'error', timestamp: '2026-08-26T09:25:00', read: false, link: '/evs' },
  { id: 'N007', message: 'Battery swap completed for EV115.', type: 'success', timestamp: '2026-08-26T09:20:00', read: true, link: '/battery-swap' },
  { id: 'N008', message: '5 vehicles currently in charging queue.', type: 'info', timestamp: '2026-08-26T09:00:00', read: true, link: '/charging-queue' },
];
