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
import CheckCircleOutline from '@mui/icons-material/CheckCircleOutline';
import WarningAmberOutlined from '@mui/icons-material/WarningAmberOutlined';
import HistoryOutlined from '@mui/icons-material/HistoryOutlined';
import Chip from '@mui/material/Chip';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import StatCard from '@/components/common/StatCard';
import EmptyState from '@/components/common/EmptyState';
import { useApp } from '@/context/AppContext';

export default function BatterySwap() {
  const theme = useTheme();
  const { batteries, evs, swapQueue, addSwapRequest, showSnackbar, transactions } = useApp();
  const [form, setForm] = useState({ evId: '', currentBattery: '', requiredCapacity: '' });

  const availableBatteries = batteries.filter(b => b.status === 'available');
  const swapTransactions = transactions.filter(t => t.type === 'battery-swap');

  const handleEvChange = (selectedEvId: string) => {
    const selectedEv = evs.find(e => e.id === selectedEvId);
    const assignedBat = batteries.find(b => b.currentVehicle === selectedEvId);
    setForm({
      evId: selectedEvId,
      currentBattery: assignedBat ? assignedBat.id : (form.currentBattery || 'BAT-01'),
      requiredCapacity: selectedEv ? String(selectedEv.batteryCapacity) : (form.requiredCapacity || '40.5'),
    });
  };

  const reqCapNum = Number(form.requiredCapacity) || 40.5;
  const recommendedBattery = form.evId ? (
    availableBatteries.find(b => Math.abs(b.capacity - reqCapNum) < 0.1 && b.currentCharge >= 95) ||
    availableBatteries.find(b => b.capacity >= reqCapNum && b.currentCharge >= 95) ||
    availableBatteries.find(b => b.currentCharge >= 95) ||
    null
  ) : null;

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
                onChange={e => handleEvChange(e.target.value)}
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

            {form.evId && (
              <Box
                sx={{
                  mt: 2.5,
                  p: 2,
                  borderRadius: 2,
                  border: `1px solid ${recommendedBattery ? alpha(theme.palette.success.main, 0.4) : alpha(theme.palette.warning.main, 0.4)}`,
                  bgcolor: recommendedBattery ? alpha(theme.palette.success.main, 0.06) : alpha(theme.palette.warning.main, 0.06),
                  display: 'flex',
                  alignItems: 'center',
                  gap: 1.5,
                }}
              >
                {recommendedBattery ? (
                  <>
                    <CheckCircleOutline sx={{ color: theme.palette.success.main }} />
                    <Box sx={{ flex: 1 }}>
                      <Typography variant="subtitle2" sx={{ fontWeight: 600, color: theme.palette.success.main }}>
                        Compatible Replacement: {recommendedBattery.id} ({recommendedBattery.capacity} kWh, {recommendedBattery.currentCharge}% SoC)
                      </Typography>
                      <Typography variant="caption" color="text.secondary">
                        Healthy replacement pack ready for immediate swap. Old battery will be automatically placed in charging dock.
                      </Typography>
                    </Box>
                    <Chip label="Ready to Swap" size="small" color="success" variant="outlined" sx={{ fontWeight: 600 }} />
                  </>
                ) : (
                  <>
                    <WarningAmberOutlined sx={{ color: theme.palette.warning.main }} />
                    <Box sx={{ flex: 1 }}>
                      <Typography variant="subtitle2" sx={{ fontWeight: 600, color: theme.palette.warning.main }}>
                        No Ready Batteries (≥95% SoC) Available in Inventory
                      </Typography>
                      <Typography variant="caption" color="text.secondary">
                        Submitting this request will automatically queue {form.evId} in the FIFO Swap Queue until dock charging completes.
                      </Typography>
                    </Box>
                    <Chip label="Will Queue (FIFO)" size="small" color="warning" variant="outlined" sx={{ fontWeight: 600 }} />
                  </>
                )}
              </Box>
            )}
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

      {/* Completed Swap History */}
      <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, mb: 3 }}>
        <Typography variant="h5" sx={{ mb: 2, display: 'flex', alignItems: 'center', gap: 1 }}>
          <HistoryOutlined fontSize="small" />
          Completed Swap History
        </Typography>
        {swapTransactions.length === 0 ? (
          <EmptyState title="No swap history" description="Completed battery swaps will appear here with transaction and battery details." />
        ) : (
          <TableContainer>
            <Table size="small">
              <TableHead>
                <TableRow>
                  <TableCell>Transaction ID</TableCell>
                  <TableCell>Vehicle ID</TableCell>
                  <TableCell>Battery / Station</TableCell>
                  <TableCell align="right">Energy Delivered</TableCell>
                  <TableCell align="right">Amount</TableCell>
                  <TableCell>Date & Time</TableCell>
                  <TableCell>Status</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {swapTransactions.map(txn => (
                  <TableRow key={txn.id} hover>
                    <TableCell sx={{ fontWeight: 600 }}>{txn.id}</TableCell>
                    <TableCell sx={{ fontWeight: 500 }}>{txn.evId}</TableCell>
                    <TableCell>{txn.stationOrBattery}</TableCell>
                    <TableCell align="right">{txn.energyDelivered} kWh</TableCell>
                    <TableCell align="right" sx={{ fontWeight: 600, color: theme.palette.success.main }}>
                      ₹{txn.amount.toFixed(2)}
                    </TableCell>
                    <TableCell>
                      {new Date(txn.date).toLocaleDateString('en-IN', {
                        day: 'numeric',
                        month: 'short',
                        hour: '2-digit',
                        minute: '2-digit',
                      })}
                    </TableCell>
                    <TableCell><StatusChip status={txn.status} /></TableCell>
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
