import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import Box from '@mui/material/Box';
import Menu from '@mui/material/Menu';
import MenuItem from '@mui/material/MenuItem';
import Typography from '@mui/material/Typography';
import Badge from '@mui/material/Badge';
import IconButton from '@mui/material/IconButton';
import Tooltip from '@mui/material/Tooltip';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';
import Divider from '@mui/material/Divider';
import Button from '@mui/material/Button';
import { useTheme, alpha } from '@mui/material/styles';

import NotificationsOutlined from '@mui/icons-material/NotificationsOutlined';
import DarkModeOutlined from '@mui/icons-material/DarkModeOutlined';
import LightModeOutlined from '@mui/icons-material/LightModeOutlined';
import SearchOutlined from '@mui/icons-material/SearchOutlined';
import MenuOutlined from '@mui/icons-material/MenuOutlined';
import InfoOutlined from '@mui/icons-material/InfoOutlined';
import WarningAmberOutlined from '@mui/icons-material/WarningAmberOutlined';
import CheckCircleOutline from '@mui/icons-material/CheckCircleOutline';
import ErrorOutline from '@mui/icons-material/ErrorOutline';
import DoneAllOutlined from '@mui/icons-material/DoneAllOutlined';

import { useApp } from '@/context/AppContext';
import type { Notification } from '@/types';

const iconMap: Record<string, React.ReactNode> = {
  info: <InfoOutlined sx={{ color: 'info.main' }} fontSize="small" />,
  warning: <WarningAmberOutlined sx={{ color: 'warning.main' }} fontSize="small" />,
  success: <CheckCircleOutline sx={{ color: 'success.main' }} fontSize="small" />,
  error: <ErrorOutline sx={{ color: 'error.main' }} fontSize="small" />,
};

interface TopBarProps {
  onMenuClick: () => void;
  onSearchOpen: () => void;
  showMenuButton: boolean;
}

export default function TopBar({ onMenuClick, onSearchOpen, showMenuButton }: TopBarProps) {
  const theme = useTheme();
  const navigate = useNavigate();
  const { notifications, markNotificationRead, markAllNotificationsRead, settings, toggleTheme } = useApp();
  const [notifAnchor, setNotifAnchor] = useState<null | HTMLElement>(null);

  const unreadCount = notifications.filter(n => !n.read).length;

  const formatTime = (ts: string) => {
    const d = new Date(ts);
    return d.toLocaleTimeString('en-IN', { hour: '2-digit', minute: '2-digit' });
  };

  return (
    <Box
      component="header"
      sx={{
        height: 64,
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'space-between',
        px: { xs: 2, md: 3 },
        borderBottom: `1px solid ${theme.palette.divider}`,
        bgcolor: alpha(theme.palette.background.paper, 0.8),
        backdropFilter: 'blur(8px)',
        position: 'sticky',
        top: 0,
        zIndex: theme.zIndex.appBar,
      }}
    >
      {/* Left */}
      <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
        {showMenuButton && (
          <IconButton onClick={onMenuClick} edge="start" aria-label="Open menu">
            <MenuOutlined />
          </IconButton>
        )}
      </Box>

      {/* Right */}
      <Box sx={{ display: 'flex', alignItems: 'center', gap: 0.5 }}>
        {/* Search */}
        <Tooltip title="Search (Ctrl+K)">
          <IconButton onClick={onSearchOpen} aria-label="Search">
            <SearchOutlined />
          </IconButton>
        </Tooltip>

        {/* Theme */}
        <Tooltip title={settings.themeMode === 'light' ? 'Dark mode' : 'Light mode'}>
          <IconButton onClick={toggleTheme} aria-label="Toggle theme">
            {settings.themeMode === 'light' ? <DarkModeOutlined /> : <LightModeOutlined />}
          </IconButton>
        </Tooltip>

        {/* Notifications */}
        <Tooltip title="Notifications">
          <IconButton onClick={e => setNotifAnchor(e.currentTarget)} aria-label="Notifications">
            <Badge badgeContent={unreadCount} color="error" max={9}>
              <NotificationsOutlined />
            </Badge>
          </IconButton>
        </Tooltip>
        <Menu
          anchorEl={notifAnchor}
          open={Boolean(notifAnchor)}
          onClose={() => setNotifAnchor(null)}
          PaperProps={{
            sx: { width: 360, maxHeight: 480, borderRadius: 3 },
          }}
          transformOrigin={{ horizontal: 'right', vertical: 'top' }}
          anchorOrigin={{ horizontal: 'right', vertical: 'bottom' }}
        >
          <Box sx={{ px: 2, py: 1.5, display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <Typography variant="h6">Notifications</Typography>
            {unreadCount > 0 && (
              <Button
                size="small"
                startIcon={<DoneAllOutlined />}
                onClick={() => markAllNotificationsRead()}
              >
                Mark all read
              </Button>
            )}
          </Box>
          <Divider />
          {notifications.length === 0 ? (
            <Box sx={{ p: 3, textAlign: 'center' }}>
              <Typography variant="body2" color="text.secondary">No notifications</Typography>
            </Box>
          ) : (
            notifications.slice(0, 8).map(n => (
              <MenuItem
                key={n.id}
                onClick={() => {
                  markNotificationRead(n.id);
                  setNotifAnchor(null);
                  if (n.link) navigate(n.link);
                }}
                sx={{
                  py: 1.5,
                  px: 2,
                  alignItems: 'flex-start',
                  gap: 1.5,
                  bgcolor: n.read ? 'transparent' : alpha(theme.palette.primary.main, 0.04),
                  whiteSpace: 'normal',
                }}
              >
                <ListItemIcon sx={{ mt: 0.5, minWidth: 28 }}>
                  {iconMap[n.type]}
                </ListItemIcon>
                <ListItemText
                  primary={
                    <Typography variant="body2" sx={{ fontWeight: n.read ? 400 : 600 }}>
                      {n.message}
                    </Typography>
                  }
                  secondary={formatTime(n.timestamp)}
                />
              </MenuItem>
            ))
          )}
        </Menu>
      </Box>
    </Box>
  );
}
