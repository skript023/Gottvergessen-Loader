# Electron frontend

This frontend loads the standard C ABI `native-core.dll` through
Koffi. The DLL is built directly by the root `CMakeLists.txt` and has no
dependency on Electron or Node.js headers.

## Development

From this directory:

```powershell
npm install
npm run dev
```

The native build is generated in `../out/build/electron/bin/Release` and is
x64-only. The Electron target currently uses the in-house native strategies
(`CreateRemoteThread`, `Thread Hijack`, `QueueUserAPC`, and `Reflective DLL
Injection`) and does not package a GuidedHacking runtime. The legacy
`manual_map_injection.cpp` wrapper is intentionally not part of the Electron
CMake target until it is replaced by an independently implemented mapper.

Do not copy or redistribute GuidedHacking source, binaries, or derivative
implementations merely to bypass its license. If that runtime is used for
private testing, keep it outside distributable packages and obtain permission
for any broader distribution.

## Astra builds

The application version starts at 1.0.0 in package.json. Run npm run dist to
build Astra-1.0.0-portable.exe in dist. Packaging preserves the configured
version; use npm run version:bump explicitly when preparing the next release.
The updater reads this same version, regardless of the executable filename.

Hover or focus the connection badge to see the server host, HTTP response
latency, protocol, response status, server software (when advertised), and
last check time. Checks use the configured backend's /client/check-update
endpoint without session credentials, every 15 seconds with a 5-second
deadline. HTTP latency is not ICMP ping or a measure of full backend health.
Non-2xx responses show DEGRADED; network failures show OFFLINE.

Run node --test scripts/serverStatus.test.cjs to test connection monitoring.
