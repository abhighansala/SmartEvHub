import React from 'react';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Divider from '@mui/material/Divider';
import Chip from '@mui/material/Chip';
import { useTheme, alpha } from '@mui/material/styles';

import ReceiptLongOutlined from '@mui/icons-material/ReceiptLongOutlined';
import CheckCircleOutline from '@mui/icons-material/CheckCircleOutline';
import PrintOutlined from '@mui/icons-material/PrintOutlined';
import CloseOutlined from '@mui/icons-material/CloseOutlined';

import type { Transaction } from '@/types';

interface ReceiptModalProps {
  open: boolean;
  onClose: () => void;
  transaction: Transaction | null;
  evModel?: string;
}

export default function ReceiptModal({ open, onClose, transaction, evModel }: ReceiptModalProps) {
  const theme = useTheme();

  if (!transaction) return null;

  const handlePrint = () => {
    window.print();
  };

  const formattedDate = transaction.date
    ? new Date(transaction.date).toLocaleString('en-IN', {
        dateStyle: 'medium',
        timeStyle: 'medium',
      })
    : new Date().toLocaleString('en-IN');

  return (
    <Dialog open={open} onClose={onClose} maxWidth="sm" fullWidth>
      <DialogTitle sx={{ pb: 1, display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1.5 }}>
          <Box
            sx={{
              width: 36,
              height: 36,
              borderRadius: 2,
              bgcolor: alpha(theme.palette.success.main, 0.15),
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
            }}
          >
            <ReceiptLongOutlined sx={{ color: theme.palette.success.main, fontSize: 22 }} />
          </Box>
          <Typography variant="h6" fontWeight={700}>
            Charging & Billing Receipt
          </Typography>
        </Box>
        <Chip
          icon={<CheckCircleOutline />}
          label="PAID & COMPLETED"
          color="success"
          size="small"
          sx={{ fontWeight: 700, fontSize: '0.75rem' }}
        />
      </DialogTitle>

      <DialogContent sx={{ pt: 2 }}>
        {/* Printable Formal Receipt Card */}
        <Box
          id="printable-receipt"
          sx={{
            p: 3,
            borderRadius: 2,
            border: `1px solid ${theme.palette.divider}`,
            bgcolor: alpha(theme.palette.background.default, 0.6),
            fontFamily: 'monospace',
          }}
        >
          {/* Header */}
          <Box sx={{ textAlign: 'center', mb: 2 }}>
            <Typography variant="h5" fontWeight={800} sx={{ letterSpacing: 1.5, color: 'primary.main' }}>
              SMART EV HUB
            </Typography>
            <Typography variant="caption" color="text.secondary" display="block">
              Smart Electric Vehicle Charging & Battery Swap System
            </Typography>
            <Typography variant="caption" color="text.secondary" display="block">
              OOP Lab Final Submission • Official Tax Invoice
            </Typography>
          </Box>

          <Divider sx={{ my: 1.5, borderStyle: 'dashed' }} />

          {/* Details Table */}
          <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1, fontSize: '0.875rem' }}>
            <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
              <Typography variant="body2" color="text.secondary">Transaction ID:</Typography>
              <Typography variant="body2" fontWeight={700}>{transaction.id}</Typography>
            </Box>

            {transaction.sessionId && (
              <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                <Typography variant="body2" color="text.secondary">Session Reference:</Typography>
                <Typography variant="body2" fontWeight={600}>{transaction.sessionId}</Typography>
              </Box>
            )}

            <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
              <Typography variant="body2" color="text.secondary">Vehicle ID:</Typography>
              <Typography variant="body2" fontWeight={700}>{transaction.evId}</Typography>
            </Box>

            {evModel && (
              <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                <Typography variant="body2" color="text.secondary">Vehicle Model:</Typography>
                <Typography variant="body2">{evModel}</Typography>
              </Box>
            )}

            <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
              <Typography variant="body2" color="text.secondary">Service Type:</Typography>
              <Typography variant="body2" sx={{ textTransform: 'capitalize', fontWeight: 600 }}>
                {transaction.type === 'charging' ? '⚡ High-Power EV Charging' : '🔄 Battery Swap Service'}
              </Typography>
            </Box>

            <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
              <Typography variant="body2" color="text.secondary">Bay / Station:</Typography>
              <Typography variant="body2" fontWeight={600}>{transaction.stationOrBattery}</Typography>
            </Box>

            <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
              <Typography variant="body2" color="text.secondary">Energy Delivered:</Typography>
              <Typography variant="body2" fontWeight={600}>
                {(transaction.energyDelivered || 0).toFixed(2)} kWh
              </Typography>
            </Box>

            <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
              <Typography variant="body2" color="text.secondary">Duration:</Typography>
              <Typography variant="body2">{transaction.duration} minutes</Typography>
            </Box>

            <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
              <Typography variant="body2" color="text.secondary">Tariff / Rate:</Typography>
              <Typography variant="body2" sx={{ textAlign: 'right' }}>
                {transaction.type === 'battery-swap'
                  ? '₹300.00 Base + ₹5.00 / kWh'
                  : '₹18.00 / kWh (Slab discount > 30 kWh, Min ₹100)'}
              </Typography>
            </Box>

            <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
              <Typography variant="body2" color="text.secondary">Date & Time:</Typography>
              <Typography variant="body2">{formattedDate}</Typography>
            </Box>
          </Box>

          <Divider sx={{ my: 2, borderStyle: 'dashed' }} />

          {/* Total Amount */}
          <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <Typography variant="subtitle1" fontWeight={800}>
              TOTAL CHARGES (INR):
            </Typography>
            <Typography variant="h5" fontWeight={800} color="primary.main">
              ₹{(transaction.amount || 0).toFixed(2)}
            </Typography>
          </Box>

          <Divider sx={{ my: 2, borderStyle: 'dashed' }} />

          <Box sx={{ textAlign: 'center', mt: 1 }}>
            <Typography variant="caption" color="text.secondary" display="block">
              Thank you for charging with Smart EV Hub!
            </Typography>
            <Typography variant="caption" color="text.secondary" display="block">
              Zero emissions. Clean energy mobility.
            </Typography>
          </Box>
        </Box>
      </DialogContent>

      <DialogActions sx={{ px: 3, pb: 2.5 }}>
        <Button startIcon={<PrintOutlined />} onClick={handlePrint} variant="outlined">
          Print Receipt
        </Button>
        <Button onClick={onClose} variant="contained">
          Close
        </Button>
      </DialogActions>
    </Dialog>
  );
}
