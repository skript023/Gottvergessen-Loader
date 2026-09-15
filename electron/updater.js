const fs = require("node:fs");
const path = require("node:path");
const http = require("node:http");
const https = require("node:https");
const crypto = require("node:crypto");
const { spawn } = require("node:child_process");
const { app, ipcMain, shell } = require("electron");

class ClientUpdater {
  constructor() {
    this.currentVersion = this.resolveCurrentVersion();
    this.activeDownload = null;
    this.updateState = {
      isChecking: false,
      hasUpdate: false,
      isMandatory: false,
      latestVersion: "",
      releaseNotes: "",
      latestRelease: null,
      modules: [],
      state: "idle", // 'idle' | 'downloading' | 'paused' | 'ready-to-install' | 'error'
      downloadedBytes: 0,
      totalBytes: 0,
      percent: 0,
      speed: 0,
      error: null
    };
  }

  getUpdateDir() {
    const dir = path.join(app.getPath("userData"), "updates");
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(dir, { recursive: true });
    }
    return dir;
  }

  getModulesDir() {
    const dir = path.join(app.getPath("appData"), "Ellohim Menu", "modules");
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(dir, { recursive: true });
    }
    return dir;
  }

  getBackendUrl(native) {
    if (process.env.VITE_BACKEND_URL) return process.env.VITE_BACKEND_URL;
    if (process.env.BACKEND_URL) return process.env.BACKEND_URL;
    if (native && typeof native.getBackendUrl === "function") {
      try {
        const url = native.getBackendUrl();
        if (url) return url;
      } catch (_) {}
    }
    return "https://apie.rena.my.id";
  }

  broadcast(mainWindow, event, data) {
    if (mainWindow && !mainWindow.isDestroyed()) {
      mainWindow.webContents.send(event, data);
    }
  }

  resolveCurrentVersion() {
    let pkgVer = "1.0.0";
    try {
      pkgVer = require("./package.json").version || "1.0.0";
    } catch (_) {}

    let baseVer = "";
    try {
      baseVer = app.getVersion();
    } catch (_) {}
    if (!baseVer || baseVer === "1.0.0") {
      baseVer = pkgVer;
    }

    try {
      const exeTarget = process.env.PORTABLE_EXECUTABLE_FILE || process.execPath || "";
      const exeName = path.basename(exeTarget);
      const match = exeName.match(/(\d+\.\d+\.\d+)/);
      if (match && this.isVersionNewer(baseVer, match[1])) {
        baseVer = match[1];
      }
    } catch (_) {}
    return baseVer || "1.0.0";
  }

  getHistoryFilePath() {
    return path.join(this.getUpdateDir(), "update-history.json");
  }

  recordInstalledVersion(version) {
    try {
      const historyFile = this.getHistoryFilePath();
      const data = {
        version,
        installedAt: Date.now()
      };
      fs.writeFileSync(historyFile, JSON.stringify(data, null, 2));
    } catch (_) {}
  }

  isLoopingOnVersion(serverVersion) {
    try {
      const historyFile = this.getHistoryFilePath();
      if (!fs.existsSync(historyFile)) return false;
      const data = JSON.parse(fs.readFileSync(historyFile, "utf8"));
      if (data && data.version === serverVersion) {
        const elapsed = Date.now() - (data.installedAt || 0);
        // If an update was installed within the last 5 minutes and the version is still reported as older
        if (elapsed < 300000) {
          return true;
        }
      }
    } catch (_) {}
    return false;
  }

  isVersionNewer(current, latest) {
    if (!latest || !current) return false;
    const cParts = String(current).replace(/^v/, "").split(".").map((n) => parseInt(n, 10) || 0);
    const lParts = String(latest).replace(/^v/, "").split(".").map((n) => parseInt(n, 10) || 0);
    for (let i = 0; i < Math.max(cParts.length, lParts.length); i++) {
      const c = cParts[i] || 0;
      const l = lParts[i] || 0;
      if (l > c) return true;
      if (l < c) return false;
    }
    return false;
  }

  async checkUpdate(backendUrl) {
    this.updateState.isChecking = true;
    this.updateState.error = null;
    this.currentVersion = this.resolveCurrentVersion();

    // 1. Guard: Check if running in development mode or explicitly bypassed
    const isDev = !app.isPackaged || process.argv.includes("--no-updater") || process.argv.includes("--skip-update") || process.env.SKIP_UPDATE === "1";
    if (isDev) {
      console.log(`[AutoUpdater] Dev/unpackaged environment or --skip-update flag detected (version ${this.currentVersion}). Auto-update bypassed.`);
      this.updateState.hasUpdate = false;
      this.updateState.isMandatory = false;
      this.updateState.isChecking = false;
      return { success: true, ...this.updateState, isDev: true, message: "Auto-updater bypassed in development" };
    }

    try {
      const url = new URL(`${backendUrl}/client/check-update`);
      url.searchParams.set("version", this.currentVersion);

      const client = url.protocol === "https:" ? https : http;

      const payload = await new Promise((resolve, reject) => {
        const req = client.get(url.toString(), { timeout: 10000 }, (res) => {
          if (res.statusCode < 200 || res.statusCode >= 300) {
            reject(new Error(`Server responded with status ${res.statusCode}`));
            res.resume();
            return;
          }
          let raw = "";
          res.on("data", (chunk) => (raw += chunk));
          res.on("end", () => {
            try {
              resolve(JSON.parse(raw));
            } catch (err) {
              reject(err);
            }
          });
        });

        req.on("error", reject);
        req.on("timeout", () => {
          req.destroy();
          reject(new Error("Update check request timed out"));
        });
      });

      const data = payload.data || {};
      const latestVer = data.latest_version || this.currentVersion;
      const isNewer = this.isVersionNewer(this.currentVersion, latestVer);

      // 2. Guard: Check if running executable SHA-256 matches latest release checksum
      if (isNewer && data.latest_release && data.latest_release.checksum) {
        try {
          const currentExe = process.env.PORTABLE_EXECUTABLE_FILE || process.execPath;
          if (currentExe && fs.existsSync(currentExe)) {
            const currentHash = await this.computeFileSha256(currentExe);
            if (currentHash.toLowerCase() === data.latest_release.checksum.toLowerCase()) {
              console.log(`[AutoUpdater] Current running executable checksum matches server latest release (${currentHash}). Client is already on this build.`);
              this.updateState.hasUpdate = false;
              this.updateState.isMandatory = false;
              this.updateState.latestVersion = latestVer;
              this.updateState.isChecking = false;
              return { success: true, ...this.updateState, message: "Client binary is identical to latest release" };
            }
          }
        } catch (hashErr) {
          console.warn("[AutoUpdater] Failed to verify current executable hash:", hashErr);
        }
      }

      // Guard against infinite relaunch loops when server hosts a binary with stale package.json
      if (isNewer && this.isLoopingOnVersion(latestVer)) {
        console.warn(`[AutoUpdater] Prevented relaunch loop! Version ${latestVer} was applied recently, but client still reports version ${this.currentVersion}.`);
        this.updateState.hasUpdate = false;
        this.updateState.isMandatory = false;
        this.updateState.latestVersion = latestVer;
        this.updateState.isChecking = false;
        this.updateState.error = `Loop prevented: Server version ${latestVer} was installed, but binary package.json version is still ${this.currentVersion}.`;
        return { success: true, ...this.updateState, loopPrevented: true };
      }

      // If client is already on the latest version, clear history marker
      if (!isNewer) {
        try {
          const historyFile = this.getHistoryFilePath();
          if (fs.existsSync(historyFile)) fs.unlinkSync(historyFile);
        } catch (_) {}
      }

      this.updateState.hasUpdate = Boolean(data.has_update) && isNewer;
      this.updateState.isMandatory = Boolean(data.is_mandatory) && isNewer;
      this.updateState.latestVersion = latestVer;
      this.updateState.latestRelease = data.latest_release || null;
      this.updateState.releaseNotes = data.latest_release?.release_notes || "";
      this.updateState.modules = data.modules || [];

      // Check if update is already downloaded and verified
      if (this.updateState.hasUpdate && this.updateState.latestRelease) {
        const targetExe = path.join(this.getUpdateDir(), `Gottvergessen-Loader-${this.updateState.latestVersion}.exe`);
        if (fs.existsSync(targetExe)) {
          const stats = fs.statSync(targetExe);
          if (stats.size === this.updateState.latestRelease.file_size) {
            const hash = await this.computeFileSha256(targetExe);
            if (hash.toLowerCase() === this.updateState.latestRelease.checksum.toLowerCase()) {
              this.updateState.state = "ready-to-install";
              this.updateState.percent = 100;
              this.updateState.downloadedBytes = stats.size;
              this.updateState.totalBytes = stats.size;
            }
          }
        }
      }

      this.updateState.isChecking = false;
      return { success: true, ...this.updateState };
    } catch (err) {
      this.updateState.isChecking = false;
      this.updateState.error = err.message;
      return { success: false, error: err.message, ...this.updateState };
    }
  }

  computeFileSha256(filePath) {
    return new Promise((resolve, reject) => {
      const hash = crypto.createHash("sha256");
      const stream = fs.createReadStream(filePath);
      stream.on("data", (chunk) => hash.update(chunk));
      stream.on("end", () => resolve(hash.digest("hex")));
      stream.on("error", reject);
    });
  }

  startDownload(mainWindow, backendUrl) {
    if (this.activeDownload) {
      return { success: true, message: "Download already active" };
    }

    if (!this.updateState.latestRelease) {
      return { success: false, error: "No active release available to download" };
    }

    const release = this.updateState.latestRelease;
    const updateDir = this.getUpdateDir();
    const finalExePath = path.join(updateDir, `Gottvergessen-Loader-${release.version}.exe`);
    const partPath = path.join(updateDir, `Gottvergessen-Loader-${release.version}.exe.part`);
    const metaPath = path.join(updateDir, `Gottvergessen-Loader-${release.version}.json`);

    // Determine starting byte offset for resumption
    let startByte = 0;
    if (fs.existsSync(partPath)) {
      try {
        startByte = fs.statSync(partPath).size;
      } catch (_) {
        startByte = 0;
      }
    }

    const expectedTotal = release.file_size;
    if (startByte >= expectedTotal && expectedTotal > 0) {
      // File already fully downloaded in part file, verify
      this.verifyAndCompleteDownload(mainWindow, partPath, finalExePath, metaPath, release);
      return { success: true, message: "Completing existing download" };
    }

    const downloadUrl = release.download_url.startsWith("http")
      ? release.download_url
      : `${backendUrl}${release.download_url}`;

    const headers = {
      "User-Agent": "Gottvergessen-Loader-AutoUpdater/1.0"
    };

    if (startByte > 0) {
      headers["Range"] = `bytes=${startByte}-`;
    }

    this.updateState.state = "downloading";
    this.updateState.downloadedBytes = startByte;
    this.updateState.totalBytes = expectedTotal;
    this.updateState.percent = expectedTotal > 0 ? Math.round((startByte / expectedTotal) * 100) : 0;
    this.updateState.error = null;

    let writeStream;
    try {
      writeStream = fs.createWriteStream(partPath, { flags: startByte > 0 ? "a" : "w" });
    } catch (err) {
      this.updateState.state = "error";
      this.updateState.error = `Cannot create file: ${err.message}`;
      this.broadcast(mainWindow, "updater:status", this.updateState);
      return { success: false, error: err.message };
    }

    const urlObj = new URL(downloadUrl);
    const client = urlObj.protocol === "https:" ? https : http;

    let bytesReceivedSession = 0;
    let lastTime = Date.now();
    let speed = 0;

    const req = client.get(urlObj.toString(), { headers }, (res) => {
      // Handle redirects
      if ([301, 302, 307, 308].includes(res.statusCode) && res.headers.location) {
        writeStream.close();
        this.activeDownload = null;
        release.download_url = res.headers.location;
        return this.startDownload(mainWindow, backendUrl);
      }

      const isPartial = res.statusCode === 206;
      const isFull = res.statusCode === 200;

      if (!isPartial && !isFull) {
        writeStream.close();
        this.activeDownload = null;
        this.updateState.state = "error";
        this.updateState.error = `Server returned HTTP ${res.statusCode}`;
        this.broadcast(mainWindow, "updater:status", this.updateState);
        return;
      }

      // If server doesn't support Range and responded with 200, restart from 0
      if (isFull && startByte > 0) {
        writeStream.close();
        try {
          fs.unlinkSync(partPath);
        } catch (_) {}
        writeStream = fs.createWriteStream(partPath, { flags: "w" });
        this.updateState.downloadedBytes = 0;
        startByte = 0;
      }

      // Read total bytes from Content-Range or Content-Length
      const contentRange = res.headers["content-range"];
      if (contentRange) {
        const match = contentRange.match(/\/(\d+)/);
        if (match) {
          this.updateState.totalBytes = parseInt(match[1], 10);
        }
      } else if (res.headers["content-length"] && !isPartial) {
        this.updateState.totalBytes = parseInt(res.headers["content-length"], 10);
      }

      // Save state meta
      fs.writeFileSync(
        metaPath,
        JSON.stringify({
          version: release.version,
          totalBytes: this.updateState.totalBytes,
          checksum: release.checksum,
          updatedAt: new Date().toISOString()
        })
      );

      res.on("data", (chunk) => {
        writeStream.write(chunk);
        this.updateState.downloadedBytes += chunk.length;
        bytesReceivedSession += chunk.length;

        const now = Date.now();
        const elapsed = (now - lastTime) / 1000;
        if (elapsed >= 0.5) {
          speed = Math.round(bytesReceivedSession / elapsed);
          bytesReceivedSession = 0;
          lastTime = now;
        }

        const total = this.updateState.totalBytes || expectedTotal;
        this.updateState.percent = total > 0 ? Math.min(100, Math.round((this.updateState.downloadedBytes / total) * 100)) : 0;
        this.updateState.speed = speed;

        this.broadcast(mainWindow, "updater:progress", {
          ...this.updateState,
          downloadedBytes: this.updateState.downloadedBytes,
          totalBytes: total,
          percent: this.updateState.percent,
          speed
        });
      });

      res.on("end", () => {
        writeStream.end(async () => {
          this.activeDownload = null;
          await this.verifyAndCompleteDownload(mainWindow, partPath, finalExePath, metaPath, release);
        });
      });

      res.on("error", (err) => {
        writeStream.close();
        this.activeDownload = null;
        this.updateState.state = "error";
        this.updateState.error = err.message;
        this.broadcast(mainWindow, "updater:status", this.updateState);
      });
    });

    req.on("error", (err) => {
      writeStream.close();
      this.activeDownload = null;
      this.updateState.state = "error";
      this.updateState.error = err.message;
      this.broadcast(mainWindow, "updater:status", this.updateState);
    });

    this.activeDownload = { req, writeStream, partPath, metaPath };
    this.broadcast(mainWindow, "updater:status", this.updateState);
    return { success: true };
  }

  async verifyAndCompleteDownload(mainWindow, partPath, finalExePath, metaPath, release) {
    this.updateState.state = "verifying";
    this.broadcast(mainWindow, "updater:status", this.updateState);

    try {
      const calculatedHash = await this.computeFileSha256(partPath);
      if (release.checksum && calculatedHash.toLowerCase() !== release.checksum.toLowerCase()) {
        throw new Error(`SHA-256 validation failed! Expected ${release.checksum}, calculated ${calculatedHash}`);
      }

      // Rename .part to .exe
      if (fs.existsSync(finalExePath)) {
        try {
          fs.unlinkSync(finalExePath);
        } catch (_) {}
      }
      fs.renameSync(partPath, finalExePath);

      // Clean up metadata
      if (fs.existsSync(metaPath)) {
        try {
          fs.unlinkSync(metaPath);
        } catch (_) {}
      }

      this.updateState.state = "ready-to-install";
      this.updateState.percent = 100;
      this.updateState.speed = 0;
      this.broadcast(mainWindow, "updater:status", {
        ...this.updateState,
        exePath: finalExePath
      });
    } catch (err) {
      this.updateState.state = "error";
      this.updateState.error = err.message;
      this.broadcast(mainWindow, "updater:status", this.updateState);
    }
  }

  pauseDownload(mainWindow) {
    if (!this.activeDownload) return { success: false, message: "No active download" };

    try {
      this.activeDownload.req.destroy();
      this.activeDownload.writeStream.close();
    } catch (_) {}

    this.activeDownload = null;
    this.updateState.state = "paused";
    this.updateState.speed = 0;
    this.broadcast(mainWindow, "updater:status", this.updateState);
    return { success: true, message: "Download paused" };
  }

  cancelDownload(mainWindow) {
    this.pauseDownload(mainWindow);

    if (this.updateState.latestRelease) {
      const partPath = path.join(this.getUpdateDir(), `Gottvergessen-Loader-${this.updateState.latestRelease.version}.exe.part`);
      const metaPath = path.join(this.getUpdateDir(), `Gottvergessen-Loader-${this.updateState.latestRelease.version}.json`);
      if (fs.existsSync(partPath)) try { fs.unlinkSync(partPath); } catch (_) {}
      if (fs.existsSync(metaPath)) try { fs.unlinkSync(metaPath); } catch (_) {}
    }

    this.updateState.state = "idle";
    this.updateState.downloadedBytes = 0;
    this.updateState.percent = 0;
    this.updateState.speed = 0;
    this.broadcast(mainWindow, "updater:status", this.updateState);
    return { success: true };
  }

  installUpdate(cleanupFn, getNative, getRunnerPath) {
    if (!this.updateState.latestRelease) {
      return { success: false, error: "No update release found" };
    }

    const downloadedExe = path.join(
      this.getUpdateDir(),
      `Gottvergessen-Loader-${this.updateState.latestRelease.version}.exe`
    );

    if (!fs.existsSync(downloadedExe)) {
      return { success: false, error: "Downloaded executable file not found" };
    }

    // Record installation attempt to prevent infinite relaunch loops if version doesn't bump
    this.recordInstalledVersion(this.updateState.latestRelease.version);

    // In development mode, launching the downloaded portable build directly avoids overwriting electron.exe
    if (!app.isPackaged) {
      shell.openPath(downloadedExe);
      setTimeout(() => {
        if (typeof cleanupFn === "function") {
          cleanupFn();
        } else {
          app.exit(0);
        }
      }, 400);
      return { success: true };
    }

    const targetExe = process.env.PORTABLE_EXECUTABLE_FILE || process.execPath;
    const currentPid = process.pid;
    const parentPid = process.ppid || 0;
    const runnerPath = typeof getRunnerPath === "function" ? getRunnerPath() : null;
    const native = typeof getNative === "function" ? getNative() : null;

    // 1. Primary: Native C++ update runner with Win32 Job Breakaway
    if (runnerPath && fs.existsSync(runnerPath) && native && typeof native.applyUpdate === "function") {
      const launched = native.applyUpdate(runnerPath, downloadedExe, targetExe, currentPid, parentPid);
      if (launched) {
        setTimeout(() => {
          if (typeof cleanupFn === "function") cleanupFn();
          else app.exit(0);
        }, 350);
        return { success: true, method: "native-c++" };
      }
    }

    // 2. Direct spawn of native C++ runner executable if FFI call failed
    if (runnerPath && fs.existsSync(runnerPath)) {
      const child = spawn(runnerPath, [String(currentPid), downloadedExe, targetExe, String(parentPid)], {
        detached: true,
        stdio: "ignore",
        windowsHide: true
      });
      child.unref();
      setTimeout(() => {
        if (typeof cleanupFn === "function") cleanupFn();
        else app.exit(0);
      }, 350);
      return { success: true, method: "native-exe" };
    }

    // 3. Fallback: ShellExecuteEx via shell.openPath if runner not found
    shell.openPath(downloadedExe);
    setTimeout(() => {
      if (typeof cleanupFn === "function") cleanupFn();
      else app.exit(0);
    }, 400);
    return { success: true, method: "shell-fallback" };
  }

  async syncModules(backendUrl, mainWindow) {
    if (!this.updateState.modules || this.updateState.modules.length === 0) {
      return { success: true, message: "No modules to sync" };
    }

    const baseModulesDir = this.getModulesDir();
    const results = [];

    for (const mod of this.updateState.modules) {
      if (!mod.is_required || mod.status !== "active") continue;

      const modTargetDir = mod.target_path ? path.join(baseModulesDir, mod.target_path) : baseModulesDir;
      if (!fs.existsSync(modTargetDir)) fs.mkdirSync(modTargetDir, { recursive: true });

      const modFilePath = path.join(modTargetDir, mod.file_name);
      let needsDownload = true;

      if (fs.existsSync(modFilePath)) {
        try {
          const hash = await this.computeFileSha256(modFilePath);
          if (hash.toLowerCase() === mod.checksum.toLowerCase()) {
            needsDownload = false;
          }
        } catch (_) {}
      }

      if (needsDownload) {
        this.broadcast(mainWindow, "updater:module-sync", {
          name: mod.name,
          version: mod.version,
          status: "downloading"
        });

        const modUrl = mod.download_url.startsWith("http")
          ? mod.download_url
          : `${backendUrl}${mod.download_url}`;

        try {
          await this.downloadSingleFile(modUrl, modFilePath);
          results.push({ name: mod.name, success: true });
        } catch (err) {
          results.push({ name: mod.name, success: false, error: err.message });
        }
      } else {
        results.push({ name: mod.name, success: true, cached: true });
      }
    }

    return { success: true, results };
  }

  downloadSingleFile(urlStr, destPath) {
    return new Promise((resolve, reject) => {
      const url = new URL(urlStr);
      const client = url.protocol === "https:" ? https : http;
      const file = fs.createWriteStream(destPath);

      client.get(url.toString(), (res) => {
        if ([301, 302, 307, 308].includes(res.statusCode) && res.headers.location) {
          file.close();
          return this.downloadSingleFile(res.headers.location, destPath).then(resolve).catch(reject);
        }

        if (res.statusCode < 200 || res.statusCode >= 300) {
          file.close();
          return reject(new Error(`Module download HTTP ${res.statusCode}`));
        }

        res.pipe(file);
        file.on("finish", () => {
          file.close(resolve);
        });
      }).on("error", (err) => {
        file.close();
        try { fs.unlinkSync(destPath); } catch (_) {}
        reject(err);
      });
    });
  }
}

