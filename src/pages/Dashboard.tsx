import { useState, useMemo } from 'react';
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
import Avatar from '@mui/material/Avatar';
import { useTheme, alpha } from '@mui/material/styles';

import DirectionsCarOutlined from '@mui/icons-material/DirectionsCarOutlined';
import EvStationOutlined from '@mui/icons-material/EvStationOutlined';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';
import FormatListNumberedOutlined from '@mui/icons-material/FormatListNumberedOutlined';
import BatteryChargingFullOutlined from '@mui/icons-material/BatteryChargingFullOutlined';
import ArrowForwardOutlined from '@mui/icons-material/ArrowForwardOutlined';
import ReceiptLongOutlined from '@mui/icons-material/ReceiptLongOutlined';
import AddOutlined from '@mui/icons-material/AddOutlined';
import CheckCircleOutline from '@mui/icons-material/CheckCircleOutline';
import HourglassEmptyOutlined from '@mui/icons-material/HourglassEmptyOutlined';

import { PieChart, Pie, Cell, ResponsiveContainer, Tooltip as RechartsTooltip } from 'recharts';

import PageHeader from '@/components/common/PageHeader';
import StatCard from '@/components/common/StatCard';
import StatusChip from '@/components/common/StatusChip';
import { useApp } from '@/context/AppContext';

