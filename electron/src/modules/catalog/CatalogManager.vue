<script setup lang="ts">
import { useRouter } from 'vue-router';
import { useBinariesStore } from '../../stores/binaries';
import { useGamesStore } from '../../stores/games';

const router = useRouter();
const binariesStore = useBinariesStore();
const gamesStore = useGamesStore();

function selectAndConfigure(index: number) {
  binariesStore.selectBinary(index);
  const binary = binariesStore.binaries[index];
  const matchedGame = gamesStore.games.find(
    (game) => gamesStore.getGameMatchedBinary(game)?.id === binary?.id
  );

  router.push(matchedGame
    ? { path: '/dashboard', query: { game: matchedGame.id } }
    : { path: '/dashboard' });
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
      <!-- Skeleton Loading Cards while Syncing -->
      <template v-if="binariesStore.isSyncing">
        <div v-for="i in 4" :key="i" class="catalog-card-skeleton">
          <div class="catalog-card-header">
            <div class="skeleton-line tag shimmer"></div>
            <div class="skeleton-line badge shimmer"></div>
          </div>
          <div class="skeleton-line title shimmer"></div>
          <div class="skeleton-grid-lines">
            <div class="skeleton-line detail shimmer"></div>
            <div class="skeleton-line detail shimmer"></div>
            <div class="skeleton-line detail shimmer"></div>
            <div class="skeleton-line detail shimmer"></div>
          </div>
          <div class="skeleton-line btn shimmer"></div>
        </div>
      </template>

      <div v-else-if="binariesStore.binaries.length === 0" class="empty-state" style="grid-column: 1 / -1;">
        No binaries available.
      </div>

      <div
        v-else
        v-for="(binary, index) in binariesStore.binaries"
        :key="binary.id || index"
        class="catalog-card"
        :class="{ selected: index === binariesStore.selectedBinaryIndex }"
        role="button"
        tabindex="0"
        @click="selectAndConfigure(index)"
        @keydown.enter.prevent="selectAndConfigure(index)"
        @keydown.space.prevent="selectAndConfigure(index)"
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
            <strong style="color: var(--primary-cyan);">{{ binary.target_process || 'Not set' }}</strong>
          </div>
          <div class="catalog-detail">
            <span>METHOD</span>
            <strong style="color: #c084fc;">
              {{ binariesStore.getShortModeName(binary.injection_mode ?? 2) }}
            </strong>
          </div>
          <div v-if="binary.expiry_date" class="catalog-detail">
            <span>LICENSE EXPIRY</span>
            <strong style="color: #4ef0a8;">
              {{ binary.expiry_date }}
            </strong>
          </div>
        </div>

        <button class="btn-select-binary" @click.stop="selectAndConfigure(index)">
          {{ index === binariesStore.selectedBinaryIndex ? '✓ Selected' : 'Select & Configure' }}
        </button>
      </div>
    </div>
  </div>
</template>
