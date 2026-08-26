import { useNavigate, useLocation } from 'react-router-dom';
import Box from '@mui/material/Box';
import Drawer from '@mui/material/Drawer';
import List from '@mui/material/List';
import ListItem from '@mui/material/ListItem';
import ListItemButton from '@mui/material/ListItemButton';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';
import Typography from '@mui/material/Typography';
import Divider from '@mui/material/Divider';
import IconButton from '@mui/material/IconButton';
import Button from '@mui/material/Button';
import Avatar from '@mui/material/Avatar';
import Tooltip from '@mui/material/Tooltip';
import useMediaQuery from '@mui/material/useMediaQuery';
import { useTheme } from '@mui/material/styles';
import { useApp } from '@/context/AppContext';

import DashboardOutlined from '@mui/icons-material/DashboardOutlined';
import DirectionsCarOutlined from '@mui/icons-material/DirectionsCarOutlined';
import EvStationOutlined from '@mui/icons-material/EvStationOutlined';
import BatteryChargingFullOutlined from '@mui/icons-material/BatteryChargingFullOutlined';
import SwapHorizOutlined from '@mui/icons-material/SwapHorizOutlined';
import FormatListNumberedOutlined from '@mui/icons-material/FormatListNumberedOutlined';
import ElectricBoltOutlined from '@mui/icons-material/ElectricBoltOutlined';
import ReceiptLongOutlined from '@mui/icons-material/ReceiptLongOutlined';
import AssessmentOutlined from '@mui/icons-material/AssessmentOutlined';
import BarChartOutlined from '@mui/icons-material/BarChartOutlined';
import SettingsOutlined from '@mui/icons-material/SettingsOutlined';
import ChevronLeft from '@mui/icons-material/ChevronLeft';
import ChevronRight from '@mui/icons-material/ChevronRight';
import BoltOutlined from '@mui/icons-material/BoltOutlined';
import SwapCallsOutlined from '@mui/icons-material/SwapCallsOutlined';
import LoginOutlined from '@mui/icons-material/LoginOutlined';
import LogoutOutlined from '@mui/icons-material/LogoutOutlined';

const EXPANDED_WIDTH = 260;
const COLLAPSED_WIDTH = 72;

interface NavItem {
  label: string;
  path: string;
  icon: React.ReactNode;
}

interface NavGroup {
  title: string;
  items: NavItem[];
}

const navGroups: NavGroup[] = [
  {
    title: '',
    items: [
      { label: 'Dashboard', path: '/dashboard', icon: <DashboardOutlined /> },
    ],
  },
  {
    title: 'Management',
    items: [
      { label: 'EV Management', path: '/evs', icon: <DirectionsCarOutlined /> },
      { label: 'Charging Stations', path: '/stations', icon: <EvStationOutlined /> },
      { label: 'Batteries', path: '/batteries', icon: <BatteryChargingFullOutlined /> },
      { label: 'Battery Swap', path: '/battery-swap', icon: <SwapHorizOutlined /> },
    ],
  },
  {
    title: 'Operations',
    items: [
      { label: 'Charging Queue', path: '/charging-queue', icon: <FormatListNumberedOutlined /> },
      { label: 'Swap Queue', path: '/swap-queue', icon: <SwapCallsOutlined /> },
      { label: 'Active Sessions', path: '/sessions', icon: <ElectricBoltOutlined /> },
      { label: 'Transactions', path: '/transactions', icon: <ReceiptLongOutlined /> },
    ],
  },
  {
    title: 'Analytics',
    items: [
      { label: 'Reports', path: '/reports', icon: <AssessmentOutlined /> },
      { label: 'Analytics', path: '/analytics', icon: <BarChartOutlined /> },
    ],
  },
  {
    title: 'System',
    items: [
      { label: 'Settings', path: '/settings', icon: <SettingsOutlined /> },
    ],
  },
];

interface SidebarProps {
  open: boolean;
  collapsed: boolean;
  onToggleCollapse: () => void;
  onClose: () => void;
}

