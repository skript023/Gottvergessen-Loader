<script setup lang="ts">
import { ref, computed } from 'vue';
import { useGamesStore } from '../../../stores/games';
import { useBinariesStore } from '../../../stores/binaries';
import { useAuthStore } from '../../../stores/auth';
import { useDiagnosticsStore } from '../../../stores/diagnostics';

const gamesStore = useGamesStore();
const binariesStore = useBinariesStore();
const auth = useAuthStore();
const diagnostics = useDiagnosticsStore();

const failedImage = ref(false);
const saveSuccess = ref(false);

const game = computed(() => gamesStore.selectedGame);
const binary = computed(() => gamesStore.matchedBinary);

const isBusy = computed(() => {
  return ['launching', 'waiting', 'injecting'].includes(gamesStore.launchStatus);
});

const isRunning = computed(() => gamesStore.launchStatus === 'running');

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
        <div class="game-hero-header">
          <div class="game-hero-avatar">
            <img 
              v-if="game.iconUrl && !failedImage" 
              :src="game.iconUrl" 
              :alt="game.name"
              @error="failedImage = true"
            />
            <div v-else class="game-hero-avatar-fallback">
              {{ game.name.substring(0, 2).toUpperCase() }}
            </div>
          </div>

          <div class="game-hero-meta">
            <div class="game-tags-row">
              <span class="badge-platform" :class="platformBadgeClass">
                {{ game.platform.toUpperCase() }}
              </span>
              <span v-if="game.appId" class="badge-appid">ID: {{ game.appId }}</span>
              <span v-if="game.isCustom" class="badge-custom">CUSTOM EXECUTABLE</span>
              <span v-if="binary" class="badge-server-linked" title="Payload linked from Ellohim Server">
                ⚡ SERVER MOD LINKED
              </span>
              <span v-else class="badge-unsupported" title="This game is not supported by Quantum Mod">
                🛡️ NOT SUPPORTED BY QUANTUM MOD
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

        <!-- Big Action Bar: Only shown when game has a linked mod payload -->
        <div v-if="binary" class="game-action-bar">
          <div class="play-btn-wrapper">
            <button
              class="btn-wand-play"
              :class="{
                'btn-busy': isBusy,
                'btn-running': isRunning,
                'btn-error': gamesStore.launchStatus === 'error'
              }"
              :disabled="isBusy"
              @click="handlePlay"
            >
              <!-- State: Busy / Launching -->
              <template v-if="isBusy">
                <div class="wand-spinner"></div>
                <div class="play-btn-text-block">
                  <span class="play-main-text">
                    {{ gamesStore.launchStatus === 'launching' ? 'LAUNCHING...' : 'INITIALIZING...' }}
                  </span>
                  <span class="play-sub-text">Connecting to Ellohim Core</span>
                </div>
              </template>

              <!-- State: Running & Injected -->
              <template v-else-if="isRunning">
                <svg width="22" height="22" viewBox="0 0 24 24" fill="currentColor">
                  <rect x="6" y="6" width="12" height="12" rx="2" />
                </svg>
                <div class="play-btn-text-block">
                  <span class="play-main-text">STOP GAME</span>
                  <span class="play-sub-text">PID: {{ gamesStore.runningPid }} • MOD ACTIVE</span>
                </div>
              </template>

              <!-- State: Error / Retry -->
              <template v-else-if="gamesStore.launchStatus === 'error'">
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
                    Launch & Inject {{ binary.file_name || 'DLL' }}
                  </span>
                </div>
              </template>
            </button>
          </div>

          <!-- Status Indicator Pill -->
          <div class="launch-status-pill" :class="`status-${gamesStore.launchStatus}`">
            <span class="status-indicator-dot"></span>
            <span class="status-indicator-text">
              {{ gamesStore.launchMessage || `Ready to inject ${binary.name} into ${gamesStore.customTargetProcess || game.exeName}` }}
            </span>
          </div>
        </div>
      </div>
    </div>

    <!-- Quantum Mod Unsupported Notice View (Matches Image 3) -->
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
            This game is <strong>not supported by Quantum Mod</strong>. This is due either to technical aspects that make it impractical to mod or to the possible multiplayer nature of the game.
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
          <span class="config-status-tag">AUTO-ATTACH</span>
        </div>

        <div class="config-card-body">
          <p class="config-desc">Executable process that loader monitors on startup:</p>
          <div class="input-with-action">
            <input
              v-model="gamesStore.customTargetProcess"
              type="text"
              class="wand-input"
              placeholder="e.g. Game.exe"
            />
            <button
              v-if="game.exeName && gamesStore.customTargetProcess !== game.exeName"
              class="btn-reset-target"
              @click="gamesStore.customTargetProcess = game.exeName"
              title="Reset to detected executable"
            >
              Reset
            </button>
          </div>
        </div>
      </div>

      <!-- Associated Binary / Mod Card -->
      <div class="config-card">
        <div class="config-card-header">
          <div class="config-title-wrap">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2" />
            </svg>
            <h3>Ellohim Server Payload</h3>
          </div>
          <span class="config-status-tag tag-ready">SYNCED</span>
        </div>

        <div class="config-card-body">
          <p class="config-desc">
            DLL payload from Ellohim backend
            <span class="binary-filename-tag">({{ binary.file_name || 'payload.dll' }})</span>:
          </p>
          <select
            :value="gamesStore.selectedBinaryId || ''"
            @change="gamesStore.onBinarySelected(($event.target as HTMLSelectElement).value || null)"
            class="wand-select"
          >
            <option :value="''">
              Auto-Matched ({{ binary.name }})
            </option>
            <option
              v-for="b in binariesStore.binaries"
              :key="b.id"
              :value="b.id"
            >
              {{ b.name || b.game || b.file_name }} (v{{ b.version || '1.0' }})
            </option>
          </select>
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
          <select v-model="gamesStore.selectedMode" class="wand-select">
            <option :value="2">Manual Map (Kernel/Stealth) [Recommended]</option>
            <option :value="1">Thread Hijack (Evasion)</option>
            <option :value="0">CreateRemoteThread (Standard)</option>
            <option :value="3">Reflective DLL Injection</option>
          </select>
        </div>
      </div>
    </div>

    <!-- Ellohim-Server Sync & License Bar: Only shown when binary is linked -->
    <div v-if="binary" class="server-sync-bar">
      <div class="server-sync-info">
        <div class="server-status-dot"></div>
        <span>Ellohim Server: <strong>https://apie.rena.my.id</strong></span>
        <span>•</span>
        <span>User: <strong>{{ auth.displayName }}</strong> ({{ auth.role || 'Member' }})</span>
        <span>•</span>
        <span>Assigned DLL: <strong style="color: #38bdf8;">{{ binary.file_name || 'payload.dll' }}</strong></span>
      </div>

      <div class="server-sync-action">
        <button
          class="btn-save-server"
          :disabled="gamesStore.isSavingServerConfig"
          @click="handleSaveToServer"
        >
          <span v-if="saveSuccess">✓ Saved to Ellohim Server!</span>
          <span v-else-if="gamesStore.isSavingServerConfig">Saving to Server...</span>
          <span v-else>Save Mapping to Ellohim Server</span>
        </button>
      </div>
    </div>

    <!-- Live Diagnostics Log Snippet -->
    <div class="game-log-panel">
      <div class="game-log-header">
        <span class="log-title">Live Engine Log</span>
        <span class="log-count">{{ diagnostics.logs.length }} events</span>
      </div>
      <div class="game-log-stream">
        <div v-if="diagnostics.logs.length === 0" class="log-empty">
          Engine initialized. Ready to launch {{ game.name }}.
        </div>
        <div
          v-for="(item, idx) in diagnostics.logs.slice(-5)"
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
