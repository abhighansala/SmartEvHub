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
import IconButton from '@mui/material/IconButton';
import Tooltip from '@mui/material/Tooltip';
import Chip from '@mui/material/Chip';
import { useTheme, alpha } from '@mui/material/styles';

import PlayArrowOutlined from '@mui/icons-material/PlayArrowOutlined';
import DeleteOutline from '@mui/icons-material/DeleteOutline';
import ArrowForwardOutlined from '@mui/icons-material/ArrowForwardOutlined';

import PageHeader from '@/components/common/PageHeader';
import EmptyState from '@/components/common/EmptyState';
import { useApp } from '@/context/AppContext';

export default function SwapQueue() {
  const theme = useTheme();
  const { swapQueue, processNextSwap, removeSwapRequest } = useApp();

  return (
    <Box>
      <PageHeader
        title="Battery Swap Queue"
        subtitle="FIFO queue for battery swap requests — first in, first out"
        action={
          <Button
            variant="contained"
            startIcon={<PlayArrowOutlined />}
            onClick={processNextSwap}
            disabled={swapQueue.length === 0}
          >
            Process Next
          </Button>
        }
      />

      {/* FIFO Label */}
      <Card sx={{ p: 2, mb: 3, border: `1px solid ${theme.palette.divider}`, bgcolor: alpha(theme.palette.primary.main, 0.04) }}>
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
          <Chip label="FIFO" color="primary" size="small" sx={{ fontWeight: 700 }} />
          <Typography variant="body2" color="text.secondary">
            <strong>First In, First Out</strong> — The vehicle that requested a battery swap first will be processed first.
          </Typography>
        </Box>
      </Card>

      {/* Visual Queue */}
      {swapQueue.length > 0 && (
        <Card sx={{ p: 3, mb: 3, border: `1px solid ${theme.palette.divider}`, overflow: 'auto' }}>
          <Typography variant="h5" sx={{ mb: 2 }}>Queue Visualization</Typography>
          <Box sx={{ display: 'flex', alignItems: 'center', gap: 0, py: 2, minWidth: 'fit-content' }}>
            <Box
              sx={{
                px: 2, py: 1, borderRadius: 2,
                bgcolor: alpha(theme.palette.success.main, 0.1),
                border: `2px dashed ${theme.palette.success.main}`,
                flexShrink: 0,
              }}
            >
              <Typography variant="caption" fontWeight={700} color="success.main">FRONT</Typography>
            </Box>

            {swapQueue.map((req, i) => (
              <Box key={req.id} sx={{ display: 'flex', alignItems: 'center' }}>
                <ArrowForwardOutlined sx={{ color: 'text.secondary', mx: 1 }} />
                <Box
                  sx={{
                    px: 2.5, py: 1.5, borderRadius: 3,
                    bgcolor: i === 0 ? alpha(theme.palette.primary.main, 0.1) : alpha(theme.palette.background.default, 0.8),
                    border: `1px solid ${i === 0 ? theme.palette.primary.main : theme.palette.divider}`,
                    textAlign: 'center', flexShrink: 0, minWidth: 100,
                  }}
                >
                  <Typography variant="subtitle2" fontWeight={700}>{req.evId}</Typography>
                  <Typography variant="caption" color="text.secondary" sx={{ display: 'block' }}>{req.evModel}</Typography>
                </Box>
              </Box>
            ))}

            <ArrowForwardOutlined sx={{ color: 'text.secondary', mx: 1 }} />

            <Box
              sx={{
                px: 2, py: 1, borderRadius: 2,
                bgcolor: alpha(theme.palette.warning.main, 0.1),
                border: `2px dashed ${theme.palette.warning.main}`,
                flexShrink: 0,
              }}
            >
              <Typography variant="caption" fontWeight={700} color="warning.main">REAR</Typography>
            </Box>
          </Box>
        </Card>
      )}

      {/* Queue Table */}
      <Card sx={{ border: `1px solid ${theme.palette.divider}` }}>
        {swapQueue.length === 0 ? (
          <EmptyState title="Swap queue is empty" description="All battery swap requests have been processed." />
        ) : (
          <TableContainer>
            <Table>
              <TableHead>
                <TableRow>
                  <TableCell>Position</TableCell>
                  <TableCell>EV ID</TableCell>
                  <TableCell>Vehicle</TableCell>
                  <TableCell>Owner</TableCell>
                  <TableCell>Required Capacity</TableCell>
                  <TableCell>Requested At</TableCell>
                  <TableCell align="center">Actions</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {swapQueue.map((req, i) => (
                  <TableRow key={req.id} hover>
                    <TableCell>
                      <Chip label={i + 1} size="small" color={i === 0 ? 'primary' : 'default'} sx={{ fontWeight: 600, minWidth: 28 }} />
                    </TableCell>
                    <TableCell sx={{ fontWeight: 600 }}>{req.evId}</TableCell>
                    <TableCell>{req.evModel}</TableCell>
                    <TableCell>{req.owner}</TableCell>
                    <TableCell>{req.requiredCapacity} kWh</TableCell>
                    <TableCell>{new Date(req.requestedAt).toLocaleTimeString('en-IN', { hour: '2-digit', minute: '2-digit' })}</TableCell>
                    <TableCell align="center">
                      <Tooltip title="Remove from queue">
                        <IconButton size="small" onClick={() => removeSwapRequest(req.id)}>
                          <DeleteOutline fontSize="small" />
                        </IconButton>
                      </Tooltip>
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          </TableContainer>
        )}
      </Card>
    </Box>
  );
}
