import { useState, useMemo } from 'react';
import { useNavigate } from 'react-router-dom';
import Grid from '@mui/material/Grid2';
import Card from '@mui/material/Card';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Chip from '@mui/material/Chip';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import TextField from '@mui/material/TextField';
import MenuItem from '@mui/material/MenuItem';
import Slider from '@mui/material/Slider';
import LinearProgress from '@mui/material/LinearProgress';
import Skeleton from '@mui/material/Skeleton';
import Tabs from '@mui/material/Tabs';
import Tab from '@mui/material/Tab';
import InputAdornment from '@mui/material/InputAdornment';
import IconButton from '@mui/material/IconButton';
import Tooltip from '@mui/material/Tooltip';
import Alert from '@mui/material/Alert';
import { useTheme, alpha } from '@mui/material/styles';

import EvStationOutlined from '@mui/icons-material/EvStationOutlined';
import VisibilityOutlined from '@mui/icons-material/VisibilityOutlined';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';
import PowerOutlined from '@mui/icons-material/PowerOutlined';
import BuildOutlined from '@mui/icons-material/BuildOutlined';
import PowerOffOutlined from '@mui/icons-material/PowerOffOutlined';
import CheckCircleOutlined from '@mui/icons-material/CheckCircleOutlined';
import StopCircleOutlined from '@mui/icons-material/StopCircleOutlined';
import AddOutlined from '@mui/icons-material/AddOutlined';
import RefreshOutlined from '@mui/icons-material/RefreshOutlined';
import SearchOutlined from '@mui/icons-material/SearchOutlined';
import TuneOutlined from '@mui/icons-material/TuneOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import StatCard from '@/components/common/StatCard';
import EmptyState from '@/components/common/EmptyState';
import ConfirmDialog from '@/components/common/ConfirmDialog';
import { useApp } from '@/context/AppContext';
import type { Station, StationStatus, ConnectorType } from '@/types';

