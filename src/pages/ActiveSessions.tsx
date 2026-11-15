import { useState, useMemo, useEffect } from 'react';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Chip from '@mui/material/Chip';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import LinearProgress from '@mui/material/LinearProgress';
import Tooltip from '@mui/material/Tooltip';
import ToggleButton from '@mui/material/ToggleButton';
import ToggleButtonGroup from '@mui/material/ToggleButtonGroup';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import CircularProgress from '@mui/material/CircularProgress';
import { useTheme, alpha } from '@mui/material/styles';

import CheckCircleOutline from '@mui/icons-material/CheckCircleOutline';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';
import HistoryOutlined from '@mui/icons-material/HistoryOutlined';
import ReceiptLongOutlined from '@mui/icons-material/ReceiptLongOutlined';
import StopCircleOutlined from '@mui/icons-material/StopCircleOutlined';

import PageHeader from '@/components/common/PageHeader';
import EmptyState from '@/components/common/EmptyState';
import ReceiptModal from '@/components/common/ReceiptModal';
import { useApp } from '@/context/AppContext';
import type { ChargingSession, Transaction } from '@/types';

export default function ActiveSessions() {
  const theme = useTheme();
  const { sessions, transactions, completeSession, refreshBackendData, currentUser } = useApp();
  const [filter, setFilter] = useState<'active' | 'completed' | 'all'>('active');

  // Confirmation dialog
  const [stoppingSession, setStoppingSession] = useState<ChargingSession | null>(null);
  const [stopLoading, setStopLoading] = useState(false);

  // Receipt Modal
  const [receiptTxn, setReceiptTxn] = useState<Transaction | null>(null);
  const [receiptModel, setReceiptModel] = useState<string>('');

  // Auto-refresh active sessions periodically
  useEffect(() => {
    const hasActive = sessions.some(s => s.status === 'active');
    if (!hasActive) return;

    const interval = setInterval(() => {
      refreshBackendData();
    }, 6000);

    return () => clearInterval(interval);
  }, [sessions, refreshBackendData]);

  const activeSessions = useMemo(() => sessions.filter(s => s.status === 'active'), [sessions]);
  const completedSessions = useMemo(() => sessions.filter(s => s.status === 'completed'), [sessions]);

  const displayedSessions = useMemo(() => {
    if (filter === 'active') return activeSessions;
    if (filter === 'completed') return completedSessions;
    return sessions;
  }, [filter, activeSessions, completedSessions, sessions]);

  const formatTime = (iso?: string | null) => {
    if (!iso) return '-';
    return new Date(iso).toLocaleTimeString('en-IN', { hour: '2-digit', minute: '2-digit' });
  };

  const getDuration = (start: string, end?: string | null) => {
    const startTime = new Date(start).getTime();
    const endTime = end ? new Date(end).getTime() : Date.now();
    const diff = Math.max(0, endTime - startTime);
    const h = Math.floor(diff / 3600000);
    const m = Math.round((diff % 3600000) / 60000);
    return h > 0 ? `${h}h ${m}m` : `${m}m`;
  };

  const handleConfirmStop = async () => {
    if (!stoppingSession) return;
    setStopLoading(true);

    try {
      const res = await completeSession(stoppingSession.id);
      setStopLoading(false);
      const evModel = stoppingSession.evModel;
      setStoppingSession(null);

      if (res && res.success && res.transaction) {
        setReceiptModel(evModel);
        setReceiptTxn({
          id: res.transaction.id,
          evId: res.transaction.evId,
          type: 'charging',
          stationOrBattery: res.transaction.stationOrBattery || stoppingSession.stationId,
          date: res.transaction.date || new Date().toISOString(),
          amount: res.transaction.amount,
          energyDelivered: res.transaction.energyDelivered,
          duration: res.transaction.duration,
          status: 'completed',
          sessionId: stoppingSession.id,
        });
      } else if (res && res.id) {
        // Fallback local transaction returned
        setReceiptModel(evModel);
        setReceiptTxn(res);
      }
    } catch (e) {
      setStopLoading(false);
      setStoppingSession(null);
    }
  };

  const openReceiptForCompletedSession = (session: ChargingSession) => {
    // Try to find matching transaction
    const matchedTxn = transactions.find(t => t.sessionId === session.id || (t.evId === session.evId && t.type === 'charging'));
    if (matchedTxn) {
      setReceiptModel(session.evModel);
      setReceiptTxn(matchedTxn);
    } else {
      // Synthesize receipt from completed session
      setReceiptModel(session.evModel);
      setReceiptTxn({
        id: `TXN-${session.id.replace('SES-', '')}`,
        evId: session.evId,
        type: 'charging',
        stationOrBattery: session.stationId,
        date: session.endTime || session.startTime,
        amount: session.estimatedCost,
        energyDelivered: session.energyConsumed || 0,
        duration: Math.round(
          (new Date(session.endTime || Date.now()).getTime() - new Date(session.startTime).getTime()) / 60000
        ),
        status: 'completed',
        sessionId: session.id,
      });
    }
  };

  return (
    <Box>
      <PageHeader
        title="Charging Sessions"
        subtitle="Monitor live constant-power charging sessions and access official receipts"
        action={
          <ToggleButtonGroup
            value={filter}
            exclusive
            onChange={(_, val) => val && setFilter(val)}
            size="small"
          >
            <ToggleButton value="active" sx={{ px: 2, gap: 1 }}>
              <ElectricBoltOutlined fontSize="small" />
              Active ({activeSessions.length})
            </ToggleButton>
            <ToggleButton value="completed" sx={{ px: 2, gap: 1 }}>
              <HistoryOutlined fontSize="small" />
              Completed ({completedSessions.length})
            </ToggleButton>
            <ToggleButton value="all" sx={{ px: 2 }}>
              All ({sessions.length})
            </ToggleButton>
          </ToggleButtonGroup>
        }
      />

      <Card sx={{ border: `1px solid ${theme.palette.divider}` }}>
        {displayedSessions.length === 0 ? (
          <EmptyState
            title={filter === 'active' ? "No active charging sessions" : "No sessions found"}
            description={
              filter === 'active'
                ? "All charging bays are currently idle. Start a session from Charging Stations."
                : "No charging session history matches this filter."
            }
          />
        ) : (
          <TableContainer>
            <Table>
              <TableHead>
                <TableRow>
                  <TableCell>Session ID</TableCell>
                  <TableCell>EV ID</TableCell>
                  <TableCell>Vehicle Model</TableCell>
                  <TableCell>Station / Bay</TableCell>
                  <TableCell>Start Time</TableCell>
                  <TableCell>Battery Progress</TableCell>
                  <TableCell align="right">Target</TableCell>
                  <TableCell>Duration</TableCell>
                  <TableCell align="right">Delivered (kWh)</TableCell>
                  <TableCell align="right">Amount</TableCell>
                  <TableCell align="center">Actions</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {displayedSessions.map(s => {
                  const currentSoC = s.currentCharge ?? (s.batteryEnd ?? s.batteryStart);
                  const isActive = s.status === 'active';

                  return (
                    <TableRow key={s.id} hover>
                      <TableCell sx={{ fontWeight: 500 }}>{s.id}</TableCell>
                      <TableCell sx={{ fontWeight: 600 }}>{s.evId}</TableCell>
                      <TableCell>{s.evModel}</TableCell>
                      <TableCell sx={{ fontWeight: 600, color: theme.palette.primary.main }}>
                        {s.stationId} {s.bayId ? `(${s.bayId})` : ''}
                      </TableCell>
                      <TableCell>{formatTime(s.startTime)}</TableCell>
                      <TableCell sx={{ minWidth: 150 }}>
                        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                          <LinearProgress
                            variant="determinate"
                            value={currentSoC}
                            sx={{
                              flex: 1,
                              height: 8,
                              borderRadius: 4,
                              bgcolor: alpha(theme.palette.info.main, 0.1),
                              '& .MuiLinearProgress-bar': {
                                borderRadius: 4,
                                bgcolor:
                                  currentSoC < 30
                                    ? theme.palette.error.main
                                    : currentSoC < 70
                                    ? theme.palette.warning.main
                                    : theme.palette.success.main,
                              },
                            }}
                          />
                          <Typography variant="caption" fontWeight={700} sx={{ minWidth: 36 }}>
                            {Math.round(currentSoC)}%
                          </Typography>
                        </Box>
                      </TableCell>
                      <TableCell align="right">{s.targetCharge}%</TableCell>
                      <TableCell>{getDuration(s.startTime, s.endTime)}</TableCell>
                      <TableCell align="right" sx={{ fontWeight: 500 }}>
                        {(s.energyConsumed || 0).toFixed(1)}
                      </TableCell>
                      <TableCell align="right" sx={{ fontWeight: 700, color: 'primary.main' }}>
                        ₹{(s.estimatedCost || 0).toFixed(2)}
                      </TableCell>
                      <TableCell align="center">
                        {isActive ? (
                          <Tooltip title="Stop charging and compute final bill">
                            <Button
                              size="small"
                              variant="contained"
                              color="error"
                              startIcon={<StopCircleOutlined />}
                              onClick={() => setStoppingSession(s)}
                            >
                              Stop
                            </Button>
                          </Tooltip>
                        ) : (
                          <Button
                            size="small"
                            variant="outlined"
                            startIcon={<ReceiptLongOutlined />}
                            onClick={() => openReceiptForCompletedSession(s)}
                          >
                            Receipt
                          </Button>
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

      {/* Stop Charging Confirmation Dialog */}
      <Dialog
        open={!!stoppingSession}
        onClose={() => !stopLoading && setStoppingSession(null)}
        maxWidth="xs"
        fullWidth
      >
        <DialogTitle sx={{ fontWeight: 700 }}>
          Stop Charging Session?
        </DialogTitle>
        <DialogContent sx={{ pt: 1 }}>
          <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
            Completing this session will stop power delivery at <strong>{stoppingSession?.stationId}</strong>, update your vehicle's battery level in the database, calculate the final tariff, and release the bay for waiting vehicles in the FIFO queue.
          </Typography>

          {stoppingSession && (
            <Card sx={{ p: 2, bgcolor: alpha(theme.palette.background.default, 0.6), border: `1px solid ${theme.palette.divider}` }}>
              <Box sx={{ display: 'flex', justifyContent: 'space-between', mb: 1 }}>
                <Typography variant="caption" color="text.secondary">Vehicle:</Typography>
                <Typography variant="body2" fontWeight={600}>{stoppingSession.evModel} ({stoppingSession.evId})</Typography>
              </Box>
              <Box sx={{ display: 'flex', justifyContent: 'space-between', mb: 1 }}>
                <Typography variant="caption" color="text.secondary">Battery Level:</Typography>
                <Typography variant="body2" fontWeight={600}>
                  {stoppingSession.batteryStart}% → {stoppingSession.targetCharge}%
                </Typography>
              </Box>
              <Box sx={{ display: 'flex', justifyContent: 'space-between', mb: 1 }}>
                <Typography variant="caption" color="text.secondary">Estimated Energy:</Typography>
                <Typography variant="body2" fontWeight={600}>
                  {(stoppingSession.energyConsumed || 0).toFixed(1)} kWh
                </Typography>
              </Box>
              <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                <Typography variant="caption" color="text.secondary">Bill Amount:</Typography>
                <Typography variant="body2" fontWeight={700} color="primary.main">
                  ₹{(stoppingSession.estimatedCost || 0).toFixed(2)}
                </Typography>
              </Box>
            </Card>
          )}
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2 }}>
          <Button onClick={() => setStoppingSession(null)} disabled={stopLoading}>
            Cancel
          </Button>
          <Button
            variant="contained"
            color="error"
            onClick={handleConfirmStop}
            disabled={stopLoading}
            startIcon={stopLoading ? <CircularProgress size={18} color="inherit" /> : <StopCircleOutlined />}
          >
            {stopLoading ? 'Stopping...' : 'Confirm & Complete'}
          </Button>
        </DialogActions>
      </Dialog>

      {/* Official Receipt Modal */}
      <ReceiptModal
        open={!!receiptTxn}
        onClose={() => setReceiptTxn(null)}
        transaction={receiptTxn}
        evModel={receiptModel}
      />
    </Box>
  );
}
