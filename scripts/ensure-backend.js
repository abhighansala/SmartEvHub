import { execSync } from 'child_process';
import fs from 'fs';
import path from 'path';

/**
 * Ensures the C++ backend binary exists in backend/build.
 * If unconfigured or missing, configures CMake and builds the backend.
 *
 * @param {string} rootDir Project root directory
 * @returns {string} Absolute path to the backend executable
 */
export function ensureBackendBuilt(rootDir) {
  const exePath = path.join(
    rootDir,
    'backend',
    'build',
    process.platform === 'win32' ? 'smart_ev_hub_backend.exe' : 'smart_ev_hub_backend'
  );
  const cachePath = path.join(rootDir, 'backend', 'build', 'CMakeCache.txt');

  let needsBuild = !fs.existsSync(exePath);
  if (!needsBuild) {
    try {
      const exeMtime = fs.statSync(exePath).mtimeMs;
      const mainMtime = fs.statSync(path.join(rootDir, 'backend', 'src', 'main.cpp')).mtimeMs;
      if (mainMtime > exeMtime) {
        needsBuild = true;
      }
    } catch {}
  }

  if (needsBuild) {
    if (!fs.existsSync(exePath)) {
      console.log('[Dev] Backend executable not found.');
    } else {
      console.log('[Dev] Backend source files are newer than executable. Rebuilding...');
    }
    if (!fs.existsSync(cachePath)) {
      console.log('[Dev] CMake build tree not configured. Running initial CMake configure...');
      const configureCmd = process.platform === 'win32'
        ? 'cmake -B backend/build -S backend -G "MinGW Makefiles"'
        : 'cmake -B backend/build -S backend';
      try {
        execSync(configureCmd, { cwd: rootDir, stdio: 'inherit' });
      } catch {
        console.log('[Dev] Note: MinGW generator configure failed, trying default generator...');
        execSync('cmake -B backend/build -S backend', { cwd: rootDir, stdio: 'inherit' });
      }
    }
    console.log('[Dev] Compiling backend with CMake...');
    try {
      execSync('cmake --build backend/build', { cwd: rootDir, stdio: 'inherit' });
    } catch (err) {
      console.error('[Dev] Backend compilation failed:', err.message);
      process.exit(1);
    }
  }

  return exePath;
}
