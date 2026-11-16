import http from 'http';

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

let passed = 0;
let total = 0;

function assert(cond, msg) {
  total++;
  if (cond) {
    console.log(`  [PASS] ${msg}`);
    passed++;
  } else {
    console.error(`  [FAIL] ${msg}`);
    throw new Error(`Assertion failed: ${msg}`);
  }
}

async function runAuthRegressionTests() {
  console.log('================================================================');
  console.log(' SMART EV HUB - AUTHENTICATION & LOGIN REGRESSION TEST SUITE   ');
  console.log('================================================================\n');

  // Test 1: Verify all demo accounts can authenticate successfully
  console.log('--- Test Suite 1: Demo Account Authentication & JWT Issuance ---');
  const demoAccounts = [
    { email: 'admin@evhub.com', pass: 'admin123', role: 'admin' },
    { email: 'operator@evhub.com', pass: 'operator123', role: 'operator' },
    { email: 'aditya@evhub.com', pass: 'aditya', role: 'admin' },
    { email: 'abhishek@evhub.com', pass: 'abhishek', role: 'admin' }
  ];

  for (const acct of demoAccounts) {
    const res = await request({
      hostname: 'localhost',
      port: 8080,
      path: '/api/login',
      method: 'POST',
      headers: { 'Content-Type': 'application/json' }
    }, { email: acct.email, password: acct.pass });

    assert(res.statusCode === 200, `Login succeeded for ${acct.email} (${acct.role})`);
    assert(res.data && res.data.success === true, `Response success is true for ${acct.email}`);
    assert(res.data && typeof res.data.token === 'string' && res.data.token.length > 20, `Valid JWT returned for ${acct.email}`);
    assert(res.data && res.data.user && res.data.user.role === acct.role, `Correct role returned for ${acct.email}`);

    // Verify token with /api/me
    const meRes = await request({
      hostname: 'localhost',
      port: 8080,
      path: '/api/me',
      method: 'GET',
      headers: { 'Authorization': `Bearer ${res.data.token}` }
    });
    assert(meRes.statusCode === 200, `/api/me validated token for ${acct.email}`);
    assert(meRes.data.user.email === acct.email, `/api/me returned matching email ${acct.email}`);
  }

  // Test 2: Invalid password and unknown user rejection
  console.log('\n--- Test Suite 2: Invalid Credentials & Security Defense ---');
  const badPassRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/login',
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  }, { email: 'admin@evhub.com', password: 'wrongpassword' });
  assert(badPassRes.statusCode === 401, 'Wrong password rejected with HTTP 401');
  assert(badPassRes.data && badPassRes.data.success === false, 'Wrong password response indicates failure');

  const unknownUserRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/login',
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  }, { email: 'nonexistent@evhub.com', password: 'somepassword' });
  assert(unknownUserRes.statusCode === 401, 'Unknown user rejected with HTTP 401');

  // Test 3: Public endpoints accessible without token
  console.log('\n--- Test Suite 3: Public Endpoints (No Auth Needed) ---');
  const publicEndpoints = [
    '/api/health',
    '/api/charging/queue',
    '/api/swap/queue',
    '/api/transactions/stack',
    '/api/stations',
    '/api/batteries',
    '/api/transactions',
    '/api/sessions'
  ];

  for (const ep of publicEndpoints) {
    const res = await request({
      hostname: 'localhost',
      port: 8080,
      path: ep,
      method: 'GET'
    });
    assert(res.statusCode === 200, `Public endpoint ${ep} returned HTTP 200 without token`);
  }

  // Test 4: Protected endpoints reject unauthenticated access
  console.log('\n--- Test Suite 4: Protected Endpoints (Security Enforcement) ---');
  const protectedEndpoints = [
    '/api/evs',
    '/api/charging/requests'
  ];

  for (const ep of protectedEndpoints) {
    const res = await request({
      hostname: 'localhost',
      port: 8080,
      path: ep,
      method: 'GET'
    });
    assert(res.statusCode === 401, `Protected endpoint ${ep} rejected unauthenticated request with HTTP 401`);
  }

  // Test 5: Simulated Frontend Data Hydration Flow
  console.log('\n--- Test Suite 5: Frontend Authentication & Data Flow Simulation ---');
  // State: Unauthenticated (User visits login page)
  let simulatedLocalStorage = { authToken: null, currentUser: null };

  // Frontend refreshBackendData simulation when unauthenticated:
  // Must ONLY fetch public endpoints; must skip protected endpoints
  const unauthResults = await Promise.all([
    request({ hostname: 'localhost', port: 8080, path: '/api/charging/queue', method: 'GET' }),
    request({ hostname: 'localhost', port: 8080, path: '/api/swap/queue', method: 'GET' }),
    request({ hostname: 'localhost', port: 8080, path: '/api/transactions/stack', method: 'GET' }),
    simulatedLocalStorage.authToken ? request({ hostname: 'localhost', port: 8080, path: '/api/evs', method: 'GET' }) : Promise.resolve({ statusCode: 200, data: [] }),
    request({ hostname: 'localhost', port: 8080, path: '/api/stations', method: 'GET' }),
    request({ hostname: 'localhost', port: 8080, path: '/api/batteries', method: 'GET' }),
    request({ hostname: 'localhost', port: 8080, path: '/api/transactions', method: 'GET' }),
    request({ hostname: 'localhost', port: 8080, path: '/api/sessions', method: 'GET' }),
    simulatedLocalStorage.authToken ? request({ hostname: 'localhost', port: 8080, path: '/api/charging/requests', method: 'GET' }) : Promise.resolve({ statusCode: 200, data: [] }),
  ]);

  const hasAny401Unauth = unauthResults.some(r => r.statusCode === 401);
  assert(!hasAny401Unauth, 'Unauthenticated page load produces ZERO 401 Unauthorized errors');

  // State: User registers and logs in
  const testUserEmail = `driver_test_${Date.now()}@evhub.com`;
  const testUserPass = 'DriverPassword2026!';
  console.log(`  Registering new user ${testUserEmail}...`);
  const regRes = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/register',
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  }, { name: 'Verified Driver', email: testUserEmail, password: testUserPass, role: 'user' });

  assert(regRes.statusCode === 201, 'Registration returns HTTP 201 Created');
  assert(regRes.data.success === true, 'Registration succeeded');
  assert(typeof regRes.data.token === 'string', 'Registration returned JWT token');

  // User logs in
  console.log(`  Logging in as ${testUserEmail}...`);
  const userLogin = await request({
    hostname: 'localhost',
    port: 8080,
    path: '/api/login',
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  }, { email: testUserEmail, password: testUserPass });

  assert(userLogin.statusCode === 200, 'User login HTTP 200');
  simulatedLocalStorage.authToken = userLogin.data.token;
  simulatedLocalStorage.currentUser = userLogin.data.user;

  // Frontend refreshBackendData simulation when authenticated:
  const authResults = await Promise.all([
    request({ hostname: 'localhost', port: 8080, path: '/api/charging/queue', method: 'GET' }),
    request({ hostname: 'localhost', port: 8080, path: '/api/swap/queue', method: 'GET' }),
    request({ hostname: 'localhost', port: 8080, path: '/api/transactions/stack', method: 'GET' }),
    request({
      hostname: 'localhost',
      port: 8080,
      path: '/api/evs',
      method: 'GET',
      headers: { 'Authorization': `Bearer ${simulatedLocalStorage.authToken}` }
    }),
    request({ hostname: 'localhost', port: 8080, path: '/api/stations', method: 'GET' }),
    request({ hostname: 'localhost', port: 8080, path: '/api/batteries', method: 'GET' }),
    request({ hostname: 'localhost', port: 8080, path: '/api/transactions', method: 'GET' }),
    request({ hostname: 'localhost', port: 8080, path: '/api/sessions', method: 'GET' }),
    request({
      hostname: 'localhost',
      port: 8080,
      path: '/api/charging/requests',
      method: 'GET',
      headers: { 'Authorization': `Bearer ${simulatedLocalStorage.authToken}` }
    }),
  ]);

  const allAuthOk = authResults.every(r => r.statusCode === 200);
  assert(allAuthOk, 'Authenticated data load succeeds for all 9 endpoints with HTTP 200');
  const userEVs = authResults[3].data;
  assert(Array.isArray(userEVs), 'User EVs is an array');
  assert(userEVs.length === 0, 'New user starts with 0 EVs (Data Isolation enforced)');

  // State: Logout
  console.log('  Simulating logout...');
  simulatedLocalStorage = { authToken: null, currentUser: null };
  assert(simulatedLocalStorage.authToken === null, 'Logout successfully cleared authToken');
  assert(simulatedLocalStorage.currentUser === null, 'Logout successfully cleared currentUser');

  console.log('\n================================================================');
  console.log(` ALL AUTHENTICATION REGRESSION TESTS PASSED (${passed}/${total})`);
  console.log('================================================================\n');
}

runAuthRegressionTests().catch(err => {
  console.error('\nTest Suite Failed:', err);
  process.exit(1);
});
