import { useState } from 'react';
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
import { useTheme, alpha } from '@mui/material/styles';

import EvStationOutlined from '@mui/icons-material/EvStationOutlined';
import VisibilityOutlined from '@mui/icons-material/VisibilityOutlined';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';
import PowerOutlined from '@mui/icons-material/PowerOutlined';
import CheckCircleOutline from '@mui/icons-material/CheckCircleOutline';
import HourglassTopOutlined from '@mui/icons-material/HourglassTopOutlined';
import AddOutlined from '@mui/icons-material/AddOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import { useApp } from '@/context/AppContext';
import { api } from '@/services/api';
import type { Station } from '@/types';

export default function ChargingStations() {
  const theme = useTheme();
  const navigate = useNavigate();
  const { stations, evs, currentUser, refreshBackendData, showSnackbar, releaseStation } = useApp();

  const [dialogOpen, setDialogOpen] = useState(false);
  const [selectedStation, setSelectedStation] = useState<Station | null>(null);
  const [selectedEvId, setSelectedEvId] = useState('');
  const [targetCharge, setTargetCharge] = useState<number>(85);
  const [loading, setLoading] = useState(false);
  const [errorMsg, setErrorMsg] = useState('');

  // Outcome feedback modal
  const [resultModalOpen, setResultModalOpen] = useState(false);
  const [resultData, setResultData] = useState<{
    allocated: boolean;
    queued: boolean;
    stationId?: string;
    bayId?: string;
    sessionId?: string;
    message?: string;
  } | null>(null);

  const selectedEV = evs.find(e => e.id === selectedEvId);
  const selectedStationObj = selectedStation ? stations.find(s => s.id === selectedStation.id) : null;

  // Calculate tariff preview
  const estimatedKWh = selectedEV ? Math.max(0, ((targetCharge - selectedEV.currentCharge) / 100) * selectedEV.batteryCapacity) : 0;
  const estimatedCost = estimatedKWh * 20.0; // ₹20 / kWh standard tariff

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

  return (
    <Box>
      <PageHeader
        title="Charging Stations"
        subtitle="Explore high-power DC & AC charging bays across the hub network"
        action={
          <Button
            variant="contained"
            startIcon={<ElectricBoltOutlined />}
            onClick={() => openRequestModal()}
          >
            Request Charging
          </Button>
        }
      />

      <Grid container spacing={2.5}>
        {stations.map(station => (
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
                <StatusChip status={station.status} />
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
                    {station.status === 'available'
                      ? 'None (Available)'
                      : (station.currentEV || (station.status === 'charging' ? 'Occupied' : 'None'))}
                  </Typography>
                </Box>
              </Box>

              <Box sx={{ mt: 2, pt: 2, borderTop: `1px solid ${theme.palette.divider}`, display: 'flex', gap: 1, flexDirection: 'column' }}>
                <Box sx={{ display: 'flex', gap: 1 }}>
                  <Button
                    size="small"
                    startIcon={<VisibilityOutlined />}
                    onClick={(e) => { e.stopPropagation(); navigate(`/stations/${station.id}`); }}
                    fullWidth
                  >
                    View Details
                  </Button>
                  {station.status === 'available' ? (
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
                </Box>
                {(currentUser?.role === 'operator' || currentUser?.role === 'admin') && station.status !== 'available' && (
                  <Button
                    size="small"
                    variant="text"
                    color="error"
                    onClick={async (e) => {
                      e.stopPropagation();
                      await releaseStation(station.id);
                    }}
                    fullWidth
                  >
                    Release Bay (Reset to Available)
                  </Button>
                )}
              </Box>
            </Card>
          </Grid>
        ))}
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
