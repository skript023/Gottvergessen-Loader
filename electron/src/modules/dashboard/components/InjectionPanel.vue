<script setup lang="ts">
import { ref, computed } from 'vue';
import { useRouter } from 'vue-router';
import { useBinariesStore } from '../../../stores/binaries';
import { useProcessStore } from '../../../stores/process';
import { useInjectionStore } from '../../../stores/injection';
import ProgressHUD from '../../../components/ProgressHUD.vue';

const router = useRouter();
const binariesStore = useBinariesStore();
const processStore = useProcessStore();
const injection = useInjectionStore();

const showManualBox = ref(false);
const manualInput = ref('');

const selectedMode = computed({
  get: () => {
    return binariesStore.activeBinary?.injection_mode !== undefined
      ? String(binariesStore.activeBinary.injection_mode)
      : '2';
  },
  set: (val: string) => {
    binariesStore.updateActiveMode(parseInt(val, 10));
  }
});

function toggleManualBox() {
  showManualBox.value = !showManualBox.value;
  if (showManualBox.value) {
    manualInput.value = binariesStore.activeBinary?.target_process || processStore.selectedProcess?.name || '';
  }
}

function saveManualTarget() {
  if (!manualInput.value.trim()) return;
  processStore.setManualTarget(manualInput.value.trim());
  showManualBox.value = false;
}

function cancelManualTarget() {
  showManualBox.value = false;
}

function handleManualKeydown(e: KeyboardEvent) {
  if (e.key === 'Enter') saveManualTarget();
  else if (e.key === 'Escape') cancelManualTarget();
}

function jumpToProcesses() {
  router.push('/processes');
}

function handleExecute() {
  injection.executeInjection();
}
</script>

<template>
  <div class="panel-cyber">
    <div class="panel-header">
      <div>
        <h3 class="panel-title">Injection Controller</h3>
        <p class="panel-sub">Configure memory injection and launch payload</p>
      </div>
      <span class="badge-stealth">STEALTH MODE</span>
    </div>

    <div class="injection-form">
      <!-- Target Process Field -->
      <div class="control-box">
        <div class="control-box-header">
          <span class="control-label">Target Hook Process</span>
          <div class="target-controls-header">
            <button class="btn-link" @click="toggleManualBox" title="Set process executable manually">
              ✏️ Set Exe
            </button>
            <button class="btn-link" @click="jumpToProcesses">Browse List →</button>
          </div>
        </div>

        <!-- Target selection pill -->
        <div
          class="target-selection-pill"
          :class="{
            'has-target': Boolean(processStore.selectedProcess),
            'waiting-process': processStore.selectedProcess && processStore.selectedProcess.pid === 0
          }"
        >
          <template v-if="processStore.selectedProcess">
            <span class="target-icon">{{ processStore.selectedProcess.pid > 0 ? '🎯' : '⏳' }}</span>
            <span class="target-text">
              <strong>{{ processStore.selectedProcess.name }}</strong>
              <template v-if="processStore.selectedProcess.pid > 0">
                (PID: {{ processStore.selectedProcess.pid }}) [{{ processStore.selectedProcess.arch }}]
              </template>
              <template v-else>
                (Waiting for game to launch...)
              </template>
            </span>
          </template>
          <template v-else>
            <span class="target-icon">🎯</span>
            <span class="target-text">No target process selected</span>
          </template>
        </div>

        <!-- Manual input box -->
        <div v-if="showManualBox" class="manual-target-input-row">
          <input
            v-model="manualInput"
            type="text"
            placeholder="e.g. GTA5.exe or RDR2.exe"
            spellcheck="false"
            autofocus
            @keydown="handleManualKeydown"
          />
          <button class="btn-pill-save" type="button" @click="saveManualTarget">Save</button>
          <button class="btn-pill-cancel" type="button" @click="cancelManualTarget">✕</button>
        </div>

        <!-- Cloud sync status -->
        <div class="target-cloud-status" :class="binariesStore.syncStatus">
          <span class="sync-dot"></span>
          <span>{{ binariesStore.syncText }}</span>
        </div>
      </div>

      <!-- Injection Method Selector -->
      <div class="control-box">
        <label class="control-label" for="mode-select">Memory Injection Method</label>
        <div class="select-wrapper">
          <select id="mode-select" v-model="selectedMode">
            <option value="2">Manual Map (Stealth Kernel / Recommended)</option>
            <option value="0">CreateRemoteThread (Standard Win32)</option>
            <option value="1">Thread Hijack (APC / Thread Context)</option>
            <option value="3">Reflective DLL Injection</option>
          </select>
        </div>
        <span class="input-hint">Manual Map bypasses standard Windows loader hooks.</span>
      </div>

      <!-- Execute button -->
      <button
        class="btn-launch-injection"
        :disabled="!processStore.isTargetReady || injection.isInjecting"
        @click="handleExecute"
      >
        <div class="btn-launch-content">
          <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
            <polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/>
          </svg>
          <span>{{ injection.isInjecting ? 'INJECTING PAYLOAD...' : 'EXECUTE INJECTION' }}</span>
        </div>
      </button>

      <!-- Operation Progress HUD -->
      <ProgressHUD />

      <!-- Status Feedback Message -->
      <div
        v-if="injection.feedbackMessage"
        class="status-feedback"
        :style="{
          color: injection.feedbackStatus === 'success' ? '#34d399' : injection.feedbackStatus === 'error' ? '#f87171' : 'var(--text-muted)'
        }"
      >
        {{ injection.feedbackMessage }}
      </div>
    </div>
  </div>
</template>
