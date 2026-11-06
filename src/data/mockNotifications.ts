import type { Notification } from '@/types';

export const mockNotifications: Notification[] = [
  { id: 'N001', message: 'EV-103 has been waiting for 35 minutes.', type: 'warning', timestamp: '2026-10-18T09:45:00', read: false, link: '/charging-queue' },
  { id: 'N002', message: 'Station CS-1 is now available.', type: 'success', timestamp: '2026-10-18T09:30:00', read: false, link: '/stations' },
  { id: 'N003', message: 'Charging session for EV-101 completed.', type: 'success', timestamp: '2026-10-18T09:15:00', read: false, link: '/sessions' },
  { id: 'N004', message: 'Battery BAT-07 requires maintenance.', type: 'error', timestamp: '2026-10-18T08:30:00', read: true, link: '/batteries' },
  { id: 'N005', message: 'New EV registered: EV-106 — Kia EV6.', type: 'info', timestamp: '2026-10-18T08:00:00', read: true, link: '/evs' },
  { id: 'N006', message: 'EV-104 battery critically low at 12%.', type: 'error', timestamp: '2026-10-18T09:25:00', read: false, link: '/evs' },
  { id: 'N007', message: 'Battery swap completed for EV-105.', type: 'success', timestamp: '2026-10-18T09:20:00', read: true, link: '/battery-swap' },
  { id: 'N008', message: '2 vehicles currently in charging queue.', type: 'info', timestamp: '2026-10-18T09:00:00', read: true, link: '/charging-queue' },
];
