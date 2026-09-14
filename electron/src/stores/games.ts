import { defineStore } from 'pinia';
import { ref, computed, watch } from 'vue';
import type { InstalledGameItem, BinaryItem } from '../types/loader';
import { useDiagnosticsStore } from './diagnostics';
import { useBinariesStore } from './binaries';
import { useProcessStore } from './process';

export type GameLaunchState = 'idle' | 'launching' | 'waiting' | 'injecting' | 'running' | 'error';

export const useGamesStore = defineStore('games', () => {
  const diagnostics = useDiagnosticsStore();
  const binariesStore = useBinariesStore();
  const processStore = useProcessStore();

  const games = ref<InstalledGameItem[]>([]);
  const selectedGameId = ref<string | null>(null);
  const searchQuery = ref('');
  const isScanning = ref(false);

  // Per-game launch state tracking
  const launchStatus = ref<GameLaunchState>('idle');
  const launchMessage = ref('');
  const runningPid = ref<number | null>(null);
  const customTargetProcess = ref('');
  const selectedBinaryId = ref<string | null>(null);
  const selectedMode = ref<number>(2);
  const isSavingServerConfig = ref(false);

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
   * Priority 3: Title boundary / prefix match (e.g. "Valheim Mod" for "Valheim")
   * Priority 4: Verified Target Process match (only if compatible with game name)
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
      const bTarget = (b.target_process || '').toLowerCase().trim();
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

    // 3. Prefix/Title boundary match (e.g. "Valheim Mod" or "Valheim Internal" for game "Valheim")
    const boundaryMatch = binariesStore.binaries.find((b) => {
      const { bName, bGame } = getBinaryTokens(b);
      const matchName = (token: string, target: string) => {
        if (!token || !target || token.length < 4 || target.length < 4) return false;
        return (token.startsWith(target) && target.length >= 4) || (target.startsWith(token) && token.length >= 4);
      };
      return matchName(bName, cleanGName) || matchName(bGame, cleanGName);
    });
    if (boundaryMatch) return boundaryMatch;

    // 4. Target process match (only if the binary does NOT manifestly belong to a different game)
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
    if (selectedBinaryId.value) {
      const explicit = binariesStore.binaries.find((b) => b.id === selectedBinaryId.value);
      if (explicit) return explicit;
    }
    return getGameMatchedBinary(selectedGame.value);
  });

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
    } catch (err: any) {
      diagnostics.addLog(`Failed scanning installed games: ${err.message}`, 'error');
    } finally {
      isScanning.value = false;
    }
  }

  function selectGame(gameId: string | null) {
    selectedGameId.value = gameId;
    if (gameId) {
      const g = games.value.find((item) => item.id === gameId);
      if (g) {
        const matched = getGameMatchedBinary(g);
        if (matched) {
          selectedBinaryId.value = matched.id;
          customTargetProcess.value = g.exeName || matched.target_process || '';
          selectedMode.value = matched.injection_mode ?? 2;

          const bIdx = binariesStore.binaries.findIndex((b) => b.id === matched.id);
          if (bIdx >= 0) binariesStore.selectBinary(bIdx);

          diagnostics.addLog(
            `Matched Ellohim Server binary: ${matched.name} (${matched.file_name || 'payload.dll'}) -> ${g.name}`,
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
      diagnostics.addLog('Returned to Control Center Overview', 'info');
    }
  }

  function onBinarySelected(binaryId: string | null) {
    selectedBinaryId.value = binaryId;
    if (binaryId) {
      const b = binariesStore.binaries.find((item) => item.id === binaryId);
      if (b) {
        if (b.target_process) {
          customTargetProcess.value = b.target_process;
        } else if (selectedGame.value?.exeName) {
          customTargetProcess.value = selectedGame.value.exeName;
        }
        if (b.injection_mode !== undefined) {
          selectedMode.value = b.injection_mode;
        }
        const bIdx = binariesStore.binaries.findIndex((item) => item.id === binaryId);
        if (bIdx >= 0) binariesStore.selectBinary(bIdx);
        diagnostics.addLog(`Selected binary payload: ${b.name} (${b.file_name || 'payload.dll'})`, 'info');
      }
    } else if (selectedGame.value) {
      const matched = getGameMatchedBinary(selectedGame.value);
      if (matched) {
        selectedBinaryId.value = matched.id;
        customTargetProcess.value = selectedGame.value.exeName || matched.target_process || '';
        selectedMode.value = matched.injection_mode ?? 2;
        const bIdx = binariesStore.binaries.findIndex((item) => item.id === matched.id);
        if (bIdx >= 0) binariesStore.selectBinary(bIdx);
      } else {
        selectedBinaryId.value = null;
        customTargetProcess.value = selectedGame.value.exeName || '';
      }
    }
    if (customTargetProcess.value) {
      processStore.setManualTarget(customTargetProcess.value, false);
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
          const bIdx = binariesStore.binaries.findIndex((b) => b.id === matched.id);
          if (bIdx >= 0) binariesStore.selectBinary(bIdx);
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

  function handleProcessTerminated(reason?: string) {
    stopProcessLivenessMonitor();
    if (launchStatus.value === 'running') {
      const gName = selectedGame.value?.name || 'Game';
      launchStatus.value = 'idle';
      runningPid.value = null;
      launchMessage.value = '';
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
        `[ELOHIM SERVER SYNC] Successfully updated ${binary.name} mapping on backend!`,
        'success'
      );
      return true;
    } catch (err: any) {
      diagnostics.addLog(`[SERVER SYNC ERROR] Could not save setting to Ellohim-Server: ${err.message}`, 'error');
      return false;
    } finally {
      isSavingServerConfig.value = false;
    }
  }

  async function launchAndInject() {
    const game = selectedGame.value;
    if (!game) return;

    const targetProc = customTargetProcess.value || game.exeName || '';
    const binary = matchedBinary.value;
    const binaryIndex = binary
      ? binariesStore.binaries.findIndex((b) => b.id === binary.id)
      : -1;

    launchStatus.value = 'launching';
    launchMessage.value = `Opening ${game.name} via ${game.platform.toUpperCase()}...`;
    diagnostics.addLog(
      `Launching ${game.name} [Target: ${targetProc || 'None'}, DLL: ${binary ? binary.file_name : 'None'}]...`,
      'info'
    );

    const stateTimer = setTimeout(() => {
      if (launchStatus.value === 'launching') {
        launchStatus.value = 'waiting';
        launchMessage.value = `Waiting for process "${targetProc}" to start...`;
      }
    }, 1800);

    try {
      if (!window.loader?.playAndInject) {
        throw new Error('Game launcher API is not available');
      }

      const result = await window.loader.playAndInject({
        launchUri: game.launchUri,
        exePath: game.platform === 'custom' ? game.launchUri : undefined,
        targetProcess: targetProc,
        binaryIndex: binaryIndex >= 0 ? binaryIndex : undefined,
        mode: selectedMode.value
      });

      clearTimeout(stateTimer);

      if (result.success) {
        launchStatus.value = 'running';
        runningPid.value = result.pid || null;
        launchMessage.value = result.injected
          ? `Playing with Ellohim Payload Injected (PID: ${result.pid})`
          : `Game running (PID: ${result.pid})`;
        diagnostics.addLog(
          `${game.name} launched successfully! ${result.injected ? `[Injected ${binary?.name || 'DLL'}]` : ''} PID: ${result.pid}`,
          'success'
        );
        if (result.pid) {
          startProcessLivenessMonitor(result.pid, targetProc || game.exeName || '');
        }
      } else {
        launchStatus.value = 'error';
        launchMessage.value = result.message || 'Failed to start or hook game.';
        diagnostics.addLog(`Launch failed: ${result.message}`, 'error');
      }
    } catch (err: any) {
      clearTimeout(stateTimer);
      launchStatus.value = 'error';
      launchMessage.value = err.message || 'Launch error occurred.';
      diagnostics.addLog(`Launch error for ${game.name}: ${err.message}`, 'error');
    }
  }

  async function stopGame() {
    stopProcessLivenessMonitor();
    if (!runningPid.value) {
      launchStatus.value = 'idle';
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
    launchMessage.value = '';
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
    customTargetProcess,
    selectedBinaryId,
    selectedMode,
    isSavingServerConfig,
    filteredGames,
    matchedBinary,
    getGameMatchedBinary,
    scanGames,
    selectGame,
    onBinarySelected,
    saveGameMappingToServer,
    launchAndInject,
    stopGame,
    browseAndAddCustom,
    removeCustomGame
  };
});
