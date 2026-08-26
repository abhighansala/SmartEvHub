import { useState } from 'react';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import Grid from '@mui/material/Grid2';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import LinearProgress from '@mui/material/LinearProgress';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import TextField from '@mui/material/TextField';
import MenuItem from '@mui/material/MenuItem';
import { useTheme, alpha } from '@mui/material/styles';

import AddOutlined from '@mui/icons-material/AddOutlined';
import BatteryFullOutlined from '@mui/icons-material/BatteryFullOutlined';
import BatteryChargingFullOutlined from '@mui/icons-material/BatteryChargingFullOutlined';
import Battery0BarOutlined from '@mui/icons-material/Battery0BarOutlined';
import BuildOutlined from '@mui/icons-material/BuildOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatCard from '@/components/common/StatCard';
import StatusChip from '@/components/common/StatusChip';
import { useApp } from '@/context/AppContext';
import type { BatteryStatus } from '@/types';

export default function BatteryManagement() {
  const theme = useTheme();
  const { batteries, addBattery } = useApp();
  const [dialogOpen, setDialogOpen] = useState(false);
  const [form, setForm] = useState({ id: '', capacity: '', currentCharge: '', status: 'available' as BatteryStatus, cycleCount: '' });

  const available = batteries.filter(b => b.status === 'available').length;
  const inUse = batteries.filter(b => b.status === 'inuse').length;
  const charging = batteries.filter(b => b.status === 'charging').length;
  const unavailable = batteries.filter(b => b.status === 'maintenance' || b.status === 'unavailable').length;

  const handleAdd = () => {
    addBattery({
      id: form.id || `BAT${String(batteries.length + 1).padStart(3, '0')}`,
      capacity: Number(form.capacity) || 40.5,
      currentCharge: Number(form.currentCharge) || 100,
      status: form.status,
      currentVehicle: null,
      cycleCount: Number(form.cycleCount) || 0,
    });
    setDialogOpen(false);
    setForm({ id: '', capacity: '', currentCharge: '', status: 'available', cycleCount: '' });
  };

  const getBatteryColor = (charge: number) =>
    charge < 20 ? theme.palette.error.main : charge < 50 ? theme.palette.warning.main : theme.palette.success.main;

  return (
    <Box>
      <PageHeader
        title="Battery Management"
        subtitle="Track and manage battery inventory"
        action={
          <Button variant="contained" startIcon={<AddOutlined />} onClick={() => setDialogOpen(true)}>
            Add Battery
          </Button>
        }
      />

      <Grid container spacing={2.5} sx={{ mb: 3 }}>
        <Grid size={{ xs: 6, md: 3 }}>
          <StatCard title="Total Batteries" value={batteries.length} icon={BatteryFullOutlined} color={theme.palette.primary.main} />
        </Grid>
        <Grid size={{ xs: 6, md: 3 }}>
          <StatCard title="Available" value={available} icon={BatteryFullOutlined} color={theme.palette.success.main} />
        </Grid>
        <Grid size={{ xs: 6, md: 3 }}>
          <StatCard title="In Use / Charging" value={inUse + charging} icon={BatteryChargingFullOutlined} color={theme.palette.info.main} />
        </Grid>
        <Grid size={{ xs: 6, md: 3 }}>
          <StatCard title="Unavailable" value={unavailable} icon={BuildOutlined} color={theme.palette.error.main} />
        </Grid>
      </Grid>

      <Card sx={{ border: `1px solid ${theme.palette.divider}` }}>
        <TableContainer>
          <Table>
            <TableHead>
              <TableRow>
                <TableCell>Battery ID</TableCell>
                <TableCell align="right">Capacity</TableCell>
                <TableCell>Charge Level</TableCell>
                <TableCell>Status</TableCell>
                <TableCell>Current Vehicle</TableCell>
                <TableCell align="right">Cycles</TableCell>
                <TableCell>Last Updated</TableCell>
              </TableRow>
            </TableHead>
            <TableBody>
              {batteries.map(b => (
                <TableRow key={b.id} hover>
                  <TableCell sx={{ fontWeight: 600 }}>{b.id}</TableCell>
                  <TableCell align="right">{b.capacity} kWh</TableCell>
                  <TableCell sx={{ minWidth: 150 }}>
                    <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                      <LinearProgress
                        variant="determinate"
                        value={b.currentCharge}
                        sx={{
                          flex: 1,
                          height: 8,
                          borderRadius: 4,
                          bgcolor: alpha(getBatteryColor(b.currentCharge), 0.1),
                          '& .MuiLinearProgress-bar': { borderRadius: 4, bgcolor: getBatteryColor(b.currentCharge) },
                        }}
                      />
                      <Typography variant="caption" fontWeight={600} sx={{ minWidth: 32 }}>
                        {b.currentCharge}%
                      </Typography>
                    </Box>
                  </TableCell>
                  <TableCell><StatusChip status={b.status} /></TableCell>
                  <TableCell>{b.currentVehicle || '—'}</TableCell>
                  <TableCell align="right">{b.cycleCount}</TableCell>
                  <TableCell>{new Date(b.lastUpdated).toLocaleTimeString('en-IN', { hour: '2-digit', minute: '2-digit' })}</TableCell>
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </TableContainer>
      </Card>

      <Dialog open={dialogOpen} onClose={() => setDialogOpen(false)} maxWidth="sm" fullWidth>
        <DialogTitle>Add Battery</DialogTitle>
        <DialogContent sx={{ display: 'flex', flexDirection: 'column', gap: 2.5, pt: '16px !important' }}>
          <TextField label="Battery ID" value={form.id} onChange={e => setForm(p => ({ ...p, id: e.target.value }))} placeholder="e.g. BAT021" />
          <TextField label="Capacity (kWh)" type="number" value={form.capacity} onChange={e => setForm(p => ({ ...p, capacity: e.target.value }))} />
          <TextField label="Current Charge (%)" type="number" value={form.currentCharge} onChange={e => setForm(p => ({ ...p, currentCharge: e.target.value }))} slotProps={{ htmlInput: { min: 0, max: 100 } }} />
          <TextField label="Cycle Count" type="number" value={form.cycleCount} onChange={e => setForm(p => ({ ...p, cycleCount: e.target.value }))} />
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2 }}>
          <Button onClick={() => setDialogOpen(false)}>Cancel</Button>
          <Button variant="contained" onClick={handleAdd}>Add Battery</Button>
        </DialogActions>
      </Dialog>
    </Box>
  );
}
