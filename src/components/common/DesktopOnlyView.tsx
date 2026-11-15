import { useState, useEffect } from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Paper from '@mui/material/Paper';
import Chip from '@mui/material/Chip';
import LinearProgress from '@mui/material/LinearProgress';
import Tooltip from '@mui/material/Tooltip';
import { useTheme, alpha, keyframes } from '@mui/material/styles';

import Computer from '@mui/icons-material/Computer';
import Smartphone from '@mui/icons-material/Smartphone';
import Bolt from '@mui/icons-material/Bolt';
import ContentCopy from '@mui/icons-material/ContentCopy';
import CheckCircleOutline from '@mui/icons-material/CheckCircleOutline';
import EvStation from '@mui/icons-material/EvStation';
import Visibility from '@mui/icons-material/Visibility';
import ElectricCar from '@mui/icons-material/ElectricCar';
import Devices from '@mui/icons-material/Devices';

const pulseGlow = keyframes`
  0%, 100% {
    transform: scale(1);
    opacity: 0.6;
  }
  50% {
    transform: scale(1.08);
    opacity: 0.9;
  }
`;

const floatAnim = keyframes`
  0%, 100% {
    transform: translateY(0px);
  }
  50% {
    transform: translateY(-6px);
  }
`;

interface DesktopOnlyViewProps {
  minWidth?: number;
  onBypass?: () => void;
}

