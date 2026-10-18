import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Typography from '@mui/material/Typography';
import TextField from '@mui/material/TextField';
import Button from '@mui/material/Button';
import CircularProgress from '@mui/material/CircularProgress';
import Chip from '@mui/material/Chip';
import BoltOutlined from '@mui/icons-material/BoltOutlined';
import StorageOutlined from '@mui/icons-material/StorageOutlined';
import { useApp } from '@/context/AppContext';
import { loginUser } from '@/services/authService';

export const Login = () => {
  const navigate = useNavigate();
  const { login, showSnackbar } = useApp();
  const [identifier, setIdentifier] = useState('');
  const [password, setPassword] = useState('');
  const [loading, setLoading] = useState(false);

  const handleLogin = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!identifier.trim() || !password.trim()) {
      showSnackbar('Please enter both email/username and password', 'warning');
      return;
    }

    setLoading(true);
    try {
      const result = await loginUser(identifier, password);
      login(result.user);
      if (result.source === 'backend_sqlite') {
        showSnackbar(`Welcome, ${result.user.name}! (Authenticated via SQLite)`, 'success');
      } else {
        showSnackbar(`Welcome, ${result.user.name}! (Offline mode)`, 'info');
      }
      navigate('/dashboard');
    } catch (err: any) {
      showSnackbar(err.message || 'Invalid credentials', 'error');
    } finally {
      setLoading(false);
    }
  };

  const handleQuickFill = (id: string, pass: string) => {
    setIdentifier(id);
    setPassword(pass);
  };

  return (
    <Box
      sx={{
        minHeight: '100vh',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        bgcolor: 'background.default',
        p: 2,
      }}
    >
      <Card sx={{ maxWidth: 440, width: '100%', borderRadius: 4, boxShadow: 6 }}>
        <CardContent sx={{ p: 4, display: 'flex', flexDirection: 'column', alignItems: 'center' }}>
          <Box
            sx={{
              width: 56,
              height: 56,
              borderRadius: 3,
              bgcolor: 'primary.main',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              mb: 2,
            }}
          >
            <BoltOutlined sx={{ fontSize: 32, color: '#fff' }} />
          </Box>
          <Typography variant="h5" fontWeight="bold" gutterBottom>
            Welcome Back
          </Typography>
          <Typography variant="body2" color="text.secondary" sx={{ mb: 3, textAlign: 'center' }}>
            Sign in to the Smart EV Hub Management System
          </Typography>

          <form onSubmit={handleLogin} style={{ width: '100%' }}>
            <TextField
              fullWidth
              label="Email or Username"
              placeholder="admin@evhub.com or Aditya"
              variant="outlined"
              margin="normal"
              value={identifier}
              onChange={(e) => setIdentifier(e.target.value)}
              autoFocus
              autoComplete="off"
              disabled={loading}
            />
            <TextField
              fullWidth
              label="Password"
              type="password"
              variant="outlined"
              margin="normal"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              sx={{ mb: 2 }}
              autoComplete="off"
              disabled={loading}
            />
            <Button
              fullWidth
              type="submit"
              variant="contained"
              size="large"
              disabled={loading}
              sx={{ borderRadius: 2, py: 1.5, fontWeight: 'bold', mb: 3 }}
            >
              {loading ? <CircularProgress size={24} color="inherit" /> : 'Sign In'}
            </Button>
          </form>

          {/* Demo quick-fill chips for viva & quick testing */}
          <Box sx={{ width: '100%', pt: 2, borderTop: '1px solid', borderColor: 'divider' }}>
            <Typography variant="caption" color="text.secondary" sx={{ display: 'flex', alignItems: 'center', gap: 0.5, mb: 1 }}>
              <StorageOutlined fontSize="small" /> Quick Demo Accounts (SQLite Seed):
            </Typography>
            <Box sx={{ display: 'flex', flexWrap: 'wrap', gap: 1 }}>
              <Chip
                label="Aditya (Lead)"
                size="small"
                onClick={() => handleQuickFill('aditya@evhub.com', 'aditya')}
                clickable
                color="primary"
                variant="outlined"
              />
              <Chip
                label="Admin User"
                size="small"
                onClick={() => handleQuickFill('admin@evhub.com', 'admin123')}
                clickable
                variant="outlined"
              />
              <Chip
                label="Operator"
                size="small"
                onClick={() => handleQuickFill('operator@evhub.com', 'operator123')}
                clickable
                variant="outlined"
              />
            </Box>
          </Box>
        </CardContent>
      </Card>
    </Box>
  );
};
