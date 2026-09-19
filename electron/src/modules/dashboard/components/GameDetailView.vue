<script setup lang="ts">
import { ref, computed } from 'vue';
import { useRouter } from 'vue-router';
import { useGamesStore } from '../../../stores/games';
import { useAuthStore } from '../../../stores/auth';
import { useDiagnosticsStore } from '../../../stores/diagnostics';
import ProgressHUD from '../../../components/ProgressHUD.vue';

const router = useRouter();
const gamesStore = useGamesStore();
const auth = useAuthStore();
const diagnostics = useDiagnosticsStore();

const saveSuccess = ref(false);

const game = computed(() => gamesStore.selectedGame);
const binary = computed(() => gamesStore.matchedBinary);

// Launch state belongs to one game; any other game page shows as idle.
const isActiveGame = computed(() => !!game.value && gamesStore.activeGameId === game.value.id);
const launchState = computed(() => (isActiveGame.value ? gamesStore.launchStatus : 'idle'));

const isBusy = computed(() => {
  return ['launching', 'waiting', 'injecting'].includes(launchState.value);
});

const isRunning = computed(() => launchState.value === 'running');

// Another game is launching or running, so this one can't start yet.
const otherGameActive = computed(() =>
  !isActiveGame.value && !!gamesStore.activeGameId && gamesStore.launchStatus !== 'idle' && gamesStore.launchStatus !== 'error'
);

const defaultTarget = computed(() => {
  return binary.value?.target_process || (binary.value as any)?.target || game.value?.exeName || '';
});

const platformBadgeClass = computed(() => {
  switch (game.value?.platform) {
    case 'steam': return 'platform-steam';
    case 'epic': return 'platform-epic';
    default: return 'platform-custom';
  }
});

function handlePlay() {
  if (isRunning.value) {
    gamesStore.stopGame();
  } else {
    gamesStore.launchAndInject();
  }
}

async function handleSaveToServer() {
  if (!binary.value) return;
  const proc = gamesStore.customTargetProcess || game.value?.exeName || '';
  const mode = gamesStore.selectedMode;
  const ok = await gamesStore.saveGameMappingToServer(binary.value.id, proc, mode);
  if (ok) {
    saveSuccess.value = true;
    setTimeout(() => {
      saveSuccess.value = false;
    }, 2500);
  }
}

function handleRemoveCustom() {
  if (!game.value || !game.value.isCustom) return;
  const conf = confirm(`Remove "${game.value.name}" from your games list?`);
  if (conf) {
    gamesStore.removeCustomGame(game.value.id);
    router.replace({ path: '/dashboard' });
  }
}
</script>

