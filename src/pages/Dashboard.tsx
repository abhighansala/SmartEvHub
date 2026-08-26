import { useNavigate } from 'react-router-dom';
import Grid from '@mui/material/Grid2';
import Card from '@mui/material/Card';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import LinearProgress from '@mui/material/LinearProgress';
import Chip from '@mui/material/Chip';
import { useTheme, alpha } from '@mui/material/styles';

import DirectionsCarOutlined from '@mui/icons-material/DirectionsCarOutlined';
import EvStationOutlined from '@mui/icons-material/EvStationOutlined';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';
import FormatListNumberedOutlined from '@mui/icons-material/FormatListNumberedOutlined';
import BatteryChargingFullOutlined from '@mui/icons-material/BatteryChargingFullOutlined';
import ArrowForwardOutlined from '@mui/icons-material/ArrowForwardOutlined';

import { PieChart, Pie, Cell, ResponsiveContainer, Tooltip as RechartsTooltip } from 'recharts';

import PageHeader from '@/components/common/PageHeader';
import StatCard from '@/components/common/StatCard';
import StatusChip from '@/components/common/StatusChip';
import { useApp } from '@/context/AppContext';

export default function Dashboard() {
  const theme = useTheme();
  const navigate = useNavigate();
  const { evs, stations, sessions, chargingQueue, batteries, transactions } = useApp();

  const totalEVs = evs.length;
  const totalStations = stations.length;
  const currentlyCharging = sessions.length;
  const inQueue = chargingQueue.length;
  const availableBatteries = batteries.filter(b => b.status === 'available').length;

  // Station status data for donut chart
  const stationData = [
    { name: 'Available', value: stations.filter(s => s.status === 'available').length, color: theme.palette.success.main },
    { name: 'Charging', value: stations.filter(s => s.status === 'charging').length, color: theme.palette.info.main },
    { name: 'Maintenance', value: stations.filter(s => s.status === 'maintenance').length, color: theme.palette.warning.main },
    { name: 'Offline', value: stations.filter(s => s.status === 'offline').length, color: theme.palette.error.main },
  ].filter(d => d.value > 0);

  // Helper: time difference
  const getWaitTime = (requestedAt: string) => {
    const diff = Date.now() - new Date(requestedAt).getTime();
    return Math.round(diff / 60000);
  };

  const formatTime = (iso: string) => {
    return new Date(iso).toLocaleTimeString('en-IN', { hour: '2-digit', minute: '2-digit' });
  };

  const getDuration = (start: string) => {
    const diff = Date.now() - new Date(start).getTime();
    const h = Math.floor(diff / 3600000);
    const m = Math.round((diff % 3600000) / 60000);
    return h > 0 ? `${h}h ${m}m` : `${m}m`;
  };

  return (
    <Box>
      <PageHeader title="Dashboard" subtitle="Overview of EV charging station activity" />

      {/* Summary Cards */}
      <Grid container spacing={2.5} sx={{ mb: 3 }}>
        <Grid size={{ xs: 12, sm: 6, md: 4, lg: 2.4 }}>
          <StatCard
            title="Total EVs"
            value={totalEVs}
            icon={DirectionsCarOutlined}
            trend={{ value: '12% vs last month', positive: true }}
            color={theme.palette.primary.main}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 4, lg: 2.4 }}>
          <StatCard
            title="Charging Stations"
            value={totalStations}
            icon={EvStationOutlined}
            subtitle="All operational"
            color={theme.palette.secondary.main}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 4, lg: 2.4 }}>
          <StatCard
            title="Currently Charging"
            value={currentlyCharging}
            icon={ElectricBoltOutlined}
            trend={{ value: '2 more than yesterday', positive: true }}
            color={theme.palette.info.main}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 4, lg: 2.4 }}>
          <StatCard
            title="In Queue"
            value={inQueue}
            icon={FormatListNumberedOutlined}
            trend={{ value: `~${inQueue > 0 ? Math.round(inQueue * 15) : 0} min wait`, positive: false }}
            color={theme.palette.warning.main}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 4, lg: 2.4 }}>
          <StatCard
            title="Available Batteries"
            value={availableBatteries}
            icon={BatteryChargingFullOutlined}
            subtitle="Ready for swap"
            color={theme.palette.success.main}
          />
        </Grid>
      </Grid>

      {/* Middle row: Queue + Station Status + Recent Transactions */}
      <Grid container spacing={2.5} sx={{ mb: 3 }}>
        {/* Charging Queue */}
        <Grid size={{ xs: 12, md: 5 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, height: '100%' }}>
            <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 2 }}>
              <Typography variant="h4">Charging Queue</Typography>
              <Button
                size="small"
                endIcon={<ArrowForwardOutlined />}
                onClick={() => navigate('/charging-queue')}
              >
                View All
              </Button>
            </Box>
            {chargingQueue.length === 0 ? (
              <Typography variant="body2" color="text.secondary" sx={{ py: 4, textAlign: 'center' }}>
                Queue is empty — all requests have been processed.
              </Typography>
            ) : (
              <TableContainer>
                <Table size="small">
                  <TableHead>
                    <TableRow>
                      <TableCell>#</TableCell>
                      <TableCell>EV ID</TableCell>
                      <TableCell>Vehicle</TableCell>
                      <TableCell align="right">Wait</TableCell>
                    </TableRow>
                  </TableHead>
                  <TableBody>
                    {chargingQueue.slice(0, 5).map((req, i) => (
                      <TableRow key={req.id}>
                        <TableCell>
                          <Chip
                            label={i + 1}
                            size="small"
                            sx={{
                              minWidth: 28,
                              bgcolor: i === 0 ? alpha(theme.palette.primary.main, 0.1) : 'transparent',
                              fontWeight: 600,
                            }}
                          />
                        </TableCell>
                        <TableCell sx={{ fontWeight: 500 }}>{req.evId}</TableCell>
                        <TableCell>{req.evModel}</TableCell>
                        <TableCell align="right">
                          <Typography variant="body2" color="text.secondary">
                            {getWaitTime(req.requestedAt)} min
                          </Typography>
                        </TableCell>
                      </TableRow>
                    ))}
                  </TableBody>
                </Table>
              </TableContainer>
            )}
            <Box sx={{ display: 'flex', justifyContent: 'space-between', mt: 2, px: 1 }}>
              <Typography variant="caption" color="text.secondary" fontWeight={600}>
                ← FRONT
              </Typography>
              <Typography variant="caption" color="text.secondary" fontWeight={600}>
                REAR →
              </Typography>
            </Box>
          </Card>
        </Grid>

        {/* Station Status */}
        <Grid size={{ xs: 12, sm: 6, md: 3.5 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, height: '100%' }}>
            <Typography variant="h4" sx={{ mb: 2 }}>Station Status</Typography>
            <Box sx={{ display: 'flex', justifyContent: 'center', mb: 2 }}>
              <ResponsiveContainer width="100%" height={160}>
                <PieChart>
                  <Pie
                    data={stationData}
                    cx="50%"
                    cy="50%"
                    innerRadius={45}
                    outerRadius={70}
                    paddingAngle={3}
                    dataKey="value"
                    stroke="none"
                  >
                    {stationData.map((entry, i) => (
                      <Cell key={i} fill={entry.color} />
                    ))}
                  </Pie>
                  <RechartsTooltip />
                </PieChart>
              </ResponsiveContainer>
            </Box>
            <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1 }}>
              {stationData.map(d => (
                <Box key={d.name} sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                  <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                    <Box sx={{ width: 10, height: 10, borderRadius: '50%', bgcolor: d.color }} />
                    <Typography variant="body2">{d.name}</Typography>
                  </Box>
                  <Typography variant="body2" fontWeight={600}>{d.value}</Typography>
                </Box>
              ))}
            </Box>
          </Card>
        </Grid>

        {/* Recent Transactions */}
        <Grid size={{ xs: 12, sm: 6, md: 3.5 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, height: '100%' }}>
            <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 2 }}>
              <Typography variant="h4">Recent Transactions</Typography>
              <Button size="small" endIcon={<ArrowForwardOutlined />} onClick={() => navigate('/transactions')}>
                View All
              </Button>
            </Box>
            <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1.5 }}>
              {transactions.slice(0, 4).map(txn => (
                <Box
                  key={txn.id}
                  sx={{
                    display: 'flex',
                    justifyContent: 'space-between',
                    alignItems: 'center',
                    p: 1.5,
                    borderRadius: 2,
                    bgcolor: alpha(theme.palette.background.default, 0.5),
                  }}
                >
                  <Box>
                    <Typography variant="body2" fontWeight={600}>{txn.id}</Typography>
                    <Typography variant="caption" color="text.secondary">
                      {txn.evId} • {txn.type === 'charging' ? 'Charging' : 'Battery Swap'}
                    </Typography>
                  </Box>
                  <Box sx={{ textAlign: 'right' }}>
                    <Typography variant="body2" fontWeight={600}>₹{txn.amount.toFixed(2)}</Typography>
                    <Typography variant="caption" color="text.secondary">{formatTime(txn.date)}</Typography>
                  </Box>
                </Box>
              ))}
            </Box>
          </Card>
        </Grid>
      </Grid>

      {/* Active Sessions */}
      <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
        <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 2 }}>
          <Typography variant="h4">Active Sessions</Typography>
          <Button size="small" endIcon={<ArrowForwardOutlined />} onClick={() => navigate('/sessions')}>
            View All
          </Button>
        </Box>
        {sessions.length === 0 ? (
          <Typography variant="body2" color="text.secondary" sx={{ py: 4, textAlign: 'center' }}>
            No active charging sessions.
          </Typography>
        ) : (
          <TableContainer>
            <Table>
              <TableHead>
                <TableRow>
                  <TableCell>EV ID</TableCell>
                  <TableCell>Vehicle</TableCell>
                  <TableCell>Station</TableCell>
                  <TableCell>Battery</TableCell>
                  <TableCell>Start Time</TableCell>
                  <TableCell>Duration</TableCell>
                  <TableCell align="right">Est. Cost</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {sessions.map(s => {
                  const progress = Math.min(
                    s.batteryStart + (s.targetCharge - s.batteryStart) * 0.6,
                    s.targetCharge
                  );
                  return (
                    <TableRow key={s.id}>
                      <TableCell sx={{ fontWeight: 500 }}>{s.evId}</TableCell>
                      <TableCell>{s.evModel}</TableCell>
                      <TableCell>{s.stationId}</TableCell>
                      <TableCell sx={{ minWidth: 120 }}>
                        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                          <LinearProgress
                            variant="determinate"
                            value={progress}
                            sx={{
                              flex: 1,
                              height: 8,
                              borderRadius: 4,
                              bgcolor: alpha(theme.palette.info.main, 0.1),
                              '& .MuiLinearProgress-bar': {
                                borderRadius: 4,
                                bgcolor: progress < 30 ? theme.palette.error.main : progress < 60 ? theme.palette.warning.main : theme.palette.success.main,
                              },
                            }}
                          />
                          <Typography variant="caption" fontWeight={600} sx={{ minWidth: 32 }}>
                            {Math.round(progress)}%
                          </Typography>
                        </Box>
                      </TableCell>
                      <TableCell>{formatTime(s.startTime)}</TableCell>
                      <TableCell>{getDuration(s.startTime)}</TableCell>
                      <TableCell align="right" sx={{ fontWeight: 500 }}>₹{s.estimatedCost.toFixed(2)}</TableCell>
                    </TableRow>
                  );
                })}
              </TableBody>
            </Table>
          </TableContainer>
        )}
      </Card>
    </Box>
  );
}
