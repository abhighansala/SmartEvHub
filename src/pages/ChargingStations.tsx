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
import { useTheme, alpha } from '@mui/material/styles';

import EvStationOutlined from '@mui/icons-material/EvStationOutlined';
import VisibilityOutlined from '@mui/icons-material/VisibilityOutlined';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';
import PowerOutlined from '@mui/icons-material/PowerOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import { useApp } from '@/context/AppContext';
import type { Station } from '@/types';

export default function ChargingStations() {
  const theme = useTheme();
  const navigate = useNavigate();
  const { stations, evs, addChargingRequest } = useApp();
  const [dialogOpen, setDialogOpen] = useState(false);
  const [selectedStation, setSelectedStation] = useState<Station | null>(null);
  const [selectedEvId, setSelectedEvId] = useState('');

  const handleStartCharging = () => {
    if (!selectedEvId || !selectedStation) return;
    const ev = evs.find(e => e.id === selectedEvId);
    addChargingRequest({
      evId: selectedEvId,
      evModel: ev?.model || 'Unknown',
      owner: ev?.owner || 'Unknown',
      currentCharge: ev?.currentCharge ?? 20,
      priority: 'normal',
    });
    setDialogOpen(false);
    setSelectedEvId('');
    setSelectedStation(null);
  };

  return (
    <Box>
      <PageHeader
        title="Charging Stations"
        subtitle="Monitor and manage charging infrastructure"
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
                  <Typography variant="body2" color="text.secondary">Power</Typography>
                  <Typography variant="body2" fontWeight={600}>{station.power} kW</Typography>
                </Box>
                <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                  <Typography variant="body2" color="text.secondary">Connector</Typography>
                  <Chip label={station.connectorType} size="small" variant="outlined" />
                </Box>
                <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                  <Typography variant="body2" color="text.secondary">Current EV</Typography>
                  <Typography variant="body2" fontWeight={500}>
                    {station.currentEV || 'None'}
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
                  View
                </Button>
                {station.status === 'available' && (
                  <Button
                    size="small"
                    variant="contained"
                    startIcon={<ElectricBoltOutlined />}
                    onClick={(e) => {
                      e.stopPropagation();
                      setSelectedStation(station);
                      setDialogOpen(true);
                    }}
                    fullWidth
                  >
                    Charge
                  </Button>
                )}
              </Box>
            </Card>
          </Grid>
        ))}
      </Grid>

      <Dialog open={dialogOpen} onClose={() => setDialogOpen(false)} maxWidth="xs" fullWidth>
        <DialogTitle>Start Charging at {selectedStation?.name}</DialogTitle>
        <DialogContent sx={{ pt: 2, display: 'flex', flexDirection: 'column', gap: 2 }}>
          <Typography variant="body2" color="text.secondary">
            Select an electric vehicle to connect to this charging station ({selectedStation?.power} kW).
          </Typography>
          <TextField
            select
            label="Select EV"
            value={selectedEvId}
            onChange={(e) => setSelectedEvId(e.target.value)}
            fullWidth
            required
          >
            {evs.map((ev) => (
              <MenuItem key={ev.id} value={ev.id}>
                {ev.model} ({ev.id}) - {ev.currentCharge}%
              </MenuItem>
            ))}
          </TextField>
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2 }}>
          <Button onClick={() => setDialogOpen(false)}>Cancel</Button>
          <Button
            variant="contained"
            disabled={!selectedEvId}
            onClick={handleStartCharging}
            startIcon={<ElectricBoltOutlined />}
          >
            Start Session
          </Button>
        </DialogActions>
      </Dialog>
    </Box>
  );
}
