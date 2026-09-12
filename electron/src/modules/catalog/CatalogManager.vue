<script setup lang="ts">
import { useRouter } from 'vue-router';
import { useBinariesStore } from '../../stores/binaries';

const router = useRouter();
const binariesStore = useBinariesStore();

function selectAndConfigure(index: number) {
  binariesStore.selectBinary(index);
  router.push('/dashboard');
}

function handleRefresh() {
  binariesStore.refreshBinaries();
}
</script>

<template>
  <div class="tab-pane active">
    <div class="pane-header">
      <div>
        <h2 class="pane-title">Authorized Software Catalog</h2>
        <p class="pane-desc">All binaries assigned to your account license with remote verification.</p>
      </div>
      <button class="btn-action" :disabled="binariesStore.isSyncing" @click="handleRefresh">
        <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <polyline points="23 4 23 10 17 10"/>
          <polyline points="1 20 1 14 7 14"/>
          <path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"/>
        </svg>
        <span>{{ binariesStore.isSyncing ? 'Refreshing...' : 'Refresh Catalog' }}</span>
      </button>
    </div>

    <div class="catalog-grid">
      <div v-if="binariesStore.binaries.length === 0" class="empty-state" style="grid-column: 1 / -1;">
        No binaries available.
      </div>

      <div
        v-for="(binary, index) in binariesStore.binaries"
        :key="binary.id || index"
        class="catalog-card"
        :class="{ selected: index === binariesStore.selectedBinaryIndex }"
      >
        <div class="catalog-card-header">
          <span class="catalog-game-tag">{{ (binary.arch || 'x64').toUpperCase() }}</span>
          <span class="catalog-status-badge">
            <span class="status-indicator-dot"></span> UNDETECTED
          </span>
        </div>

        <h3 class="catalog-title">{{ binary.name || binary.game || `Binary #${index + 1}` }}</h3>

        <div class="catalog-details">
          <div class="catalog-detail">
            <span>PAYLOAD</span>
            <strong>{{ binary.file_name || binary.filename || 'payload.dll' }}</strong>
          </div>
          <div class="catalog-detail">
            <span>VERSION</span>
            <strong>{{ binary.version || '1.0.0' }}</strong>
          </div>
          <div class="catalog-detail">
            <span>SAVED TARGET</span>
            <strong style="color: #7dd3fc;">{{ binary.target_process || 'Not set' }}</strong>
          </div>
          <div class="catalog-detail">
            <span>METHOD</span>
            <strong style="color: #c084fc;">
              {{ binariesStore.getShortModeName(binary.injection_mode ?? 2) }}
            </strong>
          </div>
          <div v-if="binary.expiry_date" class="catalog-detail">
            <span>LICENSE EXPIRY</span>
            <strong style="color: #34d399;">
              {{ binary.expiry_date }}
            </strong>
          </div>
        </div>

        <button class="btn-select-binary" @click="selectAndConfigure(index)">
          {{ index === binariesStore.selectedBinaryIndex ? '✓ Selected' : 'Select & Configure' }}
        </button>
      </div>
    </div>
  </div>
</template>
