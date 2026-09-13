<script setup lang="ts">
import { ref } from 'vue';
import { useGamesStore } from '../../stores/games';
import { useBinariesStore } from '../../stores/binaries';
import PulseHubView from './components/PulseHubView.vue';
import GameDetailView from './components/GameDetailView.vue';
import MetricsGrid from './components/MetricsGrid.vue';
import HeroBanner from './components/HeroBanner.vue';
import InjectionPanel from './components/InjectionPanel.vue';

const gamesStore = useGamesStore();
const binariesStore = useBinariesStore();

const viewMode = ref<'launcher' | 'classic'>('launcher');

function handleRefresh() {
  binariesStore.refreshBinaries();
}

function handleSelectBinary(index: number) {
  binariesStore.selectBinary(index);
}
</script>

<template>
  <div class="tab-pane active dashboard-wand-wrapper">
    <!-- View Mode Switcher Header Pill -->
    <div class="wand-view-controls">
      <div class="view-toggle-pill">
        <button
          class="toggle-btn"
          :class="{ active: viewMode === 'launcher' }"
          @click="viewMode = 'launcher'"
        >
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <polygon points="5 3 19 12 5 21 5 3" />
          </svg>
          <span>Game Launcher</span>
        </button>
        <button
          class="toggle-btn"
          :class="{ active: viewMode === 'classic' }"
          @click="viewMode = 'classic'"
        >
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <rect x="3" y="3" width="7" height="7"/><rect x="14" y="3" width="7" height="7"/>
            <rect x="14" y="14" width="7" height="7"/><rect x="3" y="14" width="7" height="7"/>
          </svg>
          <span>Manual Injector</span>
        </button>
      </div>
    </div>

    <!-- Mode 1: Game Hub / Game Detail Mode -->
    <template v-if="viewMode === 'launcher'">
      <transition name="page-fade" mode="out-in">
        <GameDetailView v-if="gamesStore.selectedGame" :key="gamesStore.selectedGame.id" />
        <PulseHubView v-else key="pulse-hub" />
      </transition>
    </template>

    <!-- Mode 2: Classic Advanced Manual Injector Grid -->
    <template v-else>
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
            <!-- Skeleton Loading Placeholders during HTTP Catalog Sync -->
            <template v-if="binariesStore.isSyncing">
              <div class="sync-banner-card">
                <div class="sync-spinner"></div>
                <span>Syncing software catalog & licenses from server...</span>
              </div>
              <div v-for="i in 3" :key="i" class="binary-card-skeleton">
                <div class="skeleton-icon shimmer"></div>
                <div class="skeleton-info">
                  <div class="skeleton-line title shimmer"></div>
                  <div class="skeleton-line meta shimmer"></div>
                  <div class="skeleton-line badges shimmer"></div>
                </div>
              </div>
            </template>

            <div v-else-if="binariesStore.binaries.length === 0" class="empty-state">
              No binaries assigned to your account.
            </div>

            <div
              v-else
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
    </template>
  </div>
</template>
