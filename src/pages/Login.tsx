import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Typography from '@mui/material/Typography';
import TextField from '@mui/material/TextField';
import Button from '@mui/material/Button';
import Chip from '@mui/material/Chip';
import CircularProgress from '@mui/material/CircularProgress';
import BoltOutlined from '@mui/icons-material/BoltOutlined';
import CheckCircleOutline from '@mui/icons-material/CheckCircleOutline';

import { useApp } from '@/context/AppContext';
import { api } from '@/services/api';
import { mockUsers } from '@/data/mockUsers';

// Login Page with C++ Backend Authentication
// Module Owner: Abhishek Ghansala (UI + DSA + Final Integration, ID: 2518132)
// Connects to Aditya Singh's backend POST /api/login endpoint with SQLite validation
export const Login = () => {
  const navigate = useNavigate();
  const { login, showSnackbar } = useApp();

  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [loading, setLoading] = useState(false);

  const handleLogin = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!email || !password) {
      showSnackbar('Please enter both email and password', 'warning');
      return;
    }

    setLoading(true);
    try {
      // 1. Try real C++ Crow backend login endpoint
      const res = await api.login(email, password);
      if (res && res.success && res.user) {
        login(res.user);
        navigate('/dashboard');
        return;
      } else {
        showSnackbar(res.message || 'Invalid credentials', 'error');
      }
    } catch (err: any) {
      // 2. If C++ backend is not running or network failed, fallback to mock database
      console.log('Backend not reachable or error:', err.message);
      const user = mockUsers.find(
        (u) =>
          (u.email?.toLowerCase() === email.toLowerCase() ||
           u.name.toLowerCase() === email.toLowerCase()) &&
          u.password === password
      );

      if (user) {
        login(user);
        showSnackbar(`Welcome ${user.name}! (Offline Mock Mode)`, 'info');
        navigate('/dashboard');
      } else {
        showSnackbar('Invalid email or password', 'error');
      }
    } finally {
      setLoading(false);
    }
  };

  const fillCredentials = (demoEmail: string, demoPass: string) => {
    setEmail(demoEmail);
    setPassword(demoPass);
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
          {/* Logo */}
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
            Smart EV Hub
          </Typography>
          <Typography variant="body2" color="text.secondary" sx={{ mb: 3, textAlign: 'center' }}>
            EV Charging & Battery-Swap Management System
          </Typography>

          {/* Quick Demo Credentials */}
          <Box sx={{ width: '100%', mb: 2, p: 1.5, bgcolor: 'action.hover', borderRadius: 2 }}>
            <Typography variant="caption" color="text.secondary" sx={{ display: 'block', mb: 1, fontWeight: 600 }}>
              Quick Demo Logins (from SQLite Users table):
            </Typography>
            <Box sx={{ display: 'flex', gap: 1, flexWrap: 'wrap' }}>
              <Chip
                label="Aditya (Lead): aditya@evhub.com"
                size="small"
                onClick={() => fillCredentials('aditya@evhub.com', 'aditya')}
                clickable
                color="primary"
                variant="outlined"
              />
              <Chip
                label="Admin: admin@evhub.com"
                size="small"
                onClick={() => fillCredentials('admin@evhub.com', 'admin123')}
                clickable
                variant="outlined"
              />
              <Chip
                label="Abhishek: abhishek@evhub.com"
                size="small"
                onClick={() => fillCredentials('abhishek@evhub.com', 'abhi123')}
                clickable
                color="secondary"
                variant="outlined"
              />
              <Chip
                label="Operator: operator@evhub.com"
                size="small"
                onClick={() => fillCredentials('operator@evhub.com', 'operator123')}
                clickable
                variant="outlined"
              />
            </Box>
          </Box>

          <form onSubmit={handleLogin} style={{ width: '100%' }}>
            <TextField
              fullWidth
              label="Email Address"
              type="email"
              variant="outlined"
              margin="normal"
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              placeholder="e.g. admin@evhub.com"
              autoFocus
              autoComplete="email"
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
              placeholder="Enter password"
              sx={{ mb: 3 }}
              autoComplete="current-password"
              disabled={loading}
            />
            <Button
              fullWidth
              type="submit"
              variant="contained"
              size="large"
              disabled={loading}
              startIcon={loading ? <CircularProgress size={20} color="inherit" /> : <CheckCircleOutline />}
              sx={{ borderRadius: 2, py: 1.5, fontWeight: 'bold' }}
            >
              {loading ? 'Authenticating...' : 'Sign In'}
            </Button>
          </form>
        </CardContent>
      </Card>
    </Box>
  );
};
export default Login;
