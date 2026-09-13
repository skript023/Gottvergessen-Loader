<script setup lang="ts">
import { useRouter } from 'vue-router';
import { useAuthStore } from '../stores/auth';
import { useGamesStore } from '../stores/games';

const router = useRouter();
const auth = useAuthStore();
const gamesStore = useGamesStore();

function handleBack() {
  window.history.back();
}

function handleForward() {
  window.history.forward();
}

function handleHome() {
  gamesStore.selectGame(null);
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
  <header class="top-bar wand-top-bar">
    <!-- Left: Navigation Controls (Steam/Wand style) -->
    <div class="top-nav-controls">
      <button class="nav-arrow-btn" @click="handleBack" title="Back">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
          <polyline points="15 18 9 12 15 6" />
        </svg>
      </button>
      <button class="nav-arrow-btn" @click="handleForward" title="Forward">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
          <polyline points="9 18 15 12 9 6" />
        </svg>
      </button>
      <button class="nav-home-btn" @click="handleHome" title="Pulse Hub">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z" />
          <polyline points="9 22 9 12 15 12 15 22" />
        </svg>
      </button>
    </div>

    <!-- Center: Search Pill (Wand style) -->
    <div class="top-search-container">
      <div class="wand-search-pill">
        <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/>
        </svg>
        <input
          v-model="gamesStore.searchQuery"
          type="text"
          placeholder="Search games, mods, or processes..."
          class="wand-search-input"
        />
        <span class="search-hotkey">⌘K</span>
      </div>
    </div>

    <!-- Right: System Status & User PRO Badge -->
    <div class="top-bar-right">
      <div class="system-pills">
        <div class="sys-pill pill-online">
          <span class="pulse-dot"></span>
          <span>ONLINE</span>
        </div>
        <div class="sys-pill pill-secure">
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>
          </svg>
          <span>STEALTH</span>
        </div>
      </div>

      <!-- Wand User Profile Pill in Header -->
      <div class="header-profile-pill" @click="handleHome" title="Gamer Profile">
        <div class="header-avatar">{{ auth.avatarInitial }}</div>
        <span class="header-username">{{ auth.displayName }}</span>
        <span class="header-pro-badge">PRO</span>
      </div>

      <button class="btn-header-logout" @click="handleLogout" title="Sign Out">
        <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"/>
          <polyline points="16 17 21 12 16 7"/><line x1="21" y1="12" x2="9" y2="12"/>
        </svg>
      </button>
    </div>
  </header>
</template>
