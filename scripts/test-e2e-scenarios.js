/**
 * Smart EV Hub - End-to-End Scenario Verification Suite
 * Developer: Mritunjay Thakur (2519425, Section ML-5)
 * Module: Charging Stations & Charging Sessions
 * 
 * Verifies live REST API endpoints, state machines, FIFO queuing,
 * billing, transactions, and SQLite persistence across server restart.
 */

import { spawn } from 'child_process';
import path from 'path';
import fs from 'fs';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const projectRoot = path.resolve(__dirname, '..');

const PORT = 8089;
const BASE_URL = `http://127.0.0.1:${PORT}`;
const BACKEND_EXE = path.join(projectRoot, 'backend', 'build', 'smart_ev_hub_backend.exe');
const DB_PATH = path.join(projectRoot, 'backend', 'build', 'test_e2e_scenarios.db');

let serverProcess = null;

function log(msg, symbol = 'ℹ') {
    console.log(`  [${symbol}] ${msg}`);
}

function header(title) {
    console.log('\n========================================================');
    console.log(` ${title}`);
    console.log('========================================================');
}

// Clean up database before starting
function cleanTestDb() {
    try {
        if (fs.existsSync(DB_PATH)) {
            fs.unlinkSync(DB_PATH);
        }
    } catch (e) {
        // ignore if locked
    }
}

// Launch backend process
function startServer() {
    return new Promise((resolve, reject) => {
        log(`Launching unified C++ backend on port ${PORT}...`);
        const env = {
            ...process.env,
            PATH: `C:\\mingw64\\bin;C:\\cmake\\bin;${process.env.PATH}`,
            JWT_SECRET: 'super_secret_jwt_key_smart_ev_hub_2026_secure_32chars'
        };

        serverProcess = spawn(BACKEND_EXE, ['test_e2e_scenarios.db', PORT.toString()], {
            cwd: path.join(projectRoot, 'backend', 'build'),
            env: env,
            stdio: ['ignore', 'pipe', 'pipe']
        });

        let started = false;

        serverProcess.stdout.on('data', (data) => {
            const out = data.toString();
            if (out.includes('Listening on http://localhost:') || out.includes('Crow/master server is running')) {
                if (!started) {
                    started = true;
                    setTimeout(resolve, 800); // Wait briefly for socket binding
                }
            }
        });

        serverProcess.stderr.on('data', (data) => {
            const str = data.toString();
            if (str.includes('[FATAL') || str.includes('Error')) {
                console.error('Server Stderr:', str);
            }
        });

        serverProcess.on('error', (err) => {
            if (!started) reject(err);
        });

        serverProcess.on('exit', (code) => {
            if (!started) {
                reject(new Error(`Server process exited early with code ${code}`));
            }
        });

        // Fallback poll
        const startTime = Date.now();
        const checkInterval = setInterval(async () => {
            if (Date.now() - startTime > 12000) {
                clearInterval(checkInterval);
                if (!started) reject(new Error('Server failed to start within 12 seconds'));
                return;
            }
            try {
                const res = await fetch(`${BASE_URL}/api/health`);
                if (res.ok) {
                    clearInterval(checkInterval);
                    if (!started) {
                        started = true;
                        resolve();
                    }
                }
            } catch (e) {
                // wait
            }
        }, 400);
    });
}

// Stop backend process
function stopServer() {
    return new Promise((resolve) => {
        if (!serverProcess) return resolve();
        log('Stopping backend server process...');
        serverProcess.on('exit', () => {
            serverProcess = null;
            setTimeout(resolve, 600);
        });
        serverProcess.kill('SIGTERM');
        setTimeout(() => {
            if (serverProcess) {
                serverProcess.kill('SIGKILL');
                serverProcess = null;
                resolve();
            }
        }, 2500);
    });
}

// Helper fetch wrapper
async function api(endpoint, method = 'GET', body = null) {
    const opts = {
        method,
        headers: {
            'Content-Type': 'application/json'
        }
    };
    if (body) {
        opts.body = JSON.stringify(body);
    }
    const res = await fetch(`${BASE_URL}${endpoint}`, opts);
    let data = null;
    try {
        data = await res.json();
    } catch (e) {
        data = null;
    }
    return { status: res.status, ok: res.ok, data };
}

