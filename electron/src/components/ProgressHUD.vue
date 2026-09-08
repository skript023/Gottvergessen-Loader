<script setup lang="ts">
import { useInjectionStore } from '../stores/injection';

const injection = useInjectionStore();
</script>

<template>
  <div v-if="injection.showProgress" class="operation-progress-card">
    <div class="progress-info-row">
      <div class="progress-stage-badge">
        <span class="spinner-dot"></span>
        <span>{{ injection.stage }}</span>
      </div>
      <strong>{{ injection.progress }}%</strong>
    </div>
    <div class="progress-track-outer">
      <div class="progress-fill-bar" :style="{ width: `${injection.progress}%` }"></div>
    </div>
  </div>
</template>
