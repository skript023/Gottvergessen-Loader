import { defineStore } from 'pinia';
import { ref } from 'vue';

export interface LogEntry {
  id: number;
  time: string;
  message: string;
  type: 'info' | 'success' | 'warn' | 'error';
}

export const useDiagnosticsStore = defineStore('diagnostics', () => {
  const logs = ref<LogEntry[]>([
    {
      id: 1,
      time: '[00:00:00]',
      message: 'Engine booted. Vue 3 frontend initialized.',
      type: 'info'
    }
  ]);

  let nextId = 2;

  function addLog(message: string, type: 'info' | 'success' | 'warn' | 'error' = 'info') {
    const now = new Date();
    const time = `[${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}:${String(now.getSeconds()).padStart(2, '0')}]`;
    logs.value.push({
      id: nextId++,
      time,
      message,
      type
    });

    // Cap at 1000 logs
    if (logs.value.length > 1000) {
      logs.value.shift();
    }
  }

  function clearLogs() {
    logs.value = [];
    addLog('Console cleared.', 'info');
  }

  return {
    logs,
    addLog,
    clearLogs
  };
});