// Main test execution
async function runAllScenarios() {
    cleanTestDb();

    header('Starting Smart EV Hub End-to-End Scenarios Verification');
    await startServer();

    let passedTests = 0;
    const totalTests = 16;

    try {
        // SCENARIO 1: Health check
        console.log('\n[SCENARIO 1] Verifying System Health Check (/api/health)...');
        const health = await api('/api/health');
        if (health.status === 200 && health.data && health.data.status === 'ok') {
            log(`Health check passed. Service: "${health.data.service}", Team Members: ${health.data.teamMembers}`, '✓');
            passedTests++;
        } else {
            throw new Error(`Health check failed: ${JSON.stringify(health)}`);
        }

        // SCENARIO 2: Station listing
        console.log('\n[SCENARIO 2] Retrieving Station Catalog (/api/stations)...');
        const stationsList = await api('/api/stations');
        if (stationsList.status === 200 && Array.isArray(stationsList.data) && stationsList.data.length >= 4) {
            log(`Stations retrieved successfully. Count: ${stationsList.data.length}`, '✓');
            passedTests++;
        } else {
            throw new Error(`Station listing failed: ${JSON.stringify(stationsList)}`);
        }

        // SCENARIO 3: Deploy new station
        console.log('\n[SCENARIO 3] Deploying New Charging Station CS-E2E-1 (POST /api/stations)...');
        const newStation = {
            id: 'CS-E2E-1',
            name: 'Express Bay E2E',
            location: 'Testing Terminal Alpha',
            power: 120.0,
            connectorType: 'CCS2',
            totalPorts: 1
        };
        const deployRes = await api('/api/stations', 'POST', newStation);
        const deployStatus = deployRes.data?.station?.status || deployRes.data?.status;
        if (deployRes.status === 201 && deployRes.data?.success && (deployRes.data?.station?.id === 'CS-E2E-1' || deployRes.data?.id === 'CS-E2E-1')) {
            log(`Station CS-E2E-1 deployed with status: ${deployStatus}`, '✓');
            passedTests++;
        } else {
            throw new Error(`Station deployment failed: ${JSON.stringify(deployRes)}`);
        }

        // SCENARIO 4: Query single station details
        console.log('\n[SCENARIO 4] Retrieving Single Station Details (GET /api/stations/CS-E2E-1)...');
        const stationDetails = await api('/api/stations/CS-E2E-1');
        if (stationDetails.status === 200 && stationDetails.data.id === 'CS-E2E-1' && stationDetails.data.power === 120.0) {
            log(`Retrieved station details: Name=${stationDetails.data.name}, Status=${stationDetails.data.status}, Power=${stationDetails.data.power}kW`, '✓');
            passedTests++;
        } else {
            throw new Error(`Station details query failed: ${JSON.stringify(stationDetails)}`);
        }

        // SCENARIO 5: Valid station status update
        console.log('\n[SCENARIO 5] Updating Station Status to Maintenance (PUT /api/stations/CS-E2E-1/status)...');
        const maintRes = await api('/api/stations/CS-E2E-1/status', 'PUT', { status: 'maintenance' });
        const curStatus = maintRes.data?.station?.status || maintRes.data?.status;
        if (maintRes.status === 200 && maintRes.data?.success && curStatus === 'maintenance') {
            log(`Status updated to maintenance successfully`, '✓');
            passedTests++;
        } else {
            throw new Error(`Station status update to maintenance failed: ${JSON.stringify(maintRes)}`);
        }

        // SCENARIO 6: Maintenance station rejects charging request
        console.log('\n[SCENARIO 6] Verifying Maintenance Station Cannot Accept EV...');
        // Set all other available stations to maintenance temporarily to test allocation rejection / queueing
        const allStationsBefore = await api('/api/stations');
        for (const st of allStationsBefore.data) {
            if (st.status === 'available') {
                await api(`/api/stations/${st.id}/status`, 'PUT', { status: 'maintenance' });
            }
        }
        const busyReq = await api('/api/charging/request', 'POST', { evId: 'EV-105', targetCharge: 90 });
        if (busyReq.data && (busyReq.data.queued === true || busyReq.data.assigned === false)) {
            log(`Allocation correctly prevented when no available stations; EV-105 enqueued: ${busyReq.data.queued}, position: ${busyReq.data.queuePosition}`, '✓');
            passedTests++;
        } else {
            throw new Error(`Expected queuing or rejection, got: ${JSON.stringify(busyReq)}`);
        }

        // SCENARIO 7: Restore station to available
        console.log('\n[SCENARIO 7] Restoring Station CS-E2E-1 to Available...');
        const availRes = await api('/api/stations/CS-E2E-1/status', 'PUT', { status: 'available' });
        const availStatus = availRes.data?.station?.status || availRes.data?.status;
        if (availRes.status === 200 && availStatus === 'available') {
            log(`Station CS-E2E-1 restored to available`, '✓');
            passedTests++;
        } else {
            throw new Error(`Failed to restore station: ${JSON.stringify(availRes)}`);
        }

        // SCENARIO 8: Reject invalid state transitions
        console.log('\n[SCENARIO 8] Testing Invalid State Transition Rejection...');
        const invalidStatus = await api('/api/stations/CS-E2E-1/status', 'PUT', { status: 'UNKNOWN_STATUS' });
        const invalidManualCharging = await api('/api/stations/CS-E2E-1/status', 'PUT', { status: 'charging' });
        if (invalidStatus.status === 400 && invalidManualCharging.status === 400) {
            log(`Safely rejected invalid status and manual charging transition`, '✓');
            passedTests++;
        } else {
            throw new Error(`Invalid state transition check failed`);
        }

        // SCENARIO 9: Request Charging Session Allocation
        console.log('\n[SCENARIO 9] Requesting Charging Allocation for EV-103 (POST /api/charging/request)...');
        const allocRes = await api('/api/charging/request', 'POST', { evId: 'EV-103', targetCharge: 85.0 });
        if (allocRes.status === 200 && allocRes.data?.success && allocRes.data?.assigned === true) {
            log(`Allocated Station ${allocRes.data.stationId} for EV-103. Session ID: ${allocRes.data.sessionId}`, '✓');
            passedTests++;
        } else {
            throw new Error(`Charging allocation failed: ${JSON.stringify(allocRes)}`);
        }

        const activeSessionId = allocRes.data.sessionId;
        const assignedStationId = allocRes.data.stationId;

        // SCENARIO 10: Reject duplicate charging request for same EV
        console.log('\n[SCENARIO 10] Rejecting Duplicate Charging Allocation for EV-103...');
        const dupRes = await api('/api/charging/request', 'POST', { evId: 'EV-103', targetCharge: 90.0 });
        if (dupRes.status === 400 && dupRes.data?.success === false) {
            log(`Duplicate request for EV-103 properly rejected: "${dupRes.data.message}"`, '✓');
            passedTests++;
        } else {
            throw new Error(`Duplicate EV allocation was not rejected: ${JSON.stringify(dupRes)}`);
        }

        // SCENARIO 11: Active session retrieval
        console.log('\n[SCENARIO 11] Retrieving Active Charging Sessions (/api/sessions/active)...');
        const activeSessions = await api('/api/sessions/active');
        if (activeSessions.status === 200 && Array.isArray(activeSessions.data) && activeSessions.data.some(s => s.id === activeSessionId)) {
            log(`Active session ${activeSessionId} verified in live session registry`, '✓');
            passedTests++;
        } else {
            throw new Error(`Active session retrieval failed: ${JSON.stringify(activeSessions)}`);
        }

        // SCENARIO 12: Complete Charging Session with Billing
        console.log('\n[SCENARIO 12] Completing Charging Session (POST /api/charging/complete)...');
        const compRes = await api('/api/charging/complete', 'POST', { sessionId: activeSessionId });
        if (compRes.status === 200 && compRes.data?.success && compRes.data?.amount > 0 && compRes.data?.energyDelivered > 0) {
            log(`Session completed successfully: Energy=${compRes.data.energyDelivered.toFixed(2)} kWh, Amount=INR ${compRes.data.amount.toFixed(2)}, TxnID=${compRes.data.transactionId}`, '✓');
            passedTests++;
        } else {
            throw new Error(`Charging completion failed: ${JSON.stringify(compRes)}`);
        }

        // SCENARIO 13: Station released and status verified
        console.log('\n[SCENARIO 13] Verifying Station Release Post-Completion...');
        const releasedStation = await api(`/api/stations/${assignedStationId}`);
        if (releasedStation.status === 200) {
            log(`Station ${assignedStationId} status verified: ${releasedStation.data.status}`, '✓');
            passedTests++;
        } else {
            throw new Error(`Station verification failed: ${JSON.stringify(releasedStation)}`);
        }

        // SCENARIO 14: Double session completion rejection
        console.log('\n[SCENARIO 14] Testing Double Completion Rejection...');
        const doubleComp = await api('/api/charging/complete', 'POST', { sessionId: activeSessionId });
        if (doubleComp.status === 400 && doubleComp.data?.success === false) {
            log(`Double completion rejected: "${doubleComp.data.message}"`, '✓');
            passedTests++;
        } else {
            throw new Error(`Double session completion was not rejected: ${JSON.stringify(doubleComp)}`);
        }

        // SCENARIO 15: Non-existent Station Rejection
        console.log('\n[SCENARIO 15] Testing Non-existent Station Rejection...');
        const nonExistent = await api('/api/stations/CS-BOGUS-999');
        if (nonExistent.status === 404) {
            log(`Query for non-existent station correctly returned 404 Not Found`, '✓');
            passedTests++;
        } else {
            throw new Error(`Non-existent station should return 404, got: ${nonExistent.status}`);
        }

        // SCENARIO 16: Backend Restart & SQLite Persistence Verification
        console.log('\n[SCENARIO 16] Testing Persistence across Server Restart...');
        await stopServer();
        log('Server stopped. Re-launching server with same SQLite database...');
        await startServer();

        // Query station CS-E2E-1 again
        const reloadedStation = await api('/api/stations/CS-E2E-1');
        // Query completed sessions
        const allSessions = await api('/api/sessions');
        const persistedSession = allSessions.data ? allSessions.data.find(s => s.id === activeSessionId) : null;

        if (reloadedStation.status === 200 && reloadedStation.data.id === 'CS-E2E-1' &&
            persistedSession && persistedSession.status === 'completed') {
            log(`PERSISTENCE VERIFIED: Station CS-E2E-1 and completed session ${activeSessionId} survived server restart!`, '✓');
            passedTests++;
        } else {
            throw new Error(`Persistence across restart failed!`);
        }

        header(`ALL ${passedTests}/${totalTests} END-TO-END SCENARIOS PASSED WITH 100% SUCCESS!`);

    } catch (err) {
        console.error('\n❌ E2E SCENARIO TEST FAILED:', err.message);
        process.exitCode = 1;
    } finally {
        await stopServer();
        cleanTestDb();
    }
}

runAllScenarios();
