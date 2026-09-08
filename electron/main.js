const { app, BrowserWindow, dialog, ipcMain } = require("electron");
const fs = require("node:fs");
const path = require("node:path");
const koffi = require("koffi");

let native;
let nativeLibrary;

app.setAppUserModelId("com.ellohim.gottvergessen-loader");

function iconPath() {
  return app.isPackaged
    ? path.join(process.resourcesPath, "assets", "logo.ico")
    : path.join(__dirname, "..", "src", "logo.ico");
}

function nativePath() {
  if (app.isPackaged) {
    return path.join(process.resourcesPath, "native", "native-core.dll");
  }
  return path.join(__dirname, "..", "out", "build", "electron", "bin", "Release", "native-core.dll");
}

function loadNative() {
  const addonPath = nativePath();
  if (!fs.existsSync(addonPath)) {
    throw new Error(`Native addon not found: ${addonPath}. Run npm run native:build first.`);
  }

  nativeLibrary = koffi.load(addonPath);
  const library = nativeLibrary;
  const initialize = library.func("bool __cdecl gv_initialize(str16 base_directory)");
  let shutdown = null;
  try {
    shutdown = library.func("void __cdecl gv_shutdown()");
  } catch (_) {}
  const listProcessesJson = library.func("str __cdecl gv_list_processes_json()");
  const setTarget = library.func("bool __cdecl gv_set_target(str process_name, uint32_t pid)");
  const setInjectionMode = library.func("bool __cdecl gv_set_injection_mode(int mode)");
  const validateLibrary = library.func("int __cdecl gv_validate_library(str16 dll_path)");
  const inject = library.func("int __cdecl gv_inject(str16 dll_path)");
  const login = library.func("int __cdecl gv_login(str username, str password, int remember_me)");
  const logout = library.func("int __cdecl gv_logout()");
  const restoreSession = library.func("int __cdecl gv_restore_session()");
  const refreshBinaries = library.func("str __cdecl gv_refresh_binaries()");
  const profileJson = library.func("str __cdecl gv_profile_json()");
  const selectBinary = library.func("int __cdecl gv_select_binary(int index)");
  const saveBinarySettings = library.func("int __cdecl gv_save_binary_settings(str binary_id, str target_process, int mode)");
  const downloadAndInject = library.func("int __cdecl gv_download_and_inject()");
  const lastError = library.func("str __cdecl gv_last_error()");
  const operationStatusJson = library.func("str __cdecl gv_operation_status_json()");
  let getToken = null;
  let getHwid = null;
  let getBackendUrl = null;
  let getDeviceName = null;
  try {
    getToken = library.func("str __cdecl gv_get_token()");
    getHwid = library.func("str __cdecl gv_get_hwid()");
    getBackendUrl = library.func("str __cdecl gv_get_backend_url()");
    getDeviceName = library.func("str __cdecl gv_get_device_name()");
  } catch (_) {}

  if (!initialize(path.join(app.getPath("appData"), "Ellohim Menu"))) {
    throw new Error(lastError() || "Native DLL initialization failed");
  }

  native = {
    shutdown() {
      if (typeof shutdown === "function") {
        try {
          shutdown();
        } catch (_) {}
      }
    },
    listProcesses() {
      const payload = listProcessesJson();
      if (!payload) throw new Error(lastError() || "Could not enumerate processes");
      return JSON.parse(payload);
    },
    operationStatus() {
      return JSON.parse(operationStatusJson());
    },
    setTarget(processName, pid) {
      if (!setTarget(processName, pid)) throw new Error(lastError() || "Could not set target");
    },
    setInjectionMode(mode) {
      if (!setInjectionMode(mode)) throw new Error(lastError() || "Could not set injection mode");
    },
    validateLibrary(dllPath) {
      return validateLibrary(dllPath) !== 0;
    },
    login(username, password, rememberMe) {
      return new Promise((resolve, reject) => {
        login.async(username, password, rememberMe ? 1 : 0, (error, result) => {
          if (error) reject(error);
          else if (result === 0) reject(new Error(lastError() || "Login failed"));
          else resolve(true);
        });
      });
    },
    restoreSession() {
      return new Promise((resolve, reject) => {
        restoreSession.async((error, result) => {
          if (error) reject(error);
          else resolve(result !== 0);
        });
      });
    },
    logout() {
      return new Promise((resolve, reject) => {
        logout.async((error, result) => {
          if (error) reject(error);
          else resolve(result !== 0);
        });
      });
    },
    refreshBinaries() {
      return new Promise((resolve, reject) => {
        refreshBinaries.async((error, payload) => {
          if (error) reject(error);
          else if (!payload) reject(new Error(lastError() || "Could not load binary catalog"));
          else resolve(JSON.parse(payload));
        });
      });
    },
    profile() {
      return new Promise((resolve, reject) => {
        profileJson.async((error, payload) => {
          if (error) reject(error);
          else if (!payload) reject(new Error(lastError() || "Could not load user profile"));
          else resolve(JSON.parse(payload));
        });
      });
    },
    selectBinary(index) {
      if (!selectBinary(index)) throw new Error(lastError() || "Could not select binary");
    },
    saveBinarySettings(binaryId, targetProcess, mode) {
      return new Promise((resolve, reject) => {
        saveBinarySettings.async(binaryId, targetProcess || "", Number(mode) || 0, (error, result) => {
          if (error) reject(error);
          else if (result === 0) reject(new Error(lastError() || "Failed to save binary settings"));
          else resolve(true);
        });
      });
    },
    downloadAndInject() {
      return new Promise((resolve, reject) => {
        downloadAndInject.async((error, result) => {
          if (error) reject(error);
          else if (result === 0) reject(new Error(lastError() || "Download/injection failed"));
          else resolve(true);
        });
      });
    },
    inject(dllPath) {
      return new Promise((resolve, reject) => {
        inject.async(dllPath, (error, result) => {
          if (error) reject(error);
          else if (result === 0) reject(new Error(lastError() || "Native injection failed"));
          else resolve(true);
        });
      });
    },
    getToken() {
      return (typeof getToken === "function" ? getToken() : "") || "";
    },
    getHwid() {
      return (typeof getHwid === "function" ? getHwid() : "") || "";
    },
    getBackendUrl() {
      return (typeof getBackendUrl === "function" ? getBackendUrl() : "") || "https://apie.rena.my.id";
    },
    getDeviceName() {
      return (typeof getDeviceName === "function" ? getDeviceName() : "") || "Desktop-PC";
    }
  };
}

