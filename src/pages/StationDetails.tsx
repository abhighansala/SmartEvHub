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

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import EmptyState from '@/components/common/EmptyState';
import { useApp } from '@/context/AppContext';

export default function StationDetails() {
  const { id } = useParams<{ id: string }>();
  const navigate = useNavigate();
  const theme = useTheme();
  const { stations, sessions, completeSession } = useApp();

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

  const getDuration = (start: string) => {
    const diff = Date.now() - new Date(start).getTime();
    const h = Math.floor(diff / 3600000);
    const m = Math.round((diff % 3600000) / 60000);
    return h > 0 ? `${h}h ${m}m` : `${m}m`;
  };

  const progress = session
    ? Math.min(session.batteryStart + (session.targetCharge - session.batteryStart) * 0.6, session.targetCharge)
    : 0;

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
                ['Power Output', `${station.power} kW`],
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
                    color: theme.palette.info.main,
                  }}
                >
                  <ElectricBoltOutlined sx={{ fontSize: 28, animation: 'pulse 1.5s infinite' }} />
                  <Typography variant="h3" color="info.main" fontWeight={700}>
                    Charging...
                  </Typography>
                </Box>

                <Box sx={{ width: '100%', textAlign: 'center' }}>
                  <Typography variant="h1" sx={{ fontSize: '3rem', fontWeight: 700, color: theme.palette.info.main }}>
                    {Math.round(progress)}%
                  </Typography>
                  <LinearProgress
                    variant="determinate"
                    value={progress}
                    sx={{
                      mt: 2,
                      height: 12,
                      borderRadius: 6,
                      bgcolor: alpha(theme.palette.info.main, 0.1),
                      '& .MuiLinearProgress-bar': { borderRadius: 6 },
                    }}
                  />
                  <Typography variant="body2" color="text.secondary" sx={{ mt: 1 }}>
                    Target: {session.targetCharge}%
                  </Typography>
                </Box>

                <Box sx={{ display: 'flex', gap: 4 }}>
                  <Box sx={{ textAlign: 'center' }}>
                    <Typography variant="h4">{session.evId}</Typography>
                    <Typography variant="caption" color="text.secondary">Vehicle</Typography>
                  </Box>
                  <Box sx={{ textAlign: 'center' }}>
                    <Typography variant="h4">{getDuration(session.startTime)}</Typography>
                    <Typography variant="caption" color="text.secondary">Duration</Typography>
                  </Box>
                  <Box sx={{ textAlign: 'center' }}>
                    <Typography variant="h4">₹{session.estimatedCost.toFixed(0)}</Typography>
                    <Typography variant="caption" color="text.secondary">Est. Cost</Typography>
                  </Box>
                </Box>

                <Button
                  variant="contained"
                  color="error"
                  startIcon={<StopCircleOutlined />}
                  onClick={() => completeSession(session.id)}
                >
                  Stop Session
                </Button>
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
