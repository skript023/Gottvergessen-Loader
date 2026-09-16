const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("loader", Object.freeze({
  getServerStatus: () => ipcRenderer.invoke("server:status"),
  listProcesses: () => ipcRenderer.invoke("native:list-processes"),
  operationStatus: () => ipcRenderer.invoke("native:operation-status"),
  login: (credentials) => ipcRenderer.invoke("native:login", credentials),
  restoreSession: () => ipcRenderer.invoke("native:restore-session"),
  logout: () => ipcRenderer.invoke("native:logout"),
  refreshBinaries: () => ipcRenderer.invoke("native:refresh-binaries"),
  saveBinarySettings: (binaryId, targetProcess, mode) =>
    ipcRenderer.invoke("native:save-binary-settings", { binaryId, targetProcess, mode }),
  inject: (request) => ipcRenderer.invoke("native:inject", request),
  getSessionInfo: () => ipcRenderer.invoke("native:get-session-info"),
  listInstalledGames: () => ipcRenderer.invoke("game:list-installed"),
  browseGameExecutable: () => ipcRenderer.invoke("game:browse-executable"),
  addCustomGame: (data) => ipcRenderer.invoke("game:add-custom", data),
  removeCustomGame: (gameId) => ipcRenderer.invoke("game:remove-custom", gameId),
  playAndInject: (params) => ipcRenderer.invoke("game:play-and-inject", params),
  killGameProcess: (pid) => ipcRenderer.invoke("game:kill-process", pid),
  isProcessRunning: (pid, processName) => ipcRenderer.invoke("game:is-process-running", { pid, processName }),
  onGameProcessExited: (callback) => {
    const sub = (_event, data) => callback(data);
    ipcRenderer.on("game:process-exited", sub);
    return () => ipcRenderer.removeListener("game:process-exited", sub);
  },
  window: {
    minimize: () => ipcRenderer.invoke("window:minimize"),
    maximize: () => ipcRenderer.invoke("window:maximize"),
    close: () => ipcRenderer.invoke("window:close"),
    isMaximized: () => ipcRenderer.invoke("window:is-maximized")
  },
  updater: {
    checkUpdate: () => ipcRenderer.invoke("updater:check-update"),
    startDownload: () => ipcRenderer.invoke("updater:start-download"),
    pauseDownload: () => ipcRenderer.invoke("updater:pause-download"),
    cancelDownload: () => ipcRenderer.invoke("updater:cancel-download"),
    install: () => ipcRenderer.invoke("updater:install"),
    getState: () => ipcRenderer.invoke("updater:get-state"),
    syncModules: () => ipcRenderer.invoke("updater:sync-modules"),
    onProgress: (callback) => {
      const sub = (_event, data) => callback(data);
      ipcRenderer.on("updater:progress", sub);
      return () => ipcRenderer.removeListener("updater:progress", sub);
    },
    onStatus: (callback) => {
      const sub = (_event, data) => callback(data);
      ipcRenderer.on("updater:status", sub);
      return () => ipcRenderer.removeListener("updater:status", sub);
    },
    onModuleSync: (callback) => {
      const sub = (_event, data) => callback(data);
      ipcRenderer.on("updater:module-sync", sub);
      return () => ipcRenderer.removeListener("updater:module-sync", sub);
    }
  }
}));

contextBridge.exposeInMainWorld("ellohim", Object.freeze({
  window: {
    minimize: () => ipcRenderer.invoke("window:minimize"),
    maximize: () => ipcRenderer.invoke("window:maximize"),
    close: () => ipcRenderer.invoke("window:close"),
    isMaximized: () => ipcRenderer.invoke("window:is-maximized")
  }
}));
