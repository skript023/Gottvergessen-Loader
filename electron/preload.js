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
  getSessionInfo: () => ipcRenderer.invoke("native:get-session-info")
}));
