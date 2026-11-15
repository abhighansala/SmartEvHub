import { spawn, execSync } from 'child_process';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const rootDir = path.resolve(__dirname, '..');

// 1. Load .env file if present
const envPath = path.join(rootDir, '.env');
if (fs.existsSync(envPath)) {
  const content = fs.readFileSync(envPath, 'utf8');
  for (const line of content.split(/\r?\n/)) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith('#')) continue;
    const eqIdx = trimmed.indexOf('=');
    if (eqIdx !== -1) {
      const key = trimmed.slice(0, eqIdx).trim();
      const val = trimmed.slice(eqIdx + 1).trim();
      if (!process.env[key]) {
        process.env[key] = val;
      }
    }
  }
}

// 2. Ensure JWT_SECRET is configured
if (!process.env.JWT_SECRET || process.env.JWT_SECRET.length < 32) {
  process.env.JWT_SECRET = 'SmartEvHub_LocalDev_SecretKey_Minimum_32_Characters_2026_Secure!';
}

// 3. Ensure executable exists
const exePath = path.join(rootDir, 'backend', 'build', process.platform === 'win32' ? 'smart_ev_hub_backend.exe' : 'smart_ev_hub_backend');
if (!fs.existsSync(exePath)) {
  console.log('[Dev] Backend executable not found. Compiling backend with CMake...');
  try {
    execSync('cmake --build backend/build', { cwd: rootDir, stdio: 'inherit' });
  } catch (err) {
    console.error('[Dev] Backend compilation failed:', err.message);
    process.exit(1);
  }
}

// 4. Spawn backend
console.log('[Dev] Starting Smart EV Hub C++ Backend...');
const backend = spawn(exePath, [], {
  cwd: rootDir,
  env: process.env,
  stdio: 'inherit'
});

const cleanup = () => {
  if (backend && !backend.killed) {
    if (process.platform === 'win32') {
      try { execSync(`taskkill /pid ${backend.pid} /T /F`, { stdio: 'ignore' }); } catch {}
    } else {
      backend.kill('SIGTERM');
    }
  }
};

process.on('SIGINT', () => { cleanup(); process.exit(0); });
process.on('SIGTERM', () => { cleanup(); process.exit(0); });
process.on('exit', cleanup);

backend.on('exit', (code) => {
  process.exit(code || 0);
});
