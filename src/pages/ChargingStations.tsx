import { useNavigate } from 'react-router-dom';
import Grid from '@mui/material/Grid2';
import Card from '@mui/material/Card';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Chip from '@mui/material/Chip';
import LinearProgress from '@mui/material/LinearProgress';
import { useTheme, alpha } from '@mui/material/styles';

import EvStationOutlined from '@mui/icons-material/EvStationOutlined';
import VisibilityOutlined from '@mui/icons-material/VisibilityOutlined';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import { useApp } from '@/context/AppContext';
import { calculateChargingTime } from '@/utils/chargingCalc';

export default function ChargingStations() {
  const theme = useTheme();
  const navigate = useNavigate();
  const { stations, sessions, evs } = useApp();

  return (
    <Box>
      <PageHeader
        title="Charging Stations"
        subtitle="Monitor and manage 20–30 kW charging infrastructure with live battery simulation"
      />

      <Box sx={{ mb: 2.5, p: 2, borderRadius: 2.5, bgcolor: alpha(theme.palette.info.main, 0.08), border: `1px solid ${alpha(theme.palette.info.main, 0.25)}`, display: 'flex', alignItems: 'center', justifyContent: 'space-between', flexWrap: 'wrap', gap: 1.5 }}>
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1.5 }}>
          <ElectricBoltOutlined color="info" sx={{ animation: 'pulse 1.5s infinite' }} />
          <Box>
            <Typography variant="subtitle2" fontWeight={700} color="text.primary">
              Live Station Progress Monitor
            </Typography>
            <Typography variant="caption" color="text.secondary">
              Station outputs range from 20 to 30 kW (25 kW = 25 kWh/hr). Connected vehicles continuously charge in real time below.
            </Typography>
          </Box>
        </Box>
        <Chip label="LIVE CHARGING ENGINE" color="success" size="small" sx={{ fontWeight: 700, fontSize: '0.75rem' }} />
      </Box>

      <Grid container spacing={2.5}>
        {stations.map(station => {
          const session = sessions.find(s => s.stationId === station.id && s.status === 'active');
          const ev = session
            ? evs.find(e => e.id === session.evId)
            : (station.currentEV ? evs.find(e => e.id === station.currentEV) : null);
          const currentProgress = ev ? ev.currentCharge : (session ? session.batteryStart : 0);
          const deliveredKwh = ev && session
            ? Math.round(Math.max(0, ((currentProgress - session.batteryStart) / 100) * ev.batteryCapacity) * 10) / 10
            : 0;

          const targetCharge = session ? session.targetCharge : 80;
          const remainingKwh = ev
            ? Math.max(0, Number((((targetCharge - currentProgress) / 100) * ev.batteryCapacity).toFixed(1)))
            : 0;
          const estRemainingTime = calculateChargingTime(remainingKwh, station.power);
          const isTargetReached = currentProgress >= targetCharge;

          return (
            <Grid key={station.id} size={{ xs: 12, sm: 6, lg: 4 }}>
              <Card
                sx={{
                  p: 3,
                  border: `1px solid ${theme.palette.divider}`,
                  cursor: 'pointer',
                  transition: 'all 0.2s',
                  '&:hover': {
                    borderColor: theme.palette.primary.main,
                    transform: 'translateY(-2px)',
                    boxShadow: `0 4px 20px ${alpha(theme.palette.primary.main, 0.1)}`,
                  },
                }}
                onClick={() => navigate(`/stations/${station.id}`)}
              >
                <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', mb: 2 }}>
                  <Box sx={{ display: 'flex', alignItems: 'center', gap: 1.5 }}>
                    <Box
                      sx={{
                        width: 44,
                        height: 44,
                        borderRadius: 2.5,
                        bgcolor: alpha(station.status === 'charging' ? theme.palette.info.main : theme.palette.primary.main, 0.1),
                        display: 'flex',
                        alignItems: 'center',
                        justifyContent: 'center',
                      }}
                    >
                      {station.status === 'charging' ? (
                        <ElectricBoltOutlined sx={{ color: theme.palette.info.main, animation: 'pulse 1.5s infinite' }} />
                      ) : (
                        <EvStationOutlined sx={{ color: theme.palette.primary.main }} />
                      )}
                    </Box>
                    <Box>
                      <Typography variant="h5">{station.name}</Typography>
                      <Typography variant="caption" color="text.secondary">{station.id} • {station.location}</Typography>
                    </Box>
                  </Box>
                  <StatusChip status={station.status} />
                </Box>

                <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1.5 }}>
                  <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                    <Typography variant="body2" color="text.secondary">Power Output</Typography>
                    <Typography variant="body2" fontWeight={600}>{station.power} kW</Typography>
                  </Box>
                  <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                    <Typography variant="body2" color="text.secondary">Connector</Typography>
                    <Chip label={station.connectorType} size="small" variant="outlined" />
                  </Box>
                  <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                    <Typography variant="body2" color="text.secondary">Current EV</Typography>
                    <Typography variant="body2" fontWeight={600}>
                      {station.currentEV ? `${station.currentEV}${ev ? ` (${ev.model})` : ''}` : 'None'}
                    </Typography>
                  </Box>

                  {/* Live Progress for Charging Stations */}
                  {station.status === 'charging' && (
                    <Box
                      sx={{
                        mt: 1,
                        p: 1.5,
                        borderRadius: 2,
                        bgcolor: alpha(isTargetReached ? theme.palette.success.main : theme.palette.info.main, 0.08),
                        border: `1px solid ${alpha(isTargetReached ? theme.palette.success.main : theme.palette.info.main, 0.2)}`,
                      }}
                    >
                      <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 0.75 }}>
                        <Box sx={{ display: 'flex', alignItems: 'center', gap: 0.5 }}>
                          <ElectricBoltOutlined
                            sx={{
                              fontSize: 16,
                              color: isTargetReached ? theme.palette.success.main : theme.palette.info.main,
                              animation: isTargetReached ? 'none' : 'pulse 1.5s infinite',
                            }}
                          />
                          <Typography
                            variant="caption"
                            fontWeight={700}
                            color={isTargetReached ? 'success.main' : 'info.main'}
                          >
                            {isTargetReached ? 'Target Reached (Ready)' : 'Live Charging Progress'}
                          </Typography>
                        </Box>
                        <Typography variant="caption" fontWeight={700} color="text.primary">
                          {Math.round(currentProgress)}% {session ? `/ ${session.targetCharge}%` : ''}
                        </Typography>
                      </Box>
                      <LinearProgress
                        variant="determinate"
                        value={currentProgress}
                        sx={{
                          height: 8,
                          borderRadius: 4,
                          bgcolor: alpha(theme.palette.info.main, 0.15),
                          '& .MuiLinearProgress-bar': {
                            borderRadius: 4,
                            bgcolor: isTargetReached
                              ? theme.palette.success.main
                              : currentProgress < 30
                              ? theme.palette.error.main
                              : currentProgress < 60
                              ? theme.palette.warning.main
                              : theme.palette.info.main,
                            transition: 'transform 0.4s ease-in-out',
                          },
                        }}
                      />
                      <Box sx={{ display: 'flex', justifyContent: 'space-between', mt: 0.75 }}>
                        <Typography variant="caption" color="text.secondary">
                          Delivered: +{deliveredKwh} kWh
                        </Typography>
                        <Typography variant="caption" fontWeight={600} color={isTargetReached ? 'success.main' : 'primary.main'}>
                          {isTargetReached ? '✓ Complete' : `⏱ ~${estRemainingTime.formatted} left`}
                        </Typography>
                      </Box>
                      <Box sx={{ mt: 0.5, pt: 0.5, borderTop: `1px dashed ${alpha(theme.palette.divider, 0.6)}`, display: 'flex', justifyContent: 'space-between' }}>
                        <Typography variant="caption" color="text.secondary">
                          Rate: {station.power} kW/hr
                        </Typography>
                        <Typography variant="caption" color="text.secondary">
                          {remainingKwh > 0 ? `${remainingKwh} kWh remaining` : 'Target reached'}
                        </Typography>
                      </Box>
                    </Box>
                  )}

                  {station.status === 'available' && (
                    <Box
                      sx={{
                        mt: 0.5,
                        p: 1.25,
                        borderRadius: 2,
                        bgcolor: alpha(theme.palette.success.main, 0.05),
                        border: `1px solid ${alpha(theme.palette.success.main, 0.15)}`,
                        display: 'flex',
                        alignItems: 'center',
                        justifyContent: 'space-between',
                      }}
                    >
                      <Typography variant="caption" color="success.main" fontWeight={600}>
                        ✓ Ready to charge next vehicle
                      </Typography>
                      <Typography variant="caption" color="text.secondary">
                        Max {station.power} kW
                      </Typography>
                    </Box>
                  )}
                </Box>

                <Box sx={{ mt: 2, pt: 2, borderTop: `1px solid ${theme.palette.divider}`, display: 'flex', gap: 1 }}>
                  <Button
                    size="small"
                    startIcon={<VisibilityOutlined />}
                    onClick={(e) => { e.stopPropagation(); navigate(`/stations/${station.id}`); }}
                    fullWidth
                  >
                    View Details
                  </Button>
                </Box>
              </Card>
            </Grid>
          );
        })}
      </Grid>
      <style>{`
        @keyframes pulse {
          0%, 100% { opacity: 1; transform: scale(1); }
          50% { opacity: 0.4; transform: scale(1.15); }
        }
      `}</style>
    </Box>
  );
}
