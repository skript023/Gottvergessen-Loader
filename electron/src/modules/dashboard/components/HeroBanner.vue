<script setup lang="ts">
import { computed } from 'vue';
import { useBinariesStore } from '../../../stores/binaries';
import { useProcessStore } from '../../../stores/process';
import { useInjectionStore } from '../../../stores/injection';

const binariesStore = useBinariesStore();
const processStore = useProcessStore();
const injection = useInjectionStore();

const active = computed(() => binariesStore.activeBinary);
const title = computed(() => active.value?.name || active.value?.game || 'Select a Software Binary');
const desc = computed(() => active.value?.description || 'Choose an authorized product binary below to prepare the remote encrypted payload stream.');
const arch = computed(() => active.value?.arch || 'x64 Native');
const version = computed(() => active.value?.version ? `v${active.value.version}` : 'Latest Release');

function handleLaunch() {
  injection.executeInjection();
}
</script>

<template>
  <div class="hero-product-banner">
    <div class="hero-left">
      <div class="hero-badge-tag">
        <span class="status-indicator-dot"></span>
        <span>STATUS: UNDETECTED & OPERATIONAL</span>
      </div>
      <h2>{{ title }}</h2>
      <p>{{ desc }}</p>
      <div class="hero-details">
        <div class="hero-detail-item">
          <span>ARCHITECTURE</span>
          <strong>{{ arch }}</strong>
        </div>
        <div class="hero-detail-item">
          <span>VERSION</span>
          <strong>{{ version }}</strong>
        </div>
        <div class="hero-detail-item">
          <span>PROTECTION</span>
          <strong>Encrypted VM</strong>
        </div>
      </div>
    </div>
    <div class="hero-right">
      <button
        class="btn-hero-launch"
        :disabled="!processStore.isTargetReady || injection.isInjecting"
        @click="handleLaunch"
      >
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
          <polygon points="5 3 19 12 5 21 5 3"/>
        </svg>
        <span>{{ injection.isInjecting ? 'Injecting...' : 'Launch Binary' }}</span>
      </button>
    </div>
  </div>
</template>
