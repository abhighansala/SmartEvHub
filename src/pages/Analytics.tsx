import { useState } from 'react';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import Grid from '@mui/material/Grid2';
import Typography from '@mui/material/Typography';
import ToggleButton from '@mui/material/ToggleButton';
import ToggleButtonGroup from '@mui/material/ToggleButtonGroup';
import { useTheme } from '@mui/material/styles';
import {
  LineChart, Line, BarChart, Bar, AreaChart, Area,
  XAxis, YAxis, CartesianGrid,
  Tooltip as RechartsTooltip, ResponsiveContainer, Legend,
} from 'recharts';

import PageHeader from '@/components/common/PageHeader';

const generateData = (period: string) => {
  const points = period === 'today' ? 12 : period === '7days' ? 7 : 30;
  return Array.from({ length: points }, (_, i) => ({
    label: period === 'today' ? `${(i * 2) % 24}:00`
      : period === '7days' ? ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'][i]
      : `Day ${i + 1}`,
    sessions: Math.floor(Math.random() * 15 + 5),
    queueLength: Math.floor(Math.random() * 8 + 1),
    utilization: Math.floor(Math.random() * 40 + 50),
    batteryUsage: Math.floor(Math.random() * 10 + 3),
  }));
};

export default function Analytics() {
  const theme = useTheme();
  const [period, setPeriod] = useState('7days');
  const data = generateData(period);

  return (
    <Box>
      <PageHeader
        title="Analytics"
        subtitle="Detailed analytics and trends"
        action={
          <ToggleButtonGroup
            value={period}
            exclusive
            onChange={(_, v) => v && setPeriod(v)}
            size="small"
          >
            <ToggleButton value="today">Today</ToggleButton>
            <ToggleButton value="7days">7 Days</ToggleButton>
            <ToggleButton value="30days">30 Days</ToggleButton>
          </ToggleButtonGroup>
        }
      />

      <Grid container spacing={2.5}>
        {/* Charging Sessions */}
        <Grid size={{ xs: 12, md: 6 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
            <Typography variant="h5" sx={{ mb: 2 }}>Charging Sessions</Typography>
            <ResponsiveContainer width="100%" height={260}>
              <LineChart data={data}>
                <CartesianGrid strokeDasharray="3 3" stroke={theme.palette.divider} />
                <XAxis dataKey="label" tick={{ fill: theme.palette.text.secondary, fontSize: 11 }} />
                <YAxis tick={{ fill: theme.palette.text.secondary, fontSize: 11 }} />
                <RechartsTooltip contentStyle={{ backgroundColor: theme.palette.background.paper, border: `1px solid ${theme.palette.divider}`, borderRadius: 8 }} />
                <Line type="monotone" dataKey="sessions" stroke={theme.palette.primary.main} strokeWidth={2} dot={{ r: 3 }} />
              </LineChart>
            </ResponsiveContainer>
          </Card>
        </Grid>

        {/* Station Utilization */}
        <Grid size={{ xs: 12, md: 6 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
            <Typography variant="h5" sx={{ mb: 2 }}>Station Utilization (%)</Typography>
            <ResponsiveContainer width="100%" height={260}>
              <AreaChart data={data}>
                <CartesianGrid strokeDasharray="3 3" stroke={theme.palette.divider} />
                <XAxis dataKey="label" tick={{ fill: theme.palette.text.secondary, fontSize: 11 }} />
                <YAxis domain={[0, 100]} tick={{ fill: theme.palette.text.secondary, fontSize: 11 }} />
                <RechartsTooltip contentStyle={{ backgroundColor: theme.palette.background.paper, border: `1px solid ${theme.palette.divider}`, borderRadius: 8 }} />
                <Area type="monotone" dataKey="utilization" stroke={theme.palette.info.main} fill={theme.palette.info.main} fillOpacity={0.15} strokeWidth={2} />
              </AreaChart>
            </ResponsiveContainer>
          </Card>
        </Grid>

        {/* Queue Length */}
        <Grid size={{ xs: 12, md: 6 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
            <Typography variant="h5" sx={{ mb: 2 }}>Queue Length</Typography>
            <ResponsiveContainer width="100%" height={260}>
              <BarChart data={data}>
                <CartesianGrid strokeDasharray="3 3" stroke={theme.palette.divider} />
                <XAxis dataKey="label" tick={{ fill: theme.palette.text.secondary, fontSize: 11 }} />
                <YAxis tick={{ fill: theme.palette.text.secondary, fontSize: 11 }} />
                <RechartsTooltip contentStyle={{ backgroundColor: theme.palette.background.paper, border: `1px solid ${theme.palette.divider}`, borderRadius: 8 }} />
                <Bar dataKey="queueLength" name="Queue Length" fill={theme.palette.warning.main} radius={[4, 4, 0, 0]} />
              </BarChart>
            </ResponsiveContainer>
          </Card>
        </Grid>

        {/* Battery Usage */}
        <Grid size={{ xs: 12, md: 6 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
            <Typography variant="h5" sx={{ mb: 2 }}>Battery Usage (Swaps)</Typography>
            <ResponsiveContainer width="100%" height={260}>
              <LineChart data={data}>
                <CartesianGrid strokeDasharray="3 3" stroke={theme.palette.divider} />
                <XAxis dataKey="label" tick={{ fill: theme.palette.text.secondary, fontSize: 11 }} />
                <YAxis tick={{ fill: theme.palette.text.secondary, fontSize: 11 }} />
                <RechartsTooltip contentStyle={{ backgroundColor: theme.palette.background.paper, border: `1px solid ${theme.palette.divider}`, borderRadius: 8 }} />
                <Line type="monotone" dataKey="batteryUsage" stroke={theme.palette.secondary.main} strokeWidth={2} dot={{ r: 3 }} />
              </LineChart>
            </ResponsiveContainer>
          </Card>
        </Grid>
      </Grid>
    </Box>
  );
}
