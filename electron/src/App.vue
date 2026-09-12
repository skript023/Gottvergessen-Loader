<script setup lang="ts">
import { computed } from 'vue';
import { useRoute } from 'vue-router';
import { useAuthStore } from './stores/auth';
import { useBinariesStore } from './stores/binaries';
import { useProcessStore } from './stores/process';
import { useInjectionStore } from './stores/injection';
import Sidebar from './components/Sidebar.vue';
import Header from './components/Header.vue';
import KickModal from './components/KickModal.vue';

const route = useRoute();
const auth = useAuthStore();
const binariesStore = useBinariesStore();
const processStore = useProcessStore();
const injection = useInjectionStore();

const isAuthRoute = computed(() => route.name === 'Login');

const isGlobalLoading = computed(() => {
  return auth.isLoading || binariesStore.isSyncing || processStore.isScanning || injection.isInjecting;
});
</script>

<template>
  <!-- Global Neon Top Progress Bar (Active during any HTTP / Native task) -->
  <div v-if="isGlobalLoading" class="global-top-progress">
    <div class="global-top-bar-indeterminate"></div>
  </div>

  <!-- High-Tech Startup Splash Loader (Prevents blank screen flash on app launch) -->
  <transition name="splash-fade">
    <div v-if="auth.isRestoring" class="startup-splash-overlay">
      <div class="splash-backdrop-glow"></div>
      <div class="splash-card">
        <div class="splash-logo-wrap">
          <div class="splash-spinner-ring"></div>
          <div class="splash-logo-inner">
            <svg width="34" height="34" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5" stroke="url(#splash-cyan-grad)" />
              <defs>
                <linearGradient id="splash-cyan-grad" x1="2" y1="2" x2="22" y2="22" gradientUnits="userSpaceOnUse">
                  <stop stop-color="#38bdf8"/>
                  <stop offset="1" stop-color="#818cf8"/>
                </linearGradient>
              </defs>
            </svg>
          </div>
        </div>
        <div class="splash-info">
          <span class="splash-eyebrow">ELLOHIM SECURITY GATEWAY</span>
          <h2 class="splash-title">Gottvergessen Loader</h2>
          <div class="splash-status-row">
            <span class="splash-pulsing-dot"></span>
            <span class="splash-status-text">Verifying hardware identity & active device session...</span>
          </div>
        </div>
        <div class="splash-progress-track">
          <div class="splash-progress-bar"></div>
        </div>
      </div>
    </div>
  </transition>

  <!-- Main Application Router View with Smooth Fade Transitions -->
  <div v-if="isAuthRoute" class="auth-wrapper">
    <router-view v-slot="{ Component }">
      <transition name="page-fade" mode="out-in">
        <component :is="Component" />
      </transition>
    </router-view>
  </div>
  <div v-else class="app-layout">
    <Sidebar />
    <main class="main-wrapper">
      <Header />
      <div class="content-scroll">
        <router-view v-slot="{ Component }">
          <transition name="page-fade" mode="out-in">
            <component :is="Component" />
          </transition>
        </router-view>
      </div>
    </main>
  </div>

  <!-- Real-time Kick / Force-Logout Notification Modal -->
  <KickModal />
</template>
