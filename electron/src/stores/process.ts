import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import type { ProcessItem } from '../types/loader';
import { useDiagnosticsStore } from './diagnostics';
import { useBinariesStore } from './binaries';

export const useProcessStore = defineStore('process', () => {
  const diagnostics = useDiagnosticsStore();
  const binariesStore = useBinariesStore();

  const processes = ref<ProcessItem[]>([]);
  const selectedProcess = ref<ProcessItem | null>(null);
  const searchQuery = ref('');
  const isScanning = ref(false);

  const filteredProcesses = computed(() => {
    const q = searchQuery.value.trim().toLowerCase();
    if (!q) return processes.value;
    return processes.value.filter(
      (p) => p.name.toLowerCase().includes(q) || String(p.pid).includes(q)
    );
  });

  const isProcessRunning = computed(() => {
    return Boolean(selectedProcess.value && selectedProcess.value.pid > 0);
  });

  const isTargetReady = computed(() => {
    return isProcessRunning.value && binariesStore.selectedBinaryIndex >= 0;
  });

  function selectProcess(proc: ProcessItem, autoSave = true) {
    selectedProcess.value = proc;
    diagnostics.addLog(`Target locked: ${proc.name} (PID: ${proc.pid}, ${proc.arch})`, 'info');

    if (autoSave && binariesStore.activeBinary) {
      if (binariesStore.activeBinary.target_process !== proc.name) {
        binariesStore.updateActiveTarget(proc.name);
      }
    }
  }

  function setManualTarget(name: string) {
    const trimmed = name.trim();
    if (!trimmed) return;

    const match = processes.value.find((p) => p.name.toLowerCase() === trimmed.toLowerCase());
    if (match) {
      selectProcess(match, true);
    } else {
      selectedProcess.value = {
        name: trimmed,
        pid: 0,
        arch: 'Configured',
        accessible: true
      };
      diagnostics.addLog(`Target configured: ${trimmed} (Waiting for process to launch...)`, 'warn');
      if (binariesStore.activeBinary) {
        binariesStore.updateActiveTarget(trimmed);
      }
    }
  }

  function autoHookFromActiveBinary() {
    const active = binariesStore.activeBinary;
    if (!active || !active.target_process) {
      selectedProcess.value = null;
      return;
    }

    const procName = active.target_process.trim();
    const match = processes.value.find((p) => p.name.toLowerCase() === procName.toLowerCase());
    if (match) {
      selectedProcess.value = match;
      diagnostics.addLog(`Auto-hooked target process ${match.name} (PID: ${match.pid}) for ${active.name || 'binary'}`, 'info');
    } else {
      selectedProcess.value = {
        name: procName,
        pid: 0,
        arch: 'Configured',
        accessible: true
      };
      diagnostics.addLog(`Target configured: ${procName} (Waiting for process to launch...)`, 'warn');
    }
  }

  async function scanProcesses() {
    isScanning.value = true;
    diagnostics.addLog('Scanning running processes...', 'info');
    try {
      if (window.loader?.listProcesses) {
        processes.value = await window.loader.listProcesses();
        diagnostics.addLog(`Process scan complete. ${processes.value.length} processes detected.`, 'success');

        // Check if currently selected process or active binary target matches
        const active = binariesStore.activeBinary;
        if (active?.target_process) {
          const match = processes.value.find(
            (p) => p.name.toLowerCase() === active.target_process!.trim().toLowerCase()
          );
          if (match && (!selectedProcess.value || selectedProcess.value.pid !== match.pid)) {
            selectProcess(match, false);
            diagnostics.addLog(`Auto-hooked running target: ${match.name} (PID: ${match.pid})`, 'success');
          }
        }
      }
    } catch (err: any) {
      diagnostics.addLog(`Process enumeration error: ${err.message}`, 'error');
    } finally {
      isScanning.value = false;
    }
  }

  return {
    processes,
    selectedProcess,
    searchQuery,
    isScanning,
    filteredProcesses,
    isProcessRunning,
    isTargetReady,
    selectProcess,
    setManualTarget,
    autoHookFromActiveBinary,
    scanProcesses
  };
});
