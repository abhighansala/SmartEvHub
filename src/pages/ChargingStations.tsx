import { useState, useEffect, useMemo } from 'react';
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
import Alert from '@mui/material/Alert';
import CircularProgress from '@mui/material/CircularProgress';
import Tabs from '@mui/material/Tabs';
import Tab from '@mui/material/Tab';
import InputAdornment from '@mui/material/InputAdornment';
import { useTheme, alpha } from '@mui/material/styles';

import EvStationOutlined from '@mui/icons-material/EvStationOutlined';
import VisibilityOutlined from '@mui/icons-material/VisibilityOutlined';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';
import CheckCircleOutline from '@mui/icons-material/CheckCircleOutline';
import HourglassTopOutlined from '@mui/icons-material/HourglassTopOutlined';
import AddOutlined from '@mui/icons-material/AddOutlined';
import SearchOutlined from '@mui/icons-material/SearchOutlined';
import BuildOutlined from '@mui/icons-material/BuildOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import { useApp } from '@/context/AppContext';
import { api } from '@/services/api';
import type { Station, StationStatus, ConnectorType } from '@/types';

export default function ChargingStations() {
  const theme = useTheme();
  const navigate = useNavigate();
  const { stations, evs, sessions, currentUser, refreshBackendData, showSnackbar } = useApp();

  const isOperatorOrAdmin = currentUser?.role === 'operator' || currentUser?.role === 'admin';

  // Auto-refresh when stations are actively charging or occupied
  useEffect(() => {
    const hasActive = stations.some(s => s.status === 'charging') ||
      sessions.some(s => s.status === 'active');
    if (!hasActive) return;

    const interval = setInterval(() => {
      refreshBackendData();
    }, 4000);

    return () => clearInterval(interval);
  }, [stations, sessions, refreshBackendData]);

  // Search & Filter state
  const [searchQuery, setSearchQuery] = useState('');
  const [statusFilter, setStatusFilter] = useState<'all' | StationStatus>('all');

  // Request Charging Dialog state
  const [dialogOpen, setDialogOpen] = useState(false);
  const [selectedStation, setSelectedStation] = useState<Station | null>(null);
  const [selectedEvId, setSelectedEvId] = useState('');
  const [targetCharge, setTargetCharge] = useState<number>(85);
  const [loading, setLoading] = useState(false);
  const [errorMsg, setErrorMsg] = useState('');

  // Outcome feedback modal state
  const [resultModalOpen, setResultModalOpen] = useState(false);
  const [resultData, setResultData] = useState<{
    allocated: boolean;
    queued: boolean;
    stationId?: string;
    bayId?: string;
    sessionId?: string;
    message?: string;
  } | null>(null);

  // Deploy Station Dialog state (Operator / Admin)
  const [addDialogOpen, setAddDialogOpen] = useState(false);
  const [deployLoading, setDeployLoading] = useState(false);
  const [deployError, setDeployError] = useState('');
  const [newStationData, setNewStationData] = useState({
    id: '',
    name: '',
    power: 50,
    connectorType: 'CCS2' as ConnectorType,
    location: '',
  });

  // Manage Status Dialog state (Operator / Admin)
  const [statusDialogOpen, setStatusDialogOpen] = useState(false);
  const [statusLoading, setStatusLoading] = useState(false);
  const [statusError, setStatusError] = useState('');
  const [stationToUpdate, setStationToUpdate] = useState<Station | null>(null);
  const [targetStatus, setTargetStatus] = useState<StationStatus>('available');

  const selectedEV = evs.find(e => e.id === selectedEvId);

  // Calculate tariff preview
  const estimatedKWh = selectedEV ? Math.max(0, ((targetCharge - selectedEV.currentCharge) / 100) * selectedEV.batteryCapacity) : 0;
  const estimatedCost = estimatedKWh <= 0 ? 0 : Math.max(100, estimatedKWh <= 30 ? estimatedKWh * 18.0 : 30.0 * 18.0 + (estimatedKWh - 30.0) * 18.0 * 0.90);

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

  const openRequestModal = (station?: Station) => {
    setSelectedStation(station || null);
    setErrorMsg('');
    if (evs.length > 0 && !selectedEvId) {
      const avail = evs.find(e => e.status === 'available');
      setSelectedEvId(avail ? avail.id : evs[0].id);
    }
    setDialogOpen(true);
  };

  const handleStartCharging = async () => {
    if (!selectedEvId) {
      setErrorMsg('Please select a registered EV');
      return;
    }

    if (selectedEV && targetCharge <= selectedEV.currentCharge) {
      setErrorMsg(`Target charge (${targetCharge}%) must be greater than current battery (${selectedEV.currentCharge}%)`);
      return;
    }

    setLoading(true);
    setErrorMsg('');

    try {
      const res: any = await api.requestCharging({
        evId: selectedEvId,
        stationId: selectedStation ? selectedStation.id : undefined,
        targetCharge,
      });

      setLoading(false);
      setDialogOpen(false);

      if (res && res.success) {
        await refreshBackendData();
        setResultData({
          allocated: !!res.allocated,
          queued: !!res.queued,
          stationId: res.stationId || selectedStation?.id,
          bayId: res.bayId || selectedStation?.location,
          sessionId: res.session?.id,
          message: res.message,
        });
        setResultModalOpen(true);
      } else {
        setErrorMsg(res?.message || 'Failed to submit charging request');
        setDialogOpen(true);
      }
    } catch (err: any) {
      setLoading(false);
      setErrorMsg(err.message || 'Error communicating with charging service');
      setDialogOpen(true);
    }
  };

  const handleDeployStation = async () => {
    if (!newStationData.id.trim() || !newStationData.name.trim() || newStationData.power <= 0) {
      setDeployError('Please fill out all required fields with valid values');
      return;
    }

    setDeployLoading(true);
    setDeployError('');

    try {
      const res: any = await api.createStation(newStationData);
      setDeployLoading(false);
      if (res && res.success) {
        setAddDialogOpen(false);
        setNewStationData({ id: '', name: '', power: 50, connectorType: 'CCS2', location: '' });
        showSnackbar('Station deployed successfully!', 'success');
        await refreshBackendData();
      } else {
        setDeployError(res?.message || 'Failed to deploy station');
      }
    } catch (err: any) {
      setDeployLoading(false);
      setDeployError(err.message || 'Network error deploying station');
    }
  };

  const openStatusModal = (station: Station) => {
    setStationToUpdate(station);
    setTargetStatus(station.status);
    setStatusError('');
    setStatusDialogOpen(true);
  };

  const handleUpdateStatus = async () => {
    if (!stationToUpdate) return;

    setStatusLoading(true);
    setStatusError('');

    try {
      const res: any = await api.updateStationStatus(stationToUpdate.id, targetStatus);
      setStatusLoading(false);
      if (res && res.success) {
        setStatusDialogOpen(false);
        showSnackbar(`Station ${stationToUpdate.id} status updated to ${targetStatus}`, 'success');
        await refreshBackendData();
      } else {
        setStatusError(res?.message || 'Failed to update status');
      }
    } catch (err: any) {
      setStatusLoading(false);
      setStatusError(err.message || 'Network error updating station status');
    }
  };

  return (
    <Box>
      <PageHeader
        title="Charging Stations"
        subtitle="Explore high-power DC & AC charging bays across the hub network"
        action={
          <Box sx={{ display: 'flex', gap: 1.5 }}>
            {isOperatorOrAdmin && (
              <Button
                variant="outlined"
                startIcon={<AddOutlined />}
                onClick={() => {
                  setDeployError('');
                  setAddDialogOpen(true);
                }}
              >
                Deploy Station
              </Button>
            )}
            <Button
              variant="contained"
              startIcon={<ElectricBoltOutlined />}
              onClick={() => openRequestModal()}
            >
              Request Charging
            </Button>
          </Box>
        }
      />

      {/* Filter and Search Bar */}
      <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 3, flexWrap: 'wrap', gap: 2 }}>
        <Tabs
          value={statusFilter}
          onChange={(_, val) => setStatusFilter(val)}
          sx={{
            minHeight: 40,
            '& .MuiTab-root': { minHeight: 40, textTransform: 'none', fontWeight: 600, py: 0.5 },
          }}
        >
          <Tab label={`All (${stations.length})`} value="all" />
          <Tab label={`Available (${stations.filter(s => s.status === 'available').length})`} value="available" />
          <Tab label={`Charging (${stations.filter(s => s.status === 'charging').length})`} value="charging" />
          <Tab label={`Maintenance (${stations.filter(s => s.status === 'maintenance').length})`} value="maintenance" />
          <Tab label={`Offline (${stations.filter(s => s.status === 'offline').length})`} value="offline" />
        </Tabs>

        <TextField
          placeholder="Search stations, bays, EVs..."
          size="small"
          value={searchQuery}
          onChange={(e) => setSearchQuery(e.target.value)}
          slotProps={{
            input: {
              startAdornment: (
                <InputAdornment position="start">
                  <SearchOutlined fontSize="small" sx={{ color: 'text.secondary' }} />
                </InputAdornment>
              ),
            },
          }}
          sx={{ width: { xs: '100%', sm: 260 } }}
        />
      </Box>

      <Grid container spacing={2.5}>
        {filteredStations.map(station => {
          const activeSession = sessions.find(s => s.stationId === station.id && s.status === 'active');
          const currentEvId = station.currentEV || activeSession?.evId;
          const currentEvObj = currentEvId ? evs.find(e => e.id === currentEvId) : null;
          const evDisplay = currentEvId
            ? (currentEvObj ? `${currentEvObj.model} (${currentEvId})` : currentEvId)
            : (station.status === 'maintenance'
                ? 'Under Maintenance'
                : station.status === 'offline'
                ? 'Station Offline'
                : 'None (Available)');
          const effectiveStatus = (activeSession && station.status === 'available') ? 'charging' : station.status;

          return (
            <Grid key={station.id} size={{ xs: 12, sm: 6, lg: 4 }}>
              <Card
                sx={{
                  p: 3,
                  border: `1px solid ${theme.palette.divider}`,
                  cursor: 'pointer',
                  transition: 'all 0.2s',
                  '&:hover': {
                    borderColor: theme.palette.primary.main,
                    transform: 'translateY(-2px)',
                    boxShadow: `0 4px 20px ${alpha(theme.palette.primary.main, 0.1)}`,
                  },
                }}
                onClick={() => navigate(`/stations/${station.id}`)}
              >
                <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', mb: 2 }}>
                  <Box sx={{ display: 'flex', alignItems: 'center', gap: 1.5 }}>
                    <Box
                      sx={{
                        width: 44,
                        height: 44,
                        borderRadius: 2.5,
                        bgcolor: alpha(theme.palette.primary.main, 0.1),
                        display: 'flex',
                        alignItems: 'center',
                        justifyContent: 'center',
                      }}
                    >
                      <EvStationOutlined sx={{ color: theme.palette.primary.main }} />
                    </Box>
                    <Box>
                      <Typography variant="h5">{station.name}</Typography>
                      <Typography variant="caption" color="text.secondary">{station.id} • {station.location}</Typography>
                    </Box>
                  </Box>
                  <StatusChip status={effectiveStatus} />
                </Box>

                <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1.5 }}>
                  <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                    <Typography variant="body2" color="text.secondary">Power Output</Typography>
                    <Typography variant="body2" fontWeight={600}>{station.power} kW</Typography>
                  </Box>
                  <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                    <Typography variant="body2" color="text.secondary">Connector</Typography>
                    <Chip label={station.connectorType} size="small" variant="outlined" />
                  </Box>
                  <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                    <Typography variant="body2" color="text.secondary">Current EV</Typography>
                    <Typography variant="body2" fontWeight={500}>
                      {evDisplay}
                    </Typography>
                  </Box>
                </Box>

                <Box sx={{ mt: 2, pt: 2, borderTop: `1px solid ${theme.palette.divider}`, display: 'flex', gap: 1 }}>
                  <Button
                    size="small"
                    startIcon={<VisibilityOutlined />}
                    onClick={(e) => { e.stopPropagation(); navigate(`/stations/${station.id}`); }}
                    fullWidth
                  >
                    View Details
                  </Button>
                  {effectiveStatus === 'available' ? (
                    <Button
                      size="small"
                      variant="contained"
                      startIcon={<ElectricBoltOutlined />}
                      onClick={(e) => {
                        e.stopPropagation();
                        openRequestModal(station);
                      }}
                      fullWidth
                    >
                      Charge Here
                    </Button>
                  ) : (
                    <Button
                      size="small"
                      variant="outlined"
                      color="warning"
                      onClick={(e) => {
                        e.stopPropagation();
                        openRequestModal(station);
                      }}
                      fullWidth
                    >
                      Queue Bay
                    </Button>
                  )}
                  {isOperatorOrAdmin && (
                    <Button
                      size="small"
                      variant="outlined"
                      color="inherit"
                      startIcon={<BuildOutlined />}
                      onClick={(e) => {
                        e.stopPropagation();
                        openStatusModal(station);
                      }}
                      sx={{ minWidth: 'auto', px: 1.5 }}
                    >
                      Status
                    </Button>
                  )}
                </Box>
              </Card>
            </Grid>
          );
        })}
      </Grid>

      {/* Request Charging Dialog */}
      <Dialog open={dialogOpen} onClose={() => !loading && setDialogOpen(false)} maxWidth="sm" fullWidth>
        <DialogTitle sx={{ fontWeight: 700 }}>
          {selectedStation ? `Request Charging at ${selectedStation.name}` : 'Request EV Charging'}
        </DialogTitle>
        <DialogContent sx={{ pt: 2, display: 'flex', flexDirection: 'column', gap: 2.5 }}>
          {errorMsg && (
            <Alert severity="error">{errorMsg}</Alert>
          )}

          {evs.length === 0 ? (
            <Box sx={{ textAlign: 'center', py: 2 }}>
              <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
                You do not have any registered electric vehicles yet.
              </Typography>
              <Button variant="contained" onClick={() => { setDialogOpen(false); navigate('/evs'); }}>
                Register Your EV
              </Button>
            </Box>
          ) : (
            <>
              {/* Select EV */}
              <TextField
                select
                label="Select Registered Vehicle"
                value={selectedEvId}
                onChange={(e) => setSelectedEvId(e.target.value)}
                fullWidth
                required
              >
                {evs.map((ev) => (
                  <MenuItem key={ev.id} value={ev.id}>
                    {ev.model} ({ev.id}) — {ev.currentCharge}% SoC ({ev.status})
                  </MenuItem>
                ))}
              </TextField>

              {/* Station Selection if not preselected */}
              <TextField
                select
                label="Charging Station / Bay"
                value={selectedStation ? selectedStation.id : 'any'}
                onChange={(e) => {
                  const val = e.target.value;
                  if (val === 'any') {
                    setSelectedStation(null);
                  } else {
                    const st = stations.find(s => s.id === val);
                    setSelectedStation(st || null);
                  }
                }}
                fullWidth
              >
                <MenuItem value="any">⚡ Any Available Bay (Fast Auto-Allocation)</MenuItem>
                {stations.map((st) => (
                  <MenuItem key={st.id} value={st.id}>
                    {st.name} ({st.id} • {st.location}) — {st.power} kW ({st.status})
                  </MenuItem>
                ))}
              </TextField>

              {/* Target SoC Slider */}
              <Box sx={{ mt: 1 }}>
                <Box sx={{ display: 'flex', justifyContent: 'space-between', mb: 1 }}>
                  <Typography variant="body2" fontWeight={600}>Target Battery Charge</Typography>
                  <Typography variant="body2" fontWeight={700} color="primary.main">{targetCharge}%</Typography>
                </Box>
                <Slider
                  value={targetCharge}
                  onChange={(_, val) => setTargetCharge(val as number)}
                  min={50}
                  max={100}
                  step={5}
                  marks={[
                    { value: 50, label: '50%' },
                    { value: 80, label: '80% (Rec)' },
                    { value: 100, label: '100%' },
                  ]}
                  valueLabelDisplay="auto"
                />
              </Box>

              {/* Estimate Calculation Summary Card */}
              {selectedEV && (
                <Card sx={{ p: 2, bgcolor: alpha(theme.palette.primary.main, 0.04), border: `1px solid ${theme.palette.divider}` }}>
                  <Typography variant="subtitle2" fontWeight={700} sx={{ mb: 1 }}>Charging Session Estimate</Typography>
                  <Grid container spacing={1.5}>
                    <Grid size={{ xs: 6 }}>
                      <Typography variant="caption" color="text.secondary">Current Charge</Typography>
                      <Typography variant="body2" fontWeight={600}>{selectedEV.currentCharge}%</Typography>
                    </Grid>
                    <Grid size={{ xs: 6 }}>
                      <Typography variant="caption" color="text.secondary">Target Charge</Typography>
                      <Typography variant="body2" fontWeight={600}>{targetCharge}%</Typography>
                    </Grid>
                    <Grid size={{ xs: 6 }}>
                      <Typography variant="caption" color="text.secondary">Energy to Deliver</Typography>
                      <Typography variant="body2" fontWeight={600}>{estimatedKWh.toFixed(1)} kWh</Typography>
                    </Grid>
                    <Grid size={{ xs: 6 }}>
                      <Typography variant="caption" color="text.secondary">Estimated Tariff</Typography>
                      <Typography variant="body2" fontWeight={700} color="primary.main">₹{estimatedCost.toFixed(2)}</Typography>
                    </Grid>
                  </Grid>
                </Card>
              )}
            </>
          )}
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2.5 }}>
          <Button onClick={() => setDialogOpen(false)} disabled={loading}>
            Cancel
          </Button>
          <Button
            variant="contained"
            disabled={!selectedEvId || loading || evs.length === 0}
            onClick={handleStartCharging}
            startIcon={loading ? <CircularProgress size={18} color="inherit" /> : <ElectricBoltOutlined />}
          >
            {loading ? 'Submitting...' : 'Submit Request'}
          </Button>
        </DialogActions>
      </Dialog>

      {/* Deploy New Station Dialog (Operator / Admin) */}
      <Dialog open={addDialogOpen} onClose={() => !deployLoading && setAddDialogOpen(false)} maxWidth="xs" fullWidth>
        <DialogTitle sx={{ fontWeight: 700 }}>Deploy New Charging Station</DialogTitle>
        <DialogContent sx={{ pt: 2, display: 'flex', flexDirection: 'column', gap: 2 }}>
          {deployError && <Alert severity="error">{deployError}</Alert>}

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
            <MenuItem value="Type 2">Type 2 (Mennekes AC)</MenuItem>
            <MenuItem value="CHAdeMO">CHAdeMO (DC Fast)</MenuItem>
            <MenuItem value="GB/T">GB/T (DC Fast)</MenuItem>
          </TextField>

          <TextField
            label="Physical Bay Location"
            value={newStationData.location}
            onChange={e => setNewStationData({ ...newStationData, location: e.target.value })}
            fullWidth
            placeholder="Bay C-1"
          />
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2.5 }}>
          <Button onClick={() => setAddDialogOpen(false)} disabled={deployLoading}>
            Cancel
          </Button>
          <Button
            variant="contained"
            onClick={handleDeployStation}
            disabled={deployLoading}
            startIcon={deployLoading ? <CircularProgress size={16} /> : <AddOutlined />}
          >
            {deployLoading ? 'Deploying...' : 'Deploy Bay'}
          </Button>
        </DialogActions>
      </Dialog>

      {/* Manage Status Dialog (Operator / Admin) */}
      <Dialog open={statusDialogOpen} onClose={() => !statusLoading && setStatusDialogOpen(false)} maxWidth="xs" fullWidth>
        <DialogTitle sx={{ fontWeight: 700 }}>Update Station Status</DialogTitle>
        <DialogContent sx={{ pt: 2, display: 'flex', flexDirection: 'column', gap: 2 }}>
          {statusError && <Alert severity="error">{statusError}</Alert>}

          {stationToUpdate && (
            <Typography variant="body2" color="text.secondary">
              Update operational state for <strong>{stationToUpdate.name}</strong> ({stationToUpdate.id}).
            </Typography>
          )}

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

      {/* Request Result / Bay Allocation Confirmation Dialog */}
      <Dialog open={resultModalOpen} onClose={() => setResultModalOpen(false)} maxWidth="xs" fullWidth>
        <DialogContent sx={{ textAlign: 'center', py: 4, display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 2 }}>
          {resultData?.allocated ? (
            <>
              <Box
                sx={{
                  width: 64,
                  height: 64,
                  borderRadius: '50%',
                  bgcolor: alpha(theme.palette.success.main, 0.15),
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                }}
              >
                <CheckCircleOutline sx={{ fontSize: 36, color: theme.palette.success.main }} />
              </Box>
              <Typography variant="h5" fontWeight={700}>
                Charging Bay Allocated!
              </Typography>
              <Typography variant="body2" color="text.secondary">
                Your vehicle has been successfully allocated to <strong>{resultData.stationId}</strong> ({resultData.bayId || 'Charging Bay'}).
                Live charging session is now active.
              </Typography>
              <Box sx={{ display: 'flex', gap: 1.5, mt: 1, width: '100%' }}>
                <Button variant="outlined" fullWidth onClick={() => setResultModalOpen(false)}>
                  Close
                </Button>
                <Button
                  variant="contained"
                  fullWidth
                  onClick={() => {
                    setResultModalOpen(false);
                    navigate('/sessions');
                  }}
                >
                  Monitor Session
                </Button>
              </Box>
            </>
          ) : (
            <>
              <Box
                sx={{
                  width: 64,
                  height: 64,
                  borderRadius: '50%',
                  bgcolor: alpha(theme.palette.warning.main, 0.15),
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                }}
              >
                <HourglassTopOutlined sx={{ fontSize: 36, color: theme.palette.warning.main }} />
              </Box>
              <Typography variant="h5" fontWeight={700}>
                Entered FIFO Waiting Queue
              </Typography>
              <Typography variant="body2" color="text.secondary">
                All charging bays are currently occupied. Your vehicle has been safely queued in the FIFO circular queue and will be automatically dispatched when a bay is released.
              </Typography>
              <Box sx={{ display: 'flex', gap: 1.5, mt: 1, width: '100%' }}>
                <Button variant="outlined" fullWidth onClick={() => setResultModalOpen(false)}>
                  Close
                </Button>
                <Button
                  variant="contained"
                  color="warning"
                  fullWidth
                  onClick={() => {
                    setResultModalOpen(false);
                    navigate('/charging-queue');
                  }}
                >
                  View Queue Position
                </Button>
              </Box>
            </>
          )}
        </DialogContent>
      </Dialog>
    </Box>
  );
}