export default function Sidebar({ open, collapsed, onToggleCollapse, onClose }: SidebarProps) {
  const theme = useTheme();
  const navigate = useNavigate();
  const location = useLocation();
  const { settings, isAuthenticated, currentUser, logout } = useApp();
  const isMobile = useMediaQuery(theme.breakpoints.down('md'));
  const isUserLoggedIn = false;

  const width = collapsed && !isMobile ? COLLAPSED_WIDTH : EXPANDED_WIDTH;
  const isExpanded = isMobile || !collapsed;

  const handleNav = (path: string) => {
    navigate(path);
    if (isMobile) onClose();
  };

  const sidebarContent = (
    <Box
      sx={{
        display: 'flex',
        flexDirection: 'column',
        height: '100%',
        overflowX: 'hidden',
      }}
    >
      {/* Brand Header */}
      <Box
        sx={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: isExpanded ? 'space-between' : 'center',
          px: isExpanded ? 2.5 : 0,
          py: 2,
          minHeight: 72,
          overflow: 'hidden',
          whiteSpace: 'nowrap',
          transition: 'padding 0.2s',
        }}
      >
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1.5 }}>
          <Tooltip title={!isExpanded && !isMobile ? 'Expand sidebar' : ''} placement="right">
            <Box
              onClick={!isMobile && collapsed ? onToggleCollapse : undefined}
              sx={{
                width: 36,
                height: 36,
                borderRadius: 2,
                bgcolor: 'primary.main',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                flexShrink: 0,
                cursor: !isMobile && collapsed ? 'pointer' : 'default',
                '&:hover': !isMobile && collapsed ? { opacity: 0.9 } : {},
                transition: 'opacity 0.2s',
              }}
            >
              <BoltOutlined sx={{ fontSize: 20, color: '#fff' }} />
            </Box>
          </Tooltip>
          <Box
            sx={{
              opacity: isExpanded ? 1 : 0,
              width: isExpanded ? 120 : 0,
              transition: 'opacity 0.2s, width 0.2s',
              overflow: 'hidden',
            }}
          >
            <Typography variant="subtitle1" sx={{ fontWeight: 700, lineHeight: 1.2 }}>
              Smart EV Hub
            </Typography>
            <Typography variant="caption" color="text.secondary">
              Management System
            </Typography>
          </Box>
        </Box>

        {/* Collapse toggle (desktop only) */}
        {!isMobile && (
          <Box
            sx={{
              opacity: isExpanded ? 1 : 0,
              width: isExpanded ? 32 : 0,
              pointerEvents: isExpanded ? 'auto' : 'none',
              transition: 'opacity 0.2s, width 0.2s',
              overflow: 'hidden',
            }}
          >
            <Tooltip title="Collapse sidebar">
              <IconButton onClick={onToggleCollapse} size="small">
                <ChevronLeft />
              </IconButton>
            </Tooltip>
          </Box>
        )}
      </Box>

      <Divider />

      {/* Navigation */}
      <Box sx={{ flex: 1, overflowY: 'auto', overflowX: 'hidden', py: 2 }}>
        {navGroups.map((group, gi) => (
          <Box key={gi} sx={{ mt: gi > 0 ? 2 : 0 }}>
            {group.title && (
              <Box
                sx={{
                  opacity: isExpanded ? 1 : 0,
                  height: isExpanded ? 24 : 0,
                  transition: 'opacity 0.2s, height 0.2s',
                  overflow: 'hidden',
                  px: 3,
                  mb: 1,
                }}
              >
                <Typography variant="overline" color="text.secondary" sx={{ display: 'block', lineHeight: 1 }}>
                  {group.title}
                </Typography>
              </Box>
            )}

            {group.title && !isExpanded && gi > 0 && (
              <Divider sx={{ my: 1.5, mx: 2 }} />
            )}

            <List disablePadding sx={{ px: 1.5 }}>
              {group.items.map(item => {
                const isActive = location.pathname === item.path || location.pathname.startsWith(item.path + '/');
                return (
                  <ListItem key={item.path} disablePadding sx={{ mb: 0.5 }}>
                    <Tooltip title={!isExpanded ? item.label : ''} placement="right" arrow>
                      <ListItemButton
                        selected={isActive}
                        onClick={() => handleNav(item.path)}
                        sx={{
                          minHeight: 44,
                          borderRadius: 2,
                          px: 2,
                          justifyContent: 'flex-start',
                          transition: 'all 0.2s',
                          '&.Mui-selected': {
                            bgcolor: 'primary.main',
                            color: 'primary.contrastText',
                            '&:hover': { bgcolor: 'primary.dark' },
                            '& .MuiListItemIcon-root': { color: 'inherit' },
                          },
                        }}
                      >
                        <ListItemIcon
                          sx={{
                            minWidth: 0,
                            mr: isExpanded ? 2 : 0,
                            justifyContent: 'center',
                            color: isActive ? 'inherit' : 'text.secondary',
                            transition: 'margin 0.2s',
                          }}
                        >
                          {item.icon}
                        </ListItemIcon>
                        <ListItemText
                          primary={item.label}
                          primaryTypographyProps={{ sx: { fontWeight: isActive ? 600 : 500 } }}
                          sx={{
                            m: 0,
                            opacity: isExpanded ? 1 : 0,
                            width: isExpanded ? 140 : 0,
                            overflow: 'hidden',
                            whiteSpace: 'nowrap',
                            transition: 'opacity 0.2s, width 0.2s',
                          }}
                        />
                      </ListItemButton>
                    </Tooltip>
                  </ListItem>
                );
              })}
            </List>
          </Box>
        ))}
      </Box>

      <Divider />

      {/* Bottom section */}
      <Box sx={{ p: 2, display: 'flex', flexDirection: 'column', gap: 1 }}>
        {isAuthenticated ? (
          <Box
            sx={{
              display: 'flex',
              alignItems: 'center',
              gap: 1.5,
              px: isExpanded ? 1 : 0,
              py: isExpanded ? 1 : 0,
              justifyContent: isExpanded ? 'space-between' : 'center',
              borderRadius: 3,
              bgcolor: isExpanded ? (theme.palette.mode === 'dark' ? 'rgba(255,255,255,0.04)' : 'rgba(0,0,0,0.03)') : 'transparent',
              transition: 'all 0.2s',
              overflow: 'hidden',
              whiteSpace: 'nowrap',
            }}
          >
            <Tooltip title={!isExpanded && currentUser ? `${currentUser.name} — ${currentUser.role}` : ""} placement="right">
              <Avatar sx={{ width: 32, height: 32, bgcolor: 'secondary.main', fontSize: '0.875rem', flexShrink: 0 }}>
                {currentUser?.name.substring(0, 2).toUpperCase()}
              </Avatar>
            </Tooltip>
            <Box
              sx={{
                opacity: isExpanded ? 1 : 0,
                width: isExpanded ? 'auto' : 0,
                transition: 'opacity 0.2s, width 0.2s',
                overflow: 'hidden',
                flexGrow: 1,
              }}
            >
              <Typography variant="body2" sx={{ fontWeight: 600 }}>{currentUser?.name}</Typography>
              <Typography variant="caption" color="text.secondary" sx={{ textTransform: 'capitalize' }}>{currentUser?.role}</Typography>
            </Box>
            
            {isExpanded && (
              <Tooltip title="Logout">
                <IconButton size="small" onClick={logout} sx={{ flexShrink: 0 }}>
                  <LogoutOutlined fontSize="small" />
                </IconButton>
              </Tooltip>
            )}
          </Box>
        ) : (
          <Box sx={{ px: isExpanded ? 1 : 0 }}>
            {isExpanded ? (
              <Button
                variant="contained"
                fullWidth
                startIcon={<LoginOutlined />}
                onClick={() => handleNav('/login')}
                sx={{ borderRadius: 2 }}
              >
                Login
              </Button>
            ) : (
              <Tooltip title="Login" placement="right">
                <IconButton 
                  color="primary" 
                  onClick={() => handleNav('/login')}
                  sx={{ bgcolor: 'primary.main', color: 'primary.contrastText', '&:hover': { bgcolor: 'primary.dark' } }}
                >
                  <LoginOutlined />
                </IconButton>
              </Tooltip>
            )}
          </Box>
        )}
      </Box>
    </Box>
  );


  if (isMobile) {
    return (
      <Drawer
        open={open}
        onClose={onClose}
        sx={{
          '& .MuiDrawer-paper': {
            width: EXPANDED_WIDTH,
            bgcolor: 'background.paper',
            borderRight: `1px solid ${theme.palette.divider}`,
          },
        }}
      >
        {sidebarContent}
      </Drawer>
    );
  }

  return (
    <Box
      component="nav"
      sx={{
        width,
        flexShrink: 0,
        transition: theme.transitions.create('width', {
          easing: theme.transitions.easing.sharp,
          duration: theme.transitions.duration.enteringScreen,
        }),
      }}
    >
      <Box
        sx={{
          width,
          height: '100vh',
          position: 'fixed',
          top: 0,
          left: 0,
          borderRight: `1px solid ${theme.palette.divider}`,
          bgcolor: 'background.paper',
          transition: theme.transitions.create('width', {
            easing: theme.transitions.easing.sharp,
            duration: theme.transitions.duration.enteringScreen,
          }),
          overflow: 'hidden',
          zIndex: theme.zIndex.drawer,
        }}
      >
        {sidebarContent}
      </Box>
    </Box>
  );
}

export { EXPANDED_WIDTH, COLLAPSED_WIDTH };
