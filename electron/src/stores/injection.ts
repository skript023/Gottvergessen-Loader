import { defineStore } from 'pinia';
import { ref } from 'vue';
import { useDiagnosticsStore } from './diagnostics';
import { useBinariesStore } from './binaries';
import { useProcessStore } from './process';

export const useInjectionStore = defineStore('injection', () => {
  const diagnostics = useDiagnosticsStore();
  const binariesStore = useBinariesStore();
  const processStore = useProcessStore();

  const isInjecting = ref(false);
  const showProgress = ref(false);
  const progress = ref(0);
  const stage = ref('Initializing secure stream...');
  const feedbackMessage = ref('');
  const feedbackStatus = ref<'idle' | 'success' | 'error'>('idle');

  async function executeInjection() {
    const proc = processStore.selectedProcess;
    const binaryIdx = binariesStore.selectedBinaryIndex;
    const activeBin = binariesStore.activeBinary;

    if (!proc || proc.pid <= 0 || binaryIdx < 0 || !activeBin) {
      feedbackMessage.value = 'Please select a valid running target process and binary.';
      feedbackStatus.value = 'error';
      return;
    }

    isInjecting.value = true;
    showProgress.value = true;
    progress.value = 5;
    stage.value = 'Connecting to secure payload vault...';
    feedbackMessage.value = 'Injection operation in progress...';
    feedbackStatus.value = 'idle';

    const mode = activeBin.injection_mode !== undefined ? Number(activeBin.injection_mode) : 2;
    diagnostics.addLog(
      `Initiating injection: Mode ${mode} -> ${proc.name} (PID: ${proc.pid})`,
      'info'
    );

    let timer: any = null;
    if (window.loader?.operationStatus) {
      timer = setInterval(async () => {
        try {
          const status = await window.loader!.operationStatus();
          progress.value = Math.max(0, Math.min(100, Number(status.progress) || 0));
          stage.value = status.stage || 'Executing operation...';
        } catch (_) {}
      }, 120);
    }

    try {
      if (!window.loader?.inject) {
        throw new Error('Loader native injection API is unavailable');
      }

      const success = await window.loader.inject({
        pid: proc.pid,
        processName: proc.name,
        binaryIndex: binaryIdx,
        mode
      });

      if (success) {
        progress.value = 100;
        stage.value = 'Injection Complete';
        feedbackStatus.value = 'success';
        feedbackMessage.value = `Payload injected into ${proc.name} successfully!`;
        diagnostics.addLog(`Injection succeeded into ${proc.name} [PID: ${proc.pid}]`, 'success');
      } else {
        progress.value = 0;
        stage.value = 'Failed';
        feedbackStatus.value = 'error';
        feedbackMessage.value = 'Injection failed. Check the native engine log.';
        diagnostics.addLog('Native engine reported injection failure.', 'error');
      }
    } catch (err: any) {
      progress.value = 0;
      stage.value = 'Error';
      feedbackStatus.value = 'error';
      feedbackMessage.value = `Error: ${err.message}`;
      diagnostics.addLog(`Injection error: ${err.message}`, 'error');
    } finally {
      if (timer) clearInterval(timer);
      isInjecting.value = false;
    }
  }

  return {
    isInjecting,
    showProgress,
    progress,
    stage,
    feedbackMessage,
    feedbackStatus,
    executeInjection
  };
});
