import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import type { BinaryItem } from '../types/loader';
import { useDiagnosticsStore } from './diagnostics';
import { useAuthStore } from './auth';

export const useBinariesStore = defineStore('binaries', () => {
  const diagnostics = useDiagnosticsStore();

  const binaries = ref<BinaryItem[]>([]);
  const selectedBinaryIndex = ref<number>(-1);
  const isSyncing = ref(false);
  const syncStatus = ref<'idle' | 'saving' | 'synced' | 'error'>('idle');
  const syncText = ref('Settings synced with Cloud');

  const activeBinary = computed<BinaryItem | null>(() => {
    if (selectedBinaryIndex.value >= 0 && selectedBinaryIndex.value < binaries.value.length) {
      return binaries.value[selectedBinaryIndex.value];
    }
    return null;
  });

  function getModeName(mode?: number): string {
    switch (Number(mode)) {
      case 0: return 'CreateRemoteThread';
      case 1: return 'Thread Hijack';
      case 2: return 'QueueUserAPC + Handle Hijack';
      case 3: return 'Reflective DLL';
      default: return 'CreateRemoteThread';
    }
  }

  function getShortModeName(mode?: number): string {
    switch (Number(mode)) {
      case 0: return 'Remote Thread';
      case 1: return 'Thread Hijack';
      case 2: return 'QueueUserAPC';
      case 3: return 'Reflective';
      default: return 'Remote Thread';
    }
  }

  function restoreLocalFallback(binary: BinaryItem) {
    if (!binary.target_process && (binary as any).target) {
      binary.target_process = (binary as any).target;
    }
    try {
      const localStore = JSON.parse(localStorage.getItem('ellohim_binary_settings') || '{}');
      if (localStore[binary.id]) {
        const cachedTarget = (localStore[binary.id].target_process || '').trim();
        const bName = (binary.name || '').toLowerCase().replace(/[^a-z0-9]/g, '');
        const targetStem = cachedTarget.replace(/\.exe$/i, '').replace(/[^a-z0-9]/g, '');

        // Sanity check: do not restore cached target if it blatantly contradicts binary identity
        if (cachedTarget && (!bName || bName.length < 3 || targetStem.includes(bName) || bName.includes(targetStem))) {
          if (!binary.target_process) {
            binary.target_process = cachedTarget;
          }
        }
        if (binary.injection_mode === undefined && localStore[binary.id].injection_mode !== undefined) {
          binary.injection_mode = localStore[binary.id].injection_mode;
        }
      }
    } catch (_) {}
  }

  function setBinaries(items: BinaryItem[]) {
    const selectedId = activeBinary.value?.id || null;
    binaries.value = items.map((b) => {
      const clone = { ...b };
      if (!clone.target_process && (b as any).target) {
        clone.target_process = (b as any).target;
      }
      restoreLocalFallback(clone);
      return clone;
    });

    selectedBinaryIndex.value = selectedId
      ? binaries.value.findIndex((binary) => binary.id === selectedId)
      : -1;
  }

  function selectBinary(index: number) {
    if (index < 0 || index >= binaries.value.length) return;
    selectedBinaryIndex.value = index;
    const b = binaries.value[index];
    restoreLocalFallback(b);

    syncStatus.value = 'synced';
    syncText.value = `Active: ${b.target_process || 'No target set'} • ${getShortModeName(b.injection_mode ?? 2)}`;
    diagnostics.addLog(`Selected binary: ${b.name || 'Binary'} (v${b.version || '1.0.0'})`, 'info');
  }

  let persistTimeout: any = null;
  async function persistBinarySettings(targetBinary?: BinaryItem) {
    const item = targetBinary || activeBinary.value;
    if (!item || !item.id) return;

    syncStatus.value = 'saving';
    syncText.value = 'Syncing to Cloud...';

    // LocalStorage fallback
    try {
      const localStore = JSON.parse(localStorage.getItem('ellohim_binary_settings') || '{}');
      localStore[item.id] = {
        target_process: item.target_process || '',
        injection_mode: Number(item.injection_mode) || 0
      };
      localStorage.setItem('ellohim_binary_settings', JSON.stringify(localStore));
    } catch (_) {}

    clearTimeout(persistTimeout);
    persistTimeout = setTimeout(async () => {
      try {
        if (window.loader?.saveBinarySettings) {
          await window.loader.saveBinarySettings(
            item.id,
            item.target_process || '',
            Number(item.injection_mode) || 0
          );
          syncStatus.value = 'synced';
          syncText.value = `Synced: ${item.target_process || 'No target'} • ${getShortModeName(item.injection_mode)}`;
          diagnostics.addLog(`Cloud saved [${item.name}]: target=${item.target_process || 'None'}, mode=${getShortModeName(item.injection_mode)}`, 'success');
        }
      } catch (err: any) {
        syncStatus.value = 'error';
        syncText.value = 'Local saved (Cloud error)';
        diagnostics.addLog(`Failed to sync settings to cloud: ${err.message}`, 'warn');
      }
    }, 400);
  }

  function updateActiveTarget(processName: string) {
    if (activeBinary.value) {
      activeBinary.value.target_process = processName;
      persistBinarySettings(activeBinary.value);
    }
  }

  function updateActiveMode(mode: number) {
    if (activeBinary.value) {
      activeBinary.value.injection_mode = mode;
      persistBinarySettings(activeBinary.value);
    }
  }

  async function refreshBinaries() {
    isSyncing.value = true;
    diagnostics.addLog('Syncing binary catalog from server...', 'info');
    try {
      if (window.loader?.refreshBinaries) {
        const items = await window.loader.refreshBinaries();
        setBinaries(items);
        diagnostics.addLog(`Catalog synced. ${items.length} binaries available.`, 'success');
      }
    } catch (err: any) {
      diagnostics.addLog(`Failed to refresh catalog: ${err.message}`, 'error');
      if (err.message && (err.message.includes('401') || err.message.toLowerCase().includes('unauthorized') || err.message.toLowerCase().includes('login is required'))) {
        useAuthStore().handleKick('Session revoked or expired by security server.');
      }
    } finally {
      isSyncing.value = false;
    }
  }

  return {
    binaries,
    selectedBinaryIndex,
    activeBinary,
    isSyncing,
    syncStatus,
    syncText,
    getModeName,
    getShortModeName,
    setBinaries,
    selectBinary,
    updateActiveTarget,
    updateActiveMode,
    persistBinarySettings,
    refreshBinaries
  };
});