function requireNative() {
  if (!native) throw new Error("Native addon is not initialized");
  return native;
}

function validatePid(pid) {
  if (!Number.isSafeInteger(pid) || pid <= 0 || pid > 0xffffffff) {
    throw new TypeError("Invalid PID");
  }
}

function validateMode(mode) {
  if (!Number.isInteger(mode) || mode < 0 || mode > 3) {
    throw new TypeError("Invalid injection mode");
  }
}

function validateDllPath(dllPath) {
  if (typeof dllPath !== "string" || path.extname(dllPath).toLowerCase() !== ".dll") {
    throw new TypeError("A .dll file is required");
  }
  const resolved = path.resolve(dllPath);
  if (!fs.existsSync(resolved) || !fs.statSync(resolved).isFile()) {
    throw new Error("DLL file does not exist");
  }
  return resolved;
}

function registerIpc() {
  ipcMain.handle("native:list-processes", () => requireNative().listProcesses());
  ipcMain.handle("native:operation-status", () => requireNative().operationStatus());

  ipcMain.handle("native:login", async (_event, credentials) => {
    if (!credentials || typeof credentials.username !== "string" || typeof credentials.password !== "string") {
      throw new TypeError("Invalid credentials");
    }
    if (credentials.username.length < 1 || credentials.username.length > 128 || credentials.password.length < 1 || credentials.password.length > 256) {
      throw new TypeError("Invalid credential length");
    }
    await requireNative().login(credentials.username, credentials.password, credentials.rememberMe === true);
    try {
      const addon = requireNative();
      const [binaries, profile] = await Promise.all([
        addon.refreshBinaries(), addon.profile().catch(() => ({}))
      ]);
      const token = addon.getToken();
      const hwid = addon.getHwid();
      const backendUrl = addon.getBackendUrl();
      const deviceName = addon.getDeviceName();
      return { binaries, profile, token, hwid, backendUrl, deviceName };
    } catch (error) {
      throw new Error(`Login succeeded, but catalog loading failed: ${error.message}`);
    }
  });

  ipcMain.handle("native:restore-session", async () => {
    const restored = await requireNative().restoreSession();
    if (!restored) return null;
    const addon = requireNative();
    const [binaries, profile] = await Promise.all([
      addon.refreshBinaries(), addon.profile().catch(() => ({}))
    ]);
    const token = addon.getToken();
    const hwid = addon.getHwid();
    const backendUrl = addon.getBackendUrl();
    const deviceName = addon.getDeviceName();
    return { binaries, profile, token, hwid, backendUrl, deviceName };
  });

  ipcMain.handle("native:get-session-info", () => {
    const addon = requireNative();
    return {
      token: addon.getToken(),
      hwid: addon.getHwid(),
      backendUrl: addon.getBackendUrl(),
      deviceName: addon.getDeviceName()
    };
  });

  ipcMain.handle("native:logout", async () => {
    return await requireNative().logout();
  });

  ipcMain.handle("native:refresh-binaries", () => requireNative().refreshBinaries());

  ipcMain.handle("native:save-binary-settings", (_event, { binaryId, targetProcess, mode }) => {
    return requireNative().saveBinarySettings(binaryId, targetProcess, mode);
  });

  ipcMain.handle("native:inject", async (_event, request) => {
    if (!request || typeof request !== "object") throw new TypeError("Invalid request");
    validatePid(request.pid);
    validateMode(request.mode);
    if (typeof request.processName !== "string" || request.processName.length < 1 || request.processName.length > 260) {
      throw new TypeError("Invalid process name");
    }

    const addon = requireNative();
    addon.setTarget(request.processName, request.pid);
    addon.setInjectionMode(request.mode);
    if (!Number.isInteger(request.binaryIndex) || request.binaryIndex < 0) throw new TypeError("Invalid binary selection");
    addon.selectBinary(request.binaryIndex);
    return await addon.downloadAndInject();
  });
}

