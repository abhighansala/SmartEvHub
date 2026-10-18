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
import LinearProgress from '@mui/material/LinearProgress';
import Tooltip from '@mui/material/Tooltip';
import { useTheme, alpha } from '@mui/material/styles';

import Chip from '@mui/material/Chip';
import CheckCircleOutline from '@mui/icons-material/CheckCircleOutline';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';
import BatteryChargingFullOutlined from '@mui/icons-material/BatteryChargingFullOutlined';

import PageHeader from '@/components/common/PageHeader';
import EmptyState from '@/components/common/EmptyState';
import { useApp } from '@/context/AppContext';
import { calculateChargingTime } from '@/utils/chargingCalc';

export default function ActiveSessions() {
  const theme = useTheme();
  const { sessions, evs, stations, completeSession } = useApp();

  const formatTime = (iso: string) =>
    new Date(iso).toLocaleTimeString('en-IN', { hour: '2-digit', minute: '2-digit' });

  const getDuration = (start: string) => {
    const diff = Date.now() - new Date(start).getTime();
    const totalMinutes = Math.round(diff / 60000);
    const safeMinutes = (totalMinutes >= 0 && totalMinutes < 300) ? totalMinutes : 35;
    const h = Math.floor(safeMinutes / 60);
    const m = safeMinutes % 60;
    return h > 0 ? `${h}h ${m}m` : `${m}m`;
  };

  return (
    <Box>
      <PageHeader
        title="Active Sessions"
        subtitle="Monitor and manage ongoing charging sessions"
      />

      <Box sx={{ mb: 2.5, p: 2, borderRadius: 2.5, bgcolor: alpha(theme.palette.info.main, 0.08), border: `1px solid ${alpha(theme.palette.info.main, 0.25)}`, display: 'flex', alignItems: 'center', justifyContent: 'space-between', flexWrap: 'wrap', gap: 1.5 }}>
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1.5 }}>
          <ElectricBoltOutlined color="info" sx={{ animation: 'pulse 1.5s infinite' }} />
          <Box>
            <Typography variant="subtitle2" fontWeight={700} color="text.primary">
              Live Charging Simulation Active
            </Typography>
            <Typography variant="caption" color="text.secondary">
              Vehicles are actively drawing power from 20–30 kW stations. Battery charge and energy delivered update live.
            </Typography>
          </Box>
        </Box>
        <Chip label="SIMULATION ACTIVE" color="success" size="small" sx={{ fontWeight: 700, fontSize: '0.75rem' }} />
      </Box>

      <Card sx={{ border: `1px solid ${theme.palette.divider}` }}>
        {sessions.length === 0 ? (
          <EmptyState
            title="No active sessions"
            description="There are no ongoing charging sessions at this time."
          />
        ) : (
          <TableContainer>
            <Table>
              <TableHead>
                <TableRow>
                  <TableCell>Session ID</TableCell>
                  <TableCell>EV</TableCell>
                  <TableCell>Vehicle</TableCell>
                  <TableCell>Station</TableCell>
                  <TableCell>Start</TableCell>
                  <TableCell sx={{ minWidth: 200 }}>Battery Progress</TableCell>
                  <TableCell align="right">Target</TableCell>
                  <TableCell>Est. Remaining</TableCell>
                  <TableCell>Duration</TableCell>
                  <TableCell align="right">Est. Cost</TableCell>
                  <TableCell align="center">Actions</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {sessions.map(s => {
                  const ev = evs.find(e => e.id === s.evId);
                  const station = stations.find(st => st.id === s.stationId);
                  const stationPower = station?.power || 25;
                  const currentProgress = ev ? ev.currentCharge : s.batteryStart;
                  const deliveredKwh = ev
                    ? Math.round(Math.max(0, ((currentProgress - s.batteryStart) / 100) * ev.batteryCapacity) * 10) / 10
                    : 0;
                  const remainingKwh = ev
                    ? Math.max(0, Number((((s.targetCharge - currentProgress) / 100) * ev.batteryCapacity).toFixed(1)))
                    : 0;
                  const estTimeRemaining = calculateChargingTime(remainingKwh, stationPower);
                  const isTargetReached = currentProgress >= s.targetCharge;

                  return (
                    <TableRow key={s.id} hover>
                      <TableCell sx={{ fontWeight: 500 }}>{s.id}</TableCell>
                      <TableCell sx={{ fontWeight: 600 }}>{s.evId}</TableCell>
                      <TableCell>{s.evModel}</TableCell>
                      <TableCell>
                        <Typography variant="body2" fontWeight={600}>{s.stationId}</Typography>
                        {station && (
                          <Typography variant="caption" color="text.secondary">{station.power} kW</Typography>
                        )}
                      </TableCell>
                      <TableCell>{formatTime(s.startTime)}</TableCell>
                      <TableCell sx={{ minWidth: 200 }}>
                        <Box sx={{ display: 'flex', flexDirection: 'column', gap: 0.5 }}>
                          <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                            <LinearProgress
                              variant="determinate"
                              value={currentProgress}
                              sx={{
                                flex: 1, height: 8, borderRadius: 4,
                                bgcolor: alpha(theme.palette.info.main, 0.1),
                                '& .MuiLinearProgress-bar': {
                                  borderRadius: 4,
                                  bgcolor: currentProgress < 30 ? theme.palette.error.main : currentProgress < 60 ? theme.palette.warning.main : theme.palette.success.main,
                                  transition: 'transform 0.4s ease-in-out',
                                },
                              }}
                            />
                            <Box sx={{ display: 'flex', alignItems: 'center', gap: 0.5, minWidth: 50 }}>
                              <ElectricBoltOutlined
                                sx={{
                                  fontSize: 16,
                                  color: isTargetReached ? theme.palette.success.main : theme.palette.info.main,
                                  animation: isTargetReached ? 'none' : 'pulse 1.5s infinite',
                                }}
                              />
                              <Typography variant="caption" fontWeight={700}>
                                {Math.round(currentProgress)}%
                              </Typography>
                            </Box>
                          </Box>
                          <Typography variant="caption" color="text.secondary">
                            Started: {s.batteryStart}% • +{deliveredKwh} kWh delivered
                          </Typography>
                        </Box>
                      </TableCell>
                      <TableCell align="right">
                        <Chip
                          label={`${s.targetCharge}%`}
                          size="small"
                          color={isTargetReached ? 'success' : 'default'}
                          variant={isTargetReached ? 'filled' : 'outlined'}
                        />
                      </TableCell>
                      <TableCell>
                        {isTargetReached ? (
                          <Chip label="Ready" size="small" color="success" sx={{ fontWeight: 700 }} />
                        ) : (
                          <Box>
                            <Typography variant="body2" fontWeight={700} color="primary.main">
                              ~{estTimeRemaining.formatted}
                            </Typography>
                            <Typography variant="caption" color="text.secondary">
                              @ {stationPower} kW/hr
                            </Typography>
                          </Box>
                        )}
                      </TableCell>
                      <TableCell>{getDuration(s.startTime)}</TableCell>
                      <TableCell align="right" sx={{ fontWeight: 500 }}>₹{s.estimatedCost.toFixed(2)}</TableCell>
                      <TableCell align="center">
                        <Tooltip title={isTargetReached ? 'Target reached — ready to finish' : 'Complete session'}>
                          <Button
                            size="small"
                            variant={isTargetReached ? 'contained' : 'outlined'}
                            color="success"
                            startIcon={<CheckCircleOutline />}
                            onClick={() => completeSession(s.id)}
                          >
                            {isTargetReached ? 'Finish' : 'Complete'}
                          </Button>
                        </Tooltip>
                      </TableCell>
                    </TableRow>
                  );
                })}
              </TableBody>
            </Table>
          </TableContainer>
        )}
      </Card>
      <style>{`
        @keyframes pulse {
          0%, 100% { opacity: 1; transform: scale(1); }
          50% { opacity: 0.4; transform: scale(1.15); }
        }
      `}</style>
    </Box>
  );
}
