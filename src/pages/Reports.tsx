import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import Grid from '@mui/material/Grid2';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import { useTheme } from '@mui/material/styles';
import {
  BarChart, Bar, LineChart, Line, XAxis, YAxis, CartesianGrid,
  Tooltip as RechartsTooltip, ResponsiveContainer, Legend,
} from 'recharts';

import DownloadOutlined from '@mui/icons-material/DownloadOutlined';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';
import SwapHorizOutlined from '@mui/icons-material/SwapHorizOutlined';
import BoltOutlined from '@mui/icons-material/BoltOutlined';
import AttachMoneyOutlined from '@mui/icons-material/AttachMoneyOutlined';
import TimerOutlined from '@mui/icons-material/TimerOutlined';
import HourglassEmptyOutlined from '@mui/icons-material/HourglassEmptyOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatCard from '@/components/common/StatCard';
import { useApp } from '@/context/AppContext';

import { useState, useEffect } from 'react';
import { api } from '@/services/api';

const defaultWeeklyData = [
  { day: 'Mon', sessions: 12, swaps: 4 },
  { day: 'Tue', sessions: 15, swaps: 6 },
  { day: 'Wed', sessions: 10, swaps: 3 },
  { day: 'Thu', sessions: 18, swaps: 7 },
  { day: 'Fri', sessions: 22, swaps: 5 },
  { day: 'Sat', sessions: 25, swaps: 9 },
  { day: 'Sun', sessions: 20, swaps: 8 },
];

const defaultStationUtilization = [
  { station: 'CS-1', utilization: 85 },
  { station: 'CS-2', utilization: 65 },
  { station: 'CS-3', utilization: 72 },
  { station: 'CS-4', utilization: 20 },
  { station: 'CS-5', utilization: 90 },
  { station: 'CS-6', utilization: 45 },
];

export default function Reports() {
  const theme = useTheme();
  const { transactions, isBackendOnline, showSnackbar } = useApp();

  const [backendReports, setBackendReports] = useState<any>(null);
  const [weeklyChartData, setWeeklyChartData] = useState(defaultWeeklyData);
  const [stationUtilData, setStationUtilData] = useState(defaultStationUtilization);

  useEffect(() => {
    if (isBackendOnline) {
      api.getReports().then(rep => {
        if (rep) setBackendReports(rep);
      }).catch(() => {});

      api.getAnalytics().then(ana => {
        if (ana && ana.weeklyData) setWeeklyChartData(ana.weeklyData);
        if (ana && ana.stationUtilization) setStationUtilData(ana.stationUtilization);
      }).catch(() => {});
    }
  }, [isBackendOnline]);

  const totalRevenue = backendReports?.totalRevenue ?? transactions.reduce((s, t) => s + t.amount, 0);
  const totalEnergy = backendReports?.totalEnergyDelivered ?? transactions.reduce((s, t) => s + t.energyDelivered, 0);
  const chargingSessions = backendReports?.chargingSessions ?? transactions.filter(t => t.type === 'charging').length;
  const batterySwaps = backendReports?.batterySwaps ?? transactions.filter(t => t.type === 'battery-swap').length;
  const avgDuration = backendReports?.averageDuration ?? (transactions.length > 0
    ? Math.round(transactions.reduce((s, t) => s + t.duration, 0) / transactions.length) : 0);
  const avgQueueTime = backendReports?.averageWaitingTime ?? 25;

  return (
    <Box>
      <PageHeader
        title="Reports"
        subtitle="Summary reports of charging operations"
        action={
          <Button
            variant="outlined"
            startIcon={<DownloadOutlined />}
            onClick={() => showSnackbar('Report export will be connected to backend later.', 'info')}
          >
            Export Report
          </Button>
        }
      />

      {/* KPI Cards */}
      <Grid container spacing={2.5} sx={{ mb: 3 }}>
        <Grid size={{ xs: 6, md: 2 }}>
          <StatCard title="Charging Sessions" value={chargingSessions} icon={ElectricBoltOutlined} color={theme.palette.info.main} />
        </Grid>
        <Grid size={{ xs: 6, md: 2 }}>
          <StatCard title="Battery Swaps" value={batterySwaps} icon={SwapHorizOutlined} color={theme.palette.secondary.main} />
        </Grid>
        <Grid size={{ xs: 6, md: 2 }}>
          <StatCard title="Energy Delivered" value={`${totalEnergy.toFixed(0)} kWh`} icon={BoltOutlined} color={theme.palette.warning.main} />
        </Grid>
        <Grid size={{ xs: 6, md: 2 }}>
          <StatCard title="Total Revenue" value={`₹${totalRevenue.toFixed(0)}`} icon={AttachMoneyOutlined} color={theme.palette.success.main} />
        </Grid>
        <Grid size={{ xs: 6, md: 2 }}>
          <StatCard title="Avg Duration" value={`${avgDuration} min`} icon={TimerOutlined} color={theme.palette.primary.main} />
        </Grid>
        <Grid size={{ xs: 6, md: 2 }}>
          <StatCard title="Avg Queue Time" value="25 min" icon={HourglassEmptyOutlined} color={theme.palette.error.main} />
        </Grid>
      </Grid>

      {/* Charts */}
      <Grid container spacing={2.5}>
        <Grid size={{ xs: 12, md: 7 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
            <Typography variant="h5" sx={{ mb: 2 }}>Sessions & Swaps This Week</Typography>
            <ResponsiveContainer width="100%" height={300}>
              <BarChart data={weeklyChartData}>
                <CartesianGrid strokeDasharray="3 3" stroke={theme.palette.divider} />
                <XAxis dataKey="day" tick={{ fill: theme.palette.text.secondary, fontSize: 12 }} />
                <YAxis tick={{ fill: theme.palette.text.secondary, fontSize: 12 }} />
                <RechartsTooltip
                  contentStyle={{
                    backgroundColor: theme.palette.background.paper,
                    border: `1px solid ${theme.palette.divider}`,
                    borderRadius: 8,
                  }}
                />
                <Legend />
                <Bar dataKey="sessions" name="Charging Sessions" fill={theme.palette.primary.main} radius={[4, 4, 0, 0]} />
                <Bar dataKey="swaps" name="Battery Swaps" fill={theme.palette.secondary.main} radius={[4, 4, 0, 0]} />
              </BarChart>
            </ResponsiveContainer>
          </Card>
        </Grid>
        <Grid size={{ xs: 12, md: 5 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
            <Typography variant="h5" sx={{ mb: 2 }}>Station Utilization (%)</Typography>
            <ResponsiveContainer width="100%" height={300}>
              <BarChart data={stationUtilData} layout="vertical">
                <CartesianGrid strokeDasharray="3 3" stroke={theme.palette.divider} />
                <XAxis type="number" domain={[0, 100]} tick={{ fill: theme.palette.text.secondary, fontSize: 12 }} />
                <YAxis dataKey="station" type="category" tick={{ fill: theme.palette.text.secondary, fontSize: 12 }} width={40} />
                <RechartsTooltip
                  contentStyle={{
                    backgroundColor: theme.palette.background.paper,
                    border: `1px solid ${theme.palette.divider}`,
                    borderRadius: 8,
                  }}
                />
                <Bar dataKey="utilization" name="Utilization %" fill={theme.palette.info.main} radius={[0, 4, 4, 0]} />
              </BarChart>
            </ResponsiveContainer>
          </Card>
        </Grid>
      </Grid>
    </Box>
  );
}
