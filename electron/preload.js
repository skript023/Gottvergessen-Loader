const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("loader", Object.freeze({
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
  window: {
    minimize: () => ipcRenderer.invoke("window:minimize"),
    maximize: () => ipcRenderer.invoke("window:maximize"),
    close: () => ipcRenderer.invoke("window:close"),
    isMaximized: () => ipcRenderer.invoke("window:is-maximized")
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
