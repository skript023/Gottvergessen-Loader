const { app, BrowserWindow, dialog, ipcMain } = require("electron");
const fs = require("node:fs");
const path = require("node:path");
const koffi = require("koffi");

let native;

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

  const library = koffi.load(addonPath);
  const initialize = library.func("bool __cdecl gv_initialize(str16 base_directory)");
  const listProcessesJson = library.func("str __cdecl gv_list_processes_json()");
  const setTarget = library.func("bool __cdecl gv_set_target(str process_name, uint32_t pid)");
  const setInjectionMode = library.func("bool __cdecl gv_set_injection_mode(int mode)");
  const validateLibrary = library.func("int __cdecl gv_validate_library(str16 dll_path)");
  const inject = library.func("int __cdecl gv_inject(str16 dll_path)");
  const login = library.func("int __cdecl gv_login(str username, str password, int remember_me)");
  const restoreSession = library.func("int __cdecl gv_restore_session()");
  const refreshBinaries = library.func("str __cdecl gv_refresh_binaries()");
  const selectBinary = library.func("int __cdecl gv_select_binary(int index)");
  const downloadAndInject = library.func("int __cdecl gv_download_and_inject()");
  const lastError = library.func("str __cdecl gv_last_error()");

  if (!initialize(path.join(app.getPath("appData"), "Ellohim Menu"))) {
    throw new Error(lastError() || "Native DLL initialization failed");
  }

  native = {
    listProcesses() {
      const payload = listProcessesJson();
      if (!payload) throw new Error(lastError() || "Could not enumerate processes");
      return JSON.parse(payload);
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
    refreshBinaries() {
      return new Promise((resolve, reject) => {
        refreshBinaries.async((error, payload) => {
          if (error) reject(error);
          else if (!payload) reject(new Error(lastError() || "Could not load binary catalog"));
          else resolve(JSON.parse(payload));
        });
      });
    },
    selectBinary(index) {
      if (!selectBinary(index)) throw new Error(lastError() || "Could not select binary");
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

  ipcMain.handle("native:login", async (_event, credentials) => {
    if (!credentials || typeof credentials.username !== "string" || typeof credentials.password !== "string") {
      throw new TypeError("Invalid credentials");
    }
    if (credentials.username.length < 1 || credentials.username.length > 128 || credentials.password.length < 1 || credentials.password.length > 256) {
      throw new TypeError("Invalid credential length");
    }
    await requireNative().login(credentials.username, credentials.password, credentials.rememberMe === true);
    return requireNative().refreshBinaries();
  });

  ipcMain.handle("native:restore-session", async () => {
    const restored = await requireNative().restoreSession();
    if (!restored) return null;
    return requireNative().refreshBinaries();
  });

  ipcMain.handle("native:refresh-binaries", () => requireNative().refreshBinaries());

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

function createWindow() {
  const window = new BrowserWindow({
    width: 1050,
    height: 720,
    minWidth: 820,
    minHeight: 560,
    backgroundColor: "#090d16",
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
  window.loadFile(path.join(__dirname, "renderer", "index.html"));
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
  app.quit();
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") app.quit();
});
