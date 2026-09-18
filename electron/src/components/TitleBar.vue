<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import { useAuthStore } from '../stores/auth';
import ServerStatus from './ServerStatus.vue';

const auth = useAuthStore();
const isMaximized = ref(false);

async function checkMaximized() {
  try {
    if (window.loader?.window?.isMaximized) {
      isMaximized.value = await window.loader.window.isMaximized();
    } else if (window.ellohim?.window?.isMaximized) {
      isMaximized.value = await window.ellohim.window.isMaximized();
    }
  } catch (_) {}
}

onMounted(() => {
  checkMaximized();
  window.addEventListener('resize', checkMaximized);
});

onUnmounted(() => {
  window.removeEventListener('resize', checkMaximized);
});

async function handleMinimize() {
  try {
    if (window.loader?.window?.minimize) {
      await window.loader.window.minimize();
    } else if (window.ellohim?.window?.minimize) {
      await window.ellohim.window.minimize();
    }
  } catch (err) {
    console.error('Failed to minimize window:', err);
  }
}

async function handleMaximize() {
  try {
    if (window.loader?.window?.maximize) {
      await window.loader.window.maximize();
      await checkMaximized();
    } else if (window.ellohim?.window?.maximize) {
      await window.ellohim.window.maximize();
      await checkMaximized();
    }
  } catch (err) {
    console.error('Failed to maximize window:', err);
  }
}

async function handleClose() {
  try {
    if (window.loader?.window?.close) {
      await window.loader.window.close();
    } else if (window.ellohim?.window?.close) {
      await window.ellohim.window.close();
    }
  } catch (err) {
    console.error('Failed to close window:', err);
  }
}
</script>

<template>
  <header class="custom-titlebar">
    <!-- Left: App Icon & Title Strip -->
    <div class="titlebar-left">
      <span class="titlebar-title-text">Astra Launcher</span>
    </div>

    <!-- Draggable Middle Area -->
    <div class="titlebar-drag-spacer" @dblclick="handleMaximize"></div>

    <!-- Right: Account Status & Window Action Controls -->
    <div class="titlebar-right">
      <!-- System Status Pills (when authenticated) -->
      <div v-if="auth.isAuthenticated" class="system-pills titlebar-pills">
        <ServerStatus />
      </div>

      <!-- Windows 11 Style Controls (Minimize, Maximize/Restore, Close) -->
      <div class="titlebar-controls">
        <button
          class="titlebar-win-btn"
          title="Minimize"
          @click="handleMinimize"
        >
          <svg width="10" height="1" viewBox="0 0 10 1">
            <rect width="10" height="1" fill="currentColor" />
          </svg>
        </button>

        <button
          class="titlebar-win-btn"
          :title="isMaximized ? 'Restore' : 'Maximize'"
          @click="handleMaximize"
        >
          <svg v-if="isMaximized" width="10" height="10" viewBox="0 0 10 10" fill="none" stroke="currentColor" stroke-width="1.1">
            <rect x="2" y="0.5" width="7.5" height="7.5" />
            <polyline points="0.5,2.5 0.5,9.5 7.5,9.5" />
          </svg>
          <svg v-else width="10" height="10" viewBox="0 0 10 10" fill="none" stroke="currentColor" stroke-width="1.1">
            <rect x="0.5" y="0.5" width="9" height="9" />
          </svg>
        </button>

        <button
          class="titlebar-win-btn close"
          title="Close"
          @click="handleClose"
        >
          <svg width="10" height="10" viewBox="0 0 10 10" stroke="currentColor" stroke-width="1.2">
            <line x1="1" y1="1" x2="9" y2="9" />
            <line x1="9" y1="1" x2="1" y2="9" />
          </svg>
        </button>
      </div>
    </div>
  </header>
</template>
