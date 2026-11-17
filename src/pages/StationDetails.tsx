import { useState, useEffect } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import Grid from '@mui/material/Grid2';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import LinearProgress from '@mui/material/LinearProgress';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import TextField from '@mui/material/TextField';
import MenuItem from '@mui/material/MenuItem';
import Alert from '@mui/material/Alert';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import { useTheme, alpha } from '@mui/material/styles';

import ArrowBackOutlined from '@mui/icons-material/ArrowBackOutlined';
import StopCircleOutlined from '@mui/icons-material/StopCircleOutlined';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';
import BuildOutlined from '@mui/icons-material/BuildOutlined';
import PowerOffOutlined from '@mui/icons-material/PowerOffOutlined';
import HistoryOutlined from '@mui/icons-material/HistoryOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import EmptyState from '@/components/common/EmptyState';
import { useApp } from '@/context/AppContext';
import { api } from '@/services/api';
import type { StationStatus } from '@/types';

export default function StationDetails() {
  const { id } = useParams<{ id: string }>();
  const navigate = useNavigate();
  const theme = useTheme();
  const { stations, sessions, currentUser, completeSession, refreshBackendData, showSnackbar } = useApp();

  const isOperatorOrAdmin = currentUser?.role === 'operator' || currentUser?.role === 'admin';

  const [confirmStopOpen, setConfirmStopOpen] = useState(false);
  const [statusDialogOpen, setStatusDialogOpen] = useState(false);
  const [targetStatus, setTargetStatus] = useState<StationStatus>('available');
  const [statusLoading, setStatusLoading] = useState(false);
  const [statusError, setStatusError] = useState('');

  const station = stations.find(s => s.id === id);
  if (!station) {
    return (
      <EmptyState
        title="Station Not Found"
        description={`No station found with ID "${id}".`}
        actionLabel="Back to Stations"
        onAction={() => navigate('/stations')}
      />
    );
  }

  const session = sessions.find(
    s => (s.stationId === station.id || (station.currentEV && s.evId === station.currentEV)) && s.status === 'active'
  );

  const isOccupied = station.status === 'charging' || !!session;

  // Real-time synchronization when station is occupied
  useEffect(() => {
    if (!isOccupied) return;
    const interval = setInterval(() => {
      refreshBackendData();
    }, 3000);
    return () => clearInterval(interval);
  }, [isOccupied, refreshBackendData]);

  const getDuration = (start?: string | null) => {
    if (!start) return 'Active';
    const diff = Math.max(0, Date.now() - new Date(start).getTime());
    const h = Math.floor(diff / 3600000);
    const m = Math.round((diff % 3600000) / 60000);
    return h > 0 ? `${h}h ${m}m` : `${m}m`;
  };

  const progress = session
    ? (session.currentCharge ?? (session.batteryEnd ?? session.batteryStart))
    : 50;

  // Past sessions for this specific charging bay
  const stationSessions = sessions.filter(s => s.stationId === station.id);

  const handleUpdateStatus = async () => {
    setStatusLoading(true);
    setStatusError('');

    try {
      const res: any = await api.updateStationStatus(station.id, targetStatus);
      setStatusLoading(false);
      if (res && res.success) {
        setStatusDialogOpen(false);
        showSnackbar(`Station status updated to ${targetStatus}`, 'success');
        await refreshBackendData();
      } else {
        setStatusError(res?.message || 'Failed to update status');
      }
    } catch (err: any) {
      setStatusLoading(false);
      setStatusError(err.message || 'Error updating status');
    }
  };

  const handleConfirmStop = async () => {
    setConfirmStopOpen(false);
    await completeSession(session ? session.id : station.id);
  };

  return (
    <Box>
      <PageHeader
        title={`${station.name}`}
        subtitle={`${station.id} • ${station.location}`}
        action={
          <Box sx={{ display: 'flex', gap: 1.5 }}>
            {isOperatorOrAdmin && (
              <Button
                variant="outlined"
                startIcon={<BuildOutlined />}
                onClick={() => {
                  setTargetStatus(station.status);
                  setStatusError('');
                  setStatusDialogOpen(true);
                }}
              >
                Manage Status
              </Button>
            )}
            <Button startIcon={<ArrowBackOutlined />} onClick={() => navigate('/stations')}>
              Back
            </Button>
          </Box>
        }
      />

      <Grid container spacing={2.5}>
        <Grid size={{ xs: 12, md: 5 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
            <Typography variant="h5" sx={{ mb: 2 }}>Station Information</Typography>
            <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
              {[
                ['Station ID', station.id],
                ['Name', station.name],
                ['Power Output', `${station.power} kW`],
                ['Connector', station.connectorType],
                ['Location', station.location],
              ].map(([label, value]) => (
                <Box key={label} sx={{ display: 'flex', justifyContent: 'space-between' }}>
                  <Typography variant="body2" color="text.secondary">{label}</Typography>
                  <Typography variant="body2" fontWeight={500}>{value}</Typography>
                </Box>
              ))}
              <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                <Typography variant="body2" color="text.secondary">Status</Typography>
                <StatusChip status={station.status} />
              </Box>
              <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                <Typography variant="body2" color="text.secondary">Current EV</Typography>
                <Typography variant="body2" fontWeight={500}>{station.currentEV || (session ? session.evId : 'None')}</Typography>
              </Box>
            </Box>
          </Card>
        </Grid>

        <Grid size={{ xs: 12, md: 7 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, height: '100%' }}>
            {isOccupied ? (
              <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 3 }}>
                <Box
                  sx={{
                    display: 'flex',
                    alignItems: 'center',
                    gap: 1,
                    color: theme.palette.info.main,
                  }}
                >
                  <ElectricBoltOutlined sx={{ fontSize: 28, animation: 'pulse 1.5s infinite' }} />
                  <Typography variant="h3" color="info.main" fontWeight={700}>
                    Charging...
                  </Typography>
                </Box>

                <Box sx={{ width: '100%', textAlign: 'center' }}>
                  <Typography variant="h1" sx={{ fontSize: '3rem', fontWeight: 700, color: theme.palette.info.main }}>
                    {Math.round(progress)}%
                  </Typography>
                  <LinearProgress
                    variant="determinate"
                    value={Math.min(100, Math.max(0, progress))}
                    sx={{
                      mt: 2,
                      height: 12,
                      borderRadius: 6,
                      bgcolor: alpha(theme.palette.info.main, 0.1),
                      '& .MuiLinearProgress-bar': { borderRadius: 6 },
                    }}
                  />
                  <Typography variant="body2" color="text.secondary" sx={{ mt: 1 }}>
                    Target: {session?.targetCharge || 85}%
                    {session?.simulatedRemainingMinutes != null && session.simulatedRemainingMinutes > 0 && (
                      <> • {session.speedMultiplier && session.speedMultiplier > 1 ? `${session.simulatedRemainingMinutes}m simulated (~${session.demoRemainingMinutes || Math.ceil(session.simulatedRemainingMinutes / session.speedMultiplier)}m demo)` : `${session.simulatedRemainingMinutes}m remaining`}</>
                    )}
                  </Typography>
                </Box>

                <Box sx={{ display: 'flex', gap: 3, flexWrap: 'wrap', justifyContent: 'center' }}>
                  <Box sx={{ textAlign: 'center' }}>
                    <Typography variant="h4">{session?.evId || station.currentEV || 'Electric Vehicle'}</Typography>
                    <Typography variant="caption" color="text.secondary">Vehicle</Typography>
                  </Box>
                  <Box sx={{ textAlign: 'center' }}>
                    <Typography variant="h4">{getDuration(session?.startTime || station.sessionStart)}</Typography>
                    <Typography variant="caption" color="text.secondary">Duration</Typography>
                  </Box>
                  <Box sx={{ textAlign: 'center' }}>
                    <Typography variant="h4">{(session?.energyConsumed || 0).toFixed(1)} kWh</Typography>
                    <Typography variant="caption" color="text.secondary">Delivered</Typography>
                  </Box>
                  <Box sx={{ textAlign: 'center' }}>
                    <Typography variant="h4">₹{(session?.estimatedCost || 0).toFixed(0)}</Typography>
                    <Typography variant="caption" color="text.secondary">Est. Cost</Typography>
                  </Box>
                </Box>

                <Button
                  variant="contained"
                  color="error"
                  startIcon={<StopCircleOutlined />}
                  onClick={() => setConfirmStopOpen(true)}
                >
                  Stop Session
                </Button>
              </Box>
            ) : station.status === 'maintenance' ? (
              <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', height: '100%', gap: 2, py: 4 }}>
                <Box
                  sx={{
                    width: 80,
                    height: 80,
                    borderRadius: '50%',
                    bgcolor: alpha(theme.palette.warning.main, 0.1),
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                  }}
                >
                  <BuildOutlined sx={{ fontSize: 36, color: theme.palette.warning.main }} />
                </Box>
                <Typography variant="h4">Station Under Maintenance</Typography>
                <Typography variant="body2" color="text.secondary" textAlign="center">
                  This charging bay is currently undergoing scheduled technical maintenance and is temporarily unavailable.
                </Typography>
              </Box>
            ) : station.status === 'offline' ? (
              <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', height: '100%', gap: 2, py: 4 }}>
                <Box
                  sx={{
                    width: 80,
                    height: 80,
                    borderRadius: '50%',
                    bgcolor: alpha(theme.palette.text.disabled, 0.1),
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                  }}
                >
                  <PowerOffOutlined sx={{ fontSize: 36, color: theme.palette.text.secondary }} />
                </Box>
                <Typography variant="h4">Station Offline</Typography>
                <Typography variant="body2" color="text.secondary" textAlign="center">
                  This station is currently offline and not communicating with the charging network.
                </Typography>
              </Box>
            ) : (
              <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', height: '100%', gap: 2, py: 4 }}>
                <Box
                  sx={{
                    width: 80,
                    height: 80,
                    borderRadius: '50%',
                    bgcolor: alpha(theme.palette.success.main, 0.1),
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                  }}
                >
                  <Typography variant="h3" sx={{ color: theme.palette.success.main }}>✓</Typography>
                </Box>
                <Typography variant="h4">Station Available</Typography>
                <Typography variant="body2" color="text.secondary">
                  Ready to accept a charging session.
                </Typography>
                <Button
                  variant="contained"
                  startIcon={<ElectricBoltOutlined />}
                  onClick={() => navigate('/stations')}
                  sx={{ mt: 1 }}
                >
                  Request Charging Here
                </Button>
              </Box>
            )}
          </Card>
        </Grid>

        {/* Station Session History Table */}
        <Grid size={{ xs: 12 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, mt: 1 }}>
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 2 }}>
              <HistoryOutlined sx={{ color: theme.palette.primary.main }} />
              <Typography variant="h5">Recent Charging Sessions at this Bay</Typography>
            </Box>

            {stationSessions.length === 0 ? (
              <Typography variant="body2" color="text.secondary" sx={{ py: 2, textAlign: 'center' }}>
                No recorded charging sessions at {station.name} yet.
              </Typography>
            ) : (
              <TableContainer>
                <Table size="small">
                  <TableHead>
                    <TableRow>
                      <TableCell>Session ID</TableCell>
                      <TableCell>Vehicle</TableCell>
                      <TableCell>SoC Progress</TableCell>
                      <TableCell>Energy</TableCell>
                      <TableCell>Cost</TableCell>
                      <TableCell>Duration</TableCell>
                      <TableCell>Status</TableCell>
                    </TableRow>
                  </TableHead>
                  <TableBody>
                    {stationSessions.map((s) => (
                      <TableRow key={s.id} hover>
                        <TableCell sx={{ fontWeight: 600 }}>{s.id}</TableCell>
                        <TableCell>{s.evId}</TableCell>
                        <TableCell>
                          {s.batteryStart}% → {s.batteryEnd ?? s.targetCharge}%
                        </TableCell>
                        <TableCell>{(s.energyConsumed || 0).toFixed(1)} kWh</TableCell>
                        <TableCell>₹{(s.estimatedCost || 0).toFixed(0)}</TableCell>
                        <TableCell>{s.duration ? `${s.duration}m` : '15m'}</TableCell>
                        <TableCell>
                          <StatusChip status={s.status} />
                        </TableCell>
                      </TableRow>
                    ))}
                  </TableBody>
                </Table>
              </TableContainer>
            )}
          </Card>
        </Grid>
      </Grid>

      {/* Confirmation Dialog for Stopping Session */}
      <Dialog open={confirmStopOpen} onClose={() => setConfirmStopOpen(false)}>
        <DialogTitle sx={{ fontWeight: 700 }}>Complete Charging Session?</DialogTitle>
        <DialogContent>
          <Typography variant="body2">
            Are you sure you want to stop and complete this active charging session? Billing tariff and transaction receipt will be finalized.
          </Typography>
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2 }}>
          <Button onClick={() => setConfirmStopOpen(false)}>Cancel</Button>
          <Button variant="contained" color="error" onClick={handleConfirmStop}>
            Stop & Complete
          </Button>
        </DialogActions>
      </Dialog>

      {/* Manage Status Dialog */}
      <Dialog open={statusDialogOpen} onClose={() => !statusLoading && setStatusDialogOpen(false)} maxWidth="xs" fullWidth>
        <DialogTitle sx={{ fontWeight: 700 }}>Update Station Status</DialogTitle>
        <DialogContent sx={{ pt: 2, display: 'flex', flexDirection: 'column', gap: 2 }}>
          {statusError && <Alert severity="error">{statusError}</Alert>}

          <Typography variant="body2" color="text.secondary">
            Change operational status for <strong>{station.name}</strong> ({station.id}).
          </Typography>

          <TextField
            select
            label="Operational Status"
            value={targetStatus}
            onChange={e => setTargetStatus(e.target.value as StationStatus)}
            fullWidth
          >
            <MenuItem value="available">Available (Online & accepting EVs)</MenuItem>
            <MenuItem value="maintenance">Maintenance (Under repair / servicing)</MenuItem>
            <MenuItem value="offline">Offline (Decommissioned / network cut)</MenuItem>
          </TextField>

          <Alert severity="info" sx={{ fontSize: '0.8rem' }}>
            Note: Active charging stations cannot be manually set to available until the charging session is completed.
          </Alert>
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2.5 }}>
          <Button onClick={() => setStatusDialogOpen(false)} disabled={statusLoading}>
            Cancel
          </Button>
          <Button
            variant="contained"
            color="primary"
            onClick={handleUpdateStatus}
            disabled={statusLoading}
          >
            {statusLoading ? 'Updating...' : 'Update Status'}
          </Button>
        </DialogActions>
      </Dialog>

      <style>{`
        @keyframes pulse {
          0%, 100% { opacity: 1; }
          50% { opacity: 0.5; }
        }
      `}</style>
    </Box>
  );
}
