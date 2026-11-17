import { useState, useMemo, useEffect } from 'react';
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
import Button from '@mui/material/Button';
import { useTheme, alpha } from '@mui/material/styles';

import SearchOutlined from '@mui/icons-material/SearchOutlined';
import ArrowDownwardOutlined from '@mui/icons-material/ArrowDownwardOutlined';
import ReceiptLongOutlined from '@mui/icons-material/ReceiptLongOutlined';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import EmptyState from '@/components/common/EmptyState';
import ReceiptModal from '@/components/common/ReceiptModal';
import { useApp } from '@/context/AppContext';
import type { Transaction } from '@/types';

export default function Transactions() {
  const theme = useTheme();
  const { transactions, transactionStackMeta, evs, refreshBackendData } = useApp();

  useEffect(() => {
    refreshBackendData();
  }, [refreshBackendData]);
  const [tab, setTab] = useState(0);
  const [search, setSearch] = useState('');
  const [selectedTxn, setSelectedTxn] = useState<Transaction | null>(null);

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

  const getModelForEv = (evId: string) => {
    const ev = evs.find(e => e.id === evId);
    return ev?.model || 'Electric Vehicle';
  };

  return (
    <Box>
      <PageHeader
        title="Transactions & Billing"
        subtitle="View operational transaction records, tax invoices and C++ LIFO stack history"
      />

      <Grid container spacing={2.5}>
        {/* Transaction Stack (LIFO) */}
        <Grid size={{ xs: 12, md: 4 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}`, height: '100%' }}>
            <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', mb: 2 }}>
              <Typography variant="h5" fontWeight={700}>Transaction Stack</Typography>
              <Chip label={`TOP: ${transactionStackMeta.topIndex}`} size="small" color="primary" variant="outlined" />
            </Box>

            <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', mb: 2, flexWrap: 'wrap', gap: 1 }}>
              <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                <Chip label="C++ LIFO Stack" color="secondary" size="small" sx={{ fontWeight: 700 }} />
                <Typography variant="caption" color="text.secondary">
                  <strong>Last In, First Out</strong> — Pure manual C++ stack
                </Typography>
              </Box>
              <Chip label={`Capacity: ${transactionStackMeta.capacity}`} size="small" variant="outlined" />
            </Box>

            {/* Stack Visual */}
            <Box sx={{ display: 'flex', flexDirection: 'column', gap: 0 }}>
              {/* TOP label */}
              <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 1, justifyContent: 'center' }}>
                <Typography variant="caption" fontWeight={700} color="primary.main">TOP OF STACK</Typography>
                <ArrowDownwardOutlined sx={{ fontSize: 14, color: 'primary.main' }} />
              </Box>

              {stackItems.length === 0 ? (
                <Box sx={{ py: 3, textAlign: 'center' }}>
                  <Typography variant="caption" color="text.secondary">Stack is currently empty</Typography>
                </Box>
              ) : (
                stackItems.map((txn, i) => (
                  <Box
                    key={txn.id}
                    onClick={() => setSelectedTxn(txn)}
                    sx={{
                      p: 2,
                      border: `1px solid ${i === 0 ? theme.palette.primary.main : theme.palette.divider}`,
                      borderRadius: i === 0 ? '12px 12px 0 0' : i === stackItems.length - 1 ? '0 0 12px 12px' : 0,
                      borderTop: i > 0 ? 'none' : undefined,
                      bgcolor: i === 0 ? alpha(theme.palette.primary.main, 0.05) : 'background.paper',
                      display: 'flex',
                      justifyContent: 'space-between',
                      alignItems: 'center',
                      cursor: 'pointer',
                      transition: 'all 0.2s',
                      '&:hover': {
                        bgcolor: alpha(theme.palette.primary.main, 0.1),
                      },
                    }}
                  >
                    <Box>
                      <Typography variant="subtitle2" fontWeight={700}>{txn.id}</Typography>
                      <Typography variant="caption" color="text.secondary">
                        {txn.evId} • {txn.type === 'charging' ? 'Charging' : 'Swap'}
                      </Typography>
                    </Box>
                    <Box sx={{ textAlign: 'right' }}>
                      <Typography variant="body2" fontWeight={700} color="primary.main">₹{txn.amount.toFixed(2)}</Typography>
                      <Typography variant="caption" color="text.secondary">Click for Receipt</Typography>
                    </Box>
                  </Box>
                ))
              )}

              <Box sx={{ display: 'flex', justifyContent: 'center', mt: 1 }}>
                <Typography variant="caption" fontWeight={700} color="text.secondary">BOTTOM OF STACK</Typography>
              </Box>
            </Box>
          </Card>
        </Grid>

        {/* Transaction Table */}
        <Grid size={{ xs: 12, md: 8 }}>
          <Card sx={{ border: `1px solid ${theme.palette.divider}` }}>
            <Box sx={{ borderBottom: `1px solid ${theme.palette.divider}`, px: 2 }}>
              <Tabs value={tab} onChange={(_, v) => setTab(v)}>
                <Tab label="All Operations" />
                <Tab label="EV Charging" />
                <Tab label="Battery Swaps" />
              </Tabs>
            </Box>

            <Box sx={{ p: 2 }}>
              <TextField
                placeholder="Search by Transaction ID or Vehicle ID..."
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
                      <TableCell>Transaction ID</TableCell>
                      <TableCell>Vehicle ID</TableCell>
                      <TableCell>Type</TableCell>
                      <TableCell>Station / Bay</TableCell>
                      <TableCell>Date</TableCell>
                      <TableCell align="right">Amount</TableCell>
                      <TableCell>Status</TableCell>
                      <TableCell align="center">Action</TableCell>
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
                        <TableCell align="right" sx={{ fontWeight: 700, color: 'primary.main' }}>
                          ₹{t.amount.toFixed(2)}
                        </TableCell>
                        <TableCell><StatusChip status={t.status} /></TableCell>
                        <TableCell align="center">
                          <Button
                            size="small"
                            variant="outlined"
                            startIcon={<ReceiptLongOutlined />}
                            onClick={() => setSelectedTxn(t)}
                          >
                            Receipt
                          </Button>
                        </TableCell>
                      </TableRow>
                    ))}
                  </TableBody>
                </Table>
              </TableContainer>
            )}
          </Card>
        </Grid>
      </Grid>

      {/* Official Formatted Receipt Modal */}
      <ReceiptModal
        open={!!selectedTxn}
        onClose={() => setSelectedTxn(null)}
        transaction={selectedTxn}
        evModel={selectedTxn ? getModelForEv(selectedTxn.evId) : undefined}
      />
    </Box>
  );
}
