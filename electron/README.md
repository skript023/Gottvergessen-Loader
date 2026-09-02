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

The native build is generated in `../out/build/electron/bin/Release`. Use a matching x64 DLL
and target process. Thread Hijack and Manual Map also require the existing
`GH Injector - x64.dll` beside the Electron executable/native addon.
