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

import CheckCircleOutline from '@mui/icons-material/CheckCircleOutline';

import PageHeader from '@/components/common/PageHeader';
import EmptyState from '@/components/common/EmptyState';
import { useApp } from '@/context/AppContext';

export default function ActiveSessions() {
  const theme = useTheme();
  const { sessions, completeSession } = useApp();

  const formatTime = (iso: string) =>
    new Date(iso).toLocaleTimeString('en-IN', { hour: '2-digit', minute: '2-digit' });

  const getDuration = (start: string) => {
    const diff = Date.now() - new Date(start).getTime();
    const h = Math.floor(diff / 3600000);
    const m = Math.round((diff % 3600000) / 60000);
    return h > 0 ? `${h}h ${m}m` : `${m}m`;
  };

  return (
    <Box>
      <PageHeader
        title="Active Sessions"
        subtitle="Monitor and manage ongoing charging sessions"
      />

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
                  <TableCell>Battery</TableCell>
                  <TableCell align="right">Target</TableCell>
                  <TableCell>Duration</TableCell>
                  <TableCell align="right">Est. Cost</TableCell>
                  <TableCell align="center">Actions</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {sessions.map(s => {
                  const progress = Math.min(
                    s.batteryStart + (s.targetCharge - s.batteryStart) * 0.6,
                    s.targetCharge
                  );
                  return (
                    <TableRow key={s.id} hover>
                      <TableCell sx={{ fontWeight: 500 }}>{s.id}</TableCell>
                      <TableCell sx={{ fontWeight: 600 }}>{s.evId}</TableCell>
                      <TableCell>{s.evModel}</TableCell>
                      <TableCell>{s.stationId}</TableCell>
                      <TableCell>{formatTime(s.startTime)}</TableCell>
                      <TableCell sx={{ minWidth: 130 }}>
                        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                          <LinearProgress
                            variant="determinate"
                            value={progress}
                            sx={{
                              flex: 1, height: 8, borderRadius: 4,
                              bgcolor: alpha(theme.palette.info.main, 0.1),
                              '& .MuiLinearProgress-bar': {
                                borderRadius: 4,
                                bgcolor: progress < 30 ? theme.palette.error.main : progress < 60 ? theme.palette.warning.main : theme.palette.success.main,
                              },
                            }}
                          />
                          <Typography variant="caption" fontWeight={600} sx={{ minWidth: 32 }}>
                            {Math.round(progress)}%
                          </Typography>
                        </Box>
                      </TableCell>
                      <TableCell align="right">{s.targetCharge}%</TableCell>
                      <TableCell>{getDuration(s.startTime)}</TableCell>
                      <TableCell align="right" sx={{ fontWeight: 500 }}>₹{s.estimatedCost.toFixed(2)}</TableCell>
                      <TableCell align="center">
                        <Tooltip title="Complete session">
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
