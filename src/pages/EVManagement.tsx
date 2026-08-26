import { useState, useMemo } from 'react';
import { useNavigate } from 'react-router-dom';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import Button from '@mui/material/Button';
import TextField from '@mui/material/TextField';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import IconButton from '@mui/material/IconButton';
import Tooltip from '@mui/material/Tooltip';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import MenuItem from '@mui/material/MenuItem';
import InputAdornment from '@mui/material/InputAdornment';
import { useTheme } from '@mui/material/styles';

import AddOutlined from '@mui/icons-material/AddOutlined';
import SearchOutlined from '@mui/icons-material/SearchOutlined';
import VisibilityOutlined from '@mui/icons-material/VisibilityOutlined';
import EditOutlined from '@mui/icons-material/EditOutlined';
import DeleteOutline from '@mui/icons-material/DeleteOutline';

import PageHeader from '@/components/common/PageHeader';
import StatusChip from '@/components/common/StatusChip';
import ConfirmDialog from '@/components/common/ConfirmDialog';
import EmptyState from '@/components/common/EmptyState';
import { useApp } from '@/context/AppContext';
import type { EV, EVStatus } from '@/types';

export default function EVManagement() {
  const theme = useTheme();
  const navigate = useNavigate();
  const { evs, addEV, deleteEV, updateEV } = useApp();

  const [search, setSearch] = useState('');
  const [filterStatus, setFilterStatus] = useState<string>('all');
  const [formOpen, setFormOpen] = useState(false);
  const [editEV, setEditEV] = useState<EV | null>(null);
  const [deleteId, setDeleteId] = useState<string | null>(null);

  // Form state
  const [formData, setFormData] = useState({ id: '', owner: '', model: '', batteryCapacity: '', currentCharge: '' });
  const [formErrors, setFormErrors] = useState<Record<string, string>>({});

  const filtered = useMemo(() => {
    return evs.filter(ev => {
      const matchSearch = !search ||
        ev.id.toLowerCase().includes(search.toLowerCase()) ||
        ev.owner.toLowerCase().includes(search.toLowerCase()) ||
        ev.model.toLowerCase().includes(search.toLowerCase());
      const matchStatus = filterStatus === 'all' || ev.status === filterStatus;
      return matchSearch && matchStatus;
    });
  }, [evs, search, filterStatus]);

  const openForm = (ev?: EV) => {
    if (ev) {
      setEditEV(ev);
      setFormData({
        id: ev.id,
        owner: ev.owner,
        model: ev.model,
        batteryCapacity: String(ev.batteryCapacity),
        currentCharge: String(ev.currentCharge),
      });
    } else {
      setEditEV(null);
      setFormData({ id: '', owner: '', model: '', batteryCapacity: '', currentCharge: '' });
    }
    setFormErrors({});
    setFormOpen(true);
  };

  const validate = () => {
    const errors: Record<string, string> = {};
    if (!formData.id.trim()) errors.id = 'Vehicle ID is required';
    else if (!editEV && evs.some(e => e.id === formData.id)) errors.id = 'Vehicle ID already exists';
    if (!formData.owner.trim()) errors.owner = 'Owner name is required';
    if (!formData.model.trim()) errors.model = 'Vehicle model is required';
    const cap = Number(formData.batteryCapacity);
    if (!formData.batteryCapacity || isNaN(cap) || cap <= 0) errors.batteryCapacity = 'Must be a positive number';
    const charge = Number(formData.currentCharge);
    if (formData.currentCharge === '' || isNaN(charge) || charge < 0 || charge > 100) errors.currentCharge = 'Must be 0–100';
    setFormErrors(errors);
    return Object.keys(errors).length === 0;
  };

  const handleSubmit = () => {
    if (!validate()) return;
    if (editEV) {
      updateEV(editEV.id, {
        owner: formData.owner,
        model: formData.model,
        batteryCapacity: Number(formData.batteryCapacity),
        currentCharge: Number(formData.currentCharge),
      });
    } else {
      addEV({
        id: formData.id,
        owner: formData.owner,
        model: formData.model,
        batteryCapacity: Number(formData.batteryCapacity),
        currentCharge: Number(formData.currentCharge),
        status: 'available' as EVStatus,
      });
    }
    setFormOpen(false);
  };

  const formatDate = (iso: string) => {
    return new Date(iso).toLocaleDateString('en-IN', { day: 'numeric', month: 'short', year: 'numeric' });
  };

  return (
    <Box>
      <PageHeader
        title="EV Management"
        subtitle="Manage registered electric vehicles"
        action={
          <Button variant="contained" startIcon={<AddOutlined />} onClick={() => openForm()}>
            Register EV
          </Button>
        }
      />

      {/* Filters */}
      <Card sx={{ p: 2, mb: 2.5, border: `1px solid ${theme.palette.divider}`, display: 'flex', gap: 2, flexWrap: 'wrap' }}>
        <TextField
          placeholder="Search EVs..."
          value={search}
          onChange={e => setSearch(e.target.value)}
          size="small"
          sx={{ minWidth: 240 }}
          slotProps={{
            input: {
              startAdornment: <InputAdornment position="start"><SearchOutlined fontSize="small" /></InputAdornment>,
            },
          }}
        />
        <TextField
          select
          value={filterStatus}
          onChange={e => setFilterStatus(e.target.value)}
          size="small"
          sx={{ minWidth: 160 }}
          label="Status"
        >
          <MenuItem value="all">All Statuses</MenuItem>
          <MenuItem value="available">Available</MenuItem>
          <MenuItem value="charging">Charging</MenuItem>
          <MenuItem value="waiting">Waiting</MenuItem>
          <MenuItem value="swapping">Swapping</MenuItem>
        </TextField>
      </Card>

      {/* Table */}
      <Card sx={{ border: `1px solid ${theme.palette.divider}` }}>
        {filtered.length === 0 ? (
          <EmptyState
            title="No electric vehicles found"
            description={search || filterStatus !== 'all' ? 'Try adjusting your search or filters.' : 'No EVs registered yet.'}
            actionLabel={!search && filterStatus === 'all' ? 'Register EV' : undefined}
            onAction={!search && filterStatus === 'all' ? () => openForm() : undefined}
          />
        ) : (
          <TableContainer>
            <Table>
              <TableHead>
                <TableRow>
                  <TableCell>EV ID</TableCell>
                  <TableCell>Owner</TableCell>
                  <TableCell>Model</TableCell>
                  <TableCell align="right">Capacity</TableCell>
                  <TableCell align="right">Charge</TableCell>
                  <TableCell>Status</TableCell>
                  <TableCell>Last Activity</TableCell>
                  <TableCell align="center">Actions</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {filtered.map(ev => (
                  <TableRow key={ev.id} hover>
                    <TableCell sx={{ fontWeight: 600 }}>{ev.id}</TableCell>
                    <TableCell>{ev.owner}</TableCell>
                    <TableCell>{ev.model}</TableCell>
                    <TableCell align="right">{ev.batteryCapacity} kWh</TableCell>
                    <TableCell align="right">{ev.currentCharge}%</TableCell>
                    <TableCell><StatusChip status={ev.status} /></TableCell>
                    <TableCell>{formatDate(ev.lastActivity)}</TableCell>
                    <TableCell align="center">
                      <Tooltip title="View details">
                        <IconButton size="small" onClick={() => navigate(`/evs/${ev.id}`)}>
                          <VisibilityOutlined fontSize="small" />
                        </IconButton>
                      </Tooltip>
                      <Tooltip title="Edit">
                        <IconButton size="small" onClick={() => openForm(ev)}>
                          <EditOutlined fontSize="small" />
                        </IconButton>
                      </Tooltip>
                      <Tooltip title="Delete">
                        <IconButton size="small" onClick={() => setDeleteId(ev.id)}>
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

      {/* Register/Edit Dialog */}
      <Dialog open={formOpen} onClose={() => setFormOpen(false)} maxWidth="sm" fullWidth>
        <DialogTitle>{editEV ? 'Edit EV' : 'Register EV'}</DialogTitle>
        <DialogContent sx={{ display: 'flex', flexDirection: 'column', gap: 2.5, pt: '16px !important' }}>
          <TextField
            label="Vehicle ID"
            value={formData.id}
            onChange={e => setFormData(prev => ({ ...prev, id: e.target.value }))}
            error={!!formErrors.id}
            helperText={formErrors.id}
            disabled={!!editEV}
            required
          />
          <TextField
            label="Owner Name"
            value={formData.owner}
            onChange={e => setFormData(prev => ({ ...prev, owner: e.target.value }))}
            error={!!formErrors.owner}
            helperText={formErrors.owner}
            required
          />
          <TextField
            label="Vehicle Model"
            value={formData.model}
            onChange={e => setFormData(prev => ({ ...prev, model: e.target.value }))}
            error={!!formErrors.model}
            helperText={formErrors.model}
            required
          />
          <TextField
            label="Battery Capacity (kWh)"
            type="number"
            value={formData.batteryCapacity}
            onChange={e => setFormData(prev => ({ ...prev, batteryCapacity: e.target.value }))}
            error={!!formErrors.batteryCapacity}
            helperText={formErrors.batteryCapacity}
            required
          />
          <TextField
            label="Current Charge (%)"
            type="number"
            value={formData.currentCharge}
            onChange={e => setFormData(prev => ({ ...prev, currentCharge: e.target.value }))}
            error={!!formErrors.currentCharge}
            helperText={formErrors.currentCharge || '0–100'}
            required
            slotProps={{ htmlInput: { min: 0, max: 100 } }}
          />
        </DialogContent>
        <DialogActions sx={{ px: 3, pb: 2 }}>
          <Button onClick={() => setFormOpen(false)}>Cancel</Button>
          <Button variant="contained" onClick={handleSubmit}>
            {editEV ? 'Save Changes' : 'Register EV'}
          </Button>
        </DialogActions>
      </Dialog>

      {/* Delete Confirmation */}
      <ConfirmDialog
        open={!!deleteId}
        title="Delete EV"
        message={`Are you sure you want to remove ${deleteId}? This action cannot be undone.`}
        onConfirm={() => { if (deleteId) deleteEV(deleteId); setDeleteId(null); }}
        onCancel={() => setDeleteId(null)}
      />
    </Box>
  );
}
