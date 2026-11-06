import { useState, useMemo } from 'react';
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
import { useTheme, alpha } from '@mui/material/styles';

import CheckCircleOutline from '@mui/icons-material/CheckCircleOutline';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';
import HistoryOutlined from '@mui/icons-material/HistoryOutlined';

import PageHeader from '@/components/common/PageHeader';
import EmptyState from '@/components/common/EmptyState';
import { useApp } from '@/context/AppContext';

export default function ActiveSessions() {
  const theme = useTheme();
  const { sessions, completeSession } = useApp();
  const [filter, setFilter] = useState<'active' | 'completed' | 'all'>('active');

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

  return (
    <Box>
      <PageHeader
        title="Charging Sessions"
        subtitle="Monitor live charging sessions and view historical completed sessions"
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
                  <TableCell>Station</TableCell>
                  <TableCell>Start Time</TableCell>
                  <TableCell>Battery Progress</TableCell>
                  <TableCell align="right">Target</TableCell>
                  <TableCell>Duration</TableCell>
                  <TableCell align="right">Energy (kWh)</TableCell>
                  <TableCell align="right">Est. Cost</TableCell>
                  <TableCell align="center">Status / Actions</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {displayedSessions.map(s => {
                  const progress = s.batteryEnd ?? s.batteryStart;
                  const isActive = s.status === 'active';

                  return (
                    <TableRow key={s.id} hover>
                      <TableCell sx={{ fontWeight: 500 }}>{s.id}</TableCell>
                      <TableCell sx={{ fontWeight: 600 }}>{s.evId}</TableCell>
                      <TableCell>{s.evModel}</TableCell>
                      <TableCell sx={{ fontWeight: 600, color: theme.palette.primary.main }}>
                        {s.stationId}
                      </TableCell>
                      <TableCell>{formatTime(s.startTime)}</TableCell>
                      <TableCell sx={{ minWidth: 140 }}>
                        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                          <LinearProgress
                            variant="determinate"
                            value={progress}
                            sx={{
                              flex: 1,
                              height: 8,
                              borderRadius: 4,
                              bgcolor: alpha(theme.palette.info.main, 0.1),
                              '& .MuiLinearProgress-bar': {
                                borderRadius: 4,
                                bgcolor:
                                  progress < 30
                                    ? theme.palette.error.main
                                    : progress < 70
                                    ? theme.palette.warning.main
                                    : theme.palette.success.main,
                              },
                            }}
                          />
                          <Typography variant="caption" fontWeight={600} sx={{ minWidth: 32 }}>
                            {Math.round(progress)}%
                          </Typography>
                        </Box>
                      </TableCell>
                      <TableCell align="right">{s.targetCharge}%</TableCell>
                      <TableCell>{getDuration(s.startTime, s.endTime)}</TableCell>
                      <TableCell align="right" sx={{ fontWeight: 500 }}>
                        {(s.energyConsumed || 0).toFixed(1)}
                      </TableCell>
                      <TableCell align="right" sx={{ fontWeight: 600 }}>
                        ₹{(s.estimatedCost || 0).toFixed(2)}
                      </TableCell>
                      <TableCell align="center">
                        {isActive ? (
                          <Tooltip title="Complete session and record transaction">
                            <Button
                              size="small"
                              variant="outlined"
                              color="success"
                              startIcon={<CheckCircleOutline />}
                              onClick={() => completeSession(s.id)}
                            >
                              Complete
                            </Button>
                          </Tooltip>
                        ) : (
                          <Chip
                            label="Completed"
                            color="success"
                            size="small"
                            variant="outlined"
                            sx={{ fontWeight: 600 }}
                          />
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
  );
}
