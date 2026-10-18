/**
 * Smart EV Hub — Charging & Queue Calculation Utilities
 * 
 * Standards specified:
 * - Station ratings: 20 kW to 30 kW (average ~25 kW)
 * - Charging rate: 25 kW delivers 25 kWh in 1 hour
 * - Formula: Time (hours) = Energy (kWh) / Station Power (kW)
 *   e.g. 100 kWh battery => 100 / 25 = 4.0 hours (240 minutes)
 */

export const DEFAULT_STATION_POWER_KW = 25; // 25 kW delivery rate

/**
 * Calculates energy required in kWh to charge an EV battery to target percentage.
 */
export function calculateRequiredEnergy(
  batteryCapacityKwh: number,
  currentChargePercent: number,
  targetChargePercent: number = 80
): number {
  const neededPercent = Math.max(0, targetChargePercent - currentChargePercent);
  return Number(((batteryCapacityKwh * neededPercent) / 100).toFixed(2));
}

/**
 * Calculates charging time based on required energy and station power.
 * 
 * Example:
 * - 100 kWh on 25 kW station = 4 hours (240 min)
 * - 25 kWh on 25 kW station = 1 hour (60 min)
 * - 30 kWh on 25 kW station = 1.2 hours (72 min = 1h 12m)
 */
export function calculateChargingTime(
  energyKwh: number,
  stationPowerKw: number = DEFAULT_STATION_POWER_KW
): {
  hours: number;
  minutes: number;
  totalMinutes: number;
  formatted: string;
} {
  const power = stationPowerKw > 0 ? stationPowerKw : DEFAULT_STATION_POWER_KW;
  const totalHours = energyKwh / power;
  const totalMinutes = Math.round(totalHours * 60);
  const hours = Math.floor(totalMinutes / 60);
  const minutes = totalMinutes % 60;

  let formatted = '';
  if (hours > 0) {
    formatted = `${hours}h${minutes > 0 ? ` ${minutes}m` : ''}`;
  } else {
    formatted = `${Math.max(1, minutes)}m`;
  }

  return { hours, minutes, totalMinutes, formatted };
}

/**
 * Calculates charging time range specifically for 25 kW to 29 kW delivery rates.
 * (e.g. 25-29 kW per hour)
 */
export function calculateTimeRange25to29kW(energyKwh: number): {
  minFormatted: string;
  maxFormatted: string;
  rangeFormatted: string;
} {
  const at29 = calculateChargingTime(energyKwh, 29);
  const at25 = calculateChargingTime(energyKwh, 25);

  const rangeFormatted = at29.formatted === at25.formatted
    ? `~${at25.formatted}`
    : `~${at29.formatted} – ${at25.formatted}`;

  return {
    minFormatted: at29.formatted,
    maxFormatted: at25.formatted,
    rangeFormatted,
  };
}

/**
 * Calculates clean, realistic queue wait time.
 * Prevents absurd display of stale dates (e.g. 76547 min).
 */
export function getRealisticWaitMinutes(requestedAt: string, queuePosition: number = 0): number {
  const diffMs = Date.now() - new Date(requestedAt).getTime();
  const rawMinutes = Math.round(diffMs / 60000);

  // If the timestamp is fresh (0 to 180 min ago), show actual elapsed minutes
  if (rawMinutes >= 0 && rawMinutes <= 180) {
    return rawMinutes;
  }

  // Gracefully fallback for stale mock timestamps to realistic queue delay based on position
  // e.g. Position 0: 15 min, Position 1: 12 min, Position 2: 9 min, Position 3: 6 min, Position 4: 3 min
  return Math.max(2, 16 - queuePosition * 3);
}

/**
 * Formats wait time string (e.g. "12 min" or "< 5 min").
 */
export function formatWaitTime(minutes: number): string {
  if (minutes <= 1) return 'Just now';
  if (minutes < 60) return `${minutes} min`;
  const h = Math.floor(minutes / 60);
  const m = minutes % 60;
  return `${h}h ${m}m`;
}
