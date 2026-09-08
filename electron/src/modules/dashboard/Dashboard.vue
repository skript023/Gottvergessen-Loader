<script setup lang="ts">
import { useBinariesStore } from '../../stores/binaries';
import MetricsGrid from './components/MetricsGrid.vue';
import HeroBanner from './components/HeroBanner.vue';
import InjectionPanel from './components/InjectionPanel.vue';

const binariesStore = useBinariesStore();

function handleRefresh() {
  binariesStore.refreshBinaries();
}

function handleSelectBinary(index: number) {
  binariesStore.selectBinary(index);
}
</script>

<template>
  <div class="tab-pane active">
    <!-- Top Stat Cards -->
    <MetricsGrid />

    <!-- Hero Product Banner -->
    <HeroBanner />

    <!-- Split Grid: Left Binary Picker & Right Injection Engine -->
    <div class="dashboard-split-grid">
      <!-- Left: Accessible Binaries Picker -->
      <div class="panel-cyber">
        <div class="panel-header">
          <div>
            <h3 class="panel-title">Authorized Binaries</h3>
            <p class="panel-sub">Select the binary payload you wish to inject</p>
          </div>
          <button
            class="btn-icon-text"
            :disabled="binariesStore.isSyncing"
            @click="handleRefresh"
            title="Refresh list"
          >
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <polyline points="23 4 23 10 17 10"/>
              <polyline points="1 20 1 14 7 14"/>
              <path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"/>
            </svg>
            <span>{{ binariesStore.isSyncing ? 'Syncing...' : 'Sync' }}</span>
          </button>
        </div>

        <div class="binary-cards-container">
          <div v-if="binariesStore.binaries.length === 0" class="empty-state">
            No binaries assigned to your account.
          </div>

          <div
            v-for="(binary, index) in binariesStore.binaries"
            :key="binary.id || index"
            class="binary-card-item"
            :class="{ selected: index === binariesStore.selectedBinaryIndex }"
            @click="handleSelectBinary(index)"
          >
            <div class="binary-item-left">
              <div class="binary-item-icon">⚡</div>
              <div class="binary-item-info">
                <strong>{{ binary.name || binary.game || `Binary #${index + 1}` }}</strong>
                <div class="binary-item-meta">
                  <span>{{ binary.file_name || binary.filename || 'payload.dll' }}</span>
                  <span>•</span>
                  <span class="binary-tag">SAFE</span>
                  <span>•</span>
                  <span>v{{ binary.version || '1.0.0' }}</span>
                </div>
                <div class="binary-settings-summary">
                  <span class="badge-setting-proc" title="Saved Target Executable">
                    🎯 {{ binary.target_process || 'No Target' }}
                  </span>
                  <span class="badge-setting-mode" title="Saved Injection Mode">
                    ⚡ {{ binariesStore.getShortModeName(binary.injection_mode ?? 2) }}
                  </span>
                </div>
              </div>
            </div>
            <button class="binary-item-action">
              {{ index === binariesStore.selectedBinaryIndex ? 'Active' : 'Select' }}
            </button>
          </div>
        </div>
      </div>

      <!-- Right: Injection Engine & Target Setup -->
      <InjectionPanel />
    </div>
  </div>
</template>
