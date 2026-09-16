# Astra Electron development

Astra uses Electron, Vue 3, TypeScript, and Pinia. The main process loads the standard C ABI `native-core.dll` through Koffi. The native library is built by the root CMake project and does not depend on Node.js or Electron headers.

See the [main README](../README.md) for prerequisites, features, packaging, and the command reference. All commands below run from this directory.

## Development

```powershell
npm ci
npm run dev
```

For UI changes with an existing native build:

```powershell
npm start
```

For Vite hot reload, run `npm run dev:ui` in one terminal. In a second PowerShell terminal, launch Electron against that server:

```powershell
$env:VITE_DEV_SERVER_URL = "http://localhost:5173"
npx electron .
```

Build the native components first with `npm run native:build`. Use the actual Vite URL if port 5173 is occupied. The browser-only Vite view has no native bridge.

## Native build outputs

The build script configures x64 and Win32 Release builds with the `Visual Studio 18 2026` generator.

| Output | Location relative to the repository root |
| --- | --- |
| x64 native library | `out/build/electron/bin/Release/native-core.dll` |
| x86 native library | `out/build/electron-x86/bin/Release/native-core-x86.dll` |
| Update helper | `out/build/electron/bin/Release/update-runner.exe` |

The x86 library is also copied into the x64 output directory. The current Electron package is x64 and packages `native-core.dll` and `update-runner.exe`; it does not include the x86 library in its `extraResources` list.

The Electron target builds the in-house CreateRemoteThread, Thread Hijack, QueueUserAPC, and Reflective DLL implementations. It does not package the GuidedHacking runtime, and the legacy `manual_map_injection.cpp` wrapper is excluded from this target. Any use or redistribution of third-party runtime components must follow their applicable licenses.

## Backend and local data

The updater and connection monitor resolve their backend in this order:

1. `VITE_BACKEND_URL` environment variable.
2. `BACKEND_URL` environment variable.
3. The native core's configured backend URL.
4. The fallback `https://apie.rena.my.id`.

These environment overrides apply to the updater and connection monitor. Native authentication uses the configuration in [environment.hpp](../src/api/environment.hpp); Release builds currently select its production backend.

Native application data is stored in the repository's `data` directory during development. Portable builds use a `data` directory beside the portable executable. Update downloads are stored under Electron's `userData` directory in `updates`.

Automatic application update checks are bypassed in unpackaged development runs. The connection badge still probes the backend independently.

## Versioning and packaging

```powershell
npm run dist
```

The version in [package.json](package.json) is currently `1.0.0`, producing `dist/Astra-1.0.0-portable.exe`. Packaging does not change the version. Use `npm run version:bump` explicitly when preparing the next patch release.

## Verification

```powershell
npx vue-tsc --noEmit
node --test scripts/serverStatus.test.cjs
npm run build:ui
```

The connection tests use a local HTTP server to cover successful responses, HTTP failures, timeouts, invalid configuration, deduplicated/cached requests, and exclusion of server identity from connection-status responses.
