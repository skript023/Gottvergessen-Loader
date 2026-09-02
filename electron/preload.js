const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("loader", Object.freeze({
  listProcesses: () => ipcRenderer.invoke("native:list-processes"),
  operationStatus: () => ipcRenderer.invoke("native:operation-status"),
  login: (credentials) => ipcRenderer.invoke("native:login", credentials),
  restoreSession: () => ipcRenderer.invoke("native:restore-session"),
  refreshBinaries: () => ipcRenderer.invoke("native:refresh-binaries"),
  inject: (request) => ipcRenderer.invoke("native:inject", request)
}));
