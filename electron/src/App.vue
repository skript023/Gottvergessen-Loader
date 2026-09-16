<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue';
import { useRoute } from 'vue-router';
import { useAuthStore } from './stores/auth';
import { useBinariesStore } from './stores/binaries';
import { useProcessStore } from './stores/process';
import { useInjectionStore } from './stores/injection';
import { useUpdaterStore } from './stores/updater';
import Sidebar from './components/Sidebar.vue';
import Header from './components/Header.vue';
import KickModal from './components/KickModal.vue';
import TitleBar from './components/TitleBar.vue';
import UpdateModal from './components/UpdateModal.vue';

const route = useRoute();
const auth = useAuthStore();
const binariesStore = useBinariesStore();
const processStore = useProcessStore();
const injection = useInjectionStore();
const updater = useUpdaterStore();

const isAuthRoute = computed(() => route.name === 'Login');

const isGlobalLoading = computed(() => {
  return auth.isLoading || binariesStore.isSyncing || processStore.isScanning || injection.isInjecting;
});

// Startup Splash & Update Phases: 'checking' | 'updating' | 'restoring' | 'ready'
type StartupPhase = 'checking' | 'updating' | 'restoring' | 'ready';
const startupPhase = ref<StartupPhase>('checking');
const startupStatusText = ref('Checking for client updates...');
const isStartupActive = computed(() => startupPhase.value !== 'ready');

async function proceedToSession() {
  startupPhase.value = 'restoring';
  startupStatusText.value = 'Verifying hardware identity & active device session...';
  updater.allowModal = true;

  if (auth.isRestoring) {
    await auth.restoreSession();
  }

  // Smooth visual transition delay
  await new Promise(r => setTimeout(r, 350));
  startupPhase.value = 'ready';
}

async function startStartupDownload() {
  startupPhase.value = 'updating';
  if (updater.isReadyToInstall) {
    startupStatusText.value = 'Update verified. Relaunching Astra...';
    setTimeout(() => updater.installUpdate(), 600);
    return;
  }
  startupStatusText.value = `Downloading update v${updater.latestVersion}...`;
  await updater.startDownload();
}

async function skipStartupUpdate() {
  if (updater.isMandatory) return;
  await updater.pauseDownload();
  await proceedToSession();
}

async function retryStartupUpdate() {
  await startStartupDownload();
}

// Watch updater state during startup phase
watch(() => updater.state, (state) => {
  if (startupPhase.value !== 'updating') return;

  if (state === 'verifying') {
    startupStatusText.value = 'Verifying SHA-256 integrity checksum...';
  } else if (state === 'ready-to-install') {
    startupStatusText.value = `Update verified. Relaunching into v${updater.latestVersion}...`;
    setTimeout(() => {
      updater.installUpdate();
    }, 600);
  } else if (state === 'error') {
    startupStatusText.value = updater.error || 'Update download failed.';
  } else if (state === 'downloading') {
    startupStatusText.value = `Downloading update package v${updater.latestVersion}...`;
  }
});

onMounted(async () => {
  updater.allowModal = false;
  updater.initListeners();

  try {
    startupPhase.value = 'checking';
    startupStatusText.value = 'Checking for client updates...';

    // Fast check to server with 4s network safety timeout
    const checkPromise = updater.checkForUpdates(true, false);
    const timeoutPromise = new Promise(resolve => setTimeout(() => resolve(null), 4000));
    const res = (await Promise.race([checkPromise, timeoutPromise])) as any;

    if (res && res.hasUpdate && res.latestVersion && !res.loopPrevented) {
      if (res.isMandatory) {
        await startStartupDownload();
        return; // Hold on splash until download finishes and app restarts
      } else {
        // Optional update: let user into the app, show update modal / banner
        updater.allowModal = true;
        updater.modalVisible = true;
      }
    }
  } catch (err) {
    console.warn('[AutoUpdater] Startup update check failed, proceeding to session:', err);
  }

  // If already up to date or check timed out, proceed to session verification
  await proceedToSession();
});
</script>

<template>
  <div class="app-root-shell">
    <!-- Windows 11 Custom TitleBar Strip (Ellohim-Explorer Style) -->
    <TitleBar />

    <div class="app-root-body">
      <!-- Global Neon Top Progress Bar (Active during any HTTP / Native task) -->
      <div v-if="isGlobalLoading" class="global-top-progress">
        <div class="global-top-bar-indeterminate"></div>
      </div>

  <!-- High-Tech Startup Splash Loader with Discord-Style Updater & Download Indicator -->
  <transition name="splash-fade">
    <div v-if="isStartupActive" class="startup-splash-overlay">
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
          <span class="splash-eyebrow">
            {{ startupPhase === 'updating'
                ? (updater.isMandatory ? 'CRITICAL SYSTEM GATE • v' + updater.latestVersion : 'QUANTUM AUTO-UPDATER • v' + updater.latestVersion)
                : 'QUANTUM SECURITY GATEWAY' }}
          </span>
          <h2 class="splash-title">
            {{ startupPhase === 'updating' ? 'Updating Astra' : 'Astra' }}
          </h2>

          <div class="splash-status-row">
            <span class="splash-pulsing-dot" :class="{ 'dot-amber': updater.isPaused, 'dot-danger': updater.state === 'error' }"></span>
            <span class="splash-status-text">{{ startupStatusText }}</span>
          </div>
        </div>

        <!-- Indeterminate Progress Track for Checking & Session Restoring -->
        <div v-if="startupPhase !== 'updating'" class="splash-progress-track">
          <div class="splash-progress-bar"></div>
        </div>

        <!-- Determinate Discord-Style Progress & Metrics for Updating -->
        <div v-else class="splash-updater-box">
          <div class="splash-progress-header">
            <span class="splash-metric-val">
              {{ updater.isVerifying ? 'Verifying Integrity' : (updater.isReadyToInstall ? 'Ready to Install' : 'Downloading') }}
            </span>
            <span class="splash-progress-percent font-mono">{{ updater.percent }}%</span>
          </div>

          <div class="splash-progress-track determinate">
            <div
              class="splash-progress-fill"
              :class="{
                'fill-paused': updater.isPaused,
                'fill-verifying': updater.isVerifying || updater.isReadyToInstall
              }"
              :style="{ width: `${updater.percent}%` }"
            ></div>
          </div>

          <div class="splash-metrics-row">
            <span class="splash-metric-numbers font-mono">
              <span>{{ updater.formattedDownloaded }}</span>
              <span>/</span>
              <span>{{ updater.formattedTotal }}</span>
            </span>
            <span v-if="!updater.isPaused && !updater.isVerifying" class="splash-metric-speed font-mono">
              {{ updater.formattedSpeed }}
            </span>
          </div>

          <!-- Error Alert with Retry & Launch Option -->
          <div v-if="updater.state === 'error'" class="splash-error-box">
            <span>{{ updater.error || 'Network connection failed during update download.' }}</span>
            <div style="display: flex; align-items: center; gap: 8px;">
              <button class="splash-retry-btn" @click="retryStartupUpdate">Retry Download</button>
              <button v-if="!updater.isMandatory" class="splash-skip-btn" @click="proceedToSession">Launch Anyway</button>
            </div>
          </div>

          <!-- Skip button for optional updates -->
          <button
            v-if="!updater.isMandatory && updater.state !== 'error' && !updater.isReadyToInstall"
            class="splash-skip-btn"
            @click="skipStartupUpdate"
          >
            Skip update & continue to Astra
          </button>
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

    <!-- Client Auto-Updater Resumable Modal -->
    <UpdateModal />
    </div>
  </div>
</template>
