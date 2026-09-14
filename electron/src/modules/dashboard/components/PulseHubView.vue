<script setup lang="ts">
import { ref, computed } from 'vue';
import { useAuthStore } from '../../../stores/auth';
import { useGamesStore } from '../../../stores/games';
import { useBinariesStore } from '../../../stores/binaries';

const auth = useAuthStore();
const gamesStore = useGamesStore();
const binariesStore = useBinariesStore();

const failedImages = ref<Record<string, boolean>>({});

const topGames = computed(() => {
  return gamesStore.games.slice(0, 8);
});

function selectGame(id: string) {
  gamesStore.selectGame(id);
}
</script>

<template>
  <div class="pulse-hub-container">
    <!-- User Hero Card (Gottvergessen Panoramic Aesthetic) -->
    <div class="wand-hero-card">
      <div class="wand-hero-glow"></div>
      <div class="wand-hero-inner">
        <div class="hero-profile-row">
          <div class="hero-avatar-box">
            <div class="hero-avatar">{{ auth.avatarInitial }}</div>
            <div class="hero-online-badge"></div>
          </div>

          <div class="hero-identity">
            <div class="hero-name-row">
              <span class="hero-username">{{ auth.displayName || 'Operative' }}</span>
              <span class="hero-pro-badge">PRO</span>
            </div>
            <span class="hero-handle">{{ auth.displayHandle }} • {{ auth.role || 'Member' }}</span>
          </div>
        </div>

        <!-- Stats Grid with Real Ellohim-Server Info -->
        <div class="hero-stats-row">
          <div class="hero-stat-col">
            <span class="stat-label">License Status</span>
            <span class="stat-value highlight-cyan">{{ auth.role.toLowerCase().includes('admin') ? 'Unlimited Master' : 'Active License' }}</span>
          </div>
          <div class="hero-stat-col">
            <span class="stat-label">Server Binaries</span>
            <span class="stat-value highlight-cyan">{{ binariesStore.binaries.length }} Authorized</span>
          </div>
          <div class="hero-stat-col">
            <span class="stat-label">Installed Games</span>
            <span class="stat-value">{{ gamesStore.games.length }} Detected</span>
          </div>
          <div class="hero-stat-col">
            <span class="stat-label">Device HWID</span>
            <span class="stat-value highlight-emerald">Locked</span>
          </div>
        </div>
      </div>
    </div>

    <!-- Achievements & Badges Showcase -->
    <div class="wand-section">
      <div class="section-title-row">
        <h3 class="section-title">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <circle cx="12" cy="8" r="7" />
            <polyline points="8.21 13.89 7 23 12 20 17 23 15.79 13.88" />
          </svg>
          Operative Credentials
        </h3>
        <span class="section-count">4 Badges Active</span>
      </div>

      <div class="achievements-row">
        <div class="badge-gem-item">
          <div class="badge-gem gem-blue">
            <div class="gem-glint"></div>
            <svg width="30" height="30" viewBox="0 0 24 24" fill="currentColor">
              <path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5" />
            </svg>
          </div>
          <span class="badge-gem-label">QUANTUM CORE</span>
        </div>

        <div class="badge-gem-item">
          <div class="badge-gem gem-cyan">
            <div class="gem-glint"></div>
            <span class="gem-text-icon">G</span>
          </div>
          <span class="badge-gem-label">GOTTVERGESSEN</span>
        </div>

        <div class="badge-gem-item">
          <div class="badge-gem gem-crystal">
            <div class="gem-glint"></div>
            <div class="crystal-inner">
              <span class="crystal-number">x64</span>
              <span class="crystal-sub">KERNEL</span>
            </div>
          </div>
          <span class="badge-gem-label">STEALTH MAPPER</span>
        </div>

        <div class="badge-gem-item">
          <div class="badge-gem gem-emerald">
            <div class="gem-glint"></div>
            <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z" />
            </svg>
          </div>
          <span class="badge-gem-label">MEMORY CLOAK</span>
        </div>
      </div>
    </div>

    <!-- Quick Play / Installed Games Spotlight -->
    <div class="wand-section">
      <div class="section-title-row">
        <h3 class="section-title">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <rect x="2" y="3" width="20" height="14" rx="2" ry="2"/>
            <line x1="8" y1="21" x2="16" y2="21"/><line x1="12" y1="17" x2="12" y2="21"/>
          </svg>
          Installed Game Library
        </h3>
        <span class="section-count">{{ gamesStore.games.length }} Games Detected</span>
      </div>

      <div class="quick-games-grid">
        <div
          v-for="g in topGames"
          :key="g.id"
          class="quick-game-card"
          @click="selectGame(g.id)"
        >
          <div
            class="quick-game-cover"
            :style="{ backgroundImage: (g.bannerUrl && !failedImages[g.id]) ? `url(${g.bannerUrl})` : undefined }"
          >
            <div class="quick-game-platform-tag">{{ g.platform.toUpperCase() }}</div>
            <div v-if="gamesStore.getGameMatchedBinary(g)" class="quick-game-mod-tag">
              SERVER MOD
            </div>
            <div class="quick-game-play-hover">
              <svg width="32" height="32" viewBox="0 0 24 24" fill="currentColor">
                <polygon points="5 3 19 12 5 21 5 3" />
              </svg>
            </div>
          </div>
          <div class="quick-game-info">
            <h4 class="quick-game-title">{{ g.name }}</h4>
            <span class="quick-game-exe">{{ g.exeName || 'Target executable' }}</span>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>