export default function DesktopOnlyView({ minWidth = 1024, onBypass }: DesktopOnlyViewProps) {
  const theme = useTheme();
  const isDark = theme.palette.mode === 'dark';
  const [copied, setCopied] = useState(false);
  const [dimensions, setDimensions] = useState({
    width: typeof window !== 'undefined' ? window.innerWidth : 0,
    height: typeof window !== 'undefined' ? window.innerHeight : 0,
  });

  useEffect(() => {
    const handleResize = () => {
      setDimensions({
        width: window.innerWidth,
        height: window.innerHeight,
      });
    };
    window.addEventListener('resize', handleResize);
    return () => window.removeEventListener('resize', handleResize);
  }, []);

  const progressPercent = Math.min(100, Math.round((dimensions.width / minWidth) * 100));

  const handleCopyLink = async () => {
    try {
      if (navigator.clipboard) {
        await navigator.clipboard.writeText(window.location.href);
      } else {
        // Fallback for older contexts
        const input = document.createElement('input');
        input.value = window.location.href;
        document.body.appendChild(input);
        input.select();
        document.execCommand('copy');
        document.body.removeChild(input);
      }
      setCopied(true);
      setTimeout(() => setCopied(false), 2500);
    } catch {
      setCopied(true);
      setTimeout(() => setCopied(false), 2500);
    }
  };

  return (
    <Box
      sx={{
        minHeight: '100vh',
        width: '100vw',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        position: 'relative',
        overflow: 'hidden',
        bgcolor: isDark ? '#0b0f19' : '#f4f6fa',
        color: isDark ? '#f1f5f9' : '#0f172a',
        p: { xs: 2, sm: 3 },
      }}
    >
      {/* Dynamic Ambient Background Glows */}
      <Box
        sx={{
          position: 'absolute',
          top: '-15%',
          left: '50%',
          transform: 'translateX(-50%)',
          width: { xs: 320, sm: 600 },
          height: { xs: 320, sm: 600 },
          borderRadius: '50%',
          background: isDark
            ? 'radial-gradient(circle, rgba(103,80,164,0.35) 0%, rgba(0,107,94,0.15) 50%, transparent 70%)'
            : 'radial-gradient(circle, rgba(208,188,255,0.45) 0%, rgba(160,213,204,0.25) 50%, transparent 70%)',
          filter: 'blur(60px)',
          pointerEvents: 'none',
          animation: `${pulseGlow} 6s ease-in-out infinite`,
        }}
      />

      <Box
        sx={{
          position: 'absolute',
          bottom: '-20%',
          right: '-10%',
          width: { xs: 280, sm: 500 },
          height: { xs: 280, sm: 500 },
          borderRadius: '50%',
          background: isDark
            ? 'radial-gradient(circle, rgba(0,107,94,0.25) 0%, transparent 70%)'
            : 'radial-gradient(circle, rgba(160,213,204,0.4) 0%, transparent 70%)',
          filter: 'blur(70px)',
          pointerEvents: 'none',
        }}
      />

      {/* Main Glassmorphic Container Card */}
      <Paper
        elevation={0}
        sx={{
          position: 'relative',
          zIndex: 1,
          maxWidth: 620,
          width: '100%',
          borderRadius: { xs: 4, sm: 5 },
          p: { xs: 3, sm: 4.5 },
          backdropFilter: 'blur(20px)',
          backgroundColor: isDark ? alpha('#111827', 0.82) : alpha('#ffffff', 0.9),
          border: '1px solid',
          borderColor: isDark ? alpha('#ffffff', 0.1) : alpha('#6750a4', 0.12),
          boxShadow: isDark
            ? '0 25px 60px -15px rgba(0, 0, 0, 0.7), 0 0 40px rgba(103, 80, 164, 0.15)'
            : '0 25px 60px -15px rgba(103, 80, 164, 0.12), 0 0 30px rgba(0, 107, 94, 0.08)',
          display: 'flex',
          flexDirection: 'column',
          alignItems: 'center',
          textAlign: 'center',
        }}
      >
        {/* Top Header Pill / Status Badge */}
        <Box
          sx={{
            display: 'inline-flex',
            alignItems: 'center',
            gap: 1,
            px: 2,
            py: 0.75,
            borderRadius: 999,
            backgroundColor: isDark ? alpha('#006B5E', 0.25) : alpha('#006B5E', 0.1),
            border: '1px solid',
            borderColor: isDark ? alpha('#A0D5CC', 0.35) : alpha('#006B5E', 0.3),
            mb: 3,
          }}
        >
          {/* Live pulsing dot */}
          <Box
            sx={{
              width: 8,
              height: 8,
              borderRadius: '50%',
              bgcolor: isDark ? '#4ade80' : '#16a34a',
              boxShadow: isDark ? '0 0 10px #4ade80' : '0 0 8px #16a34a',
            }}
          />
          <Typography
            variant="caption"
            sx={{
              fontWeight: 700,
              letterSpacing: '0.08em',
              textTransform: 'uppercase',
              color: isDark ? '#A0D5CC' : '#006B5E',
              fontSize: '0.72rem',
            }}
          >
            Desktop View Required
          </Typography>
        </Box>

        {/* Hero Visual Display with Animated Monitor */}
        <Box
          sx={{
            position: 'relative',
            mb: 3,
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            animation: `${floatAnim} 4s ease-in-out infinite`,
          }}
        >
          {/* Glow ring behind icon */}
          <Box
            sx={{
              position: 'absolute',
              width: 110,
              height: 110,
              borderRadius: '50%',
              background: isDark
                ? 'radial-gradient(circle, rgba(208, 188, 255, 0.25) 0%, transparent 70%)'
                : 'radial-gradient(circle, rgba(103, 80, 164, 0.2) 0%, transparent 70%)',
              filter: 'blur(12px)',
            }}
          />

          {/* Graphic composition */}
          <Box
            sx={{
              position: 'relative',
              width: 104,
              height: 104,
              borderRadius: 4,
              background: isDark
                ? 'linear-gradient(135deg, rgba(30, 41, 59, 0.9) 0%, rgba(15, 23, 42, 0.95) 100%)'
                : 'linear-gradient(135deg, #ede9fe 0%, #e0e7ff 100%)',
              border: '1px solid',
              borderColor: isDark ? alpha('#D0BCFF', 0.25) : alpha('#6750A4', 0.2),
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              boxShadow: '0 12px 30px rgba(0,0,0,0.15)',
            }}
          >
            <Computer
              sx={{
                fontSize: 52,
                color: isDark ? '#D0BCFF' : '#6750A4',
              }}
            />

            {/* Overlaid EV lightning badge */}
            <Box
              sx={{
                position: 'absolute',
                bottom: -6,
                right: -6,
                width: 34,
                height: 34,
                borderRadius: '50%',
                bgcolor: isDark ? '#006B5E' : '#006B5E',
                border: '2px solid',
                borderColor: isDark ? '#111827' : '#ffffff',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                color: '#ffffff',
                boxShadow: '0 4px 12px rgba(0, 107, 94, 0.4)',
              }}
            >
              <Bolt sx={{ fontSize: 20 }} />
            </Box>
          </Box>
        </Box>

        {/* Brand & Main Title */}
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 1 }}>
          <ElectricCar sx={{ color: isDark ? '#A0D5CC' : '#006B5E', fontSize: 22 }} />
          <Typography
            variant="overline"
            sx={{
              fontWeight: 700,
              fontSize: '0.8rem',
              letterSpacing: '0.12em',
              color: isDark ? '#A0D5CC' : '#006B5E',
            }}
          >
            SMART EV HUB COMMAND CENTER
          </Typography>
        </Box>

        <Typography
          variant="h4"
          component="h1"
          sx={{
            fontWeight: 800,
            fontSize: { xs: '1.45rem', sm: '1.85rem' },
            lineHeight: 1.25,
            mb: 1.5,
            background: isDark
              ? 'linear-gradient(135deg, #ffffff 30%, #D0BCFF 100%)'
              : 'linear-gradient(135deg, #1C1B1F 30%, #4F378B 100%)',
            WebkitBackgroundClip: 'text',
            WebkitTextFillColor: 'transparent',
          }}
        >
          Desktop Display Required
        </Typography>

        <Typography
          variant="body1"
          sx={{
            color: isDark ? '#94a3b8' : '#64748b',
            fontSize: { xs: '0.875rem', sm: '0.9375rem' },
            maxWidth: 520,
            lineHeight: 1.6,
            mb: 3,
          }}
        >
          Smart EV Hub is engineered for high-density charging grid telemetry, multi-bay battery
          swaps, and real-time station analytics designed exclusively for widescreen desktop
          displays.
        </Typography>

        {/* Live Viewport Resolution Bar */}
        <Paper
          elevation={0}
          sx={{
            width: '100%',
            p: 2,
            mb: 3,
            borderRadius: 3,
            bgcolor: isDark ? alpha('#000000', 0.35) : alpha('#000000', 0.03),
            border: '1px solid',
            borderColor: isDark ? alpha('#ffffff', 0.08) : alpha('#000000', 0.06),
          }}
        >
          <Box
            sx={{
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'space-between',
              mb: 1,
            }}
          >
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 0.75 }}>
              <Smartphone sx={{ fontSize: 16, color: theme.palette.warning.main }} />
              <Typography variant="caption" sx={{ fontWeight: 600, color: isDark ? '#cbd5e1' : '#475569' }}>
                Your Viewport:
              </Typography>
              <Chip
                label={`${dimensions.width}px × ${dimensions.height}px`}
                size="small"
                sx={{
                  height: 20,
                  fontSize: '0.7rem',
                  fontWeight: 700,
                  bgcolor: isDark ? alpha('#E8A317', 0.2) : alpha('#E8A317', 0.15),
                  color: isDark ? '#FCD34D' : '#B45309',
                }}
              />
            </Box>

            <Box sx={{ display: 'flex', alignItems: 'center', gap: 0.75 }}>
              <Computer sx={{ fontSize: 16, color: theme.palette.success.main }} />
              <Typography variant="caption" sx={{ fontWeight: 600, color: isDark ? '#cbd5e1' : '#475569' }}>
                Required:
              </Typography>
              <Chip
                label={`≥ ${minWidth}px`}
                size="small"
                sx={{
                  height: 20,
                  fontSize: '0.7rem',
                  fontWeight: 700,
                  bgcolor: isDark ? alpha('#1B873B', 0.2) : alpha('#1B873B', 0.15),
                  color: isDark ? '#86EFAC' : '#15803D',
                }}
              />
            </Box>
          </Box>

          <LinearProgress
            variant="determinate"
            value={progressPercent}
            sx={{
              height: 6,
              borderRadius: 3,
              bgcolor: isDark ? alpha('#ffffff', 0.1) : alpha('#000000', 0.08),
              '& .MuiLinearProgress-bar': {
                borderRadius: 3,
                background:
                  progressPercent >= 100
                    ? 'linear-gradient(90deg, #10b981 0%, #059669 100%)'
                    : 'linear-gradient(90deg, #f59e0b 0%, #6366f1 100%)',
              },
            }}
          />

          <Typography
            variant="caption"
            sx={{
              display: 'block',
              mt: 0.75,
              fontSize: '0.68rem',
              color: isDark ? '#64748b' : '#94a3b8',
              textAlign: 'center',
            }}
          >
            {dimensions.width < minWidth
              ? `Expand window width by ${minWidth - dimensions.width}px to automatically unlock`
              : 'Window width meets desktop requirements'}
          </Typography>
        </Paper>

        {/* Feature Grid Mini-Cards */}
        <Box
          sx={{
            display: 'grid',
            gridTemplateColumns: { xs: '1fr', sm: 'repeat(3, 1fr)' },
            gap: 1.5,
            width: '100%',
            mb: 3.5,
            textAlign: 'left',
          }}
        >
          <Box
            sx={{
              p: 1.5,
              borderRadius: 2.5,
              bgcolor: isDark ? alpha('#ffffff', 0.04) : alpha('#000000', 0.02),
              border: '1px solid',
              borderColor: isDark ? alpha('#ffffff', 0.06) : alpha('#000000', 0.05),
            }}
          >
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 0.5 }}>
              <EvStation sx={{ fontSize: 18, color: isDark ? '#D0BCFF' : '#6750A4' }} />
              <Typography variant="subtitle2" sx={{ fontWeight: 700, fontSize: '0.78rem' }}>
                Grid Telemetry
              </Typography>
            </Box>
            <Typography variant="caption" sx={{ color: isDark ? '#94a3b8' : '#64748b', fontSize: '0.72rem' }}>
              Multi-bay voltage & live session metrics
            </Typography>
          </Box>

          <Box
            sx={{
              p: 1.5,
              borderRadius: 2.5,
              bgcolor: isDark ? alpha('#ffffff', 0.04) : alpha('#000000', 0.02),
              border: '1px solid',
              borderColor: isDark ? alpha('#ffffff', 0.06) : alpha('#000000', 0.05),
            }}
          >
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 0.5 }}>
              <Bolt sx={{ fontSize: 18, color: isDark ? '#A0D5CC' : '#006B5E' }} />
              <Typography variant="subtitle2" sx={{ fontWeight: 700, fontSize: '0.78rem' }}>
                Battery Logistics
              </Typography>
            </Box>
            <Typography variant="caption" sx={{ color: isDark ? '#94a3b8' : '#64748b', fontSize: '0.72rem' }}>
              Queue dispatch & rapid swap bays
            </Typography>
          </Box>

          <Box
            sx={{
              p: 1.5,
              borderRadius: 2.5,
              bgcolor: isDark ? alpha('#ffffff', 0.04) : alpha('#000000', 0.02),
              border: '1px solid',
              borderColor: isDark ? alpha('#ffffff', 0.06) : alpha('#000000', 0.05),
            }}
          >
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 0.5 }}>
              <Devices sx={{ fontSize: 18, color: theme.palette.info.main }} />
              <Typography variant="subtitle2" sx={{ fontWeight: 700, fontSize: '0.78rem' }}>
                Wide Command View
              </Typography>
            </Box>
            <Typography variant="caption" sx={{ color: isDark ? '#94a3b8' : '#64748b', fontSize: '0.72rem' }}>
              Comprehensive multi-pane tables & graphs
            </Typography>
          </Box>
        </Box>

        {/* Action Buttons */}
        <Box
          sx={{
            display: 'flex',
            flexDirection: { xs: 'column', sm: 'row' },
            gap: 1.5,
            width: '100%',
            justifyContent: 'center',
          }}
        >
          <Button
            variant="contained"
            color="primary"
            startIcon={copied ? <CheckCircleOutline /> : <ContentCopy />}
            onClick={handleCopyLink}
            sx={{
              py: 1.2,
              px: 3,
              fontWeight: 600,
              borderRadius: 2.5,
              boxShadow: isDark
                ? '0 4px 14px rgba(103, 80, 164, 0.4)'
                : '0 4px 14px rgba(103, 80, 164, 0.25)',
            }}
          >
            {copied ? 'Link Copied to Clipboard!' : 'Copy Link for Desktop'}
          </Button>

          {onBypass && (
            <Tooltip title="Preview desktop layout on smaller screen for development or inspection">
              <Button
                variant="outlined"
                color="secondary"
                startIcon={<Visibility />}
                onClick={onBypass}
                sx={{
                  py: 1.2,
                  px: 2.5,
                  fontWeight: 600,
                  borderRadius: 2.5,
                  borderColor: isDark ? alpha('#A0D5CC', 0.4) : alpha('#006B5E', 0.4),
                }}
              >
                Developer Bypass
              </Button>
            </Tooltip>
          )}
        </Box>

        {/* Footer Note */}
        <Typography
          variant="caption"
          sx={{
            mt: 3,
            fontSize: '0.72rem',
            color: isDark ? '#64748b' : '#94a3b8',
          }}
        >
          Tip: Open this URL on your laptop or desktop browser, or maximize your browser window.
        </Typography>
      </Paper>
    </Box>
  );
}
