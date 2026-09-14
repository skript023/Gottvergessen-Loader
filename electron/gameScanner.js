const fs = require("node:fs");
const path = require("node:path");
const { execSync } = require("node:child_process");
const { app } = require("electron");

/**
 * Basic VDF / ACF key-value parser for Valve Steam format.
 */
function parseVdf(text) {
  const result = {};
  const stack = [result];
  const regex = /"([^"]*)"(?:\s+"([^"]*)")?/g;
  let match;

  const lines = text.split(/\r?\n/);
  for (let line of lines) {
    line = line.trim();
    if (!line || line.startsWith("//")) continue;

    if (line === "{") {
      continue;
    }
    if (line === "}") {
      if (stack.length > 1) stack.pop();
      continue;
    }

    const tokens = [];
    let m;
    regex.lastIndex = 0;
    while ((m = regex.exec(line)) !== null) {
      tokens.push(m[1] !== undefined ? m[1] : m[2]);
      if (m[2] !== undefined) tokens.push(m[2]);
    }

    if (tokens.length >= 2) {
      const current = stack[stack.length - 1];
      current[tokens[0]] = tokens[1];
    } else if (tokens.length === 1) {
      const current = stack[stack.length - 1];
      const newObj = {};
      current[tokens[0]] = newObj;
      stack.push(newObj);
    }
  }

  return result;
}

/**
 * Find executable file within a directory.
 */
function findMainExecutable(dirPath, defaultName) {
  try {
    if (!fs.existsSync(dirPath)) return defaultName ? `${defaultName}.exe` : "";
    const files = fs.readdirSync(dirPath);
    
    // Filter out installer, setup, crash reporting, redistributable files
    const exes = files.filter(f => {
      const lower = f.toLowerCase();
      return (
        lower.endsWith(".exe") &&
        !lower.includes("crash") &&
        !lower.includes("unins") &&
        !lower.includes("setup") &&
        !lower.includes("update") &&
        !lower.includes("redist") &&
        !lower.includes("dxsetup") &&
        !lower.includes("vcredist") &&
        !lower.includes("installer") &&
        !lower.includes("service") &&
        !lower.includes("eac") &&
        !lower.includes("easyanti") &&
        !lower.includes("epicgames")
      );
    });

    // 1. Look inside Win64 / Binaries subdirectories commonly used in Unreal/Unity games FIRST
    // Real game shipping executables live in Binaries/Win64 (e.g. ScarletNexus-Win64-Shipping.exe)
    // while root directory executables are usually just launcher/bootstrap stubs.
    const subdirs = ["Binaries/Win64", "Game/Binaries/Win64", "bin/x64", "bin64", "bin"];
    for (const sub of subdirs) {
      const fullSub = path.join(dirPath, sub);
      if (fs.existsSync(fullSub)) {
        try {
          const subFiles = fs.readdirSync(fullSub);
          const subExes = subFiles.filter(f => {
            const l = f.toLowerCase();
            return (
              l.endsWith(".exe") &&
              !l.includes("crash") &&
              !l.includes("report") &&
              !l.includes("unins")
            );
          });
          if (subExes.length > 0) {
            // Prioritize Shipping / Win64 executables over bootstrap stubs
            const shippingExe = subExes.find(e => /win64-shipping|shipping/i.test(e));
            if (shippingExe) return shippingExe;
            if (defaultName) {
              const sanitizedName = defaultName.replace(/[^a-zA-Z0-9]/g, "").toLowerCase();
              const match = subExes.find(e => e.replace(/[^a-zA-Z0-9]/g, "").toLowerCase().includes(sanitizedName));
              if (match) return match;
            }
            return subExes[0];
          }
        } catch (_) {}
      }
    }

    // 2. Fall back to root directory executables
    if (defaultName) {
      const sanitizedName = defaultName.replace(/[^a-zA-Z0-9]/g, "").toLowerCase();
      // Match exact or contains name
      const exactMatch = exes.find(e => {
        const cleanE = e.replace(/[^a-zA-Z0-9]/g, "").toLowerCase();
        return cleanE.startsWith(sanitizedName) || cleanE.includes(sanitizedName);
      });
      if (exactMatch) return exactMatch;
    }

    if (exes.length > 0) {
      return exes[0];
    }
  } catch (_) {}
  return defaultName ? `${defaultName}.exe` : "";
}

/**
 * Get installed Steam games from registry and libraryfolders.vdf.
 */
