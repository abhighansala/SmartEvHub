import { useParams, useNavigate } from 'react-router-dom';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import Grid from '@mui/material/Grid2';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Tabs from '@mui/material/Tabs';
import Tab from '@mui/material/Tab';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import { useTheme, alpha } from '@mui/material/styles';
import { useState } from 'react';

import ArrowBackOutlined from '@mui/icons-material/ArrowBackOutlined';
import BatteryChargingFullOutlined from '@mui/icons-material/BatteryChargingFullOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import EmptyState from '@/components/common/EmptyState';
import { useApp } from '@/context/AppContext';

export default function EVDetails() {
  const { id } = useParams<{ id: string }>();
  const navigate = useNavigate();
  const theme = useTheme();
  const { evs, transactions, sessions } = useApp();
  const [tab, setTab] = useState(0);

  const ev = evs.find(e => e.id === id);
  if (!ev) {
    return (
      <EmptyState
        title="EV Not Found"
        description={`No electric vehicle found with ID "${id}".`}
        actionLabel="Back to EV Management"
        onAction={() => navigate('/evs')}
      />
    );
  }

  const evTransactions = transactions.filter(t => t.evId === ev.id);
  const evSession = sessions.find(s => s.evId === ev.id);

  const batteryColor = ev.currentCharge < 20 ? theme.palette.error.main
    : ev.currentCharge < 50 ? theme.palette.warning.main
    : theme.palette.success.main;

  return (
    <Box>
      <PageHeader
        title={`${ev.id} — ${ev.model}`}
        subtitle={`Owner: ${ev.owner}`}
        action={
          <Button startIcon={<ArrowBackOutlined />} onClick={() => navigate('/evs')}>
            Back
          </Button>
        }
      />

      <Grid container spacing={2.5}>
        {/* Info Card */}
        <Grid size={{ xs: 12, md: 5 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, height: '100%' }}>
            <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
              {[
                ['Vehicle ID', ev.id],
                ['Owner', ev.owner],
                ['Model', ev.model],
                ['Battery Capacity', `${ev.batteryCapacity} kWh`],
                ['Status', null],
                ['Registered', new Date(ev.registeredAt).toLocaleDateString('en-IN', { day: 'numeric', month: 'long', year: 'numeric' })],
              ].map(([label, value]) => (
                <Box key={label as string} sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                  <Typography variant="body2" color="text.secondary">{label}</Typography>
                  {label === 'Status' ? <StatusChip status={ev.status} /> : (
                    <Typography variant="body2" fontWeight={500}>{value}</Typography>
                  )}
                </Box>
              ))}
            </Box>
          </Card>
        </Grid>

        {/* Battery Gauge */}
        <Grid size={{ xs: 12, md: 7 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, height: '100%', display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center' }}>
            <Box sx={{ position: 'relative', display: 'inline-flex', mb: 2 }}>
              <Box
                sx={{
                  width: 160,
                  height: 160,
                  borderRadius: '50%',
                  border: `8px solid ${alpha(batteryColor, 0.15)}`,
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                  position: 'relative',
                  '&::after': {
                    content: '""',
                    position: 'absolute',
                    inset: 0,
                    borderRadius: '50%',
                    border: `8px solid ${batteryColor}`,
                    borderColor: `${batteryColor} transparent transparent transparent`,
                    transform: `rotate(${ev.currentCharge * 3.6 - 90}deg)`,
                    transition: 'transform 1s ease',
                  },
                }}
              >
                <Box sx={{ textAlign: 'center' }}>
                  <Typography variant="h1" sx={{ color: batteryColor, fontWeight: 700 }}>
                    {ev.currentCharge}%
                  </Typography>
                </Box>
              </Box>
            </Box>
            <Typography variant="body1" color="text.secondary">Current Battery Level</Typography>
            <Box
              sx={{
                mt: 2,
                width: '80%',
                height: 12,
                borderRadius: 6,
                bgcolor: alpha(batteryColor, 0.1),
                overflow: 'hidden',
              }}
            >
              <Box
                sx={{
                  width: `${ev.currentCharge}%`,
                  height: '100%',
                  borderRadius: 6,
                  bgcolor: batteryColor,
                  transition: 'width 1s ease',
                }}
              />
            </Box>
            {evSession && (
              <Box sx={{ mt: 2, p: 1.5, borderRadius: 2, bgcolor: alpha(theme.palette.info.main, 0.08) }}>
                <Typography variant="body2" color="info.main" fontWeight={500}>
                  Currently charging at {evSession.stationId} — Target: {evSession.targetCharge}%
                </Typography>
              </Box>
            )}
          </Card>
        </Grid>
      </Grid>

      {/* Tabs */}
      <Card sx={{ mt: 2.5, border: `1px solid ${theme.palette.divider}` }}>
        <Tabs value={tab} onChange={(_, v) => setTab(v)} sx={{ borderBottom: `1px solid ${theme.palette.divider}`, px: 2 }}>
          <Tab label="Overview" />
          <Tab label="Charging History" />
          <Tab label="Transactions" />
        </Tabs>

        <Box sx={{ p: 3 }}>
          {tab === 0 && (
            <Box>
              <Typography variant="body1">
                {ev.model} registered to {ev.owner}. Battery capacity is {ev.batteryCapacity} kWh
                with current charge at {ev.currentCharge}%.
                {ev.status === 'charging' && ' Currently undergoing a charging session.'}
                {ev.status === 'waiting' && ' Currently waiting in the charging queue.'}
              </Typography>
            </Box>
          )}
          {tab === 1 && (
            evTransactions.filter(t => t.type === 'charging').length === 0 ? (
              <EmptyState title="No charging history" description="This vehicle has no charging records yet." />
            ) : (
              <TableContainer>
                <Table size="small">
                  <TableHead>
                    <TableRow>
                      <TableCell>Transaction</TableCell>
                      <TableCell>Station</TableCell>
                      <TableCell>Date</TableCell>
                      <TableCell align="right">Energy</TableCell>
                      <TableCell align="right">Duration</TableCell>
                      <TableCell align="right">Amount</TableCell>
                    </TableRow>
                  </TableHead>
                  <TableBody>
                    {evTransactions.filter(t => t.type === 'charging').map(t => (
                      <TableRow key={t.id}>
                        <TableCell sx={{ fontWeight: 500 }}>{t.id}</TableCell>
                        <TableCell>{t.stationOrBattery}</TableCell>
                        <TableCell>{new Date(t.date).toLocaleDateString('en-IN')}</TableCell>
                        <TableCell align="right">{t.energyDelivered} kWh</TableCell>
                        <TableCell align="right">{t.duration} min</TableCell>
                        <TableCell align="right" sx={{ fontWeight: 500 }}>₹{t.amount.toFixed(2)}</TableCell>
                      </TableRow>
                    ))}
                  </TableBody>
                </Table>
              </TableContainer>
            )
          )}
          {tab === 2 && (
            evTransactions.length === 0 ? (
              <EmptyState title="No transactions" description="No transaction records for this vehicle." />
            ) : (
              <TableContainer>
                <Table size="small">
                  <TableHead>
                    <TableRow>
                      <TableCell>ID</TableCell>
                      <TableCell>Type</TableCell>
                      <TableCell>Date</TableCell>
                      <TableCell align="right">Amount</TableCell>
                      <TableCell>Status</TableCell>
                    </TableRow>
                  </TableHead>
                  <TableBody>
                    {evTransactions.map(t => (
                      <TableRow key={t.id}>
                        <TableCell sx={{ fontWeight: 500 }}>{t.id}</TableCell>
                        <TableCell><StatusChip status={t.type} /></TableCell>
                        <TableCell>{new Date(t.date).toLocaleDateString('en-IN')}</TableCell>
                        <TableCell align="right" sx={{ fontWeight: 500 }}>₹{t.amount.toFixed(2)}</TableCell>
                        <TableCell><StatusChip status={t.status} /></TableCell>
                      </TableRow>
                    ))}
                  </TableBody>
                </Table>
              </TableContainer>
            )
          )}
        </Box>
      </Card>
    </Box>
  );
}
