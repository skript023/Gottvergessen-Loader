<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref } from 'vue';
import type { ServerStatus } from '../types/loader';

const status = ref<ServerStatus | null>(null);
const failed = ref(false);
const open = ref(false);
const state = computed(() => failed.value ? 'unknown' : status.value?.state || 'checking');
let timer: ReturnType<typeof setInterval> | undefined;
let pending = false;
let disposed = false;
async function refresh() {
  if (pending || disposed) return;
  pending = true;
  try {
    if (!window.loader?.getServerStatus) throw new Error('Unavailable');
    const result = await window.loader.getServerStatus();
    if (!disposed) { status.value = result; failed.value = false; }
  } catch (_) {
    if (!disposed) { failed.value = true; status.value = null; }
  } finally { pending = false; }
}
function show() { open.value = true; void refresh(); }
onMounted(() => { void refresh(); timer = setInterval(refresh, 15000); });
onUnmounted(() => { disposed = true; clearInterval(timer); });
</script>

<template>
  <div class="server-status" @mouseenter="show" @mouseleave="open = false" @focusin="show" @focusout="open = false">
    <button type="button" class="sys-pill server-status-button" :class="`status-${state}`"
      aria-describedby="server-status-tooltip" @keydown.esc="open = false" @click="show">
      <span class="pulse-dot"></span>
      <span>{{ state.toUpperCase() }}</span>
    </button>
    <div v-if="open" id="server-status-tooltip" class="server-status-tooltip" role="tooltip">
      <strong>Connection overview</strong>
      <dl v-if="status">
        <div><dt>Status</dt><dd>{{ state.toUpperCase() }}</dd></div>
        <div><dt>Response time</dt><dd>{{ status.pingMs === null ? 'Unavailable' : `${status.pingMs} ms` }}</dd></div>
        <div><dt>Last checked</dt><dd>{{ new Date(status.checkedAt).toLocaleTimeString() }}</dd></div>
      </dl>
      <p v-if="status?.error">{{ status.error }}</p>
      <p v-if="!status">{{ failed ? 'Connection information unavailable.' : 'Checking connection...' }}</p>
      <small>Automatically refreshes every 15 seconds.</small>
    </div>
  </div>
</template>

<style scoped>
.server-status { position: relative; }
.server-status-button { cursor: help; font-family: inherit; background: #94a3b812; border-color: #94a3b840; }
.server-status-button:focus-visible { outline: 2px solid #38bdf8; outline-offset: 3px; }
.status-online { color: #4ade80; background: #10b9811f; border-color: #10b9814d; }
.status-degraded { color: #fbbf24; }
.status-offline { color: #f87171; }
.status-checking, .status-unknown { color: #94a3b8; }
.server-status-button .pulse-dot { background: currentColor; }
.server-status-button:not(.status-online) .pulse-dot { animation: none; box-shadow: none; }
.server-status-tooltip { position: absolute; z-index: 1000; top: 100%; right: 0; width: 300px; max-width: calc(100vw - 32px); padding: 16px; border: 1px solid #334155; border-radius: 12px; background: #111827; color: #e2e8f0; box-shadow: 0 12px 32px #0008; font-size: 12px; text-transform: none; letter-spacing: normal; }
.server-status-tooltip strong { font-size: 13px; }
dl { margin: 12px 0; }
dl div { display: flex; justify-content: space-between; gap: 16px; margin: 8px 0; }
dt, small { color: #94a3b8; }
dd { margin: 0; text-align: right; overflow-wrap: anywhere; }
p { color: #fbbf24; margin: 8px 0; }
</style>
