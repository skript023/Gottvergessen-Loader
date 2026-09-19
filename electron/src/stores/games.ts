import { defineStore } from 'pinia';
import { ref, computed, watch } from 'vue';
import type { InstalledGameItem, BinaryItem } from '../types/loader';
import { useDiagnosticsStore } from './diagnostics';
import { useBinariesStore } from './binaries';
import { useProcessStore } from './process';
import { useInjectionStore } from './injection';

export type GameLaunchState = 'idle' | 'launching' | 'waiting' | 'injecting' | 'running' | 'error';

export const useGamesStore = defineStore('games', () => {
  const diagnostics = useDiagnosticsStore();
  const binariesStore = useBinariesStore();
  const processStore = useProcessStore();
  const injectionStore = useInjectionStore();

  const games = ref<InstalledGameItem[]>([]);
  const selectedGameId = ref<string | null>(null);
  const searchQuery = ref('');
  const isScanning = ref(false);

  // Per-game launch state tracking
  const launchStatus = ref<GameLaunchState>('idle');
  const launchMessage = ref('');
  const runningPid = ref<number | null>(null);
  // The game that owns launchStatus/launchMessage/runningPid. Other games are idle.
  const activeGameId = ref<string | null>(null);
  const customTargetProcess = ref('');
  const selectedBinaryId = ref<string | null>(null);
  const selectedMode = ref<number>(0);
  const isSavingServerConfig = ref(false);

  // Favorites live per machine in localStorage, keyed by game id.
  const FAVORITES_KEY = 'astra.favoriteGames';
  const favoriteIds = ref<string[]>(loadFavoriteIds());

  function loadFavoriteIds(): string[] {
    try {
      const parsed = JSON.parse(localStorage.getItem(FAVORITES_KEY) || '[]');
      return Array.isArray(parsed) ? parsed.filter((id) => typeof id === 'string') : [];
    } catch (_) {
      return [];
    }
  }

  function isFavorite(gameId: string) {
    return favoriteIds.value.includes(gameId);
  }

  function toggleFavorite(gameId: string) {
    favoriteIds.value = isFavorite(gameId)
      ? favoriteIds.value.filter((id) => id !== gameId)
      : [...favoriteIds.value, gameId];
    try {
      localStorage.setItem(FAVORITES_KEY, JSON.stringify(favoriteIds.value));
    } catch (_) {}
  }

  const favoriteGames = computed(() => games.value.filter((g) => favoriteIds.value.includes(g.id)));

  // Recently opened games, newest first, for the header search pop-up.
  const RECENTS_KEY = 'astra.recentGames';
  const RECENTS_LIMIT = 4;
  const recentGameIds = ref<string[]>(loadRecentIds());

  function loadRecentIds(): string[] {
    try {
      const parsed = JSON.parse(localStorage.getItem(RECENTS_KEY) || '[]');
      return Array.isArray(parsed) ? parsed.filter((id) => typeof id === 'string') : [];
    } catch (_) {
      return [];
    }
  }

  function rememberRecentGame(gameId: string) {
    recentGameIds.value = [gameId, ...recentGameIds.value.filter((id) => id !== gameId)].slice(0, RECENTS_LIMIT);
    try {
      localStorage.setItem(RECENTS_KEY, JSON.stringify(recentGameIds.value));
    } catch (_) {}
  }

  // Always four rows: the newest picks first, topped up from the installed
  // list so the pop-up looks the same on a fresh install.
  const recentGames = computed(() => {
    const picked = recentGameIds.value
      .map((id) => games.value.find((g) => g.id === id))
      .filter((g): g is InstalledGameItem => !!g)
      .slice(0, RECENTS_LIMIT);

    for (const game of games.value) {
      if (picked.length >= RECENTS_LIMIT) break;
      if (!picked.some((g) => g.id === game.id)) picked.push(game);
    }
    return picked;
  });

  const selectedGame = computed<InstalledGameItem | null>(() => {
    if (!selectedGameId.value) return null;
    return games.value.find((g) => g.id === selectedGameId.value) || null;
  });

  const filteredGames = computed(() => {
    const q = searchQuery.value.trim().toLowerCase();
    if (!q) return games.value;
    return games.value.filter(
      (g) =>
        g.name.toLowerCase().includes(q) ||
        g.platform.toLowerCase().includes(q) ||
        (g.exeName && g.exeName.toLowerCase().includes(q))
    );
  });

  /**
   * Find if a game matches any authorized binary from Ellohim-Server.
   * Priority 1: Exact Game Name match (b.name === g.name || b.game === g.name)
   * Priority 2: Exact stem match (e.g. "valheim.exe" stem "valheim" === b.name / b.game)
   * Priority 3: Verified Target Process match (only if compatible with game name)
   */
  function getGameMatchedBinary(g: InstalledGameItem | null): BinaryItem | null {
    if (!g || !binariesStore.binaries || binariesStore.binaries.length === 0) return null;
    const cleanGName = (g.name || '').toLowerCase().replace(/[^a-z0-9]/g, '');
    const cleanExe = (g.exeName || '').toLowerCase().trim();
    const exeStem = cleanExe.replace(/\.exe$/i, '').replace(/[^a-z0-9]/g, '');

    if (!cleanGName && !exeStem) return null;

    const getBinaryTokens = (b: BinaryItem) => {
      const bName = (b.name || '').toLowerCase().replace(/[^a-z0-9]/g, '');
      const bGame = (b.game || '').toLowerCase().replace(/[^a-z0-9]/g, '');
      const rawTarget = b.target_process || (b as any).target || '';
      const bTarget = rawTarget.toLowerCase().trim();
      return { bName, bGame, bTarget };
    };

    // 1. Exact match by game title
    const exactMatch = binariesStore.binaries.find((b) => {
      const { bName, bGame } = getBinaryTokens(b);
      return (bName && bName === cleanGName) || (bGame && bGame === cleanGName);
    });
    if (exactMatch) return exactMatch;

    // 2. Exact match by game executable stem with binary name or game (e.g. valheim.exe -> valheim)
    if (exeStem && exeStem.length >= 3) {
      const stemMatch = binariesStore.binaries.find((b) => {
        const { bName, bGame } = getBinaryTokens(b);
        return (bName && bName === exeStem) || (bGame && bGame === exeStem);
      });
      if (stemMatch) return stemMatch;
    }

    // 3. Target process match (only if the binary does NOT manifestly belong to a different game)
    if (cleanExe) {
      const procMatch = binariesStore.binaries.find((b) => {
        const { bName, bGame, bTarget } = getBinaryTokens(b);
        if (!bTarget || bTarget !== cleanExe) return false;

        // Ensure binary does not have a clearly conflicting game name
        if (bName && bName.length >= 3) {
          const compatible = cleanGName.includes(bName) || bName.includes(cleanGName) ||
                             exeStem.includes(bName) || bName.includes(exeStem);
          if (!compatible) return false;
        }
        if (bGame && bGame.length >= 3) {
          const compatible = cleanGName.includes(bGame) || bGame.includes(cleanGName) ||
                             exeStem.includes(bGame) || bGame.includes(exeStem);
          if (!compatible) return false;
        }
        return true;
      });
      if (procMatch) return procMatch;
    }

    return null;
  }

  // Active matched binary for currently selected game
  const matchedBinary = computed<BinaryItem | null>(() => {
    return getGameMatchedBinary(selectedGame.value);
  });

  // Artwork comes from the Steam store in the background so the list shows up immediately.
  async function refreshGameArt() {
    if (!window.loader?.resolveGameArt) return;
    try {
      const withArt = await window.loader.resolveGameArt();
      const artById = new Map(withArt.map((g) => [g.id, g]));
      games.value = games.value.map((g) => {
        const art = artById.get(g.id);
        return art ? { ...g, iconUrl: art.iconUrl, bannerUrl: art.bannerUrl } : g;
      });
    } catch (err: any) {
      diagnostics.addLog(`Could not load game artwork: ${err.message}`, 'warn');
    }
  }

  async function scanGames() {
    isScanning.value = true;
    diagnostics.addLog('Scanning installed games across Steam, Epic, and PC library...', 'info');
    try {
      if (window.loader?.listInstalledGames) {
        games.value = await window.loader.listInstalledGames();
        diagnostics.addLog(
          `Game scan complete. Found ${games.value.length} installed games.`,
          'success'
        );
      }
      void refreshGameArt();
    } catch (err: any) {
      diagnostics.addLog(`Failed scanning installed games: ${err.message}`, 'error');
    } finally {
      isScanning.value = false;
    }
  }

  function selectGame(gameId: string | null) {
    selectedGameId.value = gameId;
    if (gameId) {
      rememberRecentGame(gameId);
      const g = games.value.find((item) => item.id === gameId);
      if (g) {
        const matched = getGameMatchedBinary(g);
        if (matched) {
          selectedBinaryId.value = matched.id;
          const targetProc = matched.target_process || (matched as any).target || g.exeName || '';
          customTargetProcess.value = targetProc;
          selectedMode.value = matched.injection_mode ?? 0;

          const bIdx = binariesStore.binaries.findIndex((b) => b.id === matched.id);
          if (bIdx >= 0) binariesStore.selectBinary(bIdx);

          diagnostics.addLog(
            `Matched Cloud Mod binary: ${matched.name} (${matched.file_name || 'payload.dll'}) -> ${g.name}`,
            'info'
          );
        } else {
          selectedBinaryId.value = null;
          customTargetProcess.value = g.exeName || '';
          diagnostics.addLog(`Selected game: ${g.name} (${g.platform.toUpperCase()}) [Vanilla / No Server Mod]`, 'info');
        }

        if (customTargetProcess.value) {
          processStore.setManualTarget(customTargetProcess.value, false);
        }
      }
    } else {
      selectedBinaryId.value = null;
      diagnostics.addLog('Returned to Control Center Overview', 'info');
    }
  }

  // Keep selected binary in sync when catalog loads or game changes
  watch(
    () => binariesStore.binaries,
    () => {
      if (selectedGame.value) {
        const matched = getGameMatchedBinary(selectedGame.value);
        if (matched) {
          selectedBinaryId.value = matched.id;
          const targetProc =
            matched.target_process ||
            (matched as any).target ||
            selectedGame.value.exeName ||
            '';
          customTargetProcess.value = targetProc;
          selectedMode.value = matched.injection_mode ?? 0;

          const bIdx = binariesStore.binaries.findIndex((b) => b.id === matched.id);
          if (bIdx >= 0) binariesStore.selectBinary(bIdx);

          if (targetProc) {
            processStore.setManualTarget(targetProc, false);
          }
        } else {
          selectedBinaryId.value = null;
        }
      }
    },
    { deep: true }
  );

  let livenessCheckTimer: any = null;

  function startProcessLivenessMonitor(pid: number, procName: string) {
    stopProcessLivenessMonitor();
    livenessCheckTimer = setInterval(async () => {
      if (launchStatus.value !== 'running' || !runningPid.value) {
        stopProcessLivenessMonitor();
        return;
      }
      try {
        if (window.loader?.isProcessRunning) {
          const isAlive = await window.loader.isProcessRunning(pid, procName);
          if (!isAlive) {
            handleProcessTerminated(`Game process "${procName || 'Game'}" (PID: ${pid}) closed.`);
          }
        }
      } catch (_) {}
    }, 1500);
  }

  function stopProcessLivenessMonitor() {
    if (livenessCheckTimer) {
      clearInterval(livenessCheckTimer);
      livenessCheckTimer = null;
    }
  }

  function hideProgressCard() {
    injectionStore.showProgress = false;
    injectionStore.progress = 0;
    injectionStore.stage = '';
    injectionStore.feedbackStatus = 'idle';
    injectionStore.feedbackMessage = '';
  }

  function handleProcessTerminated(reason?: string) {
    stopProcessLivenessMonitor();
    if (launchStatus.value === 'running') {
      const gName = games.value.find((g) => g.id === activeGameId.value)?.name || 'Game';
      launchStatus.value = 'idle';
      runningPid.value = null;
      activeGameId.value = null;
      launchMessage.value = '';
      hideProgressCard();
      diagnostics.addLog(reason || `${gName} process ended. Ready to play again.`, 'info');
    }
  }

  // Global listener for native process exit event
  if (typeof window !== 'undefined' && window.loader?.onGameProcessExited) {
    window.loader.onGameProcessExited((data) => {
      if (runningPid.value && data.pid === runningPid.value) {
        handleProcessTerminated(`Detected process "${data.processName || 'Game'}" (PID: ${data.pid}) exited.`);
      }
    });
  }

  async function saveGameMappingToServer(binaryId: string, processName: string, mode: number) {
    const binary = binariesStore.binaries.find((b) => b.id === binaryId);
    if (!binary) return false;

    isSavingServerConfig.value = true;
    try {
      binary.target_process = processName;
      binary.injection_mode = mode;
      await binariesStore.persistBinarySettings(binary);
      diagnostics.addLog(
        `[CLOUD SYNC] Successfully updated ${binary.name} mapping on server!`,
        'success'
      );
      return true;
    } catch (err: any) {
      diagnostics.addLog(`[SERVER SYNC ERROR] Could not save setting to Cloud: ${err.message}`, 'error');
      return false;
    } finally {
      isSavingServerConfig.value = false;
    }
  }

  async function launchAndInject() {
    const game = selectedGame.value;
    if (!game) return;
    if (activeGameId.value && activeGameId.value !== game.id && ['launching', 'waiting', 'injecting', 'running'].includes(launchStatus.value)) {
      const other = games.value.find((g) => g.id === activeGameId.value);
      diagnostics.addLog(`Stop ${other?.name || 'the running game'} before launching ${game.name}.`, 'warn');
      return;
    }

    // Resolve again at click time. Never reuse the global catalog selection:
    // the payload identity must be derived from the game being launched.
    const binary = getGameMatchedBinary(game);
    selectedBinaryId.value = binary?.id || null;
    if (binary) {
      const binaryIndex = binariesStore.binaries.findIndex((item) => item.id === binary.id);
      if (binaryIndex >= 0) binariesStore.selectBinary(binaryIndex);
    }
    const targetProc = customTargetProcess.value || game.exeName || (binary?.target_process || (binary as any)?.target) || '';

    activeGameId.value = game.id;
    launchStatus.value = 'launching';
    launchMessage.value = `Opening ${game.name} via ${game.platform.toUpperCase()}...`;
    if (binary) {
      injectionStore.isInjecting = true;
      injectionStore.showProgress = true;
      injectionStore.progress = 1;
      injectionStore.stage = 'Launching game and waiting for target process...';
      injectionStore.feedbackStatus = 'idle';
      injectionStore.feedbackMessage = 'Auto-injection operation in progress...';
    } else {
      injectionStore.isInjecting = false;
      injectionStore.showProgress = false;
      injectionStore.progress = 0;
      injectionStore.stage = 'Launch only';
      injectionStore.feedbackStatus = 'idle';
      injectionStore.feedbackMessage = '';
    }
    diagnostics.addLog(
      `Launching ${game.name} [Target: ${targetProc || 'None'}, DLL: ${binary ? binary.file_name : 'None'}, Binary ID: ${binary?.id || 'None'}]...`,
      'info'
    );

    const stateTimer = setTimeout(() => {
      if (launchStatus.value === 'launching') {
        launchStatus.value = 'waiting';
        launchMessage.value = `Waiting for process "${targetProc}" to start...`;
      }
    }, 1800);

    const operationTimer = binary
      ? setInterval(async () => {
          try {
            if (!window.loader?.operationStatus) return;
            const status = await window.loader.operationStatus();
            if (status.active) {
              // Native work starts only after main.js has found the target.
              // Replace the launch timer's stale Waiting message.
              clearTimeout(stateTimer);
              launchStatus.value = 'injecting';
              injectionStore.progress = Math.max(0, Math.min(100, Number(status.progress) || 0));
              injectionStore.stage = status.stage || 'Executing operation...';
              launchMessage.value = injectionStore.stage;
            }
          } catch (_) {}
        }, 120)
      : null;

    try {
      if (!window.loader?.playAndInject) {
        throw new Error('Game launcher API is not available');
      }

      const result = await window.loader.playAndInject({
        launchUri: game.launchUri,
        exePath: game.platform === 'custom' ? game.launchUri : undefined,
        targetProcess: targetProc,
        binaryId: binary?.id,
        mode: selectedMode.value
      });

      clearTimeout(stateTimer);

      if (result.success) {
        if (binary) {
          injectionStore.progress = 100;
          injectionStore.stage = result.injected ? 'Injection Complete' : 'Game Launched';
          injectionStore.feedbackStatus = 'success';
          injectionStore.feedbackMessage = result.message || 'Operation completed successfully.';
        }
        launchStatus.value = 'running';
        runningPid.value = result.pid || null;
        if (result.processName) {
          customTargetProcess.value = result.processName;
        }
        launchMessage.value = result.injected
          ? `Playing with Mod Payload Injected into ${result.processName || targetProc} (PID: ${result.pid})`
          : `Game running (${result.processName || targetProc}, PID: ${result.pid})`;
        diagnostics.addLog(
          `${game.name} launched successfully! ${result.injected ? `[Injected ${binary?.name || 'DLL'}]` : ''} Target: ${result.processName || targetProc}, PID: ${result.pid}`,
          'success'
        );
        if (result.pid) {
          processStore.selectProcess({
            name: result.processName || targetProc,
            pid: result.pid,
            arch: 'x64',
            accessible: true
          }, false);
          startProcessLivenessMonitor(result.pid, result.processName || targetProc);
        }
      } else {
        if (binary) {
          injectionStore.progress = 0;
          injectionStore.stage = 'Failed';
          injectionStore.feedbackStatus = 'error';
          injectionStore.feedbackMessage = result.message || 'Auto-injection failed.';
        }
        launchStatus.value = 'error';
        launchMessage.value = result.message || 'Failed to start or hook game.';
        diagnostics.addLog(`Launch failed: ${result.message}`, 'error');
      }
    } catch (err: any) {
      clearTimeout(stateTimer);
      if (binary) {
        injectionStore.progress = 0;
        injectionStore.stage = 'Error';
        injectionStore.feedbackStatus = 'error';
        injectionStore.feedbackMessage = `Error: ${err.message}`;
      }
      launchStatus.value = 'error';
      launchMessage.value = err.message || 'Launch error occurred.';
      diagnostics.addLog(`Launch error for ${game.name}: ${err.message}`, 'error');
    } finally {
      if (operationTimer) clearInterval(operationTimer);
      if (binary) injectionStore.isInjecting = false;
    }
  }

  async function stopGame() {
    stopProcessLivenessMonitor();
    if (!runningPid.value) {
      launchStatus.value = 'idle';
      activeGameId.value = null;
      hideProgressCard();
      return;
    }

    try {
      if (window.loader?.killGameProcess) {
        await window.loader.killGameProcess(runningPid.value);
        diagnostics.addLog(`Terminated game process (PID: ${runningPid.value})`, 'warn');
      }
    } catch (_) {}

    runningPid.value = null;
    launchStatus.value = 'idle';
    activeGameId.value = null;
    launchMessage.value = '';
    hideProgressCard();
  }

  async function browseAndAddCustom() {
    try {
      if (!window.loader?.browseGameExecutable || !window.loader?.addCustomGame) return;
      const fileInfo = await window.loader.browseGameExecutable();
      if (!fileInfo) return;

      const updated = await window.loader.addCustomGame({
        name: fileInfo.name,
        exePath: fileInfo.exePath
      });
      games.value = updated;
      void refreshGameArt();
      const added = games.value.find((g) => g.installDir === fileInfo.installDir);
      if (added) {
        selectGame(added.id);
      }
      diagnostics.addLog(`Added custom game: ${fileInfo.name} (${fileInfo.exePath})`, 'success');
    } catch (err: any) {
      diagnostics.addLog(`Failed adding custom game: ${err.message}`, 'error');
    }
  }

  async function removeCustomGame(gameId: string) {
    try {
      if (!window.loader?.removeCustomGame) return;
      games.value = await window.loader.removeCustomGame(gameId);
      if (selectedGameId.value === gameId) {
        selectedGameId.value = null;
      }
      diagnostics.addLog(`Removed custom game ${gameId}`, 'info');
    } catch (err: any) {
      diagnostics.addLog(`Failed removing game: ${err.message}`, 'error');
    }
  }

  return {
    games,
    selectedGameId,
    selectedGame,
    searchQuery,
    isScanning,
    launchStatus,
    launchMessage,
    runningPid,
    activeGameId,
    customTargetProcess,
    selectedBinaryId,
    selectedMode,
    isSavingServerConfig,
    filteredGames,
    favoriteGames,
    recentGames,
    isFavorite,
    toggleFavorite,
    matchedBinary,
    getGameMatchedBinary,
    scanGames,
    selectGame,
    saveGameMappingToServer,
    launchAndInject,
    stopGame,
    browseAndAddCustom,
    removeCustomGame
  };
});
