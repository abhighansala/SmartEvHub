import { useState } from 'react';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import IconButton from '@mui/material/IconButton';
import Tooltip from '@mui/material/Tooltip';
import Chip from '@mui/material/Chip';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import TextField from '@mui/material/TextField';
import MenuItem from '@mui/material/MenuItem';
import { useTheme, alpha } from '@mui/material/styles';

import PlayArrowOutlined from '@mui/icons-material/PlayArrowOutlined';
import AddOutlined from '@mui/icons-material/AddOutlined';
import DeleteOutline from '@mui/icons-material/DeleteOutline';
import ArrowForwardOutlined from '@mui/icons-material/ArrowForwardOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import EmptyState from '@/components/common/EmptyState';
import { useApp } from '@/context/AppContext';

export default function ChargingQueue() {
  const theme = useTheme();
  const { chargingQueue, chargingQueueMeta, processNextCharging, removeChargingRequest, addChargingRequest, evs } = useApp();
  const [dialogOpen, setDialogOpen] = useState(false);
  const [selectedEvId, setSelectedEvId] = useState('');
  const [priority, setPriority] = useState<'normal' | 'urgent'>('normal');

  const handleRequest = () => {
    if (!selectedEvId) return;
    const ev = evs.find(e => e.id === selectedEvId);
    addChargingRequest({
      evId: selectedEvId,
      evModel: ev?.model || 'Unknown Model',
      owner: ev?.owner || 'Unknown Owner',
      currentCharge: ev?.currentCharge ?? 20,
      priority,
    });
    setDialogOpen(false);
    setSelectedEvId('');
    setPriority('normal');
  };

  const getWaitTime = (requestedAt: string) => {
    const diff = Date.now() - new Date(requestedAt).getTime();
    return Math.round(diff / 60000);
  };

  return (
    <Box>
      <PageHeader
        title="Charging Queue"
        subtitle="FIFO Circular Queue for charging requests — Abhishek Ghansala (UI + DSA Module)"
        action={
          <Box sx={{ display: 'flex', gap: 1.5 }}>
            <Button
              variant="outlined"
              startIcon={<AddOutlined />}
              onClick={() => setDialogOpen(true)}
            >
              Request Charging
            </Button>
            <Button
              variant="contained"
              startIcon={<PlayArrowOutlined />}
              onClick={processNextCharging}
              disabled={chargingQueue.length === 0}
            >
              Process Next (Dequeue)
            </Button>
          </Box>
        }
      />

      {/* FIFO Label & C++ Queue Pointer State */}
      <Card sx={{ p: 2, mb: 3, border: `1px solid ${theme.palette.divider}`, bgcolor: alpha(theme.palette.primary.main, 0.04) }}>
        <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', flexWrap: 'wrap', gap: 1 }}>
          <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
            <Chip label="C++ FIFO Queue" color="primary" size="small" sx={{ fontWeight: 700 }} />
            <Typography variant="body2" color="text.secondary">
              <strong>First-In-First-Out</strong> — Pure manual C++ circular buffer implementation
            </Typography>
          </Box>
          <Box sx={{ display: 'flex', gap: 1, alignItems: 'center' }}>
            <Chip label={`FRONT Index: ${chargingQueueMeta.frontIndex}`} size="small" color="success" variant="outlined" />
            <Chip label={`REAR Index: ${chargingQueueMeta.rearIndex}`} size="small" color="warning" variant="outlined" />
            <Chip label={`Size: ${chargingQueue.length} / ${chargingQueueMeta.capacity}`} size="small" variant="outlined" />
          </Box>
        </Box>
      </Card>

      {/* Visual Queue */}
      {chargingQueue.length > 0 && (
        <Card sx={{ p: 3, mb: 3, border: `1px solid ${theme.palette.divider}`, overflow: 'auto' }}>
          <Typography variant="h5" sx={{ mb: 2 }}>Queue Visualization</Typography>
          <Box
            sx={{
              display: 'flex',
              alignItems: 'center',
              gap: 0,
              py: 2,
              minWidth: 'fit-content',
            }}
          >
            {/* FRONT label */}
            <Box
              sx={{
                px: 2,
                py: 1,
                borderRadius: 2,
                bgcolor: alpha(theme.palette.success.main, 0.1),
                border: `2px dashed ${theme.palette.success.main}`,
                flexShrink: 0,
              }}
            >
              <Typography variant="caption" fontWeight={700} color="success.main">FRONT</Typography>
            </Box>

            {chargingQueue.map((req, i) => (
              <Box key={req.id} sx={{ display: 'flex', alignItems: 'center' }}>
                <ArrowForwardOutlined sx={{ color: 'text.secondary', mx: 1 }} />
                <Box
                  sx={{
                    px: 2.5,
                    py: 1.5,
                    borderRadius: 3,
                    bgcolor: i === 0
                      ? alpha(theme.palette.primary.main, 0.1)
                      : alpha(theme.palette.background.default, 0.8),
                    border: `1px solid ${i === 0 ? theme.palette.primary.main : theme.palette.divider}`,
                    textAlign: 'center',
                    flexShrink: 0,
                    transition: 'all 0.3s',
                    minWidth: 100,
                  }}
                >
                  <Typography variant="subtitle2" fontWeight={700}>{req.evId}</Typography>
                  <Typography variant="caption" color="text.secondary" sx={{ display: 'block' }}>{req.evModel}</Typography>
                </Box>
              </Box>
            ))}

            <ArrowForwardOutlined sx={{ color: 'text.secondary', mx: 1 }} />

            {/* REAR label */}
            <Box
              sx={{
                px: 2,
                py: 1,
                borderRadius: 2,
                bgcolor: alpha(theme.palette.warning.main, 0.1),
                border: `2px dashed ${theme.palette.warning.main}`,
                flexShrink: 0,
              }}
            >
              <Typography variant="caption" fontWeight={700} color="warning.main">REAR</Typography>
            </Box>
          </Box>
        </Card>
      )}

      {/* Queue Table */}
      <Card sx={{ border: `1px solid ${theme.palette.divider}` }}>
        {chargingQueue.length === 0 ? (
          <EmptyState
            title="Charging queue is empty"
            description="All charging requests have been processed."
          />
        ) : (
          <TableContainer>
            <Table>
              <TableHead>
                <TableRow>
                  <TableCell>Position</TableCell>
                  <TableCell>EV ID</TableCell>
                  <TableCell>Vehicle</TableCell>
                  <TableCell>Owner</TableCell>
                  <TableCell align="right">Battery</TableCell>
                  <TableCell align="right">Wait Time</TableCell>
                  <TableCell>Priority</TableCell>
                  <TableCell align="center">Actions</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {chargingQueue.map((req, i) => (
                  <TableRow key={req.id} hover>
                    <TableCell>
                      <Chip
                        label={i + 1}
                        size="small"
                        color={i === 0 ? 'primary' : 'default'}
                        sx={{ fontWeight: 600, minWidth: 28 }}
                      />
                    </TableCell>
                    <TableCell sx={{ fontWeight: 600 }}>{req.evId}</TableCell>
                    <TableCell>{req.evModel}</TableCell>
                    <TableCell>{req.owner}</TableCell>
                    <TableCell align="right">{req.currentCharge}%</TableCell>
                    <TableCell align="right">{getWaitTime(req.requestedAt)} min</TableCell>
                    <TableCell><StatusChip status={req.priority} /></TableCell>
                    <TableCell align="center">
                      <Tooltip title="Remove from queue">
                        <IconButton size="small" onClick={() => removeChargingRequest(req.id)}>
                          <DeleteOutline fontSize="small" />
                        </IconButton>
                      </Tooltip>
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          </TableContainer>
        )}
      </Card>

      {/* Request Charging Dialog */}
      <Dialog open={dialogOpen} onClose={() => setDialogOpen(false)} maxWidth="xs" fullWidth>
        <DialogTitle>Request Charging</DialogTitle>
        <DialogContent sx={{ display: 'flex', flexDirection: 'column', gap: 2.5, pt: '16px !important' }}>
          <TextField
            select
            label="Select Electric Vehicle"
            value={selectedEvId}
            onChange={e => setSelectedEvId(e.target.value)}
            required
            fullWidth
          >
            {evs.map(ev => (
              <MenuItem key={ev.id} value={ev.id}>
                {ev.id} — {ev.model} ({ev.currentCharge}%)
              </MenuItem>
            ))}
          </TextField>

          <TextField
            select
            label="Priority"
            value={priority}
            onChange={e => setPriority(e.target.value as 'normal' | 'urgent')}
            fullWidth
          >
            <MenuItem value="normal">Normal Priority</MenuItem>
            <MenuItem value="urgent">Urgent Priority</MenuItem>
          </TextField>
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2 }}>
          <Button onClick={() => setDialogOpen(false)}>Cancel</Button>
          <Button
            variant="contained"
            onClick={handleRequest}
            disabled={!selectedEvId}
          >
            Submit Request
          </Button>
        </DialogActions>
      </Dialog>
    </Box>
  );
}
