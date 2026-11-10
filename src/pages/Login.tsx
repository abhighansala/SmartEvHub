import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Typography from '@mui/material/Typography';
import TextField from '@mui/material/TextField';
import Button from '@mui/material/Button';
import Chip from '@mui/material/Chip';
import Tabs from '@mui/material/Tabs';
import Tab from '@mui/material/Tab';
import MenuItem from '@mui/material/MenuItem';
import CircularProgress from '@mui/material/CircularProgress';
import BoltOutlined from '@mui/icons-material/BoltOutlined';
import CheckCircleOutline from '@mui/icons-material/CheckCircleOutline';
import PersonAddOutlined from '@mui/icons-material/PersonAddOutlined';

import { useApp } from '@/context/AppContext';
import { api } from '@/services/api';
import { mockUsers } from '@/data/mockUsers';

// Login & Registration Page with C++ Backend Bcrypt Authentication & JWT Issuance
// Module Owner: Abhishek Ghansala (UI + DSA + Final Integration, ID: 2518132)
// Connects to Aditya Singh's backend POST /api/login and POST /api/register endpoints
export const Login = () => {
  const navigate = useNavigate();
  const { login, showSnackbar } = useApp();

  const [tabIndex, setTabIndex] = useState(0); // 0 = Login, 1 = Register
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');

  // Register fields
  const [regName, setRegName] = useState('');
  const [regEmail, setRegEmail] = useState('');
  const [regPassword, setRegPassword] = useState('');
  const [regRole, setRegRole] = useState<'user' | 'operator'>('user');

  const [loading, setLoading] = useState(false);

  const handleLogin = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!email || !password) {
      showSnackbar('Please enter both email/username and password', 'warning');
      return;
    }

    setLoading(true);
    try {
      // 1. Try real C++ Crow backend login endpoint
      const res = await api.login(email.trim(), password);
      if (res && res.success && res.user) {
        login(res.user, res.token);
        navigate('/dashboard');
        return;
      } else {
        showSnackbar(res.message || 'Invalid credentials', 'error');
      }
    } catch (err: any) {
      console.log('Backend not reachable or error:', err.message);
      // 2. Fallback to offline mock database if server is offline
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
        showSnackbar(err.message || 'Invalid email or password', 'error');
      }
    } finally {
      setLoading(false);
    }
  };

  const handleRegister = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!regName.trim() || !regEmail.trim() || !regPassword) {
      showSnackbar('Please fill all required registration fields', 'warning');
      return;
    }
    if (regName.trim().length < 2) {
      showSnackbar('Name must be at least 2 characters long', 'warning');
      return;
    }
    if (!regEmail.includes('@') || !regEmail.includes('.')) {
      showSnackbar('Please enter a valid email address', 'warning');
      return;
    }
    if (regPassword.length < 6) {
      showSnackbar('Password must be at least 6 characters long', 'warning');
      return;
    }

    setLoading(true);
    try {
      const res = await api.register(regName.trim(), regEmail.trim(), regPassword, regRole);
      if (res && res.success && res.user) {
        login(res.user, res.token);
        showSnackbar('Account created successfully!', 'success');
        navigate('/dashboard');
      } else {
        showSnackbar(res.message || 'Registration failed', 'error');
      }
    } catch (err: any) {
      showSnackbar(err.message || 'Registration failed. Server may be offline.', 'error');
    } finally {
      setLoading(false);
    }
  };

  const fillCredentials = (demoEmail: string, demoPass: string) => {
    setEmail(demoEmail);
    setPassword(demoPass);
    setTabIndex(0);
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
      <Card sx={{ maxWidth: 460, width: '100%', borderRadius: 4, boxShadow: 6 }}>
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
              mb: 1.5,
            }}
          >
            <BoltOutlined sx={{ fontSize: 32, color: '#fff' }} />
          </Box>

          <Typography variant="h5" fontWeight="bold" gutterBottom>
            Smart EV Hub
          </Typography>
          <Typography variant="body2" color="text.secondary" sx={{ mb: 2.5, textAlign: 'center' }}>
            EV Charging & Battery-Swap Station Management
          </Typography>

          <Tabs
            value={tabIndex}
            onChange={(_, val) => setTabIndex(val)}
            variant="fullWidth"
            sx={{ width: '100%', mb: 2.5, borderBottom: 1, borderColor: 'divider' }}
          >
            <Tab label="Sign In" />
            <Tab label="Create Account" />
          </Tabs>

          {/* Quick Demo Credentials */}
          {tabIndex === 0 && (
            <Box sx={{ width: '100%', mb: 2, p: 1.5, bgcolor: 'action.hover', borderRadius: 2 }}>
              <Typography variant="caption" color="text.secondary" sx={{ display: 'block', mb: 1, fontWeight: 600 }}>
                Demo Accounts (from SQLite Database):
              </Typography>
              <Box sx={{ display: 'flex', gap: 1, flexWrap: 'wrap' }}>
                <Chip
                  label="Aditya (Lead)"
                  size="small"
                  onClick={() => fillCredentials('aditya@evhub.com', 'aditya')}
                  clickable
                  color="primary"
                  variant="outlined"
                />
                <Chip
                  label="Admin"
                  size="small"
                  onClick={() => fillCredentials('admin@evhub.com', 'admin123')}
                  clickable
                  variant="outlined"
                />
                <Chip
                  label="Abhishek"
                  size="small"
                  onClick={() => fillCredentials('abhishek@evhub.com', 'abhishek')}
                  clickable
                  color="secondary"
                  variant="outlined"
                />
                <Chip
                  label="Operator"
                  size="small"
                  onClick={() => fillCredentials('operator@evhub.com', 'operator123')}
                  clickable
                  variant="outlined"
                />
              </Box>
            </Box>
          )}

          {tabIndex === 0 ? (
            <form onSubmit={handleLogin} style={{ width: '100%' }}>
              <TextField
                fullWidth
                label="Email or Username"
                variant="outlined"
                margin="normal"
                value={email}
                onChange={(e) => setEmail(e.target.value)}
                placeholder="e.g. admin@evhub.com"
                autoFocus
                autoComplete="username"
                disabled={loading}
                required
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
                required
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
          ) : (
            <form onSubmit={handleRegister} style={{ width: '100%' }}>
              <TextField
                fullWidth
                label="Full Name"
                variant="outlined"
                margin="dense"
                value={regName}
                onChange={(e) => setRegName(e.target.value)}
                placeholder="e.g. Priyanshu Sharma"
                disabled={loading}
                required
              />
              <TextField
                fullWidth
                label="Email Address"
                type="email"
                variant="outlined"
                margin="dense"
                value={regEmail}
                onChange={(e) => setRegEmail(e.target.value)}
                placeholder="e.g. priyanshu@evhub.com"
                disabled={loading}
                required
              />
              <TextField
                fullWidth
                label="Password (min 6 characters)"
                type="password"
                variant="outlined"
                margin="dense"
                value={regPassword}
                onChange={(e) => setRegPassword(e.target.value)}
                placeholder="Choose a strong password"
                disabled={loading}
                required
              />
              <TextField
                select
                fullWidth
                label="Role"
                variant="outlined"
                margin="dense"
                value={regRole}
                onChange={(e) => setRegRole(e.target.value as 'user' | 'operator')}
                disabled={loading}
                sx={{ mb: 2.5 }}
              >
                <MenuItem value="user">User / EV Driver</MenuItem>
                <MenuItem value="operator">Station Operator</MenuItem>
              </TextField>

              <Button
                fullWidth
                type="submit"
                variant="contained"
                size="large"
                disabled={loading}
                startIcon={loading ? <CircularProgress size={20} color="inherit" /> : <PersonAddOutlined />}
                sx={{ borderRadius: 2, py: 1.5, fontWeight: 'bold' }}
              >
                {loading ? 'Registering...' : 'Create Account'}
              </Button>
            </form>
          )}
        </CardContent>
      </Card>
    </Box>
  );
};
export default Login;
