<script setup lang="ts">
import { useUpdaterStore } from '../stores/updater';

const updater = useUpdaterStore();
</script>

<template>
  <transition name="modal-fade">
    <div v-if="updater.modalVisible" class="update-modal-overlay">
      <div class="update-modal-backdrop" @click="updater.dismissModal"></div>

      <div class="update-modal-card" :class="{ 'is-mandatory': updater.isMandatory }">
        <!-- Ambient Glow -->
        <div class="modal-glow" :class="{ 'glow-danger': updater.isMandatory }"></div>

        <!-- Modal Header -->
        <div class="modal-header">
          <div class="header-icon-wrap" :class="{ 'icon-danger': updater.isMandatory }">
            <svg v-if="updater.isReadyToInstall" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
              <path d="M20 6L9 17l-5-5" stroke="#1fd98a" />
            </svg>
            <svg v-else-if="updater.isMandatory" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" stroke="#ff4d5f" />
            </svg>
            <svg v-else width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M7 10l5 5 5-5M12 15V3" stroke="#29b6f6" />
            </svg>
          </div>

          <div class="header-info">
            <div class="eyebrow-row">
              <span class="badge-eyebrow" :class="{ 'eyebrow-danger': updater.isMandatory }">
                {{ updater.isMandatory ? 'CRITICAL SYSTEM GATE' : 'AUTO-UPDATER' }}
              </span>
              <span v-if="updater.isMandatory" class="mandatory-tag">MANDATORY</span>
            </div>
            <h2 class="modal-title">
              {{ updater.isReadyToInstall
                ? 'Update Ready to Install'
                : updater.isMandatory
                  ? 'Mandatory Update Required'
                  : 'New Update Available' }}
            </h2>
            <p class="modal-subtitle">
              {{ updater.isReadyToInstall
                ? 'New client binary has been downloaded and cryptographically verified.'
                : updater.isMandatory
                  ? 'Your current client version is deprecated. You must update to continue.'
                  : 'A new release is available with new features and game compatibility improvements.' }}
            </p>
          </div>

          <button
            v-if="!updater.isMandatory && !updater.isDownloading"
            class="btn-close-modal"
            @click="updater.dismissModal"
            title="Close"
          >
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M18 6L6 18M6 6l12 12" />
            </svg>
          </button>
        </div>

        <!-- Modal Body -->
        <div class="modal-body">
          <!-- Version Info Strip -->
          <div class="version-strip">
            <div class="version-item">
              <span class="v-label">Target Release</span>
              <span class="v-val target-val font-mono">v{{ updater.latestVersion }}</span>
            </div>
            <div class="version-sep">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M5 12h14M12 5l7 7-7 7" stroke="#29b6f6" />
              </svg>
            </div>
            <div class="version-item">
              <span class="v-label">Transfer Engine</span>
              <span class="v-val font-mono text-cyan">Resumable HTTP Range</span>
            </div>
          </div>

          <!-- Release Notes -->
          <div v-if="updater.releaseNotes" class="release-notes-box">
            <div class="notes-header">
              <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z" stroke="#a1a2a6" />
                <path d="M14 2v6h6M16 13H8M16 17H8M10 9H8" stroke="#a1a2a6" />
              </svg>
              <span>Changelog & Release Notes</span>
            </div>
            <div class="notes-content">
              {{ updater.releaseNotes }}
            </div>
          </div>

          <!-- Downloading / Paused State UI -->
          <div v-if="updater.isDownloading || updater.isPaused || updater.isVerifying" class="download-status-card">
            <div class="progress-info-row">
              <div class="progress-status-badge">
                <span class="dot" :class="updater.isPaused ? 'dot-amber' : 'dot-cyan'"></span>
                <span class="status-txt">
                  {{ updater.isVerifying
                    ? 'Verifying SHA-256 integrity...'
                    : updater.isPaused
                      ? 'Download Paused'
                      : 'Streaming Chunks...' }}
                </span>
              </div>
              <div class="progress-percent font-mono">{{ updater.percent }}%</div>
            </div>

            <div class="progress-track">
              <div
                class="progress-bar-fill"
                :class="{ 'bar-paused': updater.isPaused, 'bar-verifying': updater.isVerifying }"
                :style="{ width: `${updater.percent}%` }"
              >
                <div class="progress-bar-glow"></div>
              </div>
            </div>

            <div class="progress-metrics-row">
              <span class="metric-val">
                <span class="metric-num font-mono">{{ updater.formattedDownloaded }}</span>
                <span class="metric-div">/</span>
                <span class="metric-num font-mono">{{ updater.formattedTotal }}</span>
              </span>
              <span v-if="!updater.isPaused && !updater.isVerifying" class="metric-speed font-mono">
                {{ updater.formattedSpeed }}
              </span>
            </div>
          </div>

          <!-- Ready To Install Success Banner -->
          <div v-else-if="updater.isReadyToInstall" class="ready-banner">
            <div class="ready-icon">
              <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="#1fd98a" stroke-width="2.5">
                <path d="M22 11.08V12a10 10 0 1 1-5.93-9.14" />
                <path d="M22 4L12 14.01l-3-3" />
              </svg>
            </div>
            <div class="ready-text">
              <h4>Download Complete</h4>
              <p>The executable has been saved and checked. Relaunch now to apply in-place update seamlessly.</p>
            </div>
          </div>

          <!-- Error Alert -->
          <div v-if="updater.error" class="error-alert">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#ff4d5f" stroke-width="2">
              <circle cx="12" cy="12" r="10" />
              <line x1="12" y1="8" x2="12" y2="12" />
              <line x1="12" y1="16" x2="12.01" y2="16" />
            </svg>
            <div class="error-msg">
              <strong>Update Error:</strong> {{ updater.error }}
            </div>
          </div>
        </div>

        <!-- Modal Footer Actions -->
        <div class="modal-footer">
          <!-- Ready to Install -->
          <template v-if="updater.isReadyToInstall">
            <button
              v-if="!updater.isMandatory"
              class="btn-action btn-secondary"
              @click="updater.dismissModal"
            >
              Later
            </button>
            <button class="btn-action btn-install" @click="updater.installUpdate">
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M21.5 2v6h-6M21.34 15.57a10 10 0 1 1-.57-8.38l5.67-5.67" />
              </svg>
              Restart & Apply Update
            </button>
          </template>

          <!-- Downloading -->
          <template v-else-if="updater.isDownloading">
            <button
              v-if="!updater.isMandatory"
              class="btn-action btn-secondary"
              @click="updater.cancelDownload"
            >
              Cancel
            </button>
            <button class="btn-action btn-pause" @click="updater.pauseDownload">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
                <rect x="6" y="4" width="4" height="16" rx="1" />
                <rect x="14" y="4" width="4" height="16" rx="1" />
              </svg>
              Pause Download
            </button>
          </template>

          <!-- Paused -->
          <template v-else-if="updater.isPaused">
            <button
              v-if="!updater.isMandatory"
              class="btn-action btn-secondary"
              @click="updater.cancelDownload"
            >
              Cancel
            </button>
            <button class="btn-action btn-primary" @click="updater.startDownload">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
                <polygon points="5 3 19 12 5 21 5 3" />
              </svg>
              Resume Download
            </button>
          </template>

          <!-- Idle or Error -->
          <template v-else>
            <button
              v-if="!updater.isMandatory"
              class="btn-action btn-secondary"
              @click="updater.dismissModal"
            >
              Remind Me Later
            </button>
            <button class="btn-action btn-primary" @click="updater.startDownload">
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M7 10l5 5 5-5M12 15V3" />
              </svg>
              {{ updater.error ? 'Retry Download' : 'Update Now' }}
            </button>
          </template>
        </div>
      </div>
    </div>
  </transition>
