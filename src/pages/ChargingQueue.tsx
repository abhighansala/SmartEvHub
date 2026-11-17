import React, { useState, useMemo, useEffect } from 'react';
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
import Slider from '@mui/material/Slider';
import Grid from '@mui/material/Grid';
import Tabs from '@mui/material/Tabs';
import Tab from '@mui/material/Tab';
import Stack from '@mui/material/Stack';
import { useTheme, alpha } from '@mui/material/styles';

import PlayArrowOutlined from '@mui/icons-material/PlayArrowOutlined';
import AddOutlined from '@mui/icons-material/AddOutlined';
import DeleteOutline from '@mui/icons-material/DeleteOutline';
import ArrowForwardOutlined from '@mui/icons-material/ArrowForwardOutlined';
import CancelOutlined from '@mui/icons-material/CancelOutlined';
import EvStationOutlined from '@mui/icons-material/EvStationOutlined';
import ElectricCarOutlined from '@mui/icons-material/ElectricCarOutlined';
import AccessTimeOutlined from '@mui/icons-material/AccessTimeOutlined';
import CheckCircleOutlined from '@mui/icons-material/CheckCircleOutlined';
import HourglassEmptyOutlined from '@mui/icons-material/HourglassEmptyOutlined';
import FlashOnOutlined from '@mui/icons-material/FlashOnOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import EmptyState from '@/components/common/EmptyState';
import { useApp } from '@/context/AppContext';
import type { ChargingRequest } from '@/types';

