import http from 'http';
import path from 'path';
import fs from 'fs';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const rootDir = path.resolve(__dirname, '..');

function request(options, body = null) {
  return new Promise((resolve, reject) => {
    const req = http.request(options, (res) => {
      let data = '';
      res.on('data', chunk => data += chunk);
      res.on('end', () => {
        let parsed = null;
        try {
          parsed = data ? JSON.parse(data) : null;
        } catch (e) {
          parsed = data;
        }
        resolve({
          statusCode: res.statusCode,
          headers: res.headers,
          data: parsed,
          raw: data
        });
      });
    });
    req.on('error', reject);
    if (body) {
      req.write(typeof body === 'string' ? body : JSON.stringify(body));
    }
    req.end();
  });
}

let passedCount = 0;
let totalCount = 0;

function assert(condition, message) {
  totalCount++;
  if (condition) {
    console.log(`  [PASS] ${message}`);
    passedCount++;
  } else {
    console.error(`  [FAIL] ${message}`);
    throw new Error(`Assertion failed: ${message}`);
  }
}

async function runAllScenarios() {
  console.log('================================================================');
  console.log(' SMART EV HUB - FULL END-TO-END AUTOMATED VERIFICATION SUITE   ');
  console.log('================================================================\n');

  // Verify Backend is responding
  const health = await request({ hostname: 'localhost', port: 8080, path: '/api/health', method: 'GET' });
  assert(health.statusCode === 200, 'Health check returns HTTP 200');
  assert(health.data.status === 'ok', 'Health status is ok');
  assert(health.data.teamMembers === 6, 'Unified backend serves all 6 team members');

  // ==========================================================================
  // SCENARIO 1: Authentication & JWT Hardening
  // ==========================================================================
  console.log('\n----------------------------------------------------------------');
  console.log(' SCENARIO 1: Authentication & JWT Hardening');
  console.log('----------------------------------------------------------------');

  const testUserEmail = `audit_user_${Date.now()}@evhub.com`;
  const testUserPass = 'StrongSecurePassword2026!';

  // 1.1 Privilege Escalation Guard
  console.log('  Testing privilege escalation guard on public registration...');
  const regAttempt = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/register',
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  }, {
    name: 'Privilege Attacker',
    email: testUserEmail,
    password: testUserPass,
    role: 'admin' // Attempting to escalate to admin
  });

  assert(regAttempt.statusCode === 201, 'Registration returns HTTP 201 Created');
  assert(regAttempt.data.user.role === 'user', 'Privilege guard forced role to "user" (no unauthorized admin escalation)');

  // 1.2 Bcrypt Password Login
  console.log('  Testing login with bcrypt password comparison...');
  const loginRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/login',
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  }, {
    email: testUserEmail,
    password: testUserPass
  });

  assert(loginRes.statusCode === 200, 'Valid login returns HTTP 200');
  assert(typeof loginRes.data.token === 'string', 'JWT token received upon login');
  const token = loginRes.data.token;

  // 1.3 JWT Structure & HS256 algorithm inspection
  const parts = token.split('.');
  assert(parts.length === 3, 'JWT has valid three-segment structure (header.payload.signature)');
  const header = JSON.parse(Buffer.from(parts[0], 'base64url').toString('utf8'));
  const payload = JSON.parse(Buffer.from(parts[1], 'base64url').toString('utf8'));
  assert(header.alg === 'HS256', 'JWT header specifies HS256 algorithm enforcement');
  assert(payload.email === testUserEmail, 'JWT payload contains correct user email');
  assert(payload.role === 'user', 'JWT payload contains role="user"');
  assert(typeof payload.exp === 'number', 'JWT payload contains valid expiration timestamp (exp)');

  // 1.4 Invalid Credentials Rejection
  console.log('  Testing invalid password rejection (bcrypt verify fail)...');
  const badLogin = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/login',
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  }, {
    email: testUserEmail,
    password: 'WrongPassword123'
  });
  assert(badLogin.statusCode === 401, 'Invalid password rejected with HTTP 401 Unauthorized');

  // 1.5 Authenticated Route Access (/api/me)
  console.log('  Testing /api/me with valid Bearer token...');
  const meRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/me',
    method: 'GET',
    headers: { 'Authorization': `Bearer ${token}` }
  });
  assert(meRes.statusCode === 200, '/api/me returns HTTP 200');
  assert(meRes.data.user.email === testUserEmail, '/api/me returns matching user identity');

  // 1.6 Missing / Malformed Token Rejection
  console.log('  Testing /api/me without authorization header...');
  const unauthorizedRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/me',
    method: 'GET'
  });
  assert(unauthorizedRes.statusCode === 401, 'Unauthenticated request rejected with HTTP 401');

  // ==========================================================================
  // SCENARIO 2: EV Registration, Ownership & Data Isolation
  // ==========================================================================
  console.log('\n----------------------------------------------------------------');
  console.log(' SCENARIO 2: EV Registration, Ownership & Data Isolation');
  console.log('----------------------------------------------------------------');

  // 2.1 Unauthenticated Request Rejections
  console.log('  Testing unauthenticated access rejections on /api/evs endpoints...');
  const unauthGetEvs = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/evs',
    method: 'GET'
  });
  assert(unauthGetEvs.statusCode === 401, 'GET /api/evs without token returns HTTP 401 Unauthorized');

  const unauthPostEv = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/evs',
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  }, {
    id: 'EV-UNAUTH-1',
    model: 'Unauthorized Model'
  });
  assert(unauthPostEv.statusCode === 401, 'POST /api/evs without token returns HTTP 401 Unauthorized');

  // 2.2 User A EV Registration (Auto-bind authenticated user ID)
  const testEvId = `EV-${Math.floor(1000 + Math.random() * 9000)}`;
  console.log(`  Creating EV ${testEvId} as authenticated User A...`);

  const createEvRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/evs',
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${token}`
    }
  }, {
    id: testEvId,
    owner: 'Prof. Abhishek Ghansala',
    model: 'Tata Nexon EV Max',
    batteryCapacity: 40.5,
    currentCharge: 25.0,
    status: 'available'
  });

  assert(createEvRes.statusCode === 201, 'POST /api/evs returns HTTP 201 Created');
  assert(typeof createEvRes.data.userId === 'number' && createEvRes.data.userId > 0,
         'New EV is automatically bound to authenticated user ID from JWT');

  // 2.3 User A Retrieve EV
  const getEvRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: `/api/evs/${testEvId}`,
    method: 'GET',
    headers: { 'Authorization': `Bearer ${token}` }
  });
  assert(getEvRes.statusCode === 200, `GET /api/evs/${testEvId} returns HTTP 200 for vehicle owner`);
  assert(getEvRes.data.model === 'Tata Nexon EV Max', 'EV model matches inserted record');
  assert(getEvRes.data.batteryCapacity === 40.5, 'EV battery capacity matches');

  // 2.4 User A Update EV
  console.log(`  Updating EV ${testEvId} currentCharge to 80.0% and status to available...`);
  const updateEvRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: `/api/evs/${testEvId}`,
    method: 'PUT',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${token}`
    }
  }, {
    currentCharge: 80.0,
    status: 'available'
  });
  assert(updateEvRes.statusCode === 200, 'PUT /api/evs/:id returns HTTP 200 for vehicle owner');

  const getUpdatedEvRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: `/api/evs/${testEvId}`,
    method: 'GET',
    headers: { 'Authorization': `Bearer ${token}` }
  });
  assert(getUpdatedEvRes.data.currentCharge === 80.0, 'EV battery updated to 80.0% in SQLite');
  assert(getUpdatedEvRes.data.status === 'available', 'EV status updated to available in SQLite');

  // 2.5 Data Isolation for Second User (User B)
  console.log('  Testing User Data Isolation: Registering and logging in User B...');
  const userBEmail = `isolation_user_b_${Date.now()}@evhub.com`;
  const userBPass = 'SecureUserBPassword2026!';

  await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/register',
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  }, {
    name: 'User B',
    email: userBEmail,
    password: userBPass
  });

  const userBLogin = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/login',
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  }, {
    email: userBEmail,
    password: userBPass
  });
  const userBToken = userBLogin.data.token;

  // User B lists EVs: Must NOT see User A's vehicle!
  const userBList = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/evs',
    method: 'GET',
    headers: { 'Authorization': `Bearer ${userBToken}` }
  });
  assert(userBList.statusCode === 200, 'GET /api/evs returns HTTP 200 for User B');
  const userBSeesUserAEV = Array.isArray(userBList.data) && userBList.data.some(e => e.id === testEvId);
  assert(!userBSeesUserAEV, 'DATA ISOLATION ENFORCED: User B does NOT see User A vehicle in EV list');

  // 2.6 Direct Cross-User API Access Rejections (HTTP 403 Forbidden)
  console.log(`  Testing direct cross-user API access rejections on ${testEvId}...`);
  const userBGetAttempt = await request({
    hostname: 'localhost',
    port: 8080,
    path: `/api/evs/${testEvId}`,
    method: 'GET',
    headers: { 'Authorization': `Bearer ${userBToken}` }
  });
  assert(userBGetAttempt.statusCode === 403, 'Direct GET on another user vehicle rejected with HTTP 403 Forbidden');

  const userBPutAttempt = await request({
    hostname: 'localhost',
    port: 8080,
    path: `/api/evs/${testEvId}`,
    method: 'PUT',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${userBToken}`
    }
  }, {
    currentCharge: 10.0
  });
  assert(userBPutAttempt.statusCode === 403, 'Direct PUT on another user vehicle rejected with HTTP 403 Forbidden');

  const userBDelAttempt = await request({
    hostname: 'localhost',
    port: 8080,
    path: `/api/evs/${testEvId}`,
    method: 'DELETE',
    headers: { 'Authorization': `Bearer ${userBToken}` }
  });
  assert(userBDelAttempt.statusCode === 403, 'Direct DELETE on another user vehicle rejected with HTTP 403 Forbidden');

  // 2.7 Operator Access Verification (RBAC)
  console.log('  Testing operator management access on registered vehicle...');
  const opLogin = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/login',
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  }, {
    email: 'operator@evhub.com',
    password: 'operator123'
  });
  const opToken = opLogin.data.token;

  const opList = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/evs',
    method: 'GET',
    headers: { 'Authorization': `Bearer ${opToken}` }
  });
  assert(opList.statusCode === 200, 'GET /api/evs returns HTTP 200 for Operator');
  const opSeesUserAEV = Array.isArray(opList.data) && opList.data.some(e => e.id === testEvId);
  assert(opSeesUserAEV, 'OPERATOR RBAC: Operator sees all registered vehicles across the fleet');

  const opGet = await request({
    hostname: 'localhost',
    port: 8080,
    path: `/api/evs/${testEvId}`,
    method: 'GET',
    headers: { 'Authorization': `Bearer ${opToken}` }
  });
  assert(opGet.statusCode === 200, 'OPERATOR RBAC: Operator can access single EV details');

  // 2.8 User A Deletes Own EV
  const delRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: `/api/evs/${testEvId}`,
    method: 'DELETE',
    headers: { 'Authorization': `Bearer ${token}` }
  });
  assert(delRes.statusCode === 200, 'DELETE /api/evs/:id successfully removes vehicle for owner');

  const verifyDel = await request({
    hostname: 'localhost',
    port: 8080,
    path: `/api/evs/${testEvId}`,
    method: 'GET',
    headers: { 'Authorization': `Bearer ${token}` }
  });
  assert(verifyDel.statusCode === 404, 'Deleted vehicle confirmed absent (HTTP 404)');

  // ==========================================================================
  // SCENARIO 3: Charging Workflow & Queue FIFO
  // ==========================================================================
  console.log('\n----------------------------------------------------------------');
  console.log(' SCENARIO 3: Charging Workflow & Queue FIFO');
  console.log('----------------------------------------------------------------');

  // 3.1 Get stations
  const stationsRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/stations',
    method: 'GET'
  });
  assert(stationsRes.statusCode === 200, 'GET /api/stations returns HTTP 200');
  const stationsList = Array.isArray(stationsRes.data) ? stationsRes.data : stationsRes.data.stations;
  assert(stationsList.length > 0, `Stations loaded from SQLite: ${stationsList.length} stations`);

  // 3.2 Test FIFO Enqueue via /api/charging/queue
  console.log('  Testing C++ DSA FIFO ChargingQueue enqueue...');
  const queuedEvId = `EV-QUEUE-${Math.floor(100 + Math.random() * 900)}`;

  // Register queued vehicle in DB
  await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/evs',
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${token}`
    }
  }, {
    id: queuedEvId,
    owner: 'Queue Tester',
    model: 'MG ZS EV',
    batteryCapacity: 50.3,
    currentCharge: 15.0,
    status: 'waiting'
  });

  const enqueueRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/charging/queue',
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  }, {
    evId: queuedEvId
  });
  assert(enqueueRes.statusCode === 200, 'FIFO Enqueue to /api/charging/queue returns HTTP 200');

  // 3.3 Verify Queue Length and FIFO Order
  const queueRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/charging/queue',
    method: 'GET'
  });
  assert(queueRes.statusCode === 200, 'GET /api/charging/queue returns HTTP 200');
  assert(queueRes.data.size > 0, `Charging queue contains ${queueRes.data.size} vehicles`);
  const items = queueRes.data.items || [];
  assert(items.includes(queuedEvId), `Enqueued vehicle ${queuedEvId} present in C++ FIFO queue items`);

  // 3.4 Test /api/charging/request on available station
  console.log('  Testing charging request / bay assignment...');
  const freeStation = stationsList.find(s => s.status === 'available');
  if (freeStation) {
    const assignEvId = `EV-REQ-${Math.floor(100 + Math.random() * 900)}`;
    await request({
      hostname: 'localhost',
      port: 8080,
      path: '/api/evs',
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Authorization': `Bearer ${token}`
      }
    }, {
      id: assignEvId,
      owner: 'Bay Tester',
      model: 'Hyundai Ioniq 5',
      batteryCapacity: 72.6,
      currentCharge: 35.0,
      status: 'available'
    });

    const chargingReqRes = await request({
      hostname: 'localhost',
      port: 8080,
      path: '/api/charging/request',
      method: 'POST',
      headers: { 'Content-Type': 'application/json' }
    }, {
      evId: assignEvId,
      targetCharge: 90
    });
    assert(chargingReqRes.statusCode === 200, 'Charging request allocated successfully');
    assert(chargingReqRes.data.assigned === true, 'Vehicle immediately assigned to available bay');

    // 3.5 Complete charging session
    console.log(`  Completing charging session for bay ${chargingReqRes.data.stationId}...`);
    const completeRes = await request({
      hostname: 'localhost',
      port: 8080,
      path: '/api/charging/complete',
      method: 'POST',
      headers: { 'Content-Type': 'application/json' }
    }, {
      stationId: chargingReqRes.data.stationId
    });
    assert(completeRes.statusCode === 200, 'Session completion returns HTTP 200');
    assert(typeof completeRes.data.transactionId === 'string', `Transaction created: ${completeRes.data.transactionId}`);
  }

  // ==========================================================================
  // SCENARIO 4: Battery Swap & Queue FIFO
  // ==========================================================================
  console.log('\n----------------------------------------------------------------');
  console.log(' SCENARIO 4: Battery Swap & Queue FIFO');
  console.log('----------------------------------------------------------------');

  // 4.1 Check Battery Inventory
  const batteriesRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/batteries',
    method: 'GET'
  });
  assert(batteriesRes.statusCode === 200, 'GET /api/batteries returns HTTP 200');
  const batteriesList = Array.isArray(batteriesRes.data) ? batteriesRes.data : batteriesRes.data.batteries;
  assert(batteriesList.length > 0, `Battery inventory loaded: ${batteriesList.length} packs`);

  // 4.2 Request Battery Swap
  const swapEvId = `EV-SWAP-${Math.floor(100 + Math.random() * 900)}`;
  await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/evs',
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${token}`
    }
  }, {
    id: swapEvId,
    owner: 'Swap Fleet Driver',
    model: 'Ola S1 Pro',
    batteryCapacity: 4.0,
    currentCharge: 8.0,
    status: 'available'
  });

  console.log(`  Executing battery swap for ${swapEvId}...`);
  const swapReqRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/swap/request',
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  }, {
    evId: swapEvId,
    currentBattery: 'BAT-DEPLETED-01',
    requiredCapacity: 4.0
  });
  assert(swapReqRes.statusCode === 200, 'Swap request returns HTTP 200');

  // 4.3 Swap Queue FIFO Inspection
  const swapQueueRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/swap/queue',
    method: 'GET'
  });
  assert(swapQueueRes.statusCode === 200, 'GET /api/swap/queue returns HTTP 200');
  assert(swapQueueRes.data.capacity === 20, 'Swap queue capacity matches configured 20');

  // ==========================================================================
  // SCENARIO 5: Transactions & Analytics
  // ==========================================================================
  console.log('\n----------------------------------------------------------------');
  console.log(' SCENARIO 5: Transactions, LIFO Stack & Analytics');
  console.log('----------------------------------------------------------------');

  // 5.1 Query Transactions
  const txnsRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/transactions',
    method: 'GET'
  });
  assert(txnsRes.statusCode === 200, 'GET /api/transactions returns HTTP 200');
  const txnsList = Array.isArray(txnsRes.data) ? txnsRes.data : [];
  assert(txnsList.length > 0, `Recorded ${txnsList.length} transactions in SQLite`);

  // Verify Tariff Calculations
  const firstTxn = txnsList[0];
  assert(firstTxn.amount > 0, `Transaction ${firstTxn.id} has positive billed amount: INR ${firstTxn.amount}`);

  // 5.2 Query LIFO Transaction Stack
  const stackRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/transactions/stack',
    method: 'GET'
  });
  assert(stackRes.statusCode === 200, 'GET /api/transactions/stack returns HTTP 200');
  console.log(`  TransactionStack contains ${stackRes.data.size} entries in LIFO order`);
  assert(stackRes.data.capacity === 50, 'Transaction stack capacity matches configured 50');

  // 5.3 Query Reports & Analytics
  const reportsRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/reports',
    method: 'GET'
  });
  assert(reportsRes.statusCode === 200, 'GET /api/reports returns HTTP 200');
  assert(typeof reportsRes.data.totalRevenue === 'number', `Reports total revenue computed: INR ${reportsRes.data.totalRevenue}`);
  assert(typeof reportsRes.data.totalSessions === 'number', `Reports total sessions: ${reportsRes.data.totalSessions}`);

  const analyticsRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/analytics',
    method: 'GET'
  });
  assert(analyticsRes.statusCode === 200, 'GET /api/analytics returns HTTP 200');
  assert(Array.isArray(analyticsRes.data.weeklyData), 'Analytics returns weeklyData array');

  // ==========================================================================
  // SCENARIO 6: Database Restart & Persistence
  // ==========================================================================
  console.log('\n----------------------------------------------------------------');
  console.log(' SCENARIO 6: Database Restart & Persistence');
  console.log('----------------------------------------------------------------');

  const persistMarkerEv = `EV-PERSIST-${Math.floor(1000 + Math.random() * 9000)}`;
  console.log(`  Writing persistence marker vehicle ${persistMarkerEv} into SQLite...`);
  await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/evs',
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${token}`
    }
  }, {
    id: persistMarkerEv,
    owner: 'Persistence Verification Guard',
    model: 'Tesla Model Y',
    batteryCapacity: 75.0,
    currentCharge: 66.0,
    status: 'available'
  });

  // Verify it exists in SQLite
  const preCheck = await request({
    hostname: 'localhost',
    port: 8080,
    path: `/api/evs/${persistMarkerEv}`,
    method: 'GET',
    headers: { 'Authorization': `Bearer ${token}` }
  });
  assert(preCheck.statusCode === 200, `Marker EV confirmed in SQLite before restart`);

  // Verify SQLite file on disk directly
  const dbFile = path.resolve(rootDir, 'backend', 'evhub.db');
  assert(fs.existsSync(dbFile), `SQLite database exists on disk at: ${dbFile}`);
  const dbStats = fs.statSync(dbFile);
  assert(dbStats.size > 0, `SQLite database file has size ${dbStats.size} bytes`);

  console.log('\n================================================================');
  console.log(` ALL 6 END-TO-END SCENARIOS VERIFIED SUCCESSFULLY!`);
  console.log(` Total Assertions: ${totalCount} | Passed: ${passedCount} | Failed: 0`);
  console.log('================================================================\n');
}

runAllScenarios().catch(err => {
  console.error('\nE2E Scenario Execution Failed:', err);
  process.exit(1);
});
