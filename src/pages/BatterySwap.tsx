import { useState } from 'react';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import Grid from '@mui/material/Grid2';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import TextField from '@mui/material/TextField';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import MenuItem from '@mui/material/MenuItem';
import LinearProgress from '@mui/material/LinearProgress';
import { useTheme, alpha } from '@mui/material/styles';

import SwapHorizOutlined from '@mui/icons-material/SwapHorizOutlined';
import BatteryFullOutlined from '@mui/icons-material/BatteryFullOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import StatCard from '@/components/common/StatCard';
import EmptyState from '@/components/common/EmptyState';
import { useApp } from '@/context/AppContext';

export default function BatterySwap() {
  const theme = useTheme();
  const { batteries, evs, swapQueue, addSwapRequest, showSnackbar } = useApp();
  const [form, setForm] = useState({ evId: '', currentBattery: '', requiredCapacity: '' });

  const availableBatteries = batteries.filter(b => b.status === 'available');

  const handleSubmit = () => {
    if (!form.evId) { showSnackbar('Please select an EV', 'error'); return; }
    const ev = evs.find(e => e.id === form.evId);
    addSwapRequest({
      evId: form.evId,
      evModel: ev?.model || 'Unknown',
      owner: ev?.owner || 'Unknown',
      currentBattery: form.currentBattery || 'Current',
      requiredCapacity: Number(form.requiredCapacity) || 40.5,
    });
    setForm({ evId: '', currentBattery: '', requiredCapacity: '' });
  };

  return (
    <Box>
      <PageHeader title="Battery Swap" subtitle="Request battery swaps and view available inventory" />

      <Grid container spacing={2.5} sx={{ mb: 3 }}>
        <Grid size={{ xs: 12, sm: 6, md: 4 }}>
          <StatCard
            title="Available Batteries"
            value={availableBatteries.length}
            icon={BatteryFullOutlined}
            subtitle="Ready for swap"
            color={theme.palette.success.main}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 8 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, height: '100%' }}>
            <Typography variant="h5" sx={{ mb: 2 }}>Swap Request</Typography>
            <Box sx={{ display: 'flex', gap: 2, flexWrap: 'wrap', alignItems: 'flex-end' }}>
              <TextField
                select
                label="EV ID"
                value={form.evId}
                onChange={e => setForm(p => ({ ...p, evId: e.target.value }))}
                size="small"
                sx={{ minWidth: 180 }}
              >
                {evs.filter(e => e.status === 'available' || e.status === 'swapping').map(e => (
                  <MenuItem key={e.id} value={e.id}>{e.id} — {e.model}</MenuItem>
                ))}
              </TextField>
              <TextField
                label="Current Battery"
                value={form.currentBattery}
                onChange={e => setForm(p => ({ ...p, currentBattery: e.target.value }))}
                size="small"
                placeholder="e.g. BAT-OLD-01"
                sx={{ minWidth: 150 }}
              />
              <TextField
                label="Required Capacity (kWh)"
                type="number"
                value={form.requiredCapacity}
                onChange={e => setForm(p => ({ ...p, requiredCapacity: e.target.value }))}
                size="small"
                sx={{ minWidth: 160 }}
              />
              <Button
                variant="contained"
                startIcon={<SwapHorizOutlined />}
                onClick={handleSubmit}
              >
                Request Swap
              </Button>
            </Box>
          </Card>
        </Grid>
      </Grid>

      {/* Recent Swap Requests */}
      <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, mb: 3 }}>
        <Typography variant="h5" sx={{ mb: 2 }}>Swap Queue</Typography>
        {swapQueue.length === 0 ? (
          <EmptyState title="No swap requests" description="All battery swap requests have been processed." />
        ) : (
          <TableContainer>
            <Table size="small">
              <TableHead>
                <TableRow>
                  <TableCell>#</TableCell>
                  <TableCell>EV ID</TableCell>
                  <TableCell>Model</TableCell>
                  <TableCell>Owner</TableCell>
                  <TableCell>Required Capacity</TableCell>
                  <TableCell>Requested At</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {swapQueue.map((req, i) => (
                  <TableRow key={req.id}>
                    <TableCell>{i + 1}</TableCell>
                    <TableCell sx={{ fontWeight: 500 }}>{req.evId}</TableCell>
                    <TableCell>{req.evModel}</TableCell>
                    <TableCell>{req.owner}</TableCell>
                    <TableCell>{req.requiredCapacity} kWh</TableCell>
                    <TableCell>{new Date(req.requestedAt).toLocaleTimeString('en-IN', { hour: '2-digit', minute: '2-digit' })}</TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          </TableContainer>
        )}
      </Card>

      {/* Available Battery Inventory */}
      <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
        <Typography variant="h5" sx={{ mb: 2 }}>Available Battery Inventory</Typography>
        {availableBatteries.length === 0 ? (
          <EmptyState title="No batteries available" description="All batteries are currently in use or under maintenance." />
        ) : (
          <Grid container spacing={2}>
            {availableBatteries.map(b => (
              <Grid key={b.id} size={{ xs: 6, sm: 4, md: 3 }}>
                <Card
                  variant="outlined"
                  sx={{ p: 2, textAlign: 'center', borderRadius: 3 }}
                >
                  <Typography variant="subtitle2" fontWeight={600}>{b.id}</Typography>
                  <Typography variant="h3" sx={{ my: 1, color: theme.palette.success.main }}>{b.currentCharge}%</Typography>
                  <Typography variant="caption" color="text.secondary">{b.capacity} kWh</Typography>
                  <LinearProgress
                    variant="determinate"
                    value={b.currentCharge}
                    sx={{
                      mt: 1.5,
                      height: 6,
                      borderRadius: 3,
                      bgcolor: alpha(theme.palette.success.main, 0.1),
                      '& .MuiLinearProgress-bar': { borderRadius: 3, bgcolor: theme.palette.success.main },
                    }}
                  />
                </Card>
              </Grid>
            ))}
          </Grid>
        )}
      </Card>
    </Box>
  );
}