export default function ChargingQueue() {
  const theme = useTheme();
  const {
    chargingQueue,
    chargingQueueMeta,
    chargingRequests,
    processNextCharging,
    removeChargingRequest,
    cancelChargingRequest,
    allocateBayManually,
    addChargingRequest,
    evs,
    stations,
    currentUser,
    refreshBackendData,
    showSnackbar,
  } = useApp();

  // Real-time synchronization for FIFO queue positions and automatic bay allocations
  useEffect(() => {
    const shouldPoll = chargingRequests.some(r => r.status === 'queued' || r.status === 'allocated') ||
      chargingQueue.length > 0;
    if (!shouldPoll) return;

    const interval = setInterval(() => {
      refreshBackendData();
    }, 3000);

    return () => clearInterval(interval);
  }, [chargingRequests, chargingQueue, refreshBackendData]);

  const isOperator = currentUser?.role === 'operator' || currentUser?.role === 'admin';

  // State
  const [operatorTab, setOperatorTab] = useState<number>(0);
  const [dialogOpen, setDialogOpen] = useState<boolean>(false);
  const [selectedEvId, setSelectedEvId] = useState<string>('');
  const [selectedStationId, setSelectedStationId] = useState<string>('any');
  const [targetCharge, setTargetCharge] = useState<number>(85);
  const [priority, setPriority] = useState<'normal' | 'urgent'>('normal');

  // Manual allocation dialog for operators
  const [allocDialogOpen, setAllocDialogOpen] = useState<boolean>(false);
  const [allocRequestId, setAllocRequestId] = useState<string>('');
  const [allocStationId, setAllocStationId] = useState<string>('');

  // Eligible EVs for the current user
  const selectableEvs = useMemo(() => {
    if (isOperator) return evs;
    if (!currentUser) return evs;
    return evs.filter(e => !e.userId || e.userId === currentUser.id);
  }, [evs, isOperator, currentUser]);

  // Customer-scoped requests
  const userRequests = useMemo(() => {
    if (isOperator) return chargingRequests;
    if (!currentUser) return chargingRequests;
    return chargingRequests.filter(r => {
      if (r.userId && r.userId === currentUser.id) return true;
      return selectableEvs.some(e => e.id === r.evId || e.id === r.vehicleId);
    });
  }, [chargingRequests, isOperator, currentUser, selectableEvs]);

  // KPIs for customer
  const userQueuedCount = useMemo(() => {
    return userRequests.filter(r => r.status === 'queued').length;
  }, [userRequests]);

  const userActiveCount = useMemo(() => {
    return userRequests.filter(r => r.status === 'allocated' || r.status === 'active').length;
  }, [userRequests]);

  // Estimated wait time helper
  const getWaitTime = (requestedAt?: string) => {
    if (!requestedAt) return 0;
    const diff = Date.now() - new Date(requestedAt).getTime();
    if (isNaN(diff) || diff < 0) return 0;
    return Math.round(diff / 60000);
  };

  // Submit new charging request
  const handleRequest = async () => {
    if (!selectedEvId) return;
    const ev = evs.find(e => e.id === selectedEvId);

    await addChargingRequest({
      evId: selectedEvId,
      stationId: selectedStationId === 'any' ? undefined : selectedStationId,
      targetCharge,
      evModel: ev?.model || 'Electric Vehicle',
      owner: ev?.owner || currentUser?.name || 'Customer',
      currentCharge: ev?.currentCharge ?? 20,
      priority,
      connectorType: (ev as any)?.connectorType || 'CCS2',
    });

    setDialogOpen(false);
    setSelectedEvId('');
    setSelectedStationId('any');
    setTargetCharge(85);
    setPriority('normal');
  };

  // User cancel request
  const handleCancelRequest = async (requestId: string) => {
    await cancelChargingRequest(requestId);
  };

  // Operator manual allocation
  const handleManualAllocate = async () => {
    if (!allocRequestId || !allocStationId) return;
    await allocateBayManually(allocRequestId, allocStationId);
    setAllocDialogOpen(false);
    setAllocRequestId('');
    setAllocStationId('');
  };

  return (
    <Box>
      <PageHeader
        title="Charging Queue"
        subtitle={
          isOperator
            ? "FIFO Circular Queue management, bay allocations, and real-time buffer state"
            : "Track your vehicle charging requests, queue positions, and bay allocations"
        }
        action={
          <Box sx={{ display: 'flex', gap: 1.5 }}>
            <Button
              variant={isOperator ? "outlined" : "contained"}
              startIcon={<AddOutlined />}
              onClick={() => {
                if (selectableEvs.length > 0) {
                  setSelectedEvId(selectableEvs[0].id);
                }
                setDialogOpen(true);
              }}
            >
              Request Charging
            </Button>
            {isOperator && (
              <Button
                variant="contained"
                startIcon={<PlayArrowOutlined />}
                onClick={processNextCharging}
                disabled={chargingQueue.length === 0}
              >
                Process Next (Dequeue)
              </Button>
            )}
          </Box>
        }
      />

      {/* ─────────────────────────────────────────────────────────────
          NORMAL USER VIEW (Customer Portal)
      ───────────────────────────────────────────────────────────── */}
      {!isOperator && (
        <Box>
          {/* User KPI Summary Cards */}
          <Grid container spacing={2.5} sx={{ mb: 3 }}>
            <Grid item xs={12} sm={4}>
              <Card sx={{ p: 2.5, border: `1px solid ${theme.palette.divider}`, display: 'flex', alignItems: 'center', gap: 2 }}>
                <Box
                  sx={{
                    p: 1.5,
                    borderRadius: 2,
                    bgcolor: alpha(theme.palette.warning.main, 0.1),
                    color: 'warning.main',
                    display: 'flex',
                  }}
                >
                  <HourglassEmptyOutlined />
                </Box>
                <Box>
                  <Typography variant="caption" color="text.secondary" fontWeight={600} textTransform="uppercase">
                    Waiting in Queue
                  </Typography>
                  <Typography variant="h5" fontWeight={700}>
                    {userQueuedCount} {userQueuedCount === 1 ? 'Vehicle' : 'Vehicles'}
                  </Typography>
                </Box>
              </Card>
            </Grid>

            <Grid item xs={12} sm={4}>
              <Card sx={{ p: 2.5, border: `1px solid ${theme.palette.divider}`, display: 'flex', alignItems: 'center', gap: 2 }}>
                <Box
                  sx={{
                    p: 1.5,
                    borderRadius: 2,
                    bgcolor: alpha(theme.palette.success.main, 0.1),
                    color: 'success.main',
                    display: 'flex',
                  }}
                >
                  <FlashOnOutlined />
                </Box>
                <Box>
                  <Typography variant="caption" color="text.secondary" fontWeight={600} textTransform="uppercase">
                    Allocated / Charging
                  </Typography>
                  <Typography variant="h5" fontWeight={700}>
                    {userActiveCount} {userActiveCount === 1 ? 'Bay' : 'Bays'}
                  </Typography>
                </Box>
              </Card>
            </Grid>

            <Grid item xs={12} sm={4}>
              <Card sx={{ p: 2.5, border: `1px solid ${theme.palette.divider}`, display: 'flex', alignItems: 'center', gap: 2 }}>
                <Box
                  sx={{
                    p: 1.5,
                    borderRadius: 2,
                    bgcolor: alpha(theme.palette.primary.main, 0.1),
                    color: 'primary.main',
                    display: 'flex',
                  }}
                >
                  <ElectricCarOutlined />
                </Box>
                <Box>
                  <Typography variant="caption" color="text.secondary" fontWeight={600} textTransform="uppercase">
                    My Registered EVs
                  </Typography>
                  <Typography variant="h5" fontWeight={700}>
                    {selectableEvs.length}
                  </Typography>
                </Box>
              </Card>
            </Grid>
          </Grid>

          {/* User Requests Table */}
          <Card sx={{ border: `1px solid ${theme.palette.divider}` }}>
            {userRequests.length === 0 ? (
              <EmptyState
                title="No Active or Queued Charging Requests"
                description="You do not have any vehicles currently in the charging queue. Click 'Request Charging' to get in line or allocate an available bay."
                actionLabel="Request Charging"
                onAction={() => {
                  if (selectableEvs.length > 0) {
                    setSelectedEvId(selectableEvs[0].id);
                  }
                  setDialogOpen(true);
                }}
              />
            ) : (
              <TableContainer>
                <Table>
                  <TableHead>
                    <TableRow>
                      <TableCell>Queue Position / Allocation</TableCell>
                      <TableCell>Request ID</TableCell>
                      <TableCell>Vehicle</TableCell>
                      <TableCell>Connector</TableCell>
                      <TableCell align="right">Target SoC</TableCell>
                      <TableCell>Assigned Bay</TableCell>
                      <TableCell align="right">Wait Time</TableCell>
                      <TableCell>Status</TableCell>
                      <TableCell align="center">Action</TableCell>
                    </TableRow>
                  </TableHead>
                  <TableBody>
                    {userRequests.map((req) => {
                      const isQueued = req.status === 'queued';
                      const isAllocated = req.status === 'allocated' || req.status === 'active';
                      const isDone = req.status === 'completed';

                      return (
                        <TableRow key={req.id} hover>
                          <TableCell>
                            {isQueued ? (
                              <Chip
                                icon={<HourglassEmptyOutlined />}
                                label={`Position #${req.queuePosition || 1} in FIFO Queue`}
                                color="warning"
                                size="small"
                                sx={{ fontWeight: 600 }}
                              />
                            ) : isAllocated ? (
                              <Chip
                                icon={<CheckCircleOutlined />}
                                label="Bay Allocated"
                                color="success"
                                size="small"
                                sx={{ fontWeight: 600 }}
                              />
                            ) : (
                              <Typography variant="body2" color="text.secondary">—</Typography>
                            )}
                          </TableCell>
                          <TableCell sx={{ fontWeight: 600 }}>{req.id}</TableCell>
                          <TableCell>
                            <Typography variant="subtitle2" fontWeight={600}>
                              {req.evId || req.vehicleId}
                            </Typography>
                            <Typography variant="caption" color="text.secondary">
                              {req.evModel || 'Electric Vehicle'}
                            </Typography>
                          </TableCell>
                          <TableCell>
                            <Chip
                              label={req.connectorType || 'CCS2'}
                              size="small"
                              variant="outlined"
                              sx={{ fontWeight: 600 }}
                            />
                          </TableCell>
                          <TableCell align="right" sx={{ fontWeight: 600 }}>
                            {req.targetCharge || 85}%
                          </TableCell>
                          <TableCell>
                            {req.stationId ? (
                              <Typography variant="body2" fontWeight={600} color="primary.main">
                                {req.stationId} {req.bayId ? `(${req.bayId})` : ''}
                              </Typography>
                            ) : (
                              <Typography variant="caption" color="text.secondary">
                                Waiting for available bay
                              </Typography>
                            )}
                          </TableCell>
                          <TableCell align="right">
                            {getWaitTime(req.createdAt || req.requestedAt)} min
                          </TableCell>
                          <TableCell>
                            <StatusChip status={req.status || 'queued'} />
                          </TableCell>
                          <TableCell align="center">
                            {isQueued && (
                              <Tooltip title="Cancel this charging request">
                                <Button
                                  size="small"
                                  color="error"
                                  variant="outlined"
                                  startIcon={<CancelOutlined />}
                                  onClick={() => handleCancelRequest(req.id)}
                                >
                                  Cancel
                                </Button>
                              </Tooltip>
                            )}
                          </TableCell>
                        </TableRow>
                      );
                    })}
                  </TableBody>
                </Table>
              </TableContainer>
            )}
          </Card>
        </Box>
      )}

      {/* ─────────────────────────────────────────────────────────────
          OPERATOR / ADMIN VIEW (Full Operational Queue Dashboard)
      ───────────────────────────────────────────────────────────── */}
      {isOperator && (
        <Box>
          <Tabs
            value={operatorTab}
            onChange={(_, val) => setOperatorTab(val)}
            sx={{ mb: 2.5, borderBottom: 1, borderColor: 'divider' }}
          >
            <Tab label="Live FIFO Circular Queue" />
            <Tab label={`All Charging Requests (${chargingRequests.length})`} />
          </Tabs>

          {operatorTab === 0 && (
            <Box>
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
                  <Typography variant="h6" sx={{ mb: 2 }}>Queue Visualization</Typography>
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
                    description="All charging requests have been processed and dispatched to stations."
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
            </Box>
          )}

          {operatorTab === 1 && (
            <Card sx={{ border: `1px solid ${theme.palette.divider}` }}>
              {chargingRequests.length === 0 ? (
                <EmptyState
                  title="No Charging Requests Recorded"
                  description="No customer charging requests have been logged in the SQLite database."
                />
              ) : (
                <TableContainer>
                  <Table>
                    <TableHead>
                      <TableRow>
                        <TableCell>Request ID</TableCell>
                        <TableCell>Vehicle</TableCell>
                        <TableCell>User / Owner</TableCell>
                        <TableCell>Connector</TableCell>
                        <TableCell align="right">Target SoC</TableCell>
                        <TableCell>Assigned Bay</TableCell>
                        <TableCell>Status</TableCell>
                        <TableCell align="center">Operator Actions</TableCell>
                      </TableRow>
                    </TableHead>
                    <TableBody>
                      {chargingRequests.map((req) => (
                        <TableRow key={req.id} hover>
                          <TableCell sx={{ fontWeight: 600 }}>{req.id}</TableCell>
                          <TableCell>
                            <Typography variant="subtitle2" fontWeight={600}>
                              {req.evId || req.vehicleId}
                            </Typography>
                            <Typography variant="caption" color="text.secondary">
                              {req.evModel || 'Electric Vehicle'}
                            </Typography>
                          </TableCell>
                          <TableCell>User #{req.userId || '—'}</TableCell>
                          <TableCell>
                            <Chip label={req.connectorType || 'CCS2'} size="small" variant="outlined" />
                          </TableCell>
                          <TableCell align="right" sx={{ fontWeight: 600 }}>
                            {req.targetCharge || 85}%
                          </TableCell>
                          <TableCell>
                            {req.stationId ? (
                              <Typography variant="body2" fontWeight={600} color="primary.main">
                                {req.stationId} {req.bayId ? `(${req.bayId})` : ''}
                              </Typography>
                            ) : (
                              <Typography variant="caption" color="text.secondary">
                                {req.status === 'queued' ? `Queued (#${req.queuePosition || 1})` : '—'}
                              </Typography>
                            )}
                          </TableCell>
                          <TableCell>
                            <StatusChip status={req.status || 'queued'} />
                          </TableCell>
                          <TableCell align="center">
                            {req.status === 'queued' && (
                              <Stack direction="row" spacing={1} justifyContent="center">
                                <Button
                                  size="small"
                                  variant="outlined"
                                  onClick={() => {
                                    setAllocRequestId(req.id);
                                    const freeSt = stations.find(s => s.status === 'available');
                                    setAllocStationId(freeSt ? freeSt.id : (stations[0]?.id || ''));
                                    setAllocDialogOpen(true);
                                  }}
                                >
                                  Allocate Bay
                                </Button>
                                <IconButton size="small" color="error" onClick={() => handleCancelRequest(req.id)}>
                                  <DeleteOutline fontSize="small" />
                                </IconButton>
                              </Stack>
                            )}
                          </TableCell>
                        </TableRow>
                      ))}
                    </TableBody>
                  </Table>
                </TableContainer>
              )}
            </Card>
          )}
        </Box>
      )}

      {/* ─────────────────────────────────────────────────────────────
          REQUEST CHARGING DIALOG (Unified for User & Operator)
      ───────────────────────────────────────────────────────────── */}
      <Dialog open={dialogOpen} onClose={() => setDialogOpen(false)} maxWidth="xs" fullWidth>
        <DialogTitle sx={{ pb: 1 }}>Request EV Charging</DialogTitle>
        <DialogContent sx={{ display: 'flex', flexDirection: 'column', gap: 2.5, pt: '16px !important' }}>
          <TextField
            select
            label="Select Electric Vehicle"
            value={selectedEvId}
            onChange={e => setSelectedEvId(e.target.value)}
            required
            fullWidth
            helperText={selectableEvs.length === 0 ? "No eligible EVs found. Please register an EV first." : ""}
          >
            {selectableEvs.map(ev => (
              <MenuItem key={ev.id} value={ev.id}>
                {ev.id} — {ev.model} ({ev.currentCharge}%)
              </MenuItem>
            ))}
          </TextField>

          <TextField
            select
            label="Preferred Station / Bay"
            value={selectedStationId}
            onChange={e => setSelectedStationId(e.target.value)}
            fullWidth
          >
            <MenuItem value="any">Any Available Compatible Bay (Fastest)</MenuItem>
            {stations.map(st => (
              <MenuItem key={st.id} value={st.id}>
                {st.name} ({st.id}) — {st.connectorType} ({st.status})
              </MenuItem>
            ))}
          </TextField>

          <Box>
            <Typography variant="body2" fontWeight={600} gutterBottom>
              Target State of Charge (SoC): {targetCharge}%
            </Typography>
            <Slider
              value={targetCharge}
              onChange={(_, val) => setTargetCharge(val as number)}
              min={50}
              max={100}
              step={5}
              marks={[
                { value: 50, label: '50%' },
                { value: 80, label: '80%' },
                { value: 90, label: '90%' },
                { value: 100, label: '100%' },
              ]}
              valueLabelDisplay="auto"
            />
          </Box>

          {isOperator && (
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
          )}
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

      {/* ─────────────────────────────────────────────────────────────
          OPERATOR MANUAL ALLOCATION DIALOG
      ───────────────────────────────────────────────────────────── */}
      <Dialog open={allocDialogOpen} onClose={() => setAllocDialogOpen(false)} maxWidth="xs" fullWidth>
        <DialogTitle>Manual Bay Allocation</DialogTitle>
        <DialogContent sx={{ display: 'flex', flexDirection: 'column', gap: 2, pt: '16px !important' }}>
          <Typography variant="body2" color="text.secondary">
            Assign request <strong>{allocRequestId}</strong> to an available charging bay:
          </Typography>
          <TextField
            select
            label="Charging Station"
            value={allocStationId}
            onChange={e => setAllocStationId(e.target.value)}
            fullWidth
          >
            {stations.map(st => (
              <MenuItem key={st.id} value={st.id} disabled={st.status !== 'available'}>
                {st.name} ({st.id}) — {st.connectorType} [{st.status}]
              </MenuItem>
            ))}
          </TextField>
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2 }}>
          <Button onClick={() => setAllocDialogOpen(false)}>Cancel</Button>
          <Button variant="contained" onClick={handleManualAllocate} disabled={!allocStationId}>
            Allocate Bay
          </Button>
        </DialogActions>
      </Dialog>
    </Box>
  );
}
