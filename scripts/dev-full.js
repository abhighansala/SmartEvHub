import { spawn, execSync } from 'child_process';
import fs from 'fs';
import path from 'path';
import readline from 'readline';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const rootDir = path.resolve(__dirname, '..');

// ANSI color codes
const CYAN = '\x1b[36m';
const MAGENTA = '\x1b[35m';
const RESET = '\x1b[0m';
const GREEN = '\x1b[32m';
const YELLOW = '\x1b[33m';

console.log(`${GREEN}========================================================${RESET}`);
console.log(`${GREEN}       Smart EV Hub - Unified Full-Stack Runner         ${RESET}`);
console.log(`${GREEN}       (C++ Crow Backend + React Vite Frontend)        ${RESET}`);
console.log(`${GREEN}========================================================${RESET}\n`);

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
  console.log(`${YELLOW}[Runner] Configured default 64-char development JWT_SECRET.${RESET}`);
}

// 3. Ensure backend executable exists
const exePath = path.join(rootDir, 'backend', 'build', process.platform === 'win32' ? 'smart_ev_hub_backend.exe' : 'smart_ev_hub_backend');
if (!fs.existsSync(exePath)) {
  console.log(`${YELLOW}[Runner] Backend executable not found. Compiling backend with CMake...${RESET}`);
  try {
    execSync('cmake --build backend/build', { cwd: rootDir, stdio: 'inherit' });
  } catch (err) {
    console.error(`[Runner] Backend compilation failed:`, err.message);
    process.exit(1);
  }
}

// Helper to prefix child stream
function prefixStream(stream, prefix) {
  const rl = readline.createInterface({ input: stream });
  rl.on('line', (line) => {
    console.log(`${prefix} ${line}`);
  });
}

// 4. Spawn C++ Backend
console.log(`${CYAN}[Backend] Starting C++ Crow server on port ${process.env.PORT || '8080'}...${RESET}`);
const backend = spawn(exePath, [], {
  cwd: rootDir,
  env: process.env,
  stdio: ['pipe', 'pipe', 'pipe']
});

prefixStream(backend.stdout, `${CYAN}[Backend]${RESET}`);
prefixStream(backend.stderr, `${CYAN}[Backend]${RESET}`);

// 5. Spawn Vite Frontend
const npmCmd = process.platform === 'win32' ? 'npm.cmd' : 'npm';
console.log(`${MAGENTA}[Frontend] Starting Vite development server on port 3000...${RESET}`);
const frontend = spawn(npmCmd, ['run', 'dev'], {
  cwd: rootDir,
  env: process.env,
  stdio: ['pipe', 'pipe', 'pipe'],
  shell: process.platform === 'win32'
});

prefixStream(frontend.stdout, `${MAGENTA}[Frontend]${RESET}`);
prefixStream(frontend.stderr, `${MAGENTA}[Frontend]${RESET}`);

// 6. Graceful cleanup
let isShuttingDown = false;
function shutdown() {
  if (isShuttingDown) return;
  isShuttingDown = true;
  console.log(`\n${YELLOW}[Runner] Shutting down Smart EV Hub services...${RESET}`);

  if (backend && !backend.killed) {
    if (process.platform === 'win32') {
      try { execSync(`taskkill /pid ${backend.pid} /T /F`, { stdio: 'ignore' }); } catch {}
    } else {
      backend.kill('SIGTERM');
    }
  }

  if (frontend && !frontend.killed) {
    if (process.platform === 'win32') {
      try { execSync(`taskkill /pid ${frontend.pid} /T /F`, { stdio: 'ignore' }); } catch {}
    } else {
      frontend.kill('SIGTERM');
    }
  }

  console.log(`${GREEN}[Runner] Both services stopped successfully.${RESET}`);
  process.exit(0);
}

process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);
process.on('exit', shutdown);

backend.on('exit', (code) => {
  if (!isShuttingDown) {
    console.log(`${YELLOW}[Backend] Process exited with code ${code}.${RESET}`);
    shutdown();
  }
});

frontend.on('exit', (code) => {
  if (!isShuttingDown) {
    console.log(`${YELLOW}[Frontend] Process exited with code ${code}.${RESET}`);
    shutdown();
  }
});
