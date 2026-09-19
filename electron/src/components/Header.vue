<script setup lang="ts">
import { nextTick, onBeforeUnmount, onMounted, ref } from 'vue';
import { useRouter } from 'vue-router';
import { useAuthStore } from '../stores/auth';
import SearchPalette from './SearchPalette.vue';

const router = useRouter();
const auth = useAuthStore();

const isLogoutPromptOpen = ref(false);
const isSearchOpen = ref(false);

// Gamer Profile drop-down. It is teleported and positioned from the pill's
// rect because the header's backdrop-filter and the layout's overflow would
// otherwise clip a menu that hangs below the bar.
const PROFILE_MENU_WIDTH = 248;
const profilePill = ref<HTMLElement | null>(null);
const profileMenu = ref<HTMLElement | null>(null);
const isProfileMenuOpen = ref(false);
const profileMenuStyle = ref({ top: '0px', left: '0px', width: `${PROFILE_MENU_WIDTH}px` });

function positionProfileMenu() {
  const pill = profilePill.value;
  if (!pill) return;
  const rect = pill.getBoundingClientRect();
  const maxLeft = window.innerWidth - PROFILE_MENU_WIDTH - 12;
  profileMenuStyle.value = {
    top: `${rect.bottom + 8}px`,
    left: `${Math.max(12, Math.min(rect.right - PROFILE_MENU_WIDTH, maxLeft))}px`,
    width: `${PROFILE_MENU_WIDTH}px`
  };
}

function toggleProfileMenu() {
  if (isProfileMenuOpen.value) {
    isProfileMenuOpen.value = false;
    return;
  }
  positionProfileMenu();
  isProfileMenuOpen.value = true;
}

function closeProfileMenu() {
  isProfileMenuOpen.value = false;
}

function handleProfileMenuPointerDown(event: MouseEvent) {
  if (!isProfileMenuOpen.value) return;
  const target = event.target as Node | null;
  if (!target) return;
  // The pill toggles itself; a press anywhere else outside the menu closes it.
  if (profilePill.value?.contains(target) || profileMenu.value?.contains(target)) return;
  closeProfileMenu();
}

function handleProfileMenuKeydown(event: KeyboardEvent) {
  if (event.key === 'Escape') closeProfileMenu();
}

// vue-router records the neighbouring entries on history.state, so the arrows
// can reflect what is actually reachable instead of always looking enabled.
const canGoBack = ref(false);
const canGoForward = ref(false);

function isDeadEnd(entry: unknown) {
  // Sign-in entries bounce straight back through the route guard once a
  // session is active, so treat them as no destination at all.
  return !entry || String(entry).startsWith('/login');
}

function syncHistoryArrows() {
  const state = (window.history.state || {}) as { back?: string | null; forward?: string | null };
  canGoBack.value = !isDeadEnd(state.back);
  canGoForward.value = !isDeadEnd(state.forward);
}

let stopAfterEach: (() => void) | null = null;

onMounted(() => {
  syncHistoryArrows();
  // history.state is swapped as part of the navigation, so read it afterwards.
  stopAfterEach = router.afterEach(() => {
    closeProfileMenu();
    isSearchOpen.value = false;
    nextTick(syncHistoryArrows);
  });
  document.addEventListener('mousedown', handleProfileMenuPointerDown);
  document.addEventListener('keydown', handleProfileMenuKeydown);
  window.addEventListener('resize', closeProfileMenu);
});

onBeforeUnmount(() => {
  stopAfterEach?.();
  stopAfterEach = null;
  document.removeEventListener('mousedown', handleProfileMenuPointerDown);
  document.removeEventListener('keydown', handleProfileMenuKeydown);
  window.removeEventListener('resize', closeProfileMenu);
});

function handleBack() {
  if (!canGoBack.value) return;
  router.back();
}

function handleForward() {
  if (!canGoForward.value) return;
  router.forward();
}

function handleHome() {
  router.push({ path: '/dashboard' });
}

function handleLogout() {
  // A native confirm() opens a modal child window; on this frameless window the
  // renderer never gets mouse focus back, leaving the login inputs unclickable.
  isLogoutPromptOpen.value = true;
}

function handleProfileMenuHome() {
  closeProfileMenu();
  handleHome();
}

function handleProfileMenuLogout() {
  closeProfileMenu();
  handleLogout();
}

async function confirmLogout() {
  isLogoutPromptOpen.value = false;
  await auth.logout();
  router.replace('/login');
}
</script>

