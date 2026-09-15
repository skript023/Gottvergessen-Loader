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
x64-only. Build the x64 GuidedHacking Injector locally, then place it at
`../vendor/gh-injector/GH Injector - x64.dll` (or configure CMake with
`-DGH_INJECTOR_X64_DLL=<path>`). The native build copies it beside
`native-core.dll` as `injection-core.dll`; Electron packages both files under
`resources/native`. Thread Hijack and Manual Map require this runtime.
