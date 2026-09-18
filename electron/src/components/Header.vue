<script setup lang="ts">
import { ref } from 'vue';
import { useRouter } from 'vue-router';
import { useAuthStore } from '../stores/auth';
import { useGamesStore } from '../stores/games';

const router = useRouter();
const auth = useAuthStore();
const gamesStore = useGamesStore();

const isLogoutPromptOpen = ref(false);

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

function handleLogout() {
  // A native confirm() opens a modal child window; on this frameless window the
  // renderer never gets mouse focus back, leaving the login inputs unclickable.
  isLogoutPromptOpen.value = true;
}

async function confirmLogout() {
  isLogoutPromptOpen.value = false;
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
      </div>
    </div>

    <!-- Right: System Status & User PRO Badge -->
    <div class="top-bar-right">
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

    <!-- In-app sign-out confirmation (replaces native confirm dialog).
         Teleported to <body>: the header's own backdrop-filter makes it a
         containing block, which would otherwise clip this fixed overlay to
         the titlebar strip. -->
    <Teleport to="body">
      <div v-if="isLogoutPromptOpen" class="logout-prompt-overlay" @click.self="isLogoutPromptOpen = false">
        <div class="logout-prompt-box">
          <h3 class="logout-prompt-title">Sign out?</h3>
          <p class="logout-prompt-sub">Your device session will be cleared and you will return to the sign-in screen.</p>
          <div class="logout-prompt-actions">
            <button class="logout-prompt-cancel" @click="isLogoutPromptOpen = false">Cancel</button>
            <button class="logout-prompt-confirm" @click="confirmLogout">Sign Out</button>
          </div>
        </div>
      </div>
    </Teleport>
  </header>
</template>

<style scoped>
.logout-prompt-overlay {
  position: fixed;
  inset: 0;
  z-index: 9999;
  background: rgba(12, 13, 14, 0.88);
  backdrop-filter: blur(16px);
  display: grid;
  place-items: center;
  padding: 20px;
  -webkit-app-region: no-drag;
}

.logout-prompt-box {
  width: min(380px, 95vw);
  background: rgba(27, 28, 30, 0.96);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-panel);
  padding: 24px;
  text-align: center;
}

.logout-prompt-title {
  font-size: 17px;
  font-weight: 700;
  color: #fff;
  margin-bottom: 8px;
}

.logout-prompt-sub {
  font-size: 12.5px;
  color: #a1a2a6;
  line-height: 1.5;
  margin-bottom: 20px;
}

.logout-prompt-actions {
  display: flex;
  gap: 10px;
}

.logout-prompt-actions button {
  flex: 1;
  padding: 11px 16px;
  border-radius: 12px;
  font-size: 12.5px;
  font-weight: 700;
  cursor: pointer;
  transition: all 0.15s ease;
}

.logout-prompt-cancel {
  background: transparent;
  border: 1px solid var(--border-subtle);
  color: #c5c6c8;
}

.logout-prompt-cancel:hover {
  background: rgba(255, 255, 255, 0.06);
  color: #fff;
}

.logout-prompt-confirm {
  background: #ededee;
  border: none;
  color: #202123;
}

.logout-prompt-confirm:hover {
  filter: brightness(1.1);
}
</style>
