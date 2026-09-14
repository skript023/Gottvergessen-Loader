<script setup lang="ts">
import { ref, onMounted } from 'vue';
import { useRouter } from 'vue-router';
import { useAuthStore } from '../stores/auth';
import { useBinariesStore } from '../stores/binaries';
import { useGamesStore } from '../stores/games';

const router = useRouter();
const auth = useAuthStore();
const binariesStore = useBinariesStore();
const gamesStore = useGamesStore();

const failedIcons = ref<Record<string, boolean>>({});

onMounted(() => {
  gamesStore.scanGames();
});

function handleHomeClick() {
  gamesStore.selectGame(null);
  router.push('/dashboard');
}

function handleSelectGame(gameId: string) {
  gamesStore.selectGame(gameId);
  router.push('/dashboard');
}

async function handleLogout() {
  const confirmed = confirm('Are you sure you want to sign out?');
  if (!confirmed) return;
  await auth.logout();
  router.push('/login');
}
</script>

<template>
  <aside class="sidebar wand-sidebar">
    <!-- Brand Header -->
    <div class="sidebar-brand">
      <div class="brand-badge-icon">
        <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5" stroke="#38bdf8" />
        </svg>
      </div>
      <div class="brand-text">
        <span class="brand-name">GOTTVERGESSEN</span>
        <span class="brand-version">QUANTUM GATEWAY</span>
      </div>
    </div>

    <!-- Navigation Scrollable Area -->
    <div class="sidebar-scrollable">
      <!-- Main Overview Tab -->
      <div class="sidebar-pulse-wrap">
        <button
          class="pulse-nav-btn"
          :class="{ active: !gamesStore.selectedGameId && $route.path === '/dashboard' }"
          @click="handleHomeClick"
        >
          <div class="pulse-icon-flame">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#38bdf8" stroke-width="2">
              <rect x="3" y="3" width="7" height="7"/><rect x="14" y="3" width="7" height="7"/>
              <rect x="14" y="14" width="7" height="7"/><rect x="3" y="14" width="7" height="7"/>
            </svg>
          </div>
          <span class="pulse-nav-label">Control Hub</span>
          <span class="pulse-sparkle">⚡</span>
        </button>
      </div>

      <!-- MY GAMES Section -->
      <div class="my-games-section">
        <div class="my-games-header">
          <div class="my-games-title-group">
            <span class="my-games-label">MY GAMES</span>
            <span class="my-games-counter">{{ gamesStore.games.length }}</span>
          </div>
          <div class="my-games-actions">
            <button
              class="btn-sidebar-icon"
              title="Add Custom Executable (.exe)"
              @click="gamesStore.browseAndAddCustom()"
            >
              +
            </button>
            <button
              class="btn-sidebar-icon"
              :disabled="gamesStore.isScanning"
              title="Rescan Installed Games"
              @click="gamesStore.scanGames()"
            >
              <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
                <polyline points="23 4 23 10 17 10"/>
                <polyline points="1 20 1 14 7 14"/>
                <path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"/>
              </svg>
            </button>
          </div>
        </div>

        <!-- Games Search Bar -->
        <div class="sidebar-search-box">
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/>
          </svg>
          <input
            v-model="gamesStore.searchQuery"
            type="text"
            placeholder="Filter games..."
            class="sidebar-search-input"
          />
        </div>

        <!-- Scanned Games List -->
        <div class="games-list-container">
          <div v-if="gamesStore.isScanning" class="games-loading-shimmer">
            <div v-for="i in 5" :key="i" class="game-item-skeleton"></div>
          </div>

          <div v-else-if="gamesStore.filteredGames.length === 0" class="games-empty-state">
            No games found.
          </div>

          <div
            v-else
            v-for="game in gamesStore.filteredGames"
            :key="game.id"
            class="game-list-item"
            :class="{
              active: gamesStore.selectedGameId === game.id,
              running: gamesStore.launchStatus === 'running' && gamesStore.selectedGameId === game.id
            }"
            @click="handleSelectGame(game.id)"
          >
            <div class="game-item-thumb">
              <img
                v-if="game.iconUrl && !failedIcons[game.id]"
                :src="game.iconUrl"
                :alt="game.name"
                loading="lazy"
                @error="failedIcons[game.id] = true"
              />
              <div v-else class="game-thumb-fallback">
                {{ game.name.substring(0, 1).toUpperCase() }}
              </div>
            </div>

            <div class="game-item-text-wrap">
              <span class="game-item-name" :title="game.name">
                {{ game.name }}
              </span>
              <span
                v-if="gamesStore.getGameMatchedBinary(game)"
                class="badge-game-mod"
                title="Assigned Cloud Mod Payload"
              >
                MOD
              </span>
            </div>

            <span
              v-if="gamesStore.launchStatus === 'running' && gamesStore.selectedGameId === game.id"
              class="game-playing-pulse"
              title="Playing Now"
            ></span>
          </div>
        </div>
      </div>

      <!-- Core Navigation / Advanced Tools -->
      <nav class="sidebar-nav sidebar-advanced-nav">
        <div class="nav-section-label">SYSTEM & TOOLS</div>

        <RouterLink to="/catalog" class="nav-item" active-class="active">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"/>
            <polyline points="3.27 6.96 12 12.01 20.73 6.96"/><line x1="12" y1="22.08" x2="12" y2="12"/>
          </svg>
          <span>Server Binaries</span>
          <span class="nav-badge">{{ binariesStore.binaries.length }}</span>
        </RouterLink>

        <RouterLink to="/processes" class="nav-item" active-class="active">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <rect x="4" y="4" width="16" height="16" rx="2"/><rect x="9" y="9" width="6" height="6"/>
            <line x1="9" y1="1" x2="9" y2="4"/><line x1="15" y1="1" x2="15" y2="4"/>
            <line x1="9" y1="20" x2="9" y2="23"/><line x1="15" y1="20" x2="15" y2="23"/>
          </svg>
          <span>Process Manager</span>
        </RouterLink>

        <RouterLink to="/diagnostics" class="nav-item" active-class="active">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <polyline points="4 17 10 11 4 5"/><line x1="12" y1="19" x2="20" y2="19"/>
          </svg>
          <span>Diagnostics</span>
        </RouterLink>
      </nav>
    </div>

    <!-- User Profile & Logout -->
    <div class="sidebar-footer">
      <div class="user-profile-box">
        <div class="user-avatar">{{ auth.avatarInitial }}</div>
        <div class="user-info">
          <div class="user-name-badge-row">
            <span class="user-name" :title="auth.displayName">{{ auth.displayName }}</span>
            <span class="user-badge-pro">PRO</span>
          </div>
          <span class="user-handle">{{ auth.displayHandle }}</span>
        </div>
      </div>

      <button class="btn-logout" @click="handleLogout" title="Sign out of account">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"/>
          <polyline points="16 17 21 12 16 7"/><line x1="21" y1="12" x2="9" y2="12"/>
        </svg>
      </button>
    </div>
  </aside>
</template>