const updater = new ClientUpdater();

function registerUpdaterIpc(getNative, cleanupFn, getRunnerPath) {
  ipcMain.handle("updater:check-update", async (event) => {
    const window = event.sender.getOwnerBrowserWindow();
    const backendUrl = updater.getBackendUrl(getNative ? getNative() : null);
    return await updater.checkUpdate(backendUrl);
  });

  ipcMain.handle("updater:start-download", async (event) => {
    const window = event.sender.getOwnerBrowserWindow();
    const backendUrl = updater.getBackendUrl(getNative ? getNative() : null);
    return updater.startDownload(window, backendUrl);
  });

  ipcMain.handle("updater:pause-download", async (event) => {
    const window = event.sender.getOwnerBrowserWindow();
    return updater.pauseDownload(window);
  });

  ipcMain.handle("updater:cancel-download", async (event) => {
    const window = event.sender.getOwnerBrowserWindow();
    return updater.cancelDownload(window);
  });

  ipcMain.handle("updater:install", async () => {
    return updater.installUpdate(cleanupFn, getNative, getRunnerPath);
  });

  ipcMain.handle("updater:get-state", async () => {
    return updater.updateState;
  });

  ipcMain.handle("updater:sync-modules", async (event) => {
    const window = event.sender.getOwnerBrowserWindow();
    const backendUrl = updater.getBackendUrl(getNative ? getNative() : null);
    return await updater.syncModules(backendUrl, window);
  });
}

module.exports = {
  updater,
  registerUpdaterIpc
};
