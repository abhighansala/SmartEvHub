import { useState, useEffect, useMemo } from 'react';
import { useNavigate } from 'react-router-dom';
import Dialog from '@mui/material/Dialog';
import DialogContent from '@mui/material/DialogContent';
import TextField from '@mui/material/TextField';
import List from '@mui/material/List';
import ListItemButton from '@mui/material/ListItemButton';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';
import Typography from '@mui/material/Typography';
import InputAdornment from '@mui/material/InputAdornment';
import Box from '@mui/material/Box';

import SearchOutlined from '@mui/icons-material/SearchOutlined';
import DirectionsCarOutlined from '@mui/icons-material/DirectionsCarOutlined';
import EvStationOutlined from '@mui/icons-material/EvStationOutlined';
import BatteryChargingFullOutlined from '@mui/icons-material/BatteryChargingFullOutlined';
import ReceiptLongOutlined from '@mui/icons-material/ReceiptLongOutlined';

import { useApp } from '@/context/AppContext';

interface SearchDialogProps {
  open: boolean;
  onClose: () => void;
}

interface SearchResult {
  type: 'ev' | 'station' | 'battery' | 'transaction';
  title: string;
  subtitle: string;
  link: string;
}

const typeIcons = {
  ev: <DirectionsCarOutlined />,
  station: <EvStationOutlined />,
  battery: <BatteryChargingFullOutlined />,
  transaction: <ReceiptLongOutlined />,
};

export default function SearchDialog({ open, onClose }: SearchDialogProps) {
  const [query, setQuery] = useState('');
  const navigate = useNavigate();
  const { evs, stations, batteries, transactions } = useApp();

  useEffect(() => {
    if (!open) setQuery('');
  }, [open]);

  const results = useMemo<SearchResult[]>(() => {
    if (!query.trim()) return [];
    const q = query.toLowerCase();

    const evResults: SearchResult[] = evs
      .filter(e => e.id.toLowerCase().includes(q) || e.owner.toLowerCase().includes(q) || e.model.toLowerCase().includes(q))
      .slice(0, 5)
      .map(e => ({ type: 'ev', title: `${e.id} — ${e.model}`, subtitle: e.owner, link: `/evs/${e.id}` }));

    const stationResults: SearchResult[] = stations
      .filter(s => s.id.toLowerCase().includes(q) || s.name.toLowerCase().includes(q))
      .slice(0, 3)
      .map(s => ({ type: 'station', title: `${s.id} — ${s.name}`, subtitle: `${s.power} kW • ${s.connectorType}`, link: `/stations/${s.id}` }));

    const batteryResults: SearchResult[] = batteries
      .filter(b => b.id.toLowerCase().includes(q))
      .slice(0, 3)
      .map(b => ({ type: 'battery', title: b.id, subtitle: `${b.capacity} kWh • ${b.currentCharge}%`, link: '/batteries' }));

    const txnResults: SearchResult[] = transactions
      .filter(t => t.id.toLowerCase().includes(q) || t.evId.toLowerCase().includes(q))
      .slice(0, 3)
      .map(t => ({ type: 'transaction', title: t.id, subtitle: `${t.evId} • ₹${t.amount.toFixed(2)}`, link: '/transactions' }));

    return [...evResults, ...stationResults, ...batteryResults, ...txnResults];
  }, [query, evs, stations, batteries, transactions]);

  const handleSelect = (result: SearchResult) => {
    navigate(result.link);
    onClose();
  };

  return (
    <Dialog
      open={open}
      onClose={onClose}
      maxWidth="sm"
      fullWidth
      sx={{ '& .MuiDialog-paper': { mt: 8, borderRadius: 4 } }}
      slotProps={{ backdrop: { sx: { backdropFilter: 'blur(4px)' } } }}
    >
      <DialogContent sx={{ p: 0 }}>
        <TextField
          autoFocus
          fullWidth
          placeholder="Search EVs, stations, batteries, transactions..."
          value={query}
          onChange={e => setQuery(e.target.value)}
          variant="outlined"
          slotProps={{
            input: {
              startAdornment: (
                <InputAdornment position="start">
                  <SearchOutlined color="action" />
                </InputAdornment>
              ),
              sx: { borderRadius: 0, fontSize: '1rem' },
            },
          }}
          sx={{
            '& .MuiOutlinedInput-root': {
              borderRadius: 0,
              '& fieldset': { border: 'none' },
            },
          }}
        />
        {query.trim() && (
          <>
            {results.length > 0 ? (
              <List sx={{ py: 1, maxHeight: 320, overflow: 'auto' }}>
                {results.map((r, i) => (
                  <ListItemButton key={i} onClick={() => handleSelect(r)} sx={{ py: 1.5, px: 2.5 }}>
                    <ListItemIcon sx={{ minWidth: 40, color: 'text.secondary' }}>
                      {typeIcons[r.type]}
                    </ListItemIcon>
                    <ListItemText
                      primary={<Typography variant="body2" fontWeight={500}>{r.title}</Typography>}
                      secondary={r.subtitle}
                    />
                  </ListItemButton>
                ))}
              </List>
            ) : (
              <Box sx={{ p: 4, textAlign: 'center' }}>
                <Typography variant="body2" color="text.secondary">No results found for "{query}"</Typography>
              </Box>
            )}
          </>
        )}
        {!query.trim() && (
          <Box sx={{ p: 3, textAlign: 'center' }}>
            <Typography variant="body2" color="text.secondary">
              Type to search across EVs, stations, batteries, and transactions
            </Typography>
          </Box>
        )}
      </DialogContent>
    </Dialog>
  );
}