export default function ChargingStations() {
  const theme = useTheme();
  const navigate = useNavigate();
  const {
    stations,
    evs,
    sessions,
    addChargingRequest,
    updateStation,
    addStation,
    completeSession,
    refreshBackendData,
    showSnackbar,
  } = useApp();

  // Search & Filter state
  const [searchQuery, setSearchQuery] = useState('');
  const [statusFilter, setStatusFilter] = useState<'all' | StationStatus>('all');
  const [isRefreshing, setIsRefreshing] = useState(false);

  // Dialog states
  const [chargeDialogOpen, setChargeDialogOpen] = useState(false);
  const [selectedStation, setSelectedStation] = useState<Station | null>(null);
  const [selectedEvId, setSelectedEvId] = useState('');
  const [targetCharge, setTargetCharge] = useState<number>(85);

  const [addDialogOpen, setAddDialogOpen] = useState(false);
  const [newStationData, setNewStationData] = useState({
    id: '',
    name: '',
    power: 50,
    connectorType: 'CCS2' as ConnectorType,
    location: '',
  });

  const [statusDialogOpen, setStatusDialogOpen] = useState(false);
  const [stationToUpdate, setStationToUpdate] = useState<Station | null>(null);
  const [targetStatus, setTargetStatus] = useState<StationStatus>('available');

  const [confirmCompleteOpen, setConfirmCompleteOpen] = useState(false);
  const [sessionToComplete, setSessionToComplete] = useState<string | null>(null);

  // KPI Calculations
  const stats = useMemo(() => {
    const total = stations.length;
    const available = stations.filter(s => s.status === 'available').length;
    const charging = stations.filter(s => s.status === 'charging').length;
    const maintenance = stations.filter(s => s.status === 'maintenance').length;
    const offline = stations.filter(s => s.status === 'offline').length;
    const activeSessions = sessions.filter(s => s.status === 'active').length;
    return { total, available, charging, maintenance, offline, activeSessions };
  }, [stations, sessions]);

  // Filtered station cards
  const filteredStations = useMemo(() => {
    return stations.filter(s => {
      const matchesSearch =
        s.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
        s.id.toLowerCase().includes(searchQuery.toLowerCase()) ||
        s.location.toLowerCase().includes(searchQuery.toLowerCase()) ||
        (s.currentEV && s.currentEV.toLowerCase().includes(searchQuery.toLowerCase()));

      const matchesStatus = statusFilter === 'all' || s.status === statusFilter;
      return matchesSearch && matchesStatus;
    });
  }, [stations, searchQuery, statusFilter]);

  const handleRefresh = async () => {
    setIsRefreshing(true);
    await refreshBackendData();
    setIsRefreshing(false);
    showSnackbar('Refreshed charging stations from server', 'info');
  };

  // Start Charging Action
  const handleOpenChargeDialog = (station: Station) => {
    setSelectedStation(station);
    setSelectedEvId('');
    setTargetCharge(85);
    setChargeDialogOpen(true);
  };

  const handleStartCharging = async () => {
    if (!selectedEvId || !selectedStation) return;
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
    setSelectedStation(null);
  };

  // Complete Session Action
  const handleOpenCompleteConfirm = (sessionId: string) => {
    setSessionToComplete(sessionId);
    setConfirmCompleteOpen(true);
  };

  const handleConfirmComplete = async () => {
    if (!sessionToComplete) return;
    await completeSession(sessionToComplete);
    setConfirmCompleteOpen(false);
    setSessionToComplete(null);
  };

  // Status Change Action
  const handleOpenStatusDialog = (station: Station) => {
    setStationToUpdate(station);
    setTargetStatus(station.status);
    setStatusDialogOpen(true);
  };

  const handleUpdateStatus = async () => {
    if (!stationToUpdate) return;
    if (stationToUpdate.status === 'charging' && targetStatus !== 'charging') {
      showSnackbar('Cannot manually change status while station is charging. Complete the session first.', 'error');
      setStatusDialogOpen(false);
      return;
    }
    await updateStation(stationToUpdate.id, { status: targetStatus });
    setStatusDialogOpen(false);
    setStationToUpdate(null);
  };

  // Add New Station Action
  const handleCreateStation = async () => {
    if (!newStationData.id.trim() || !newStationData.name.trim()) {
      showSnackbar('Station ID and Name are required.', 'error');
      return;
    }
    const success = await addStation({
      id: newStationData.id.trim().toUpperCase(),
      name: newStationData.name.trim(),
      power: Number(newStationData.power) || 50,
      connectorType: newStationData.connectorType,
      location: newStationData.location.trim() || 'Bay 1',
      status: 'available',
    });
    if (success) {
      setAddDialogOpen(false);
      setNewStationData({
        id: '',
        name: '',
        power: 50,
        connectorType: 'CCS2',
        location: '',
      });
    }
  };

  // Selected EV for Charge Dialog
  const selectedEv = evs.find(e => e.id === selectedEvId);
  const estEnergy = selectedEv
    ? Math.max(0, ((targetCharge - selectedEv.currentCharge) / 100) * selectedEv.batteryCapacity)
    : 0;
  const estCost = estEnergy > 0 ? (estEnergy <= 30 ? estEnergy * 18 : 30 * 18 + (estEnergy - 30) * 16.2) : 100;

  return (
    <Box>
      <PageHeader
        title="Charging Stations & Bays"
        subtitle="Real-time monitoring, automated FIFO dispatch, and hardware state control"
        action={
          <Box sx={{ display: 'flex', gap: 1.5 }}>
            <Button
              variant="outlined"
              startIcon={<RefreshOutlined />}
              onClick={handleRefresh}
              disabled={isRefreshing}
            >
              Refresh
            </Button>
            <Button
              variant="contained"
              startIcon={<AddOutlined />}
              onClick={() => setAddDialogOpen(true)}
            >
              Add Station
            </Button>
          </Box>
        }
      />

      {/* ─── KPI METRICS ROW ─── */}
      <Grid container spacing={2.5} sx={{ mb: 3 }}>
        <Grid size={{ xs: 6, sm: 4, md: 2 }}>
          <StatCard
            title="Total Bays"
            value={stats.total}
            icon={EvStationOutlined}
            color={theme.palette.primary.main}
          />
        </Grid>
        <Grid size={{ xs: 6, sm: 4, md: 2 }}>
          <StatCard
            title="Available"
            value={stats.available}
            icon={CheckCircleOutlined}
            color={theme.palette.success.main}
          />
        </Grid>
        <Grid size={{ xs: 6, sm: 4, md: 2 }}>
          <StatCard
            title="Charging"
            value={stats.charging}
            icon={ElectricBoltOutlined}
            color={theme.palette.info.main}
          />
        </Grid>
        <Grid size={{ xs: 6, sm: 4, md: 2 }}>
          <StatCard
            title="Maintenance"
            value={stats.maintenance}
            icon={BuildOutlined}
            color={theme.palette.warning.main}
          />
        </Grid>
        <Grid size={{ xs: 6, sm: 4, md: 2 }}>
          <StatCard
            title="Offline"
            value={stats.offline}
            icon={PowerOffOutlined}
            color={theme.palette.error.main}
          />
        </Grid>
        <Grid size={{ xs: 6, sm: 4, md: 2 }}>
          <StatCard
            title="Active Sessions"
            value={stats.activeSessions}
            icon={PowerOutlined}
            color={theme.palette.secondary.main}
          />
        </Grid>
      </Grid>

      {/* ─── FILTER & SEARCH BAR ─── */}
      <Card sx={{ p: 2, mb: 3, border: `1px solid ${theme.palette.divider}` }}>
        <Box sx={{ display: 'flex', flexWrap: 'wrap', gap: 2, alignItems: 'center', justifyContent: 'space-between' }}>
          <Tabs
            value={statusFilter}
            onChange={(_, val) => setStatusFilter(val)}
            variant="scrollable"
            scrollButtons="auto"
            sx={{
              minHeight: 40,
              '& .MuiTab-root': { minHeight: 40, py: 1, textTransform: 'capitalize', fontWeight: 600 },
            }}
          >
            <Tab label={`All (${stats.total})`} value="all" />
            <Tab label={`Available (${stats.available})`} value="available" />
            <Tab label={`Charging (${stats.charging})`} value="charging" />
            <Tab label={`Maintenance (${stats.maintenance})`} value="maintenance" />
            <Tab label={`Offline (${stats.offline})`} value="offline" />
          </Tabs>

          <TextField
            size="small"
            placeholder="Search station ID, name, location..."
            value={searchQuery}
            onChange={e => setSearchQuery(e.target.value)}
            sx={{ width: { xs: '100%', sm: 300 } }}
            slotProps={{
              input: {
                startAdornment: (
                  <InputAdornment position="start">
                    <SearchOutlined fontSize="small" sx={{ color: 'text.secondary' }} />
                  </InputAdornment>
                ),
              },
            }}
          />
        </Box>
      </Card>

      {/* ─── STATION CARDS GRID ─── */}
      {filteredStations.length === 0 ? (
        <EmptyState
          title="No Stations Found"
          description={searchQuery ? `No charging bays match "${searchQuery}".` : 'No stations in this category.'}
          actionLabel="Clear Filter"
          onAction={() => { setSearchQuery(''); setStatusFilter('all'); }}
        />
      ) : (
        <Grid container spacing={2.5}>
          {filteredStations.map(station => {
            const activeSession = sessions.find(s => s.stationId === station.id && s.status === 'active');
            const assignedEv = evs.find(e => e.id === station.currentEV);
            const progress = activeSession
              ? (activeSession.batteryEnd ?? activeSession.batteryStart)
              : (assignedEv?.currentCharge ?? 0);

            return (
              <Grid key={station.id} size={{ xs: 12, sm: 6, lg: 4 }}>
                <Card
                  sx={{
                    p: 3,
                    height: '100%',
                    display: 'flex',
                    flexDirection: 'column',
                    justifyContent: 'space-between',
                    border: `1px solid ${theme.palette.divider}`,
                    borderRadius: 3,
                    transition: 'all 0.2s ease-in-out',
                    '&:hover': {
                      borderColor: theme.palette.primary.main,
                      transform: 'translateY(-2px)',
                      boxShadow: `0 8px 24px ${alpha(theme.palette.primary.main, 0.12)}`,
                    },
                  }}
                >
                  <Box>
                    {/* Header: Icon, Name, ID, Status */}
                    <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', mb: 2 }}>
                      <Box sx={{ display: 'flex', alignItems: 'center', gap: 1.5 }}>
                        <Box
                          sx={{
                            width: 48,
                            height: 48,
                            borderRadius: 2.5,
                            bgcolor: station.status === 'available'
                              ? alpha(theme.palette.success.main, 0.12)
                              : station.status === 'charging'
                              ? alpha(theme.palette.info.main, 0.12)
                              : station.status === 'maintenance'
                              ? alpha(theme.palette.warning.main, 0.12)
                              : alpha(theme.palette.error.main, 0.12),
                            color: station.status === 'available'
                              ? theme.palette.success.main
                              : station.status === 'charging'
                              ? theme.palette.info.main
                              : station.status === 'maintenance'
                              ? theme.palette.warning.main
                              : theme.palette.error.main,
                            display: 'flex',
                            alignItems: 'center',
                            justifyContent: 'center',
                          }}
                        >
                          <EvStationOutlined sx={{ fontSize: 28 }} />
                        </Box>
                        <Box>
                          <Typography variant="h6" fontWeight={700} sx={{ lineHeight: 1.2 }}>
                            {station.name}
                          </Typography>
                          <Typography variant="caption" color="text.secondary">
                            {station.id} • {station.location}
                          </Typography>
                        </Box>
                      </Box>
                      <StatusChip status={station.status} />
                    </Box>

                    {/* Specifications List */}
                    <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1.2, my: 2 }}>
                      <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                        <Typography variant="body2" color="text.secondary">Power Output</Typography>
                        <Typography variant="body2" fontWeight={600}>{station.power} kW</Typography>
                      </Box>
                      <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                        <Typography variant="body2" color="text.secondary">Connector</Typography>
                        <Chip label={station.connectorType} size="small" variant="outlined" />
                      </Box>
                      <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                        <Typography variant="body2" color="text.secondary">Current Vehicle</Typography>
                        <Typography variant="body2" fontWeight={600} color={station.currentEV ? 'primary.main' : 'text.secondary'}>
                          {station.currentEV ? `${station.currentEV} (${assignedEv?.model || 'EV'})` : 'None (Available)'}
                        </Typography>
                      </Box>
                    </Box>

                    {/* Active Session Progress & Telemetry */}
                    {station.status === 'charging' && (
                      <Box sx={{ p: 1.5, mb: 2, borderRadius: 2, bgcolor: alpha(theme.palette.info.main, 0.08) }}>
                        <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 0.8 }}>
                          <Box sx={{ display: 'flex', alignItems: 'center', gap: 0.8 }}>
                            <ElectricBoltOutlined sx={{ fontSize: 16, color: theme.palette.info.main }} />
                            <Typography variant="caption" fontWeight={700} color="info.main">
                              LIVE CHARGING
                            </Typography>
                          </Box>
                          <Typography variant="caption" fontWeight={700} color="info.main">
                            {Math.round(progress)}% → {activeSession?.targetCharge || 85}%
                          </Typography>
                        </Box>
                        <LinearProgress
                          variant="determinate"
                          value={progress}
                          sx={{
                            height: 6,
                            borderRadius: 3,
                            bgcolor: alpha(theme.palette.info.main, 0.2),
                            '& .MuiLinearProgress-bar': { borderRadius: 3, bgcolor: theme.palette.info.main },
                          }}
                        />
                        <Box sx={{ display: 'flex', justifyContent: 'space-between', mt: 1 }}>
                          <Typography variant="caption" color="text.secondary">
                            Session: {activeSession?.id || 'Active'}
                          </Typography>
                          <Typography variant="caption" fontWeight={600}>
                            ₹{activeSession ? activeSession.estimatedCost.toFixed(0) : '—'}
                          </Typography>
                        </Box>
                      </Box>
                    )}
                  </Box>

                  {/* Actions Bar */}
                  <Box sx={{ pt: 2, borderTop: `1px solid ${theme.palette.divider}`, display: 'flex', gap: 1 }}>
                    <Button
                      size="small"
                      variant="outlined"
                      startIcon={<VisibilityOutlined />}
                      onClick={() => navigate(`/stations/${station.id}`)}
                      sx={{ flex: 1 }}
                    >
                      Details
                    </Button>

                    {station.status === 'available' && (
                      <Button
                        size="small"
                        variant="contained"
                        color="primary"
                        startIcon={<ElectricBoltOutlined />}
                        onClick={() => handleOpenChargeDialog(station)}
                        sx={{ flex: 1 }}
                      >
                        Charge
                      </Button>
                    )}

                    {station.status === 'charging' && activeSession && (
                      <Button
                        size="small"
                        variant="contained"
                        color="error"
                        startIcon={<StopCircleOutlined />}
                        onClick={() => handleOpenCompleteConfirm(activeSession.id)}
                        sx={{ flex: 1 }}
                      >
                        Complete
                      </Button>
                    )}

                    <Tooltip title="Update Status">
                      <IconButton
                        size="small"
                        onClick={() => handleOpenStatusDialog(station)}
                        sx={{ border: `1px solid ${theme.palette.divider}`, borderRadius: 2 }}
                      >
                        <TuneOutlined fontSize="small" />
                      </IconButton>
                    </Tooltip>
                  </Box>
                </Card>
              </Grid>
            );
          })}
        </Grid>
      )}

      {/* ─── START CHARGING DIALOG ─── */}
      <Dialog
        open={chargeDialogOpen}
        onClose={() => setChargeDialogOpen(false)}
        maxWidth="xs"
        fullWidth
      >
        <DialogTitle sx={{ fontWeight: 700 }}>
          Initiate Charging: {selectedStation?.name}
        </DialogTitle>
        <DialogContent sx={{ pt: 2, display: 'flex', flexDirection: 'column', gap: 2.5 }}>
          <Typography variant="body2" color="text.secondary">
            Connect an electric vehicle to Bay {selectedStation?.id} ({selectedStation?.power} kW).
          </Typography>

          <TextField
            select
            label="Select Registered Vehicle"
            value={selectedEvId}
            onChange={e => setSelectedEvId(e.target.value)}
            fullWidth
            required
            helperText="Only available vehicles can be assigned."
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
                <Typography variant="caption" color="text.secondary">Starting Battery:</Typography>
                <Typography variant="caption" fontWeight={600}>{selectedEv.currentCharge}%</Typography>
              </Box>
              <Box sx={{ display: 'flex', justifyContent: 'space-between', mb: 0.5 }}>
                <Typography variant="caption" color="text.secondary">Battery Capacity:</Typography>
                <Typography variant="caption" fontWeight={600}>{selectedEv.batteryCapacity} kWh</Typography>
              </Box>
              <Box sx={{ display: 'flex', justifyContent: 'space-between', mb: 0.5 }}>
                <Typography variant="caption" color="text.secondary">Energy to Transfer:</Typography>
                <Typography variant="caption" fontWeight={600}>{estEnergy.toFixed(1)} kWh</Typography>
              </Box>
              <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                <Typography variant="caption" color="text.secondary">Estimated Tariff (INR):</Typography>
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
            Assign & Begin
          </Button>
        </DialogActions>
      </Dialog>

      {/* ─── ADD STATION DIALOG ─── */}
      <Dialog
        open={addDialogOpen}
        onClose={() => setAddDialogOpen(false)}
        maxWidth="xs"
        fullWidth
      >
        <DialogTitle sx={{ fontWeight: 700 }}>Deploy New Charging Station</DialogTitle>
        <DialogContent sx={{ pt: 2, display: 'flex', flexDirection: 'column', gap: 2 }}>
          <Typography variant="body2" color="text.secondary">
            Register a new physical charging bay into the SQLite database.
          </Typography>

          <TextField
            label="Station Identifier (e.g. CS-5)"
            value={newStationData.id}
            onChange={e => setNewStationData({ ...newStationData, id: e.target.value })}
            fullWidth
            required
            placeholder="CS-5"
          />

          <TextField
            label="Station Name"
            value={newStationData.name}
            onChange={e => setNewStationData({ ...newStationData, name: e.target.value })}
            fullWidth
            required
            placeholder="Ultra Fast Charger 5"
          />

          <TextField
            label="Power Rating (kW)"
            type="number"
            value={newStationData.power}
            onChange={e => setNewStationData({ ...newStationData, power: Number(e.target.value) })}
            fullWidth
            required
          />

          <TextField
            select
            label="Connector Type"
            value={newStationData.connectorType}
            onChange={e => setNewStationData({ ...newStationData, connectorType: e.target.value as ConnectorType })}
            fullWidth
          >
            <MenuItem value="CCS2">CCS2 (Combined Charging System)</MenuItem>
            <MenuItem value="Type 2">Type 2 (AC Mennekes)</MenuItem>
            <MenuItem value="CHAdeMO">CHAdeMO (DC Fast)</MenuItem>
            <MenuItem value="GB/T">GB/T (Fast Charger)</MenuItem>
          </TextField>

          <TextField
            label="Location Bay"
            value={newStationData.location}
            onChange={e => setNewStationData({ ...newStationData, location: e.target.value })}
            fullWidth
            placeholder="Bay C-1"
          />
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2.5 }}>
          <Button onClick={() => setAddDialogOpen(false)}>Cancel</Button>
          <Button
            variant="contained"
            onClick={handleCreateStation}
            startIcon={<AddOutlined />}
            disabled={!newStationData.id || !newStationData.name}
          >
            Deploy Station
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
          Manage Station State: {stationToUpdate?.id}
        </DialogTitle>
        <DialogContent sx={{ pt: 2, display: 'flex', flexDirection: 'column', gap: 2 }}>
          {stationToUpdate?.status === 'charging' && (
            <Alert severity="warning">
              This station is currently actively charging vehicle <strong>{stationToUpdate.currentEV}</strong>.
              You cannot take it offline or mark it available until the session is completed.
            </Alert>
          )}

          <Typography variant="body2" color="text.secondary">
            Select the new operational state for {stationToUpdate?.name}:
          </Typography>

          <TextField
            select
            label="Operational Status"
            value={targetStatus}
            onChange={e => setTargetStatus(e.target.value as StationStatus)}
            fullWidth
            disabled={stationToUpdate?.status === 'charging'}
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
            disabled={stationToUpdate?.status === 'charging'}
          >
            Update State
          </Button>
        </DialogActions>
      </Dialog>

      {/* ─── COMPLETE SESSION CONFIRMATION ─── */}
      <ConfirmDialog
        open={confirmCompleteOpen}
        title="Complete Charging Session"
        message="Are you sure you want to end this active session? The station will calculate the final kWh delivered, generate an official invoice, and release or auto-dispatch the next queued EV."
        confirmLabel="Complete & Invoice"
        onConfirm={handleConfirmComplete}
        onCancel={() => setConfirmCompleteOpen(false)}
      />
    </Box>
  );
}
