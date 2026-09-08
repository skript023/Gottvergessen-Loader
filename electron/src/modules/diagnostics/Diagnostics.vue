<script setup lang="ts">
import { ref, watch, nextTick, onMounted } from 'vue';
import { useDiagnosticsStore } from '../../stores/diagnostics';

const diagnostics = useDiagnosticsStore();
const terminalRef = ref<HTMLDivElement | null>(null);

function scrollToBottom() {
  nextTick(() => {
    if (terminalRef.value) {
      terminalRef.value.scrollTop = terminalRef.value.scrollHeight;
    }
  });
}

watch(() => diagnostics.logs.length, () => {
  scrollToBottom();
});

onMounted(() => {
  scrollToBottom();
});

function handleClear() {
  diagnostics.clearLogs();
}
</script>

<template>
  <div class="tab-pane active">
    <div class="pane-header">
      <div>
        <h2 class="pane-title">Live Diagnostic Console</h2>
        <p class="pane-desc">Real-time trace logs from the C++ native engine and Electron IPC bridge.</p>
      </div>
      <button class="btn-action" @click="handleClear">Clear Console</button>
    </div>

    <div class="terminal-box">
      <div class="terminal-top">
        <span class="term-dot red"></span>
        <span class="term-dot yellow"></span>
        <span class="term-dot green"></span>
        <span class="term-title">Gottvergessen Loader Engine Trace</span>
      </div>
      <div ref="terminalRef" class="terminal-content">
        <div
          v-for="log in diagnostics.logs"
          :key="log.id"
          class="log-line"
          :class="log.type"
        >
          <span class="log-time">{{ log.time }}</span>
          <span>{{ log.message }}</span>
        </div>
      </div>
    </div>
  </div>
</template>
