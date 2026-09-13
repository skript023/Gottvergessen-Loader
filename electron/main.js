const { app, BrowserWindow, dialog, ipcMain, shell } = require("electron");
const fs = require("node:fs");
const path = require("node:path");
const { spawn, exec } = require("node:child_process");
const koffi = require("koffi");
const gameScanner = require("./gameScanner");
const { updater, registerUpdaterIpc } = require("./updater");

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

  // ==================== WINDOW CONTROLS IPC ====================
  ipcMain.handle("window:minimize", (event) => {
    const win = BrowserWindow.fromWebContents(event.sender) || BrowserWindow.getFocusedWindow();
    if (win) win.minimize();
    return true;
  });

  ipcMain.handle("window:maximize", (event) => {
    const win = BrowserWindow.fromWebContents(event.sender) || BrowserWindow.getFocusedWindow();
    if (win) {
      if (win.isMaximized()) {
        win.unmaximize();
      } else {
        win.maximize();
      }
    }
    return true;
  });

  ipcMain.handle("window:close", (event) => {
    const win = BrowserWindow.fromWebContents(event.sender) || BrowserWindow.getFocusedWindow();
    if (win) win.close();
    return true;
  });

  ipcMain.handle("window:is-maximized", (event) => {
    const win = BrowserWindow.fromWebContents(event.sender) || BrowserWindow.getFocusedWindow();
    return win ? win.isMaximized() : false;
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

  // ==================== GAME SCANNER & LAUNCHER IPC ====================
  ipcMain.handle("game:list-installed", () => {
    return gameScanner.getAllGames();
  });

  ipcMain.handle("game:browse-executable", async () => {
    const focusedWin = BrowserWindow.getFocusedWindow();
    const result = await dialog.showOpenDialog(focusedWin || undefined, {
      title: "Select Game Executable",
      properties: ["openFile"],
      filters: [{ name: "Executable (*.exe)", extensions: ["exe"] }]
    });
    if (result.canceled || !result.filePaths || result.filePaths.length === 0) {
      return null;
    }
    const exePath = result.filePaths[0];
    const exeName = path.basename(exePath);
    const parsed = path.parse(exePath);
    return {
      exePath,
      exeName,
      name: parsed.name,
      installDir: parsed.dir
    };
  });

  ipcMain.handle("game:add-custom", (_event, { name, exePath }) => {
    if (!exePath || !fs.existsSync(exePath)) {
      throw new Error("Specified executable does not exist");
    }
    const custom = gameScanner.loadCustomGames();
    const exeName = path.basename(exePath);
    const id = `custom_${Date.now()}`;
    const newGame = {
      id,
      name: name || path.parse(exePath).name,
      platform: "custom",
      installDir: path.dirname(exePath),
      exeName,
      launchUri: exePath,
      iconUrl: "",
      bannerUrl: "",
      isCustom: true
    };
    custom.push(newGame);
    gameScanner.saveCustomGames(custom);
    return gameScanner.getAllGames();
  });

  ipcMain.handle("game:remove-custom", (_event, gameId) => {
    const custom = gameScanner.loadCustomGames().filter(g => g.id !== gameId);
    gameScanner.saveCustomGames(custom);
    return gameScanner.getAllGames();
  });

  ipcMain.handle("game:play-and-inject", async (_event, params) => {
    const { launchUri, exePath, targetProcess, binaryIndex, mode } = params || {};
    const addon = requireNative();

    // 1. Configure binary if provided
    const hasBinary = Number.isInteger(binaryIndex) && binaryIndex >= 0;
    if (hasBinary) {
      addon.selectBinary(binaryIndex);
      addon.setInjectionMode(Number.isInteger(mode) ? mode : 2);
    }

    // 2. Launch game
    if (launchUri && (launchUri.startsWith("steam://") || launchUri.startsWith("com.epicgames."))) {
      await shell.openExternal(launchUri);
    } else if (exePath && fs.existsSync(exePath)) {
      const child = spawn(exePath, [], {
        detached: true,
        stdio: "ignore",
        cwd: path.dirname(exePath)
      });
      child.unref();
    } else if (launchUri) {
      await shell.openExternal(launchUri);
    } else {
      throw new Error("No launch command or executable path provided.");
    }

    // 3. If no target process specified, simply launch
    if (!targetProcess) {
      return { success: true, message: "Game launched." };
    }

    const targetLower = targetProcess.trim().toLowerCase();

    // 4. Poll process list for target process (up to 45 seconds)
    const maxPolls = 90; // 90 * 500ms = 45s
    let foundProc = null;

    for (let i = 0; i < maxPolls; i++) {
      await new Promise(r => setTimeout(r, 500));
      try {
        const procs = addon.listProcesses();
        foundProc = procs.find(p => {
          const nameLower = p.name.toLowerCase();
          return nameLower === targetLower || nameLower.startsWith(targetLower.replace(".exe", ""));
        });
        if (foundProc) break;
      } catch (_) {}
    }

    if (!foundProc) {
      throw new Error(`Game launched, but process "${targetProcess}" was not detected within 45 seconds.`);
    }

    // 5. Target discovered! Lock target
    addon.setTarget(foundProc.name, foundProc.pid);

    // 6. Wait 2 seconds for memory initialization
    await new Promise(r => setTimeout(r, 2000));

    // 7. If binary was selected, execute auto-injection
    let injected = false;
    if (hasBinary) {
      injected = await addon.downloadAndInject();
      if (!injected) {
        throw new Error("Process detected, but auto-injection failed. Check engine logs.");
      }
    }

    return {
      success: true,
      pid: foundProc.pid,
      processName: foundProc.name,
      injected,
      message: injected ? `Game running and payload injected (PID: ${foundProc.pid})` : `Game running (PID: ${foundProc.pid})`
    };
  });

  ipcMain.handle("game:kill-process", async (_event, pid) => {
    if (!pid || pid <= 0) return false;
    return new Promise(resolve => {
      exec(`taskkill /F /PID ${pid}`, err => {
        resolve(!err);
      });
    });
  });

  registerUpdaterIpc(() => native);
}

function getRendererPath() {
  const vueDist = path.join(__dirname, "dist-renderer", "index.html");
  if (fs.existsSync(vueDist)) {
    return vueDist;
  }
  return path.join(__dirname, "renderer", "index.html");
}

function getWindowStateFile() {
  return path.join(app.getPath("userData"), "window-state.json");
}

function loadWindowState() {
  const defaultState = {
    width: 1120,
    height: 740,
    isMaximized: false
  };
  try {
    const filePath = getWindowStateFile();
    if (fs.existsSync(filePath)) {
      const data = JSON.parse(fs.readFileSync(filePath, "utf8"));
      if (typeof data.width === "number" && typeof data.height === "number") {
        return {
          width: Math.max(960, data.width),
          height: Math.max(620, data.height),
          x: typeof data.x === "number" ? data.x : undefined,
          y: typeof data.y === "number" ? data.y : undefined,
          isMaximized: Boolean(data.isMaximized)
        };
      }
    }
  } catch (_) {}
  return defaultState;
}

function saveWindowState(win) {
  if (!win || win.isDestroyed()) return;
  try {
    const isMaximized = win.isMaximized();
    let bounds;
    if (isMaximized) {
      bounds = typeof win.getNormalBounds === "function" ? win.getNormalBounds() : win.getBounds();
    } else {
      bounds = win.getBounds();
    }
    const state = {
      width: bounds.width,
      height: bounds.height,
      x: bounds.x,
      y: bounds.y,
      isMaximized
    };
    fs.writeFileSync(getWindowStateFile(), JSON.stringify(state, null, 2), "utf8");
  } catch (_) {}
}

function createWindow() {
  const state = loadWindowState();

  const options = {
    width: state.width,
    height: state.height,
    minWidth: 960,
    minHeight: 620,
    frame: false,
    backgroundColor: "#080c14",
    title: "Gottvergessen Loader - Control Center",
    icon: iconPath(),
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: false
    }
  };

  if (typeof state.x === "number" && typeof state.y === "number") {
    options.x = state.x;
    options.y = state.y;
  }

  const window = new BrowserWindow(options);
  window.setMenuBarVisibility(false);

  if (state.isMaximized) {
    window.maximize();
  }

  let saveTimer = null;
  const debouncedSave = () => {
    clearTimeout(saveTimer);
    saveTimer = setTimeout(() => saveWindowState(window), 300);
  };

  window.on("resize", debouncedSave);
  window.on("move", debouncedSave);
  window.on("close", () => {
    saveWindowState(window);
  });

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
