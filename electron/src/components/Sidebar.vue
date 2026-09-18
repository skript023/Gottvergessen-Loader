<script setup lang="ts">
import { ref, computed, onMounted } from 'vue';
import brandLogo from '../../../src/logo.ico';
import { version } from '../../package.json';
import { useRouter } from 'vue-router';
import { useAuthStore } from '../stores/auth';
import { useBinariesStore } from '../stores/binaries';
import { useGamesStore } from '../stores/games';
import SidebarGameItem from './SidebarGameItem.vue';

const router = useRouter();
const auth = useAuthStore();
const binariesStore = useBinariesStore();
const gamesStore = useGamesStore();

const gamesCollapsed = ref(false);
const favoritesCollapsed = ref(false);

// Favorited games are listed only under FAVORITES.
const myGames = computed(() => gamesStore.games.filter((g) => !gamesStore.isFavorite(g.id)));
const myFilteredGames = computed(() => gamesStore.filteredGames.filter((g) => !gamesStore.isFavorite(g.id)));

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

</script>

<template>
  <aside class="sidebar wand-sidebar">
    <!-- Brand Header -->
    <div class="sidebar-brand">
      <div class="brand-badge-icon">
        <img :src="brandLogo" alt="" width="38" height="38" />
      </div>
      <div class="brand-text">
        <span class="brand-name">ASTRA</span>
        <span class="brand-version">DEVELOPMENT BUILD</span>
      </div>
    </div>

    <!-- Navigation Scrollable Area -->
    <div class="sidebar-scrollable">
      <!-- Main Overview Tab -->
      <div class="sidebar-pulse-wrap">
        <button
          class="pulse-nav-btn"
          :class="{ active: !gamesStore.selectedGameId && $route.path === '/dashboard' }"
          :aria-current="!gamesStore.selectedGameId && $route.path === '/dashboard' ? 'page' : undefined"
          @click="handleHomeClick"
        >
          <div class="pulse-icon-flame">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" aria-hidden="true">
              <rect x="3" y="3" width="7" height="7"/><rect x="14" y="3" width="7" height="7"/>
              <rect x="14" y="14" width="7" height="7"/><rect x="3" y="14" width="7" height="7"/>
            </svg>
          </div>
          <span class="pulse-nav-label">Control Hub</span>
        </button>
      </div>

      <!-- FAVORITES Section -->
      <div v-if="gamesStore.favoriteGames.length" class="my-games-section favorites-section">
        <div class="my-games-header">
          <button
            type="button"
            class="my-games-title-group my-games-toggle"
            :aria-expanded="!favoritesCollapsed"
            aria-controls="sidebar-favorites-list"
            :title="favoritesCollapsed ? 'Expand Favorites' : 'Minimize Favorites'"
            @click="favoritesCollapsed = !favoritesCollapsed"
          >
            <svg class="my-games-toggle-icon" :class="{ collapsed: favoritesCollapsed }" width="14" height="14" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true">
              <path d="M2 8h12" />
              <path v-if="favoritesCollapsed" d="M8 2v12" />
            </svg>
            <span class="my-games-label">FAVORITES</span>
            <span class="my-games-counter">{{ gamesStore.favoriteGames.length }}</span>
          </button>
        </div>

        <div v-show="!favoritesCollapsed" id="sidebar-favorites-list" class="games-list-container">
          <SidebarGameItem
            v-for="game in gamesStore.favoriteGames"
            :key="game.id"
            :game="game"
            @select="handleSelectGame"
          />
        </div>
      </div>

      <!-- MY GAMES Section -->
      <div class="my-games-section">
        <div class="my-games-header">
          <button
            type="button"
            class="my-games-title-group my-games-toggle"
            :aria-expanded="!gamesCollapsed"
            aria-controls="sidebar-games-search sidebar-games-list"
            :title="gamesCollapsed ? 'Expand My Games' : 'Minimize My Games'"
            @click="gamesCollapsed = !gamesCollapsed"
          >
            <svg class="my-games-toggle-icon" :class="{ collapsed: gamesCollapsed }" width="14" height="14" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true">
              <path d="M2 8h12" />
              <path v-if="gamesCollapsed" d="M8 2v12" />
            </svg>
            <span class="my-games-label">MY GAMES</span>
            <span class="my-games-counter">{{ myGames.length }}</span>
          </button>
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
        <div v-show="!gamesCollapsed" id="sidebar-games-search" class="sidebar-search-box">
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
        <div v-show="!gamesCollapsed" id="sidebar-games-list" class="games-list-container">
          <div v-if="gamesStore.isScanning" class="games-loading-shimmer">
            <div v-for="i in 5" :key="i" class="game-item-skeleton"></div>
          </div>

          <div v-else-if="myFilteredGames.length === 0" class="games-empty-state">
            No games found.
          </div>

          <SidebarGameItem
            v-else
            v-for="game in myFilteredGames"
            :key="game.id"
            :game="game"
            @select="handleSelectGame"
          />
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
  </aside>
</template>