</template>

<style scoped>
.update-modal-overlay {
  position: fixed;
  inset: 0;
  z-index: 9999;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 1.5rem;
}

.update-modal-backdrop {
  position: absolute;
  inset: 0;
  background: rgba(3, 7, 18, 0.85);
  backdrop-filter: blur(8px);
}

.update-modal-card {
  position: relative;
  width: 100%;
  max-width: 540px;
  background: #080d18;
  border: 1px solid rgba(41, 182, 246, 0.25);
  border-radius: var(--radius-panel);
  box-shadow:
    none;
  overflow: hidden;
  display: flex;
  flex-direction: column;
  color: #f4f4f5;
}

.update-modal-card.is-mandatory {
  border-color: rgba(255, 77, 95, 0.35);
  box-shadow:
    none;
}

.modal-glow {
  position: absolute;
  top: -80px;
  left: 50%;
  transform: translateX(-50%);
  width: 320px;
  height: 160px;
  background: none;
  pointer-events: none;
}

.modal-glow.glow-danger {
  background: none;
}

.modal-header {
  position: relative;
  display: flex;
  align-items: flex-start;
  gap: 1rem;
  padding: 1.75rem 1.75rem 1rem;
}

.header-icon-wrap {
  width: 52px;
  height: 52px;
  border-radius: var(--radius-panel);
  background: rgba(41, 182, 246, 0.08);
  border: 1px solid rgba(41, 182, 246, 0.2);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.header-icon-wrap.icon-danger {
  background: rgba(255, 77, 95, 0.08);
  border-color: rgba(255, 77, 95, 0.25);
}

.header-info {
  flex: 1;
  min-width: 0;
}

.eyebrow-row {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  margin-bottom: 0.35rem;
}

.badge-eyebrow {
  font-size: 0.65rem;
  font-weight: 700;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  color: #29b6f6;
}

.badge-eyebrow.eyebrow-danger {
  color: #ff4d5f;
}

.mandatory-tag {
  background: rgba(255, 77, 95, 0.15);
  border: 1px solid rgba(255, 77, 95, 0.3);
  color: #ff5b6b;
  font-size: 0.6rem;
  font-weight: 700;
  letter-spacing: 0.05em;
  padding: 0.1rem 0.4rem;
  border-radius: 4px;
}

.modal-title {
  font-size: 1.25rem;
  font-weight: 700;
  color: #f4f4f5;
  line-height: 1.2;
  margin: 0 0 0.35rem;
}

.modal-subtitle {
  font-size: 0.8rem;
  color: #a1a2a6;
  line-height: 1.4;
  margin: 0;
}

.btn-close-modal {
  background: transparent;
  border: none;
  color: #898b90;
  cursor: pointer;
  padding: 0.25rem;
  border-radius: 6px;
  transition: all 0.2s;
}

.btn-close-modal:hover {
  color: #f4f4f5;
  background: rgba(255, 255, 255, 0.08);
}

.modal-body {
  padding: 0.75rem 1.75rem 1.25rem;
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

.version-strip {
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: rgba(27,28,30, 0.6);
  border: 1px solid rgba(41, 182, 246, 0.15);
  border-radius: 12px;
  padding: 0.75rem 1.25rem;
}

.version-item {
  display: flex;
  flex-direction: column;
}

.v-label {
  font-size: 0.65rem;
  font-weight: 600;
  text-transform: uppercase;
  color: #898b90;
  margin-bottom: 0.15rem;
}

.v-val {
  font-size: 0.85rem;
  font-weight: 700;
  color: #dedee0;
}

.v-val.target-val {
  color: #29b6f6;
}

.text-cyan {
  color: #29b6f6;
}

.release-notes-box {
  background: rgba(24,25,27, 0.7);
  border: 1px solid rgba(41, 182, 246, 0.12);
  border-radius: 12px;
  padding: 0.75rem 1rem;
}

.notes-header {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  font-size: 0.7rem;
  font-weight: 600;
  color: #a1a2a6;
  margin-bottom: 0.5rem;
  text-transform: uppercase;
  letter-spacing: 0.05em;
}

.notes-content {
  font-size: 0.8rem;
  color: #c5c6c8;
  line-height: 1.5;
  white-space: pre-line;
  max-height: 110px;
  overflow-y: auto;
}

.download-status-card {
  background: rgba(24,25,27, 0.8);
  border: 1px solid rgba(41, 182, 246, 0.2);
  border-radius: var(--radius-panel);
  padding: 1rem;
  display: flex;
  flex-direction: column;
  gap: 0.6rem;
}

.progress-info-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.progress-status-badge {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  flex-shrink: 0;
}

.dot-cyan {
  background: #29b6f6;
  box-shadow: none;
  animation: pulse 1.5s infinite ease-in-out;
}

.dot-amber {
  background: #ffc22e;
  box-shadow: none;
}

.status-txt {
  font-size: 0.75rem;
  font-weight: 600;
  color: #dedee0;
}

.progress-percent {
  font-size: 0.85rem;
  font-weight: 700;
  color: #29b6f6;
}

.progress-track {
  width: 100%;
  height: 8px;
  background: rgba(27,28,30, 0.8);
  border-radius: 999px;
  overflow: hidden;
  position: relative;
}

.progress-bar-fill {
  height: 100%;
  background: linear-gradient(90deg, #29b6f6 0%, #29b6f6 100%);
  border-radius: 999px;
  transition: width 0.3s ease;
  position: relative;
}

.progress-bar-fill.bar-paused {
  background: linear-gradient(90deg, #e08600 0%, #ffd24a 100%);
}

.progress-bar-fill.bar-verifying {
  background: linear-gradient(90deg, #29b6f6 0%, #29b6f6 100%);
}

.progress-bar-glow {
  position: absolute;
  top: 0;
  right: 0;
  width: 10px;
  height: 100%;
  background: #fff;
  filter: blur(2px);
  opacity: 0.5;
}

.progress-metrics-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 0.72rem;
  color: #a1a2a6;
}

.metric-num {
  color: #dedee0;
  font-weight: 600;
}

.metric-div {
  margin: 0 0.25rem;
  opacity: 0.5;
}

.metric-speed {
  color: #29b6f6;
  font-weight: 600;
}

.ready-banner {
  display: flex;
  align-items: center;
  gap: 1rem;
  background: rgba(31, 217, 138, 0.08);
  border: 1px solid rgba(31, 217, 138, 0.3);
  border-radius: 12px;
  padding: 0.85rem 1rem;
}

.ready-text h4 {
  font-size: 0.85rem;
  font-weight: 700;
  color: #1fd98a;
  margin: 0 0 0.15rem;
}

.ready-text p {
  font-size: 0.75rem;
  color: #a1a2a6;
  margin: 0;
}

.error-alert {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  background: rgba(255, 77, 95, 0.1);
  border: 1px solid rgba(255, 77, 95, 0.3);
  border-radius: 10px;
  padding: 0.75rem;
  font-size: 0.75rem;
  color: #ff8d99;
}

.modal-footer {
  padding: 1rem 1.75rem 1.75rem;
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  border-top: 1px solid rgba(41, 182, 246, 0.08);
}

.btn-action {
  display: inline-flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.65rem 1.25rem;
  font-size: 0.8rem;
  font-weight: 600;
  border-radius: 10px;
  cursor: pointer;
  transition: all 0.2s;
  border: none;
}

.btn-secondary {
  background: rgba(27,28,30, 0.8);
  color: #a1a2a6;
  border: 1px solid rgba(255, 255, 255, 0.08);
}

.btn-secondary:hover {
  background: rgba(255, 255, 255, 0.08);
  color: #f4f4f5;
}

.btn-primary {
  background: #ededee;
  color: #202123;
  box-shadow: none;
}

.btn-primary:hover {
  filter: brightness(1.1);
  box-shadow: none;
}

.btn-pause {
  background: rgba(255, 194, 46, 0.15);
  border: 1px solid rgba(255, 194, 46, 0.35);
  color: #ffd24a;
}

.btn-pause:hover {
  background: rgba(255, 194, 46, 0.25);
}

.btn-install {
  background: #ededee;
  color: #202123;
  box-shadow: none;
}

.btn-install:hover {
  filter: brightness(1.1);
  box-shadow: none;
}

/* Animations */
.modal-fade-enter-active,
.modal-fade-leave-active {
  transition: opacity 0.25s ease, transform 0.25s ease;
}

.modal-fade-enter-from,
.modal-fade-leave-to {
  opacity: 0;
  transform: scale(0.96);
}

@keyframes pulse {
  0%, 100% { opacity: 1; transform: scale(1); }
  50% { opacity: 0.5; transform: scale(0.85); }
}
</style>
