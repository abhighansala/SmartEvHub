import Chip from '@mui/material/Chip';
import { useTheme } from '@mui/material/styles';
import { statusColors } from '@/theme';

type StatusKey = keyof typeof statusColors;

interface StatusChipProps {
  status: string;
  size?: 'small' | 'medium';
}

const labelMap: Record<string, string> = {
  available: 'Available',
  charging: 'Charging',
  waiting: 'Waiting',
  swapping: 'Swapping',
  maintenance: 'Maintenance',
  offline: 'Offline',
  completed: 'Completed',
  inuse: 'In Use',
  unavailable: 'Unavailable',
  pending: 'Pending',
  fault: 'Fault',
  active: 'Active',
  failed: 'Failed',
  urgent: 'Urgent',
  normal: 'Normal',
  'battery-swap': 'Battery Swap',
};

export default function StatusChip({ status, size = 'small' }: StatusChipProps) {
  const theme = useTheme();
  const isDark = theme.palette.mode === 'dark';
  const key = status.toLowerCase() as StatusKey;
  const colors = statusColors[key] || statusColors.available;

  return (
    <Chip
      label={labelMap[key] || status}
      size={size}
      sx={{
        backgroundColor: isDark ? colors.darkBg : colors.bg,
        color: isDark ? colors.darkColor : colors.color,
        fontWeight: 600,
        border: 'none',
      }}
    />
  );
}