function getRendererPath() {
  const vueDist = path.join(__dirname, "dist-renderer", "index.html");
  if (fs.existsSync(vueDist)) {
    return vueDist;
  }
  return path.join(__dirname, "renderer", "index.html");
}

function createWindow() {
  const window = new BrowserWindow({
    width: 1120,
    height: 740,
    minWidth: 960,
    minHeight: 620,
    backgroundColor: "#080c14",
    title: "Gottvergessen Loader",
    icon: iconPath(),
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true
    }
  });
  window.setMenuBarVisibility(false);

  if (process.env.VITE_DEV_SERVER_URL) {
    window.loadURL(process.env.VITE_DEV_SERVER_URL);
  } else {
    window.loadFile(getRendererPath());
  }
}

let isExiting = false;

function cleanupAndExit() {
  if (isExiting) return;
  isExiting = true;

  if (native) {
    try {
      native.shutdown();
    } catch (_) {}
    native = null;
  }

  if (nativeLibrary) {
    try {
      nativeLibrary.unload();
    } catch (_) {}
    nativeLibrary = null;
  }

  app.exit(0);
}

app.whenReady().then(() => {
  loadNative();
  registerIpc();
  createWindow();

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
}).catch((error) => {
  dialog.showErrorBox("Startup failed", error.stack || error.message);
  cleanupAndExit();
});

app.on("window-all-closed", () => {
  cleanupAndExit();
});

app.on("before-quit", () => {
  cleanupAndExit();
});

process.on("SIGINT", () => {
  cleanupAndExit();
});

process.on("SIGTERM", () => {
  cleanupAndExit();
});
