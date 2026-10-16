import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import Grid from '@mui/material/Grid2';
import Typography from '@mui/material/Typography';
import Switch from '@mui/material/Switch';
import RadioGroup from '@mui/material/RadioGroup';
import FormControlLabel from '@mui/material/FormControlLabel';
import Radio from '@mui/material/Radio';
import Divider from '@mui/material/Divider';
import { useTheme } from '@mui/material/styles';

import BoltOutlined from '@mui/icons-material/BoltOutlined';

import PageHeader from '@/components/common/PageHeader';
import { useApp } from '@/context/AppContext';

export default function Settings() {
  const theme = useTheme();
  const { settings, updateSettings, toggleTheme } = useApp();

  return (
    <Box>
      <PageHeader title="Settings" subtitle="Configure application preferences" />

      <Grid container spacing={2.5}>

        {/* Notifications */}
        <Grid size={{ xs: 12, md: 6 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
            <Typography variant="h5" sx={{ mb: 3 }}>Notifications</Typography>

            {[
              { key: 'queueAlerts' as const, label: 'Queue Alerts', desc: 'Notify when vehicles are waiting too long' },
              { key: 'sessionCompletion' as const, label: 'Session Completion', desc: 'Notify when charging sessions complete' },
              { key: 'stationAlerts' as const, label: 'Station Alerts', desc: 'Notify on station status changes' },
            ].map((item, i) => (
              <Box key={item.key}>
                <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', py: 1 }}>
                  <Box>
                    <Typography variant="body1" fontWeight={500}>{item.label}</Typography>
                    <Typography variant="body2" color="text.secondary">{item.desc}</Typography>
                  </Box>
                  <Switch
                    checked={settings.notifications[item.key]}
                    onChange={e => updateSettings({
                      notifications: { ...settings.notifications, [item.key]: e.target.checked },
                    })}
                  />
                </Box>
                {i < 2 && <Divider sx={{ my: 1 }} />}
              </Box>
            ))}
          </Card>
        </Grid>

        {/* About */}
        <Grid size={{ xs: 12 }}>
          <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
            <Typography variant="h5" sx={{ mb: 2 }}>About</Typography>
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
              <Box
                sx={{
                  width: 48, height: 48, borderRadius: 3,
                  bgcolor: 'primary.main', display: 'flex',
                  alignItems: 'center', justifyContent: 'center',
                }}
              >
                <BoltOutlined sx={{ fontSize: 28, color: '#fff' }} />
              </Box>
              <Box>
                <Typography variant="h4">Smart EV Hub</Typography>
                <Typography variant="body2" color="text.secondary">
                  EV Charging & Battery-Swap Queue Management System
                </Typography>
              </Box>
            </Box>
          </Card>

          {/* Team Members & Contributors */}
          <Grid size={{ xs: 12 }} sx={{ marginTop: 2 }}>
            <Card sx={{ p: 3, border: `1px solid ${theme.palette.divider}` }}>
              <Typography variant="h5" sx={{ mb: 2 }}>Project Team Members (Equal Contribution):</Typography>
              <Typography variant="body1" component="div" sx={{ mt: 1 }}>
                <ul style={{ listStyle: "none", paddingLeft: 0, margin: 0 }}>
                  <li style={{ marginBottom: 10 }}><strong>Aditya Singh</strong> — 2519272 (ML1) • Team Lead (Database + Backend Foundation)</li>
                  <li style={{ marginBottom: 10 }}><strong>Priyanshu Dutt</strong> — 2519460 (ML3) • EV Management Module Owner</li>
                  <li style={{ marginBottom: 10 }}><strong>Mritunjay Thakur</strong> — 2519425 (ML5) • Charging Stations Module Owner</li>
                  <li style={{ marginBottom: 10 }}><strong>Amit Singh Kaintura</strong> — 2518188 (G) • Battery & Swap Module Owner</li>
                  <li style={{ marginBottom: 10 }}><strong>Abhishek Ghansala</strong> — 2518132 (H) • UI + DSA + Final Integration</li>
                  <li style={{ marginBottom: 10 }}><strong>Agreev Singh</strong> — 2518161 (F) • Transactions + Reports + Analytics</li>
                </ul>
              </Typography>
            </Card>
          </Grid>
        </Grid>
      </Grid>
    </Box>
  );
}
