# Astra

Astra is a Windows desktop application for managing game entries, authenticated binary downloads, and native process operations. Its Electron interface uses Vue 3 and TypeScript, with a C++ core loaded through Koffi.

The current application version is **1.0.0**. The portable build is named `Astra-1.0.0-portable.exe`.

## Features

- Account sign-in, session restoration, and server-driven session updates.
- Game library scanning and custom executable entries.
- Binary catalog, downloads, and per-binary launch settings.
- Native process listing and DLL injection operations.
- Automatic application updates with download progress, pause, and resume.
- Diagnostics and connection status with general connection information on hover or keyboard focus.

## Requirements

- Windows x64.
- Node.js and npm. The current build was verified with Node.js 22.20.0.
- Git available on `PATH`.
- Visual Studio with Desktop development with C++, Windows SDK, and x64/x86 build tools.
- CMake available on `PATH` or through Visual Studio.
- Internet access for the initial dependency download; an accessible backend and account for authenticated features.

The native build script currently selects the `Visual Studio 18 2026` CMake generator. Use a Visual Studio/CMake installation that supports that generator. See [the native build script](electron/scripts/build-native.ps1) for the exact configuration.

## Run locally

From the repository root, open PowerShell and run:

```powershell
cd electron
npm ci
npm run dev
```

This builds the Vue interface, builds the native components, and opens Astra. After the native components have been built, `npm start` rebuilds the interface and launches the application without rebuilding C++.

## Build the portable application

From the `electron` directory:

```powershell
npm run dist
```

This builds the interface and native components, then packages the Windows x64 application. With the current version, the output is:

```text
electron/dist/Astra-1.0.0-portable.exe
```

Packaging preserves the version in [electron/package.json](electron/package.json). It does not automatically increment it. To prepare a new patch release, run `npm run version:bump` explicitly before building. The updater reads the package version, independent of the executable filename.

## Connection status

Hover or focus the connection badge in the dashboard header to view:

- Current connection status.
- Response time measured over HTTP, in milliseconds.
- Last check time and general connection messages.

The tooltip does not display server hosts, addresses, protocols, HTTP codes, or server software. The connection-status API returns only status, response time, check time, and general error messages.

The badge refreshes every 15 seconds with a 5-second timeout. Successful HTTP responses show **ONLINE**, other HTTP responses show **DEGRADED**, and network failures show **OFFLINE**. **CHECKING** appears before the first result; **UNKNOWN** means connection information could not be obtained from the desktop API.

The check uses `/client/check-update` without session credentials. Its latency measures the HTTP response, not ICMP ping or the health of every backend service.

## Development commands

Run these commands from `electron`:

| Command | Purpose |
| --- | --- |
| `npm run dev` | Build the UI and native components, then launch Astra. |
| `npm start` | Build the UI and launch with existing native components. |
| `npm run dev:ui` | Start the Vite UI development server. |
| `npm run build:ui` | Build the Vue interface. |
| `npm run native:configure` | Configure the native x64 and x86 builds. |
| `npm run native:build` | Build the native components in Release mode. |
| `npm run dist` | Build and package the portable Windows x64 application. |
| `npx vue-tsc --noEmit` | Check Vue and TypeScript types. |
| `node --test scripts/serverStatus.test.cjs` | Run connection-monitor tests. |

The Vite server alone does not provide the Electron IPC or native APIs. See [the Electron development guide](electron/README.md) for native outputs, backend configuration, and application data paths.

## Project structure

```text
electron/
  src/                    Vue components, stores, and application views
  main.js                 Electron main process and native bridge
  preload.js              Renderer IPC API
  updater.js              Application update handling
  serverStatus.js         HTTP connection monitoring
  scripts/build-native.ps1 Native build orchestration
src/
  electron/               Native C ABI and update runner
  api/                    Backend integration and downloads
  process/                Native process operations
scripts/                  CMake dependency configuration
CMakeLists.txt            Native targets and optional legacy ImGui frontend
```

The Electron interface is the default frontend. The legacy ImGui executable is disabled by default with `BUILD_IMGUI_FRONTEND=OFF`.

## License

See [LICENSE](LICENSE) for the repository license. Third-party components retain their respective licenses.
