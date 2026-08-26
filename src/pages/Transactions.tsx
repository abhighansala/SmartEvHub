import { useState, useMemo } from 'react';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import Grid from '@mui/material/Grid2';
import Typography from '@mui/material/Typography';
import Tabs from '@mui/material/Tabs';
import Tab from '@mui/material/Tab';
import TextField from '@mui/material/TextField';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import InputAdornment from '@mui/material/InputAdornment';
import Chip from '@mui/material/Chip';
import { useTheme, alpha } from '@mui/material/styles';

import SearchOutlined from '@mui/icons-material/SearchOutlined';
import ArrowDownwardOutlined from '@mui/icons-material/ArrowDownwardOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import EmptyState from '@/components/common/EmptyState';
import { useApp } from '@/context/AppContext';

export default function Transactions() {
  const theme = useTheme();
  const { transactions } = useApp();
  const [tab, setTab] = useState(0);
  const [search, setSearch] = useState('');

  const filtered = useMemo(() => {
    let items = transactions;
    if (tab === 1) items = items.filter(t => t.type === 'charging');
    if (tab === 2) items = items.filter(t => t.type === 'battery-swap');
    if (search) {
      const q = search.toLowerCase();
      items = items.filter(t => t.id.toLowerCase().includes(q) || t.evId.toLowerCase().includes(q));
    }
    return items;
  }, [transactions, tab, search]);

  // Stack visualization: show latest 5
  const stackItems = transactions.slice(0, 5);

  return (
    <Box>
      <PageHeader title="Transactions" subtitle="View charging and battery swap transaction history" />

      <Grid container spacing={2.5}>
        {/* Transaction Stack (LIFO) */}
        <Grid size={{ xs: 12, md: 4 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, height: '100%' }}>
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 2 }}>
              <Typography variant="h5">Recent Transaction Stack</Typography>
            </Box>

            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 2 }}>
              <Chip label="LIFO" color="secondary" size="small" sx={{ fontWeight: 700 }} />
              <Typography variant="caption" color="text.secondary">
                <strong>Last In, First Out</strong> — Newest on top
              </Typography>
            </Box>

            {/* Stack Visual */}
            <Box sx={{ display: 'flex', flexDirection: 'column', gap: 0 }}>
              {/* TOP label */}
              <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 1, justifyContent: 'center' }}>
                <Typography variant="caption" fontWeight={700} color="primary.main">TOP</Typography>
                <ArrowDownwardOutlined sx={{ fontSize: 14, color: 'primary.main' }} />
              </Box>

              {stackItems.map((txn, i) => (
                <Box
                  key={txn.id}
                  sx={{
                    p: 2,
                    border: `1px solid ${i === 0 ? theme.palette.primary.main : theme.palette.divider}`,
                    borderRadius: i === 0 ? '12px 12px 0 0' : i === stackItems.length - 1 ? '0 0 12px 12px' : 0,
                    borderTop: i > 0 ? 'none' : undefined,
                    bgcolor: i === 0 ? alpha(theme.palette.primary.main, 0.04) : 'background.paper',
                    display: 'flex',
                    justifyContent: 'space-between',
                    alignItems: 'center',
                    transition: 'all 0.3s',
                  }}
                >
                  <Box>
                    <Typography variant="subtitle2" fontWeight={600}>{txn.id}</Typography>
                    <Typography variant="caption" color="text.secondary">
                      {txn.evId} • {txn.type === 'charging' ? 'Charging' : 'Swap'}
                    </Typography>
                  </Box>
                  <Typography variant="body2" fontWeight={600}>₹{txn.amount.toFixed(0)}</Typography>
                </Box>
              ))}

              <Box sx={{ display: 'flex', justifyContent: 'center', mt: 1 }}>
                <Typography variant="caption" fontWeight={700} color="text.secondary">BOTTOM</Typography>
              </Box>
            </Box>
          </Card>
        </Grid>

        {/* Transaction Table */}
        <Grid size={{ xs: 12, md: 8 }}>
          <Card sx={{ border: `1px solid ${theme.palette.divider}` }}>
            <Box sx={{ borderBottom: `1px solid ${theme.palette.divider}`, px: 2 }}>
              <Tabs value={tab} onChange={(_, v) => setTab(v)}>
                <Tab label="All" />
                <Tab label="Charging" />
                <Tab label="Battery Swap" />
              </Tabs>
            </Box>

            <Box sx={{ p: 2 }}>
              <TextField
                placeholder="Search by Transaction ID or EV ID..."
                value={search}
                onChange={e => setSearch(e.target.value)}
                size="small"
                fullWidth
                slotProps={{
                  input: {
                    startAdornment: <InputAdornment position="start"><SearchOutlined fontSize="small" /></InputAdornment>,
                  },
                }}
              />
            </Box>

            {filtered.length === 0 ? (
              <EmptyState title="No transactions found" description="Try adjusting your filters or search." />
            ) : (
              <TableContainer>
                <Table>
                  <TableHead>
                    <TableRow>
                      <TableCell>ID</TableCell>
                      <TableCell>EV</TableCell>
                      <TableCell>Type</TableCell>
                      <TableCell>Station/Battery</TableCell>
                      <TableCell>Date</TableCell>
                      <TableCell align="right">Amount</TableCell>
                      <TableCell>Status</TableCell>
                    </TableRow>
                  </TableHead>
                  <TableBody>
                    {filtered.map(t => (
                      <TableRow key={t.id} hover>
                        <TableCell sx={{ fontWeight: 600 }}>{t.id}</TableCell>
                        <TableCell>{t.evId}</TableCell>
                        <TableCell>
                          <Chip
                            label={t.type === 'charging' ? 'Charging' : 'Battery Swap'}
                            size="small"
                            variant="outlined"
                          />
                        </TableCell>
                        <TableCell>{t.stationOrBattery}</TableCell>
                        <TableCell>{new Date(t.date).toLocaleDateString('en-IN', { day: 'numeric', month: 'short' })}</TableCell>
                        <TableCell align="right" sx={{ fontWeight: 500 }}>₹{t.amount.toFixed(2)}</TableCell>
                        <TableCell><StatusChip status={t.status} /></TableCell>
                      </TableRow>
                    ))}
                  </TableBody>
                </Table>
              </TableContainer>
            )}
          </Card>
        </Grid>
      </Grid>
    </Box>
  );
}
