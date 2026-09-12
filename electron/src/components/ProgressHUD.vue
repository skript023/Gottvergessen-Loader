<script setup lang="ts">
import { computed } from 'vue';
import { useInjectionStore } from '../stores/injection';

const injection = useInjectionStore();

const steps = [
  { id: 1, label: 'Handshake', min: 0, max: 10 },
  { id: 2, label: 'Payload / Cache', min: 11, max: 75 },
  { id: 3, label: 'AES Decrypt', min: 76, max: 88 },
  { id: 4, label: 'Memory Hook', min: 89, max: 100 }
];

const currentStepId = computed(() => {
  const p = injection.progress;
  if (p < 12) return 1;
  if (p < 76) return 2;
  if (p < 89) return 3;
  return 4;
});
</script>

<template>
  <div v-if="injection.showProgress" class="operation-progress-card">
    <div class="progress-info-row">
      <div class="progress-stage-badge">
        <span class="spinner-dot"></span>
        <span>{{ injection.stage }}</span>
      </div>
      <strong class="progress-percent-text">{{ injection.progress }}%</strong>
    </div>

    <div class="progress-track-outer">
      <div class="progress-fill-bar" :style="{ width: `${injection.progress}%` }"></div>
    </div>

    <!-- Clear Step-by-Step Process Transition Pipeline -->
    <div class="injection-steps-pipeline">
      <div
        v-for="step in steps"
        :key="step.id"
        class="step-pipeline-item"
        :class="{
          completed: injection.progress >= step.max || (injection.progress === 100),
          active: currentStepId === step.id && injection.progress < 100
        }"
      >
        <div class="step-indicator-circle">
          <span v-if="injection.progress >= step.max || injection.progress === 100">✓</span>
          <span v-else-if="currentStepId === step.id" class="step-pulse-spinner"></span>
          <span v-else>{{ step.id }}</span>
        </div>
        <span class="step-pipeline-label">{{ step.label }}</span>
      </div>
    </div>
  </div>
</template>