<template>
  <div v-if="game" class="game-detail-container">
    <!-- Cinematic Hero Banner with Game Artwork & Gradients -->
    <div class="game-hero-banner">
      <div 
        class="game-hero-backdrop" 
        :style="{ backgroundImage: game.bannerUrl ? `url(${game.bannerUrl})` : undefined }"
      ></div>
      <div class="game-hero-overlay"></div>

      <div class="game-hero-content">
        <button
          type="button"
          class="btn-hero-favorite"
          :class="{ active: gamesStore.isFavorite(game.id) }"
          :title="gamesStore.isFavorite(game.id) ? 'Remove from Favorites' : 'Add to Favorites'"
          :aria-pressed="gamesStore.isFavorite(game.id)"
          @click="gamesStore.toggleFavorite(game.id)"
        >
          <svg width="18" height="18" viewBox="0 0 24 24" :fill="gamesStore.isFavorite(game.id) ? 'currentColor' : 'none'" stroke="currentColor" stroke-width="2" stroke-linejoin="round">
            <polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2" />
          </svg>
        </button>
        <div class="game-hero-header">

          <div class="game-hero-meta">
            <div class="game-tags-row">
              <span class="badge-platform" :class="platformBadgeClass">
                {{ game.platform.toUpperCase() }}
              </span>
              <span v-if="game.appId" class="badge-appid">ID: {{ game.appId }}</span>
              <span v-if="game.isCustom" class="badge-custom">CUSTOM EXECUTABLE</span>
              <span v-if="binary" class="badge-server-linked" title="Payload linked from Cloud Security Network">
                CLOUD MOD LINKED
              </span>
              <span v-else class="badge-unsupported" title="This game is not supported by Astra">
                NOT SUPPORTED BY ASTRA
              </span>
              <span v-if="isRunning" class="badge-running-pulse">
                <span class="pulse-dot-green"></span>
                ACTIVE
              </span>
            </div>

            <h1 class="game-hero-title">{{ game.name }}</h1>
            <p class="game-hero-path" :title="game.installDir">
              📁 {{ game.installDir || 'Custom Target Directory' }}
            </p>
          </div>

          <div v-if="game.isCustom" class="game-hero-actions-top">
            <button class="btn-remove-custom" @click="handleRemoveCustom" title="Remove custom game">
              Remove
            </button>
          </div>
        </div>

        <!-- Every game can launch; only linked games execute payload injection. -->
        <div class="game-action-bar">
          <div class="play-btn-wrapper">
            <button
              class="btn-wand-play"
              :class="{
                'btn-busy': isBusy,
                'btn-running': isRunning,
                'btn-error': launchState === 'error'
              }"
              :disabled="isBusy || otherGameActive"
              :title="otherGameActive ? 'Stop the running game first' : undefined"
              @click="handlePlay"
            >
              <!-- State: Busy / Launching -->
              <template v-if="isBusy">
                <div class="wand-spinner"></div>
                <div class="play-btn-text-block">
                  <span class="play-main-text">
                    {{ launchState === 'launching' ? 'LAUNCHING...' : 'INITIALIZING...' }}
                  </span>
                  <span class="play-sub-text">
                    {{ binary ? 'Connecting to Mod Core' : 'Starting without injection' }}
                  </span>
                </div>
              </template>

              <!-- State: Running & Injected -->
              <template v-else-if="isRunning">
                <svg width="22" height="22" viewBox="0 0 24 24" fill="currentColor">
                  <rect x="6" y="6" width="12" height="12" rx="2" />
                </svg>
                <div class="play-btn-text-block">
                  <span class="play-main-text">STOP GAME</span>
                  <span class="play-sub-text">
                    PID: {{ gamesStore.runningPid }} • {{ binary ? 'MOD ACTIVE' : 'NO INJECTION' }}
                  </span>
                </div>
              </template>

              <!-- State: Error / Retry -->
              <template v-else-if="launchState === 'error'">
                <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
                  <circle cx="12" cy="12" r="10" />
                  <line x1="12" y1="8" x2="12" y2="12" />
                  <line x1="12" y1="16" x2="12.01" y2="16" />
                </svg>
                <div class="play-btn-text-block">
                  <span class="play-main-text">RETRY LAUNCH</span>
                  <span class="play-sub-text">Click to launch again</span>
                </div>
              </template>

              <!-- State: Ready to Play -->
              <template v-else>
                <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
                  <polygon points="5 3 19 12 5 21 5 3" />
                </svg>
                <div class="play-btn-text-block">
                  <span class="play-main-text">PLAY</span>
                  <span class="play-sub-text">
                    {{ binary ? `Launch & Inject ${binary.file_name || 'DLL'}` : 'Launch game without injection' }}
                  </span>
                </div>
              </template>
            </button>
          </div>

          <!-- Status Indicator Pill -->
          <div class="launch-status-pill" :class="`status-${launchState}`">
            <span class="status-indicator-dot"></span>
            <span class="status-indicator-text">
              {{
                (isActiveGame && gamesStore.launchMessage) ||
                (binary
                  ? `Ready to inject ${binary.name} into ${gamesStore.customTargetProcess || game.exeName}`
                  : 'Unsupported by Astra — launch only, no injection')
              }}
            </span>
          </div>
        </div>
      </div>
    </div>

    <ProgressHUD v-if="isActiveGame" />

    <!-- Astra Unsupported Notice View (Matches Image 3) -->
    <div v-if="!binary" class="unsupported-mod-view">
      <div class="unsupported-notice-card">
        <div class="unsupported-card-header">
          <div class="unsupported-header-left">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M12 20h9"/>
              <path d="M16.5 3.5a2.121 2.121 0 0 1 3 3L7 19l-4 1 1-4L16.5 3.5z"/>
            </svg>
            <h3 class="unsupported-title">Customize</h3>
          </div>
          <span class="badge-unsupported-tag">NOT SUPPORTED</span>
        </div>
        <div class="unsupported-card-body">
          <p class="unsupported-message-text">
            This game is <strong>not supported by Astra</strong>. You can
            still launch it normally, but no payload will be downloaded or injected.
          </p>
        </div>
      </div>
    </div>

    <!-- Mod & Injection Configuration Grid: Only shown when mod is supported -->
    <div v-if="binary" class="game-config-grid">
      <!-- Target Process Card -->
      <div class="config-card">
        <div class="config-card-header">
          <div class="config-title-wrap">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <circle cx="12" cy="12" r="10" />
              <line x1="22" y1="12" x2="18" y2="12" />
              <line x1="6" y1="12" x2="2" y2="12" />
              <line x1="12" y1="6" x2="12" y2="2" />
              <line x1="12" y1="22" x2="12" y2="18" />
            </svg>
            <h3>Target Process</h3>
          </div>
          <span v-if="isRunning && gamesStore.runningPid" class="config-status-tag tag-ready">
            LOCKED (PID: {{ gamesStore.runningPid }})
          </span>
          <span v-else class="config-status-tag">AUTO-ATTACH</span>
        </div>

        <div class="config-card-body">
          <p class="config-desc">Executable process that Astra monitors on startup:</p>
          <div class="input-with-action">
            <input
              v-model="gamesStore.customTargetProcess"
              type="text"
              class="wand-input"
              :placeholder="defaultTarget || 'e.g. Game.exe'"
              :disabled="isBusy || isRunning"
            />
            <button
              v-if="defaultTarget && gamesStore.customTargetProcess !== defaultTarget && !isRunning"
              class="btn-reset-target"
              @click="gamesStore.customTargetProcess = defaultTarget"
              title="Reset to recommended target process"
            >
              Reset
            </button>
          </div>
        </div>
      </div>

      <!-- Injection Engine Mode Card -->
      <div class="config-card">
        <div class="config-card-header">
          <div class="config-title-wrap">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z" />
            </svg>
            <h3>Injection Mode</h3>
          </div>
          <span class="config-status-tag">STEALTH</span>
        </div>

        <div class="config-card-body">
          <p class="config-desc">Execution hook method for target process:</p>
          <select v-model="gamesStore.selectedMode" class="wand-select" :disabled="isBusy || isRunning">
            <option :value="0">CreateRemoteThread (Standard) [Recommended]</option>
            <option :value="1">Thread Hijack + Handle Hijack</option>
            <option :value="2">QueueUserAPC + Handle Hijack</option>
            <option :value="3">Reflective DLL Injection</option>
          </select>
        </div>
      </div>
    </div>

    <!-- Cloud Sync & License Bar: Only shown when binary is linked -->
    <div v-if="binary" class="server-sync-bar">
      <div class="server-sync-info">
        <div class="server-status-dot"></div>
        <span>Cloud Network: <strong>Encrypted & Connected</strong></span>
        <span>•</span>
        <span>User: <strong>{{ auth.displayName }}</strong> ({{ auth.role || 'Member' }})</span>
        <span>•</span>
        <span>Assigned DLL: <strong style="color: #29b6f6;">{{ binary.file_name || 'payload.dll' }}</strong></span>
      </div>

      <div class="server-sync-action">
        <button
          class="btn-save-server"
          :disabled="gamesStore.isSavingServerConfig || isBusy || isRunning"
          @click="handleSaveToServer"
        >
          <span v-if="saveSuccess">✓ Saved to Cloud!</span>
          <span v-else-if="gamesStore.isSavingServerConfig">Saving to Cloud...</span>
          <span v-else>Save Mapping to Cloud</span>
        </button>
      </div>
    </div>

    <!-- Live Diagnostics Log Snippet -->
    <div class="game-log-panel">
      <div class="game-log-header">
        <span class="log-title">Live Engine Log</span>
        <div class="log-header-actions">
          <span class="log-count">{{ diagnostics.logs.length }} events</span>
          <button class="btn-log-open" title="Open Diagnostics" @click="router.push('/diagnostics')">
            <span>View all</span>
            <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2">
              <path d="M5 12h14M13 6l6 6-6 6" />
            </svg>
          </button>
        </div>
      </div>
      <div class="game-log-stream">
        <div v-if="diagnostics.logs.length === 0" class="log-empty">
          Engine initialized. Ready to launch {{ game.name }}.
        </div>
        <div
          v-for="(item, idx) in diagnostics.logs.slice(-10)"
          :key="idx"
          class="log-row"
          :class="`log-${item.type}`"
        >
          <span class="log-time">{{ item.time }}</span>
          <span class="log-msg">{{ item.message }}</span>
        </div>
      </div>
    </div>
  </div>
</template>
