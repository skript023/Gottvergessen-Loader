const http = require('node:http');
const https = require('node:https');
const { performance } = require('node:perf_hooks');

function probeServer(baseUrl, version, timeoutMs = 5000) {
  return new Promise((resolve) => {
    const result = {
      state: 'offline', pingMs: null, checkedAt: '', error: null
    };
    let url;
    try {
      url = new URL(`${baseUrl.replace(/\/+$/, '')}/client/check-update`);
      if (!['http:', 'https:'].includes(url.protocol)) throw new Error('Invalid protocol');
      url.searchParams.set('version', version);
    } catch (_) {
      resolve({ ...result, checkedAt: new Date().toISOString(), error: 'Connection information unavailable' });
      return;
    }
    const started = performance.now();
    let settled = false;
    let deadline;
    const finish = (values) => {
      if (settled) return;
      settled = true;
      clearTimeout(deadline);
      resolve({ ...result, ...values, checkedAt: new Date().toISOString() });
    };
    const request = (url.protocol === 'https:' ? https : http).get(url, {
      headers: { 'User-Agent': `Astra/${version}`, 'Cache-Control': 'no-cache' }
    }, (response) => {
      const status = response.statusCode || 0;
      finish({
        state: status >= 200 && status < 300 ? 'online' : 'degraded',
        pingMs: Math.round(performance.now() - started),
        error: status >= 200 && status < 300 ? null : 'Service temporarily unavailable'
      });
      response.destroy();
    });
    deadline = setTimeout(() => {
      finish({ error: 'Connection timed out' });
      request.destroy();
    }, timeoutMs);
    request.on('error', () => finish({ error: 'Unable to connect' }));
  });
}

function createServerMonitor(getBaseUrl, version) {
  let cached;
  let pending;
  return async () => {
    if (pending) return pending;
    if (cached && Date.now() - Date.parse(cached.checkedAt) < 10000) return cached;
    pending = probeServer(getBaseUrl(), version);
    try {
      cached = await pending;
      return cached;
    } finally {
      pending = null;
    }
  };
}

module.exports = { probeServer, createServerMonitor };
