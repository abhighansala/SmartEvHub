import { createTheme, type ThemeOptions } from '@mui/material/styles';

const commonTypography = {
  fontFamily: '"Inter", "Roboto", "Helvetica", "Arial", sans-serif',
  h1: { fontWeight: 700, fontSize: '2rem', lineHeight: 1.2 },
  h2: { fontWeight: 700, fontSize: '1.5rem', lineHeight: 1.3 },
  h3: { fontWeight: 600, fontSize: '1.25rem', lineHeight: 1.4 },
  h4: { fontWeight: 600, fontSize: '1.125rem', lineHeight: 1.4 },
  h5: { fontWeight: 600, fontSize: '1rem', lineHeight: 1.5 },
  h6: { fontWeight: 600, fontSize: '0.875rem', lineHeight: 1.5 },
  subtitle1: { fontWeight: 500, fontSize: '0.875rem' },
  subtitle2: { fontWeight: 500, fontSize: '0.8125rem' },
  body1: { fontSize: '0.875rem', lineHeight: 1.6 },
  body2: { fontSize: '0.8125rem', lineHeight: 1.5 },
  button: { textTransform: 'none' as const, fontWeight: 600, fontSize: '0.8125rem' },
  caption: { fontSize: '0.75rem', lineHeight: 1.5 },
  overline: { fontSize: '0.6875rem', fontWeight: 600, letterSpacing: '0.08em', textTransform: 'uppercase' as const },
};

const commonComponents: ThemeOptions['components'] = {
  MuiButton: {
    styleOverrides: {
      root: {
        borderRadius: 12,
        padding: '8px 20px',
        boxShadow: 'none',
        '&:hover': { boxShadow: 'none' },
      },
      sizeSmall: { padding: '6px 14px', fontSize: '0.75rem', borderRadius: 10 },
      sizeLarge: { padding: '10px 24px', fontSize: '0.875rem' },
    },
    defaultProps: {
      disableElevation: true,
    },
  },
  MuiCard: {
    styleOverrides: {
      root: {
        borderRadius: 16,
        boxShadow: 'none',
      },
    },
  },
  MuiPaper: {
    styleOverrides: {
      root: {
        borderRadius: 16,
      },
      elevation1: {
        boxShadow: '0 1px 3px rgba(0,0,0,0.04), 0 1px 2px rgba(0,0,0,0.06)',
      },
    },
  },
  MuiChip: {
    styleOverrides: {
      root: {
        borderRadius: 8,
        fontWeight: 500,
        fontSize: '0.75rem',
      },
      sizeSmall: {
        height: 24,
        fontSize: '0.6875rem',
      },
    },
  },
  MuiDialog: {
    styleOverrides: {
      paper: {
        borderRadius: 20,
      },
    },
  },
  MuiTextField: {
    defaultProps: {
      variant: 'outlined',
      size: 'small',
    },
    styleOverrides: {
      root: {
        '& .MuiOutlinedInput-root': {
          borderRadius: 12,
        },
      },
    },
  },
  MuiTableCell: {
    styleOverrides: {
      root: {
        borderBottom: '1px solid',
        padding: '12px 16px',
        fontSize: '0.8125rem',
      },
      head: {
        fontWeight: 600,
        fontSize: '0.75rem',
        textTransform: 'uppercase',
        letterSpacing: '0.05em',
      },
    },
  },
  MuiTooltip: {
    styleOverrides: {
      tooltip: {
        borderRadius: 8,
        fontSize: '0.75rem',
      },
    },
  },
  MuiListItemButton: {
    styleOverrides: {
      root: {
        borderRadius: 12,
        marginBottom: 2,
        '&.Mui-selected': {
          fontWeight: 600,
        },
      },
    },
  },
  MuiTab: {
    styleOverrides: {
      root: {
        textTransform: 'none',
        fontWeight: 500,
        fontSize: '0.875rem',
      },
    },
  },
  MuiAlert: {
    styleOverrides: {
      root: {
        borderRadius: 12,
      },
    },
  },
};