function getSteamInstalledGames() {
  const games = [];
  try {
    let steamPath = "";
    try {
      const output = execSync('reg query HKCU\\Software\\Valve\\Steam /v SteamPath', {
        encoding: "utf-8",
        stdio: ["ignore", "pipe", "ignore"]
      });
      const match = output.match(/SteamPath\s+REG_SZ\s+(.+)/i);
      if (match) {
        steamPath = match[1].trim().replace(/\//g, "\\");
      }
    } catch (_) {}

    if (!steamPath) {
      const defaultPaths = [
        "C:\\Program Files (x86)\\Steam",
        "C:\\Program Files\\Steam",
        "D:\\Steam",
        "E:\\Steam",
        "I:\\Steam"
      ];
      for (const p of defaultPaths) {
        if (fs.existsSync(p)) {
          steamPath = p;
          break;
        }
      }
    }

    if (!steamPath || !fs.existsSync(steamPath)) return games;

    const libraryVdfPath = path.join(steamPath, "steamapps", "libraryfolders.vdf");
    const libraryPaths = [path.join(steamPath, "steamapps")];

    if (fs.existsSync(libraryVdfPath)) {
      const vdfContent = fs.readFileSync(libraryVdfPath, "utf-8");
      const pathMatches = [...vdfContent.matchAll(/"path"\s+"([^"]+)"/gi)];
      for (const m of pathMatches) {
        const rawPath = m[1].replace(/\\\\/g, "\\");
        const candidate = path.join(rawPath, "steamapps");
        if (!libraryPaths.includes(candidate) && fs.existsSync(candidate)) {
          libraryPaths.push(candidate);
        }
      }
    }

    // Read appmanifest files in each library path
    for (const lib of libraryPaths) {
      if (!fs.existsSync(lib)) continue;
      let files = [];
      try {
        files = fs.readdirSync(lib);
      } catch (_) {
        continue;
      }

      for (const file of files) {
        if (file.toLowerCase().startsWith("appmanifest_") && file.toLowerCase().endsWith(".acf")) {
          try {
            const filePath = path.join(lib, file);
            const content = fs.readFileSync(filePath, "utf-8");
            const appIdMatch = content.match(/"appid"\s+"(\d+)"/i);
            const nameMatch = content.match(/"name"\s+"([^"]+)"/i);
            const installDirMatch = content.match(/"installdir"\s+"([^"]+)"/i);

            if (appIdMatch && nameMatch) {
              const appId = appIdMatch[1];
              const name = nameMatch[1];
              const installDirName = installDirMatch ? installDirMatch[1] : "";

              // Skip Steamworks Common Shared or Proton
              if (
                appId === "228980" ||
                name.toLowerCase().includes("steamworks") ||
                name.toLowerCase().includes("proton")
              ) {
                continue;
              }

              const fullInstallDir = path.join(lib, "common", installDirName);
              const exeName = findMainExecutable(fullInstallDir, installDirName);

              games.push({
                id: `steam_${appId}`,
                appId,
                name,
                platform: "steam",
                installDir: fullInstallDir,
                exeName,
                launchUri: `steam://rungameid/${appId}`,
                iconUrl: `https://cdn.cloudflare.steamstatic.com/steam/apps/${appId}/capsule_sm_120.jpg`,
                bannerUrl: `https://cdn.cloudflare.steamstatic.com/steam/apps/${appId}/header.jpg`,
                isCustom: false
              });
            }
          } catch (_) {}
        }
      }
    }
  } catch (err) {
    console.error("[GameScanner] Error scanning Steam:", err);
  }
  return games;
}

/**
 * Get installed Epic Games.
 */
function getEpicInstalledGames() {
  const games = [];
  try {
    const manifestsDir = "C:\\ProgramData\\Epic\\EpicGamesLauncher\\Data\\Manifests";
    if (fs.existsSync(manifestsDir)) {
      const files = fs.readdirSync(manifestsDir);
      for (const f of files) {
        if (f.toLowerCase().endsWith(".item")) {
          try {
            const raw = fs.readFileSync(path.join(manifestsDir, f), "utf-8");
            const item = JSON.parse(raw);
            if (item.DisplayName && item.InstallLocation) {
              const appName = item.AppName || item.CatalogItemId || "";
              games.push({
                id: `epic_${appName || item.DisplayName.replace(/\s+/g, "_")}`,
                appId: appName,
                name: item.DisplayName,
                platform: "epic",
                installDir: item.InstallLocation,
                exeName: item.LaunchExecutable || "",
                launchUri: appName
                  ? `com.epicgames.launcher://apps/${appName}?action=launch&silent=true`
                  : path.join(item.InstallLocation, item.LaunchExecutable || ""),
                iconUrl: "",
                bannerUrl: "",
                isCustom: false
              });
            }
          } catch (_) {}
        }
      }
    }
  } catch (err) {
    console.error("[GameScanner] Error scanning Epic:", err);
  }
  return games;
}

/**
 * Custom user-added games storage in user data.
 */
function getCustomGamesPath() {
  const base = path.join(app.getPath("appData"), "Ellohim Menu");
  if (!fs.existsSync(base)) {
    try {
      fs.mkdirSync(base, { recursive: true });
    } catch (_) {}
  }
  return path.join(base, "custom_games.json");
}

function loadCustomGames() {
  try {
    const p = getCustomGamesPath();
    if (fs.existsSync(p)) {
      const raw = fs.readFileSync(p, "utf-8");
      return JSON.parse(raw);
    }
  } catch (_) {}
  return [];
}

function saveCustomGames(games) {
  try {
    const p = getCustomGamesPath();
    fs.writeFileSync(p, JSON.stringify(games, null, 2), "utf-8");
    return true;
  } catch (err) {
    console.error("[GameScanner] Failed saving custom games:", err);
    return false;
  }
}

/**
 * Scan all available games.
 */
function getAllGames() {
  const steamGames = getSteamInstalledGames();
  const epicGames = getEpicInstalledGames();
  const customGames = loadCustomGames();

  // Deduplicate by name / id
  const map = new Map();
  for (const g of steamGames) map.set(g.id, g);
  for (const g of epicGames) map.set(g.id, g);
  for (const g of customGames) map.set(g.id, g);

  return Array.from(map.values()).sort((a, b) => a.name.localeCompare(b.name));
}

module.exports = {
  getAllGames,
  getSteamInstalledGames,
  getEpicInstalledGames,
  loadCustomGames,
  saveCustomGames,
  findMainExecutable
};
