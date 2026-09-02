const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("loader", Object.freeze({
  listProcesses: () => ipcRenderer.invoke("native:list-processes"),
  login: (credentials) => ipcRenderer.invoke("native:login", credentials),
  restoreSession: () => ipcRenderer.invoke("native:restore-session"),
  refreshBinaries: () => ipcRenderer.invoke("native:refresh-binaries"),
  inject: (request) => ipcRenderer.invoke("native:inject", request)
}));