<template>
  <header class="top-bar wand-top-bar">
    <!-- Left: Navigation Controls (Steam/Wand style) -->
    <div class="top-nav-controls">
      <button class="nav-arrow-btn" :disabled="!canGoBack" @click="handleBack" title="Back">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
          <polyline points="15 18 9 12 15 6" />
        </svg>
      </button>
      <button class="nav-arrow-btn" :disabled="!canGoForward" @click="handleForward" title="Forward">
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

    <!-- Center: Search Pill (Wand style). It only opens the search pop-up;
         the real input lives there. -->
    <div class="top-search-container">
      <button
        type="button"
        class="wand-search-pill"
        :class="{ 'is-open': isSearchOpen }"
        aria-haspopup="dialog"
        :aria-expanded="isSearchOpen"
        @click="isSearchOpen = true"
      >
        <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/>
        </svg>
        <span class="wand-search-placeholder">Search</span>
      </button>
    </div>

    <!-- Right: System Status & User PRO Badge -->
    <div class="top-bar-right">
      <!-- Wand User Profile Pill in Header: opens the profile menu. -->
      <div
        ref="profilePill"
        class="header-profile-pill"
        :class="{ 'is-open': isProfileMenuOpen }"
        role="button"
        tabindex="0"
        aria-haspopup="menu"
        :aria-expanded="isProfileMenuOpen"
        title="Gamer Profile"
        @click="toggleProfileMenu"
        @keydown.enter.prevent="toggleProfileMenu"
        @keydown.space.prevent="toggleProfileMenu"
      >
        <div class="header-avatar">{{ auth.avatarInitial }}</div>
        <span class="header-username">{{ auth.displayName }}</span>
        <span class="header-pro-badge">PRO</span>
        <svg class="header-profile-caret" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
          <polyline points="6 9 12 15 18 9" />
        </svg>
      </div>
    </div>

    <SearchPalette v-model:open="isSearchOpen" />

    <!-- Gamer Profile menu. Same card language as the other pop-ups, but
         anchored to the pill and without a blurred backdrop. -->
    <Teleport to="body">
      <transition name="menu-pop">
        <div v-if="isProfileMenuOpen" class="profile-menu-layer">
          <!-- Flat dim behind the menu: same tone as the pop-up overlays,
               without their blur. -->
          <div class="profile-menu-scrim" @click="closeProfileMenu"></div>

          <div
            ref="profileMenu"
            class="profile-menu"
            :style="profileMenuStyle"
            role="menu"
            aria-label="Gamer Profile"
          >
            <button class="profile-menu-item" role="menuitem" @click="handleProfileMenuHome">
              <span class="profile-menu-avatar">{{ auth.avatarInitial }}</span>
              <span class="profile-menu-label">My Profile</span>
            </button>

            <div class="profile-menu-sep"></div>

            <button class="profile-menu-item" role="menuitem" disabled title="Coming soon">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <circle cx="12" cy="12" r="10"/>
                <path d="M9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3"/><line x1="12" y1="17" x2="12.01" y2="17"/>
              </svg>
              <span class="profile-menu-label">Help</span>
              <svg class="profile-menu-ext" width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
                <path d="M7 17L17 7M9 7h8v8"/>
              </svg>
            </button>

            <button class="profile-menu-item" role="menuitem" disabled title="Coming soon">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M8.5 8.8a10 10 0 0 1 7 0"/>
                <path d="M15.5 16.2a10 10 0 0 1-7 0"/>
                <path d="M15.8 4.6A15 15 0 0 1 20 7.4c1.2 3.6 1.3 7.4 0 11a14 14 0 0 1-3.6 1.8l-1-1.7"/>
                <path d="M8.2 4.6A15 15 0 0 0 4 7.4c-1.2 3.6-1.3 7.4 0 11a14 14 0 0 0 3.6 1.8l1-1.7"/>
                <ellipse cx="9.3" cy="12.4" rx="1.4" ry="1.7"/>
                <ellipse cx="14.7" cy="12.4" rx="1.4" ry="1.7"/>
              </svg>
              <span class="profile-menu-label">Discord</span>
              <svg class="profile-menu-ext" width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
                <path d="M7 17L17 7M9 7h8v8"/>
              </svg>
            </button>

            <button class="profile-menu-item" role="menuitem" disabled title="Coming soon">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M21 11.5a8.38 8.38 0 0 1-9 8.4 8.5 8.5 0 0 1-3.8-.9L3 21l2-4.9A8.38 8.38 0 0 1 12 3a8.5 8.5 0 0 1 9 8.5z"/>
              </svg>
              <span class="profile-menu-label">Share feedback</span>
            </button>

            <div class="profile-menu-sep"></div>

            <button class="profile-menu-item" role="menuitem" disabled title="Coming soon">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <circle cx="12" cy="12" r="3"/>
                <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06A1.65 1.65 0 0 0 4.6 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06A1.65 1.65 0 0 0 9 4.6a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06A1.65 1.65 0 0 0 19.4 9v.09a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/>
              </svg>
              <span class="profile-menu-label">Settings</span>
            </button>

            <div class="profile-menu-sep"></div>

            <button class="profile-menu-item is-danger" role="menuitem" @click="handleProfileMenuLogout">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"/>
                <polyline points="16 17 21 12 16 7"/><line x1="21" y1="12" x2="9" y2="12"/>
              </svg>
              <span class="profile-menu-label">Sign Out</span>
            </button>
  </div>
        </div>
      </transition>
    </Teleport>

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
