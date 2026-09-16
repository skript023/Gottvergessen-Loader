const { test } = require('node:test');
const assert = require('node:assert/strict');
const http = require('node:http');
const { probeServer, createServerMonitor } = require('../serverStatus');
async function serve(t, handler) {
  const server = http.createServer(handler);
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  t.after(() => { server.closeAllConnections(); server.close(); });
  return `http://127.0.0.1:${server.address().port}`;
}
test('successful probe reports latency without exposing server identity', async t => {
  const url = await serve(t, (req, res) => {
    assert.equal(req.url, '/client/check-update?version=1.0.0');
    assert.equal(req.headers['user-agent'], 'Astra/1.0.0');
    assert.equal(req.headers.authorization, undefined);
    res.setHeader('Server', 'Astra test');
    res.end('{}');
  });
  const result = await probeServer(url, '1.0.0');
  assert.equal(result.state, 'online');
  assert.deepEqual(Object.keys(result).sort(), ['checkedAt', 'error', 'pingMs', 'state']);
  assert.ok(!JSON.stringify(result).includes('127.0.0.1'));
  assert.ok(!JSON.stringify(result).includes('Astra test'));
  assert.ok(result.pingMs >= 0);
  assert.ok(Date.parse(result.checkedAt));
});
test('HTTP errors are degraded rather than online', async t => {
  const url = await serve(t, (_, res) => { res.writeHead(503); res.end(); });
  const result = await probeServer(url, '1.0.0');
  assert.equal(result.state, 'degraded');
  assert.equal(result.error, 'Service temporarily unavailable');
  assert.deepEqual(Object.keys(result).sort(), ['checkedAt', 'error', 'pingMs', 'state']);
});
test('deadline returns offline with no fabricated ping', async t => {
  const url = await serve(t, () => {});
  const result = await probeServer(url, '1.0.0', 30);
  assert.equal(result.state, 'offline');
  assert.equal(result.pingMs, null);
  assert.equal(result.error, 'Connection timed out');
});
test('invalid configuration returns unavailable status', async () => {
  assert.equal((await probeServer('invalid', '1.0.0')).state, 'offline');
});
test('concurrent calls and hover reuse cached probe', async t => {
  let calls = 0;
  const url = await serve(t, (_, res) => { calls++; res.end('{}'); });
  const monitor = createServerMonitor(() => url, '1.0.0');
  await Promise.all([monitor(), monitor()]);
  await monitor();
  assert.equal(calls, 1);
});