export default function Dashboard() {
  const theme = useTheme();
  const navigate = useNavigate();
  const { evs, stations, sessions, chargingQueue, batteries, transactions, currentUser } = useApp();

  const isUserRole = currentUser?.role === 'user';

  const totalEVs = evs.length;
  const totalStations = stations.length;
  const activeSessions = sessions.filter(s => s.status === 'active');
  const currentlyCharging = activeSessions.length;
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

  // ──────────────────────────────────────────────────────────────────────────
  // USER ROLE DASHBOARD VIEW
  // ──────────────────────────────────────────────────────────────────────────
  if (isUserRole) {
    const userActiveSession = sessions.find(s => s.status === 'active');
    const userTotalSpent = transactions.reduce((sum, t) => sum + (t.amount || 0), 0);
    const userWaitingEv = evs.find(e => e.status === 'waiting');

    return (
      <Box>
        {/* Welcome Banner */}
        <Card
          sx={{
            p: 3,
            mb: 3,
            borderRadius: 3,
            background: `linear-gradient(135deg, ${alpha(theme.palette.primary.main, 0.08)} 0%, ${alpha(theme.palette.secondary.main, 0.12)} 100%)`,
            border: `1px solid ${alpha(theme.palette.primary.main, 0.2)}`,
            display: 'flex',
            flexDirection: { xs: 'column', sm: 'row' },
            alignItems: { xs: 'flex-start', sm: 'center' },
            justifyContent: 'space-between',
            gap: 2,
          }}
        >
          <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
            <Avatar
              sx={{
                width: 56,
                height: 56,
                bgcolor: 'primary.main',
                fontSize: '1.4rem',
                fontWeight: 700,
              }}
            >
              {currentUser?.name ? currentUser.name.substring(0, 2).toUpperCase() : 'EV'}
            </Avatar>
            <Box>
              <Typography variant="h4" fontWeight={700}>
                Welcome back, {currentUser?.name || 'EV Driver'}!
              </Typography>
              <Typography variant="body2" color="text.secondary">
                Smart EV Hub Customer Portal • Manage your electric vehicles and charging sessions
              </Typography>
            </Box>
          </Box>
          <Box sx={{ display: 'flex', gap: 1.5, flexWrap: 'wrap' }}>
            <Button
              variant="outlined"
              startIcon={<DirectionsCarOutlined />}
              onClick={() => navigate('/evs')}
            >
              My Vehicles
            </Button>
            <Button
              variant="contained"
              startIcon={<EvStationOutlined />}
              onClick={() => navigate('/stations')}
            >
              Find Charging Bay
            </Button>
          </Box>
        </Card>

        {/* User Stats Grid */}
        <Grid container spacing={2.5} sx={{ mb: 3 }}>
          <Grid size={{ xs: 12, sm: 6, md: 3 }}>
            <StatCard
              title="My Registered EVs"
              value={totalEVs}
              icon={DirectionsCarOutlined}
              subtitle="Associated with your account"
              color={theme.palette.primary.main}
            />
          </Grid>
          <Grid size={{ xs: 12, sm: 6, md: 3 }}>
            <StatCard
              title="Active Sessions"
              value={userActiveSession ? 1 : 0}
              icon={ElectricBoltOutlined}
              subtitle={userActiveSession ? `Charging at ${userActiveSession.stationId}` : 'No session running'}
              color={userActiveSession ? theme.palette.info.main : theme.palette.text.secondary}
            />
          </Grid>
          <Grid size={{ xs: 12, sm: 6, md: 3 }}>
            <StatCard
              title="Queue Status"
              value={userWaitingEv ? 'In Queue' : inQueue > 0 ? `${inQueue} in line` : 'Queue Empty'}
              icon={FormatListNumberedOutlined}
              subtitle={userWaitingEv ? `${userWaitingEv.id} waiting for bay` : 'Immediate bays available'}
              color={theme.palette.warning.main}
            />
          </Grid>
          <Grid size={{ xs: 12, sm: 6, md: 3 }}>
            <StatCard
              title="Total Expenditure"
              value={`₹${userTotalSpent.toFixed(2)}`}
              icon={ReceiptLongOutlined}
              subtitle={`${transactions.length} total operations`}
              color={theme.palette.success.main}
            />
          </Grid>
        </Grid>

        {/* Live Active Session Card (If session running) */}
        {userActiveSession && (
          <Card
            sx={{
              p: 3,
              mb: 3,
              border: `2px solid ${theme.palette.primary.main}`,
              bgcolor: alpha(theme.palette.primary.main, 0.03),
              borderRadius: 3,
            }}
          >
            <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 2, flexWrap: 'wrap', gap: 1 }}>
              <Box sx={{ display: 'flex', alignItems: 'center', gap: 1.5 }}>
                <Box
                  sx={{
                    width: 40,
                    height: 40,
                    borderRadius: '50%',
                    bgcolor: 'primary.main',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                  }}
                >
                  <ElectricBoltOutlined sx={{ color: '#fff' }} />
                </Box>
                <Box>
                  <Typography variant="h5" fontWeight={700}>
                    Live Charging Session in Progress
                  </Typography>
                  <Typography variant="caption" color="text.secondary">
                    Session ID: {userActiveSession.id} • Vehicle: {userActiveSession.evId} ({userActiveSession.evModel})
                  </Typography>
                </Box>
              </Box>
              <Box sx={{ display: 'flex', gap: 1.5 }}>
                <Button
                  variant="contained"
                  color="primary"
                  endIcon={<ArrowForwardOutlined />}
                  onClick={() => navigate('/sessions')}
                >
                  Monitor Live Session
                </Button>
              </Box>
            </Box>

            <Grid container spacing={3} sx={{ mt: 1 }}>
              <Grid size={{ xs: 12, md: 6 }}>
                <Box sx={{ display: 'flex', justifyContent: 'space-between', mb: 1 }}>
                  <Typography variant="body2" color="text.secondary">
                    Battery Progress: {userActiveSession.batteryStart}% → {Math.round(userActiveSession.currentCharge ?? userActiveSession.batteryStart)}%
                  </Typography>
                  <Typography variant="body2" fontWeight={600} color="primary.main">
                    Target: {userActiveSession.targetCharge}%
                  </Typography>
                </Box>
                <LinearProgress
                  variant="determinate"
                  value={userActiveSession.currentCharge ?? userActiveSession.batteryStart}
                  sx={{
                    height: 12,
                    borderRadius: 6,
                    bgcolor: alpha(theme.palette.info.main, 0.15),
                    '& .MuiLinearProgress-bar': {
                      borderRadius: 6,
                      bgcolor: theme.palette.success.main,
                    },
                  }}
                />
              </Grid>
              <Grid size={{ xs: 12, md: 6 }}>
                <Box sx={{ display: 'flex', justifyContent: 'space-around', textAlign: 'center' }}>
                  <Box>
                    <Typography variant="caption" color="text.secondary">Charging Station</Typography>
                    <Typography variant="h6" fontWeight={700}>{userActiveSession.stationId}</Typography>
                  </Box>
                  <Box>
                    <Typography variant="caption" color="text.secondary">Energy Delivered</Typography>
                    <Typography variant="h6" fontWeight={700}>{(userActiveSession.energyConsumed || 0).toFixed(1)} kWh</Typography>
                  </Box>
                  <Box>
                    <Typography variant="caption" color="text.secondary">Current Tariff</Typography>
                    <Typography variant="h6" fontWeight={700} color="primary.main">
                      ₹{(userActiveSession.estimatedCost || 0).toFixed(2)}
                    </Typography>
                  </Box>
                  <Box>
                    <Typography variant="caption" color="text.secondary">Duration</Typography>
                    <Typography variant="h6" fontWeight={700}>{getDuration(userActiveSession.startTime)}</Typography>
                  </Box>
                </Box>
              </Grid>
            </Grid>
          </Card>
        )}

        {/* My Registered Vehicles Section */}
        <Grid container spacing={2.5} sx={{ mb: 3 }}>
          <Grid size={{ xs: 12, md: 7 }}>
            <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, height: '100%' }}>
              <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 2 }}>
                <Typography variant="h5" fontWeight={700}>My Registered Vehicles</Typography>
                <Button
                  size="small"
                  startIcon={<AddOutlined />}
                  onClick={() => navigate('/evs')}
                >
                  Add Vehicle
                </Button>
              </Box>

              {evs.length === 0 ? (
                <Box sx={{ textAlign: 'center', py: 4 }}>
                  <DirectionsCarOutlined sx={{ fontSize: 48, color: 'text.secondary', mb: 1 }} />
                  <Typography variant="body1" fontWeight={600}>No vehicles registered yet</Typography>
                  <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
                    Register your electric vehicle to start using charging bays and swap services.
                  </Typography>
                  <Button variant="contained" onClick={() => navigate('/evs')}>
                    Register Vehicle
                  </Button>
                </Box>
              ) : (
                <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
                  {evs.map(ev => {
                    const socColor = ev.currentCharge < 30 ? theme.palette.error.main : ev.currentCharge < 60 ? theme.palette.warning.main : theme.palette.success.main;
                    return (
                      <Box
                        key={ev.id}
                        sx={{
                          p: 2,
                          borderRadius: 2,
                          border: `1px solid ${theme.palette.divider}`,
                          bgcolor: alpha(theme.palette.background.default, 0.5),
                          display: 'flex',
                          flexDirection: { xs: 'column', sm: 'row' },
                          justifyContent: 'space-between',
                          alignItems: { xs: 'flex-start', sm: 'center' },
                          gap: 1.5,
                        }}
                      >
                        <Box sx={{ minWidth: 160 }}>
                          <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                            <Typography variant="subtitle1" fontWeight={700}>{ev.model}</Typography>
                            <StatusChip status={ev.status} />
                          </Box>
                          <Typography variant="caption" color="text.secondary">
                            ID: {ev.id} • Battery: {ev.batteryCapacity} kWh
                          </Typography>
                        </Box>

                        <Box sx={{ width: { xs: '100%', sm: 180 } }}>
                          <Box sx={{ display: 'flex', justifyContent: 'space-between', mb: 0.5 }}>
                            <Typography variant="caption" color="text.secondary">Battery SoC</Typography>
                            <Typography variant="caption" fontWeight={700} sx={{ color: socColor }}>
                              {ev.currentCharge}%
                            </Typography>
                          </Box>
                          <LinearProgress
                            variant="determinate"
                            value={ev.currentCharge}
                            sx={{
                              height: 8,
                              borderRadius: 4,
                              bgcolor: alpha(socColor, 0.15),
                              '& .MuiLinearProgress-bar': { bgcolor: socColor, borderRadius: 4 },
                            }}
                          />
                        </Box>

                        <Box sx={{ display: 'flex', gap: 1 }}>
                          {ev.status === 'available' && (
                            <Button
                              size="small"
                              variant="contained"
                              startIcon={<ElectricBoltOutlined />}
                              onClick={() => navigate('/stations')}
                            >
                              Charge
                            </Button>
                          )}
                          {ev.status === 'charging' && (
                            <Button
                              size="small"
                              variant="outlined"
                              color="info"
                              onClick={() => navigate('/sessions')}
                            >
                              View Session
                            </Button>
                          )}
                          {ev.status === 'waiting' && (
                            <Button
                              size="small"
                              variant="outlined"
                              color="warning"
                              onClick={() => navigate('/charging-queue')}
                            >
                              In Queue
                            </Button>
                          )}
                        </Box>
                      </Box>
                    );
                  })}
                </Box>
              )}
            </Card>
          </Grid>

          {/* Quick Actions & Stations Overview */}
          <Grid size={{ xs: 12, md: 5 }}>
            <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, height: '100%' }}>
              <Typography variant="h5" fontWeight={700} sx={{ mb: 2 }}>Charging Bay Availability</Typography>
              <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1.5, mb: 3 }}>
                {stations.map(st => (
                  <Box
                    key={st.id}
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
                      <Typography variant="body2" fontWeight={600}>{st.name}</Typography>
                      <Typography variant="caption" color="text.secondary">
                        {st.id} • {st.power} kW ({st.connectorType})
                      </Typography>
                    </Box>
                    <StatusChip status={st.status} />
                  </Box>
                ))}
              </Box>

              <Typography variant="h6" fontWeight={700} sx={{ mb: 1.5 }}>Quick Actions</Typography>
              <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1 }}>
                <Button
                  variant="outlined"
                  fullWidth
                  startIcon={<EvStationOutlined />}
                  onClick={() => navigate('/stations')}
                  sx={{ justifyContent: 'flex-start', py: 1 }}
                >
                  Explore All Charging Bays
                </Button>
                <Button
                  variant="outlined"
                  fullWidth
                  startIcon={<BatteryChargingFullOutlined />}
                  onClick={() => navigate('/battery-swap')}
                  sx={{ justifyContent: 'flex-start', py: 1 }}
                >
                  Request Fast Battery Swap
                </Button>
                <Button
                  variant="outlined"
                  fullWidth
                  startIcon={<ReceiptLongOutlined />}
                  onClick={() => navigate('/transactions')}
                  sx={{ justifyContent: 'flex-start', py: 1 }}
                >
                  View Billing & Receipts
                </Button>
              </Box>
            </Card>
          </Grid>
        </Grid>

        {/* Recent Billing History */}
        <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
          <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 2 }}>
            <Typography variant="h5" fontWeight={700}>Recent Charging History & Bills</Typography>
            <Button size="small" endIcon={<ArrowForwardOutlined />} onClick={() => navigate('/transactions')}>
              View All Bills
            </Button>
          </Box>
          {transactions.length === 0 ? (
            <Typography variant="body2" color="text.secondary" sx={{ py: 3, textAlign: 'center' }}>
              No completed charging transactions yet. Start your first charging session to generate bills!
            </Typography>
          ) : (
            <TableContainer>
              <Table size="small">
                <TableHead>
                  <TableRow>
                    <TableCell>Transaction ID</TableCell>
                    <TableCell>Vehicle</TableCell>
                    <TableCell>Station / Bay</TableCell>
                    <TableCell align="right">Energy Delivered</TableCell>
                    <TableCell align="right">Amount</TableCell>
                    <TableCell>Date</TableCell>
                    <TableCell align="center">Status</TableCell>
                  </TableRow>
                </TableHead>
                <TableBody>
                  {transactions.slice(0, 5).map(txn => (
                    <TableRow key={txn.id} hover>
                      <TableCell sx={{ fontWeight: 600 }}>{txn.id}</TableCell>
                      <TableCell>{txn.evId}</TableCell>
                      <TableCell>{txn.stationOrBattery}</TableCell>
                      <TableCell align="right">{txn.energyDelivered ? `${txn.energyDelivered.toFixed(1)} kWh` : '-'}</TableCell>
                      <TableCell align="right" sx={{ fontWeight: 600, color: 'primary.main' }}>
                        ₹{txn.amount.toFixed(2)}
                      </TableCell>
                      <TableCell>{new Date(txn.date).toLocaleDateString('en-IN')}</TableCell>
                      <TableCell align="center">
                        <Chip label={txn.status} size="small" color="success" variant="outlined" />
                      </TableCell>
                    </TableRow>
                  ))}
                </TableBody>
              </Table>
            </TableContainer>
          )}
        </Card>
      </Box>
    );
  }

  // ──────────────────────────────────────────────────────────────────────────
  // OPERATOR / ADMIN DASHBOARD VIEW (Original System Architecture)
  // ──────────────────────────────────────────────────────────────────────────
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
        {activeSessions.length === 0 ? (
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
                {activeSessions.map(s => {
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
