import { useState, useMemo } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import Grid from '@mui/material/Grid2';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import LinearProgress from '@mui/material/LinearProgress';
import Chip from '@mui/material/Chip';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import TextField from '@mui/material/TextField';
import MenuItem from '@mui/material/MenuItem';
import Slider from '@mui/material/Slider';
import Alert from '@mui/material/Alert';
import { useTheme, alpha } from '@mui/material/styles';

import ArrowBackOutlined from '@mui/icons-material/ArrowBackOutlined';
import StopCircleOutlined from '@mui/icons-material/StopCircleOutlined';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';
import BuildOutlined from '@mui/icons-material/BuildOutlined';
import PowerOffOutlined from '@mui/icons-material/PowerOffOutlined';
import CheckCircleOutlined from '@mui/icons-material/CheckCircleOutlined';
import HistoryOutlined from '@mui/icons-material/HistoryOutlined';
import TuneOutlined from '@mui/icons-material/TuneOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import EmptyState from '@/components/common/EmptyState';
import ConfirmDialog from '@/components/common/ConfirmDialog';
import { useApp } from '@/context/AppContext';
import type { StationStatus } from '@/types';

export default function StationDetails() {
  const { id } = useParams<{ id: string }>();
  const navigate = useNavigate();
  const theme = useTheme();
  const {
    stations,
    evs,
    sessions,
    completeSession,
    addChargingRequest,
    updateStation,
    showSnackbar,
  } = useApp();

  const station = stations.find(s => s.id === id);

  // Dialog states
  const [chargeDialogOpen, setChargeDialogOpen] = useState(false);
  const [selectedEvId, setSelectedEvId] = useState('');
  const [targetCharge, setTargetCharge] = useState<number>(85);

  const [confirmCompleteOpen, setConfirmCompleteOpen] = useState(false);
  const [statusDialogOpen, setStatusDialogOpen] = useState(false);
  const [targetStatus, setTargetStatus] = useState<StationStatus>('available');

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

  // Active session on this station
  const activeSession = sessions.find(s => s.stationId === station.id && s.status === 'active');
  const assignedEv = evs.find(e => e.id === (activeSession?.evId || station.currentEV));

  // Historical sessions for this station
  const stationHistory = sessions.filter(s => s.stationId === station.id);

  const getDuration = (start: string) => {
    if (!start) return '0m';
    const diff = Math.max(0, Date.now() - new Date(start).getTime());
    const h = Math.floor(diff / 3600000);
    const m = Math.round((diff % 3600000) / 60000);
    return h > 0 ? `${h}h ${m}m` : `${Math.max(1, m)}m`;
  };

  const progress = activeSession
    ? (activeSession.batteryEnd ?? activeSession.batteryStart)
    : (assignedEv?.currentCharge ?? 0);

  const energyConsumed = activeSession?.energyConsumed ?? (
    assignedEv && activeSession
      ? Math.max(0, Math.round(((progress - activeSession.batteryStart) / 100) * assignedEv.batteryCapacity * 10) / 10)
      : 0
  );

  // Handle Complete Session
  const handleCompleteActiveSession = async () => {
    if (!activeSession) return;
    await completeSession(activeSession.id);
    setConfirmCompleteOpen(false);
  };

  // Handle Start Charging from Station Details
  const handleStartCharging = async () => {
    if (!selectedEvId) return;
    const ev = evs.find(e => e.id === selectedEvId);
    await addChargingRequest({
      evId: selectedEvId,
      evModel: ev?.model || 'Electric Vehicle',
      owner: ev?.owner || 'EV Owner',
      currentCharge: ev?.currentCharge ?? 20,
      priority: 'normal',
    });
    setChargeDialogOpen(false);
    setSelectedEvId('');
  };

  // Handle Update Status
  const handleUpdateStatus = async () => {
    if (station.status === 'charging' && targetStatus !== 'charging') {
      showSnackbar('Cannot change status while station is actively charging an EV. End session first.', 'error');
      setStatusDialogOpen(false);
      return;
    }
    await updateStation(station.id, { status: targetStatus });
    setStatusDialogOpen(false);
  };

  const selectedEv = evs.find(e => e.id === selectedEvId);
  const estEnergy = selectedEv
    ? Math.max(0, ((targetCharge - selectedEv.currentCharge) / 100) * selectedEv.batteryCapacity)
    : 0;
  const estCost = estEnergy > 0 ? (estEnergy <= 30 ? estEnergy * 18 : 30 * 18 + (estEnergy - 30) * 16.2) : 100;

  return (
    <Box>
      <PageHeader
        title={`${station.name} (${station.id})`}
        subtitle={`Location: ${station.location} • Power: ${station.power} kW • Hardware Management`}
        action={
          <Box sx={{ display: 'flex', gap: 1.5 }}>
            <Button
              startIcon={<ArrowBackOutlined />}
              onClick={() => navigate('/stations')}
              variant="outlined"
            >
              Back to Stations
            </Button>
            <Button
              startIcon={<TuneOutlined />}
              onClick={() => { setTargetStatus(station.status); setStatusDialogOpen(true); }}
              variant="outlined"
            >
              Change Status
            </Button>
          </Box>
        }
      />

      <Grid container spacing={2.5}>
        {/* ─── LEFT: STATION SPECS & CONTROL ─── */}
        <Grid size={{ xs: 12, md: 5 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, mb: 2.5 }}>
            <Typography variant="h5" sx={{ mb: 2.5, fontWeight: 700 }}>
              Hardware Specifications
            </Typography>
            <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
              {[
                ['Station Identifier', station.id],
                ['Station Name', station.name],
                ['Rated Power Output', `${station.power} kW`],
                ['Connector Standard', station.connectorType],
                ['Physical Location', station.location],
              ].map(([label, value]) => (
                <Box key={label} sx={{ display: 'flex', justifyContent: 'space-between' }}>
                  <Typography variant="body2" color="text.secondary">{label}</Typography>
                  <Typography variant="body2" fontWeight={600}>{value}</Typography>
                </Box>
              ))}
              <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                <Typography variant="body2" color="text.secondary">Operational Status</Typography>
                <StatusChip status={station.status} />
              </Box>
              <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                <Typography variant="body2" color="text.secondary">Connected Vehicle</Typography>
                <Typography variant="body2" fontWeight={600} color={station.currentEV ? 'primary.main' : 'text.secondary'}>
                  {station.currentEV ? `${station.currentEV} (${assignedEv?.model || 'EV'})` : 'None (Bay Empty)'}
                </Typography>
              </Box>
            </Box>
          </Card>

          {/* Quick Action Card */}
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
            <Typography variant="h6" fontWeight={700} sx={{ mb: 1.5 }}>
              Bay Controls
            </Typography>
            {station.status === 'available' ? (
              <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1.5 }}>
                <Typography variant="body2" color="text.secondary">
                  Bay is online and ready for charging. Click below to manually allocate a registered electric vehicle.
                </Typography>
                <Button
                  variant="contained"
                  startIcon={<ElectricBoltOutlined />}
                  onClick={() => { setSelectedEvId(''); setChargeDialogOpen(true); }}
                  fullWidth
                >
                  Connect & Charge EV
                </Button>
              </Box>
            ) : station.status === 'charging' && activeSession ? (
              <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1.5 }}>
                <Typography variant="body2" color="text.secondary">
                  Active session in progress for vehicle {station.currentEV}. Once charging is finished, click Complete Session to generate the invoice and release this bay.
                </Typography>
                <Button
                  variant="contained"
                  color="error"
                  startIcon={<StopCircleOutlined />}
                  onClick={() => setConfirmCompleteOpen(true)}
                  fullWidth
                >
                  Complete Session & Bill
                </Button>
              </Box>
            ) : (
              <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1.5 }}>
                <Alert severity="warning">
                  Station is currently in <strong>{station.status}</strong> state and cannot service vehicles.
                </Alert>
                <Button
                  variant="contained"
                  color="success"
                  startIcon={<CheckCircleOutlined />}
                  onClick={() => updateStation(station.id, { status: 'available' })}
                  fullWidth
                >
                  Mark Station Available
                </Button>
              </Box>
            )}
          </Card>
        </Grid>

        {/* ─── RIGHT: LIVE SESSION & TELEMETRY ─── */}
        <Grid size={{ xs: 12, md: 7 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, minHeight: 380, display: 'flex', flexDirection: 'column', justifyContent: 'center' }}>
            {station.status === 'charging' && activeSession ? (
              <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 3 }}>
                <Box
                  sx={{
                    display: 'flex',
                    alignItems: 'center',
                    gap: 1.5,
                    color: theme.palette.info.main,
                  }}
                >
                  <ElectricBoltOutlined sx={{ fontSize: 32, animation: 'pulse 1.5s infinite' }} />
                  <Typography variant="h4" color="info.main" fontWeight={700}>
                    Charging Active
                  </Typography>
                </Box>

                <Box sx={{ width: '100%', textAlign: 'center' }}>
                  <Typography variant="h1" sx={{ fontSize: '3.5rem', fontWeight: 800, color: theme.palette.info.main }}>
                    {Math.round(progress)}%
                  </Typography>
                  <LinearProgress
                    variant="determinate"
                    value={progress}
                    sx={{
                      mt: 2,
                      height: 14,
                      borderRadius: 7,
                      bgcolor: alpha(theme.palette.info.main, 0.15),
                      '& .MuiLinearProgress-bar': { borderRadius: 7, bgcolor: theme.palette.info.main },
                    }}
                  />
                  <Typography variant="body2" color="text.secondary" sx={{ mt: 1 }}>
                    Started at {activeSession.batteryStart}% • Target: {activeSession.targetCharge}%
                  </Typography>
                </Box>

                <Grid container spacing={2} sx={{ width: '100%', mt: 1 }}>
                  <Grid size={{ xs: 6, sm: 3 }}>
                    <Box sx={{ p: 2, bgcolor: alpha(theme.palette.primary.main, 0.05), borderRadius: 2, textAlign: 'center' }}>
                      <Typography variant="caption" color="text.secondary">EV / Model</Typography>
                      <Typography variant="body1" fontWeight={700}>{activeSession.evId}</Typography>
                      <Typography variant="caption" noWrap>{activeSession.evModel}</Typography>
                    </Box>
                  </Grid>
                  <Grid size={{ xs: 6, sm: 3 }}>
                    <Box sx={{ p: 2, bgcolor: alpha(theme.palette.info.main, 0.05), borderRadius: 2, textAlign: 'center' }}>
                      <Typography variant="caption" color="text.secondary">Duration</Typography>
                      <Typography variant="body1" fontWeight={700}>{getDuration(activeSession.startTime)}</Typography>
                      <Typography variant="caption">Elapsed</Typography>
                    </Box>
                  </Grid>
                  <Grid size={{ xs: 6, sm: 3 }}>
                    <Box sx={{ p: 2, bgcolor: alpha(theme.palette.success.main, 0.05), borderRadius: 2, textAlign: 'center' }}>
                      <Typography variant="caption" color="text.secondary">Energy Dispensed</Typography>
                      <Typography variant="body1" fontWeight={700}>{energyConsumed} kWh</Typography>
                      <Typography variant="caption">Delivered</Typography>
                    </Box>
                  </Grid>
                  <Grid size={{ xs: 6, sm: 3 }}>
                    <Box sx={{ p: 2, bgcolor: alpha(theme.palette.warning.main, 0.05), borderRadius: 2, textAlign: 'center' }}>
                      <Typography variant="caption" color="text.secondary">Est. Tariff</Typography>
                      <Typography variant="body1" fontWeight={700}>₹{activeSession.estimatedCost.toFixed(0)}</Typography>
                      <Typography variant="caption">Tariff Rule</Typography>
                    </Box>
                  </Grid>
                </Grid>

                <Button
                  variant="contained"
                  color="error"
                  startIcon={<StopCircleOutlined />}
                  onClick={() => setConfirmCompleteOpen(true)}
                  sx={{ mt: 1, px: 4 }}
                >
                  End Session & Release Bay
                </Button>
              </Box>
            ) : station.status === 'available' ? (
              <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', gap: 2, py: 4 }}>
                <Box
                  sx={{
                    width: 72,
                    height: 72,
                    borderRadius: '50%',
                    bgcolor: alpha(theme.palette.success.main, 0.12),
                    color: theme.palette.success.main,
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                  }}
                >
                  <CheckCircleOutlined sx={{ fontSize: 40 }} />
                </Box>
                <Typography variant="h4" fontWeight={700}>Station Available</Typography>
                <Typography variant="body2" color="text.secondary" sx={{ textAlign: 'center', maxWidth: 400 }}>
                  This charger ({station.power} kW) is currently idle and ready to supply electricity.
                </Typography>
                <Button
                  variant="contained"
                  startIcon={<ElectricBoltOutlined />}
                  onClick={() => { setSelectedEvId(''); setChargeDialogOpen(true); }}
                  sx={{ mt: 1 }}
                >
                  Start Charging Session
                </Button>
              </Box>
            ) : (
              <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', gap: 2, py: 4 }}>
                <Box
                  sx={{
                    width: 72,
                    height: 72,
                    borderRadius: '50%',
                    bgcolor: station.status === 'maintenance'
                      ? alpha(theme.palette.warning.main, 0.12)
                      : alpha(theme.palette.error.main, 0.12),
                    color: station.status === 'maintenance'
                      ? theme.palette.warning.main
                      : theme.palette.error.main,
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                  }}
                >
                  {station.status === 'maintenance' ? <BuildOutlined sx={{ fontSize: 36 }} /> : <PowerOffOutlined sx={{ fontSize: 36 }} />}
                </Box>
                <Typography variant="h4" fontWeight={700} sx={{ textTransform: 'capitalize' }}>
                  Station {station.status}
                </Typography>
                <Typography variant="body2" color="text.secondary" sx={{ textAlign: 'center', maxWidth: 400 }}>
                  This bay is currently {station.status}. Vehicles cannot be dispatched or connected until the station is marked available.
                </Typography>
              </Box>
            )}
          </Card>
        </Grid>

        {/* ─── FULL-WIDTH: SESSION HISTORY TABLE ─── */}
        <Grid size={{ xs: 12 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1.5, mb: 2.5 }}>
              <HistoryOutlined sx={{ color: theme.palette.primary.main }} />
              <Typography variant="h5" fontWeight={700}>
                Charging History for {station.id}
              </Typography>
            </Box>

            {stationHistory.length === 0 ? (
              <Typography variant="body2" color="text.secondary" sx={{ py: 3, textAlign: 'center' }}>
                No prior charging sessions recorded for this bay yet.
              </Typography>
            ) : (
              <TableContainer>
                <Table size="small">
                  <TableHead>
                    <TableRow sx={{ '& th': { fontWeight: 700 } }}>
                      <TableCell>Session ID</TableCell>
                      <TableCell>Vehicle</TableCell>
                      <TableCell>Model</TableCell>
                      <TableCell>Start Time</TableCell>
                      <TableCell>End Time</TableCell>
                      <TableCell align="right">Start SoC</TableCell>
                      <TableCell align="right">End SoC</TableCell>
                      <TableCell align="right">Energy (kWh)</TableCell>
                      <TableCell align="right">Amount (₹)</TableCell>
                      <TableCell align="center">Status</TableCell>
                    </TableRow>
                  </TableHead>
                  <TableBody>
                    {stationHistory.map(ses => (
                      <TableRow key={ses.id} hover>
                        <TableCell sx={{ fontFamily: 'monospace', fontWeight: 600 }}>{ses.id}</TableCell>
                        <TableCell sx={{ fontWeight: 600 }}>{ses.evId}</TableCell>
                        <TableCell>{ses.evModel}</TableCell>
                        <TableCell>{new Date(ses.startTime).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}</TableCell>
                        <TableCell>{ses.endTime ? new Date(ses.endTime).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }) : '—'}</TableCell>
                        <TableCell align="right">{ses.batteryStart}%</TableCell>
                        <TableCell align="right">{ses.batteryEnd ?? ses.batteryStart}%</TableCell>
                        <TableCell align="right">{ses.energyConsumed ? `${ses.energyConsumed} kWh` : '—'}</TableCell>
                        <TableCell align="right" sx={{ fontWeight: 700 }}>₹{ses.estimatedCost.toFixed(2)}</TableCell>
                        <TableCell align="center">
                          <Chip
                            label={ses.status}
                            size="small"
                            color={ses.status === 'active' ? 'info' : 'success'}
                            sx={{ textTransform: 'capitalize', fontWeight: 600 }}
                          />
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

      {/* ─── START CHARGING DIALOG ─── */}
      <Dialog
        open={chargeDialogOpen}
        onClose={() => setChargeDialogOpen(false)}
        maxWidth="xs"
        fullWidth
      >
        <DialogTitle sx={{ fontWeight: 700 }}>Initiate Charging: {station.name}</DialogTitle>
        <DialogContent sx={{ pt: 2, display: 'flex', flexDirection: 'column', gap: 2.5 }}>
          <Typography variant="body2" color="text.secondary">
            Select an available electric vehicle to dispatch to Bay {station.id}.
          </Typography>

          <TextField
            select
            label="Select Registered Vehicle"
            value={selectedEvId}
            onChange={e => setSelectedEvId(e.target.value)}
            fullWidth
            required
          >
            {evs
              .filter(ev => ev.status === 'available' || ev.status === 'waiting')
              .map(ev => (
                <MenuItem key={ev.id} value={ev.id}>
                  {ev.id} — {ev.model} ({ev.currentCharge}%)
                </MenuItem>
              ))}
          </TextField>

          {selectedEv && (
            <Box sx={{ p: 2, bgcolor: alpha(theme.palette.primary.main, 0.05), borderRadius: 2 }}>
              <Typography variant="subtitle2" fontWeight={700} gutterBottom>
                Session Estimations:
              </Typography>
              <Box sx={{ display: 'flex', justifyContent: 'space-between', mb: 0.5 }}>
                <Typography variant="caption" color="text.secondary">Current Charge:</Typography>
                <Typography variant="caption" fontWeight={600}>{selectedEv.currentCharge}%</Typography>
              </Box>
              <Box sx={{ display: 'flex', justifyContent: 'space-between', mb: 0.5 }}>
                <Typography variant="caption" color="text.secondary">Est. Energy:</Typography>
                <Typography variant="caption" fontWeight={600}>{estEnergy.toFixed(1)} kWh</Typography>
              </Box>
              <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                <Typography variant="caption" color="text.secondary">Est. Cost:</Typography>
                <Typography variant="caption" fontWeight={700} color="primary.main">₹{estCost.toFixed(2)}</Typography>
              </Box>
            </Box>
          )}

          <Box>
            <Typography variant="body2" fontWeight={600} gutterBottom>
              Target State of Charge (SoC): {targetCharge}%
            </Typography>
            <Slider
              value={targetCharge}
              min={Math.max(50, selectedEv?.currentCharge ? Math.ceil(selectedEv.currentCharge) : 50)}
              max={100}
              step={5}
              marks={[
                { value: 50, label: '50%' },
                { value: 80, label: '80%' },
                { value: 90, label: '90%' },
                { value: 100, label: '100%' },
              ]}
              valueLabelDisplay="auto"
              onChange={(_, val) => setTargetCharge(val as number)}
            />
          </Box>
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2.5 }}>
          <Button onClick={() => setChargeDialogOpen(false)}>Cancel</Button>
          <Button
            variant="contained"
            disabled={!selectedEvId}
            onClick={handleStartCharging}
            startIcon={<ElectricBoltOutlined />}
          >
            Start Charging
          </Button>
        </DialogActions>
      </Dialog>

      {/* ─── STATUS UPDATE DIALOG ─── */}
      <Dialog
        open={statusDialogOpen}
        onClose={() => setStatusDialogOpen(false)}
        maxWidth="xs"
        fullWidth
      >
        <DialogTitle sx={{ fontWeight: 700 }}>
          Manage Station Status: {station.id}
        </DialogTitle>
        <DialogContent sx={{ pt: 2, display: 'flex', flexDirection: 'column', gap: 2 }}>
          {station.status === 'charging' && (
            <Alert severity="warning">
              This station is currently actively charging an EV. You cannot take it offline or mark it available until the session is completed.
            </Alert>
          )}

          <TextField
            select
            label="Operational Status"
            value={targetStatus}
            onChange={e => setTargetStatus(e.target.value as StationStatus)}
            fullWidth
            disabled={station.status === 'charging'}
          >
            <MenuItem value="available">Available (Ready for EVs)</MenuItem>
            <MenuItem value="maintenance">Maintenance (Servicing)</MenuItem>
            <MenuItem value="offline">Offline (Powered Down)</MenuItem>
          </TextField>
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2.5 }}>
          <Button onClick={() => setStatusDialogOpen(false)}>Cancel</Button>
          <Button
            variant="contained"
            onClick={handleUpdateStatus}
            disabled={station.status === 'charging'}
          >
            Update State
          </Button>
        </DialogActions>
      </Dialog>

      {/* ─── CONFIRM COMPLETE DIALOG ─── */}
      <ConfirmDialog
        open={confirmCompleteOpen}
        title="Complete Charging Session"
        message={`Are you sure you want to end active charging on ${station.name}? The final energy delivered will be calculated, an official transaction receipt will be created, and waiting vehicles in the FIFO queue will be serviced.`}
        confirmLabel="Complete & Invoice"
        onConfirm={handleCompleteActiveSession}
        onCancel={() => setConfirmCompleteOpen(false)}
      />

      <style>{`
        @keyframes pulse {
          0%, 100% { opacity: 1; }
          50% { opacity: 0.5; }
        }
      `}</style>
    </Box>
  );
}
