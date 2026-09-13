const fs = require("node:fs");
const path = require("node:path");
const http = require("node:http");
const https = require("node:https");
const crypto = require("node:crypto");
const { spawn } = require("node:child_process");
const { app, ipcMain, shell } = require("electron");

class ClientUpdater {
  constructor() {
    this.currentVersion = app.isPackaged ? app.getVersion() : "1.0.0";
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

  installUpdate(cleanupFn) {
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

    // In portable electron-builder apps, the actual executable path is in PORTABLE_EXECUTABLE_FILE
    const targetExe = process.env.PORTABLE_EXECUTABLE_FILE || process.execPath;
    const currentPid = process.pid;
    const updateDir = this.getUpdateDir();
    const scriptPath = path.join(updateDir, "apply-update.ps1");
    const logPath = path.join(updateDir, "update.log");

    // Robust, Discord-grade native process runner written to a real .ps1 file (immune to CLI quote & comment bugs)
    const psContent = `
$ErrorActionPreference = 'Continue'
$logPath = '${logPath.replace(/'/g, "''")}'
function Log-Msg($msg) {
    $ts = Get-Date -Format "yyyy-MM-dd HH:mm:ss.fff"
    "[$ts] $msg" | Out-File -FilePath $logPath -Append -Encoding utf8
}

Log-Msg "=== UPDATE RUNNER LAUNCHED ==="
Log-Msg "Target PID: ${currentPid}"
Log-Msg "New Exe: ${downloadedExe.replace(/["'$`]/g, "")}"
Log-Msg "Target Exe: ${targetExe.replace(/["'$`]/g, "")}"

$targetPid = ${currentPid}
$newExe = '${downloadedExe.replace(/'/g, "''")}'
$targetExe = '${targetExe.replace(/'/g, "''")}'

if ($targetPid -gt 0) {
    Log-Msg "Waiting natively for PID $targetPid to exit..."
    try {
        Wait-Process -Id $targetPid -Timeout 10 -ErrorAction Stop
        Log-Msg "PID $targetPid terminated cleanly."
    } catch {
        Log-Msg "Wait-Process error ($($_)). Force stopping PID $targetPid..."
        Stop-Process -Id $targetPid -Force -ErrorAction SilentlyContinue
    }
}

# Wait for Windows file system locks and mutexes to be released
Start-Sleep -Milliseconds 800

# In-place replace target executable
$replaced = $false
for ($i = 1; $i -le 25; $i++) {
    try {
        Move-Item -LiteralPath $newExe -Destination $targetExe -Force -ErrorAction Stop
        $replaced = $true
        Log-Msg "Move-Item succeeded on attempt $i."
        break
    } catch {
        Log-Msg "Move-Item attempt $i failed ($($_)). Retrying in 400ms..."
        Start-Sleep -Milliseconds 400
    }
}

# Relaunch updated application
$targetDir = Split-Path -Parent $targetExe
try {
    if ($replaced) {
        Log-Msg "Relaunching target application: $targetExe in $targetDir"
        Start-Process -FilePath $targetExe -WorkingDirectory $targetDir
    } else {
        $newDir = Split-Path -Parent $newExe
        Log-Msg "Could not overwrite target. Fallback relaunching new executable directly: $newExe in $newDir"
        Start-Process -FilePath $newExe -WorkingDirectory $newDir
    }
    Log-Msg "Start-Process executed successfully."
} catch {
    Log-Msg "Start-Process failed ($($_)). Attempting Windows Explorer shell relaunch..."
    if ($replaced) {
        explorer.exe "$targetExe"
    } else {
        explorer.exe "$newExe"
    }
}

Log-Msg "=== UPDATE RUNNER FINISHED ==="
`;

    try {
      fs.writeFileSync(scriptPath, psContent, "utf8");
    } catch (err) {
      return { success: false, error: `Failed to write update script: ${err.message}` };
    }

    // Spawn via Windows Shell 'cmd.exe /c start /min' to guarantee process detachment and survival across parent exit
    const child = spawn("cmd.exe", [
      "/c",
      "start",
      "/min",
      "powershell.exe",
      "-NoProfile",
      "-ExecutionPolicy", "Bypass",
      "-WindowStyle", "Hidden",
      "-File",
      scriptPath
    ], {
      detached: true,
      stdio: "ignore",
      windowsHide: true
    });
    child.unref();

    // Give OS 500ms to register child process before terminating parent
    setTimeout(() => {
      if (typeof cleanupFn === "function") {
        cleanupFn();
      } else {
        app.exit(0);
      }
    }, 500);

    return { success: true };
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

function registerUpdaterIpc(getNative, cleanupFn) {
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
    return updater.installUpdate(cleanupFn);
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
