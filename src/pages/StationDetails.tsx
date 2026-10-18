import { useParams, useNavigate } from 'react-router-dom';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import Grid from '@mui/material/Grid2';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import LinearProgress from '@mui/material/LinearProgress';
import Chip from '@mui/material/Chip';
import { useTheme, alpha } from '@mui/material/styles';

import ArrowBackOutlined from '@mui/icons-material/ArrowBackOutlined';
import StopCircleOutlined from '@mui/icons-material/StopCircleOutlined';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';
import CheckCircleOutline from '@mui/icons-material/CheckCircleOutline';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import EmptyState from '@/components/common/EmptyState';
import { useApp } from '@/context/AppContext';
import { calculateChargingTime } from '@/utils/chargingCalc';

export default function StationDetails() {
  const { id } = useParams<{ id: string }>();
  const navigate = useNavigate();
  const theme = useTheme();
  const { stations, sessions, evs, completeSession } = useApp();

  const station = stations.find(s => s.id === id);
  if (!station) {
    return (
      <EmptyState
        title="Station Not Found"
        description={`No station found with ID "${id}".`}
        actionLabel="Back to Stations"
        onAction={() => navigate('/stations')}
      />
    );
  }

  const session = sessions.find(s => s.stationId === station.id);
  const ev = session ? evs.find(e => e.id === session.evId) : null;

  const getDuration = (start: string) => {
    const diff = Date.now() - new Date(start).getTime();
    const totalMinutes = Math.round(diff / 60000);
    const safeMinutes = (totalMinutes >= 0 && totalMinutes < 300) ? totalMinutes : 35;
    const h = Math.floor(safeMinutes / 60);
    const m = safeMinutes % 60;
    return h > 0 ? `${h}h ${m}m` : `${m}m`;
  };

  const progress = ev ? ev.currentCharge : (session ? session.batteryStart : 0);
  const targetCharge = session ? session.targetCharge : 80;
  const remainingKwh = ev && session
    ? Math.max(0, Number((((targetCharge - progress) / 100) * ev.batteryCapacity).toFixed(1)))
    : 0;
  const estRemainingTime = calculateChargingTime(remainingKwh, station.power);
  const deliveredKwh = ev && session
    ? Math.round(Math.max(0, ((progress - session.batteryStart) / 100) * ev.batteryCapacity) * 10) / 10
    : 0;
  const isTargetReached = session ? progress >= session.targetCharge : false;

  return (
    <Box>
      <PageHeader
        title={`${station.name}`}
        subtitle={`${station.id} • ${station.location}`}
        action={
          <Button startIcon={<ArrowBackOutlined />} onClick={() => navigate('/stations')}>
            Back
          </Button>
        }
      />

      <Grid container spacing={2.5}>
        <Grid size={{ xs: 12, md: 5 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
            <Typography variant="h5" sx={{ mb: 2 }}>Station Information</Typography>
            <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
              {[
                ['Station ID', station.id],
                ['Name', station.name],
                ['Power Output', `${station.power} kW (${station.power} kWh/hr Rate)`],
                ['Connector', station.connectorType],
                ['Location', station.location],
              ].map(([label, value]) => (
                <Box key={label} sx={{ display: 'flex', justifyContent: 'space-between' }}>
                  <Typography variant="body2" color="text.secondary">{label}</Typography>
                  <Typography variant="body2" fontWeight={500}>{value}</Typography>
                </Box>
              ))}
              <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                <Typography variant="body2" color="text.secondary">Status</Typography>
                <StatusChip status={station.status} />
              </Box>
              <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                <Typography variant="body2" color="text.secondary">Current EV</Typography>
                <Typography variant="body2" fontWeight={500}>{station.currentEV || 'None'}</Typography>
              </Box>
            </Box>
          </Card>
        </Grid>

        <Grid size={{ xs: 12, md: 7 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, height: '100%' }}>
            {station.status === 'charging' && session ? (
              <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 3 }}>
                <Box
                  sx={{
                    display: 'flex',
                    alignItems: 'center',
                    gap: 1,
                    color: isTargetReached ? theme.palette.success.main : theme.palette.info.main,
                  }}
                >
                  <ElectricBoltOutlined sx={{ fontSize: 28, animation: isTargetReached ? 'none' : 'pulse 1.5s infinite' }} />
                  <Typography variant="h4" color={isTargetReached ? 'success.main' : 'info.main'} fontWeight={700}>
                    {isTargetReached ? 'Target Reached (Ready)' : 'Live Charging in Progress...'}
                  </Typography>
                </Box>

                <Box sx={{ width: '100%', textAlign: 'center' }}>
                  <Typography variant="h1" sx={{ fontSize: '3.5rem', fontWeight: 800, color: isTargetReached ? theme.palette.success.main : theme.palette.info.main }}>
                    {Math.round(progress)}%
                  </Typography>
                  <LinearProgress
                    variant="determinate"
                    value={progress}
                    sx={{
                      mt: 2,
                      height: 14,
                      borderRadius: 7,
                      bgcolor: alpha(theme.palette.info.main, 0.1),
                      '& .MuiLinearProgress-bar': {
                        borderRadius: 7,
                        bgcolor: isTargetReached ? theme.palette.success.main : progress < 30 ? theme.palette.error.main : progress < 60 ? theme.palette.warning.main : theme.palette.info.main,
                        transition: 'transform 0.4s ease-in-out',
                      },
                    }}
                  />
                  <Typography variant="body2" color="text.secondary" sx={{ mt: 1.5, fontWeight: 500 }}>
                    Target: {session.targetCharge}% • Started at {session.batteryStart}% • Power Rate: {station.power} kW ({station.power} kWh/hr delivery)
                  </Typography>
                </Box>

                <Box sx={{ display: 'flex', gap: 3, flexWrap: 'wrap', justifyContent: 'center' }}>
                  <Box sx={{ textAlign: 'center', minWidth: 90 }}>
                    <Typography variant="h5" fontWeight={700}>{session.evId}</Typography>
                    <Typography variant="caption" color="text.secondary">{ev?.model || 'Vehicle'}</Typography>
                  </Box>
                  <Box sx={{ textAlign: 'center', minWidth: 100 }}>
                    <Typography variant="h5" fontWeight={700} color={isTargetReached ? 'success.main' : 'primary.main'}>
                      {isTargetReached ? '✓ Complete' : `~${estRemainingTime.formatted}`}
                    </Typography>
                    <Typography variant="caption" color="text.secondary">Est. Time Remaining</Typography>
                  </Box>
                  <Box sx={{ textAlign: 'center', minWidth: 90 }}>
                    <Typography variant="h5" fontWeight={700} color="info.main">+{deliveredKwh} kWh</Typography>
                    <Typography variant="caption" color="text.secondary">Delivered Energy</Typography>
                  </Box>
                  <Box sx={{ textAlign: 'center', minWidth: 90 }}>
                    <Typography variant="h5" fontWeight={700}>{getDuration(session.startTime)}</Typography>
                    <Typography variant="caption" color="text.secondary">Elapsed Time</Typography>
                  </Box>
                  <Box sx={{ textAlign: 'center', minWidth: 90 }}>
                    <Typography variant="h5" fontWeight={700}>₹{session.estimatedCost.toFixed(0)}</Typography>
                    <Typography variant="caption" color="text.secondary">Est. Cost</Typography>
                  </Box>
                </Box>

                <Box sx={{ display: 'flex', gap: 2 }}>
                  <Button
                    variant={isTargetReached ? 'contained' : 'outlined'}
                    color="success"
                    startIcon={<CheckCircleOutline />}
                    onClick={() => completeSession(session.id)}
                  >
                    {isTargetReached ? 'Finish Session' : 'Complete Session'}
                  </Button>
                  <Button
                    variant="outlined"
                    color="error"
                    startIcon={<StopCircleOutlined />}
                    onClick={() => completeSession(session.id)}
                  >
                    Stop Early
                  </Button>
                </Box>
              </Box>
            ) : (
              <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', height: '100%', gap: 2, py: 4 }}>
                <Box
                  sx={{
                    width: 80,
                    height: 80,
                    borderRadius: '50%',
                    bgcolor: alpha(theme.palette.success.main, 0.1),
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                  }}
                >
                  <Typography variant="h3" sx={{ color: theme.palette.success.main }}>✓</Typography>
                </Box>
                <Typography variant="h4">Station Available</Typography>
                <Typography variant="body2" color="text.secondary">
                  Ready to accept a charging session.
                </Typography>
              </Box>
            )}
          </Card>
        </Grid>
      </Grid>
      <style>{`
        @keyframes pulse {
          0%, 100% { opacity: 1; }
          50% { opacity: 0.5; }
        }
      `}</style>
    </Box>
  );
}