export const lightTheme = createTheme({
  palette: {
    mode: 'light',
    primary: {
      main: '#6750A4',
      light: '#D0BCFF',
      dark: '#4F378B',
      contrastText: '#FFFFFF',
    },
    secondary: {
      main: '#006B5E',
      light: '#A0D5CC',
      dark: '#004D40',
      contrastText: '#FFFFFF',
    },
    success: {
      main: '#1B873B',
      light: '#C8E6C9',
      dark: '#0D5A22',
    },
    warning: {
      main: '#E8A317',
      light: '#FFF3CD',
      dark: '#B07D10',
    },
    error: {
      main: '#D32F2F',
      light: '#FFCDD2',
      dark: '#B71C1C',
    },
    info: {
      main: '#1976D2',
      light: '#BBDEFB',
      dark: '#0D47A1',
    },
    background: {
      default: '#F7F5FA',
      paper: '#FFFFFF',
    },
    text: {
      primary: '#1C1B1F',
      secondary: '#625B71',
    },
    divider: '#E7E0EB',
  },
  typography: commonTypography,
  shape: { borderRadius: 12 },
  components: {
    ...commonComponents,
    MuiTableCell: {
      styleOverrides: {
        root: {
          borderBottom: '1px solid',
          padding: '12px 16px',
          fontSize: '0.8125rem',
          borderBottomColor: '#E7E0EB',
        },
        head: {
          fontWeight: 600,
          fontSize: '0.75rem',
          textTransform: 'uppercase' as const,
          letterSpacing: '0.05em',
          color: '#625B71',
          backgroundColor: '#F7F5FA',
        },
      },
    },
  },
});

export const darkTheme = createTheme({
  palette: {
    mode: 'dark',
    primary: {
      main: '#D0BCFF',
      light: '#EADDFF',
      dark: '#6750A4',
      contrastText: '#381E72',
    },
    secondary: {
      main: '#A0D5CC',
      light: '#C8EDE7',
      dark: '#006B5E',
      contrastText: '#003731',
    },
    success: {
      main: '#66BB6A',
      light: '#388E3C',
      dark: '#2E7D32',
    },
    warning: {
      main: '#FFD54F',
      light: '#FFE082',
      dark: '#F9A825',
    },
    error: {
      main: '#EF5350',
      light: '#E57373',
      dark: '#C62828',
    },
    info: {
      main: '#42A5F5',
      light: '#64B5F6',
      dark: '#1565C0',
    },
    background: {
      default: '#141218',
      paper: '#1D1B20',
    },
    text: {
      primary: '#E6E1E5',
      secondary: '#CAC4D0',
    },
    divider: '#49454F',
  },
  typography: commonTypography,
  shape: { borderRadius: 12 },
  components: {
    ...commonComponents,
    MuiTableCell: {
      styleOverrides: {
        root: {
          borderBottom: '1px solid',
          padding: '12px 16px',
          fontSize: '0.8125rem',
          borderBottomColor: '#49454F',
        },
        head: {
          fontWeight: 600,
          fontSize: '0.75rem',
          textTransform: 'uppercase' as const,
          letterSpacing: '0.05em',
          color: '#CAC4D0',
          backgroundColor: '#1D1B20',
        },
      },
    },
  },
});

// Status colors used for chips across the app
export const statusColors = {
  available: { bg: '#E8F5E9', color: '#1B873B', darkBg: '#1B3A26', darkColor: '#66BB6A' },
  charging: { bg: '#E3F2FD', color: '#1565C0', darkBg: '#0D2744', darkColor: '#42A5F5' },
  waiting: { bg: '#FFF8E1', color: '#E8A317', darkBg: '#3E2E05', darkColor: '#FFD54F' },
  fault: { bg: '#FFEBEE', color: '#C62828', darkBg: '#3E1010', darkColor: '#EF5350' },
  offline: { bg: '#FFEBEE', color: '#C62828', darkBg: '#3E1010', darkColor: '#EF5350' },
  maintenance: { bg: '#F3E5F5', color: '#7B1FA2', darkBg: '#2A1035', darkColor: '#CE93D8' },
  completed: { bg: '#E8F5E9', color: '#1B873B', darkBg: '#1B3A26', darkColor: '#66BB6A' },
  swapping: { bg: '#E0F2F1', color: '#00695C', darkBg: '#0A2E2A', darkColor: '#4DB6AC' },
  inuse: { bg: '#E3F2FD', color: '#1565C0', darkBg: '#0D2744', darkColor: '#42A5F5' },
  unavailable: { bg: '#ECEFF1', color: '#546E7A', darkBg: '#1E2D35', darkColor: '#90A4AE' },
  pending: { bg: '#FFF8E1', color: '#E8A317', darkBg: '#3E2E05', darkColor: '#FFD54F' },
} as const;
