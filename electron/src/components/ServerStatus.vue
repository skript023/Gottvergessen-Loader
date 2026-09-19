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
const TOOLTIP_WIDTH = 280;
const pillEl = ref<HTMLElement | null>(null);
const panelStyle = ref({ top: '0px', left: '0px', width: `${TOOLTIP_WIDTH}px` });

function positionPanel() {
  const pill = pillEl.value;
  if (!pill) return;
  const rect = pill.getBoundingClientRect();
  const maxLeft = window.innerWidth - TOOLTIP_WIDTH - 12;
  panelStyle.value = {
    top: `${rect.bottom + 8}px`,
    left: `${Math.max(12, Math.min(rect.right - TOOLTIP_WIDTH, maxLeft))}px`,
    width: `${TOOLTIP_WIDTH}px`
  };
}

function show() { positionPanel(); open.value = true; void refresh(); }
onMounted(() => { void refresh(); timer = setInterval(refresh, 15000); });
onUnmounted(() => { disposed = true; clearInterval(timer); });
</script>

<template>
  <div ref="pillEl" class="server-status" @mouseenter="show" @mouseleave="open = false" @focusin="show" @focusout="open = false">
    <button type="button" class="sys-pill server-status-button" :class="`status-${state}`"
      aria-describedby="server-status-tooltip" @keydown.esc="open = false" @click="show">
      <span class="pulse-dot"></span>
      <span>{{ state.toUpperCase() }}</span>
    </button>
    <!-- Dim plus panel are teleported together, like the profile menu: kept
         inside the shell the panel would sit under the dim, not above it. -->
    <Teleport to="body">
      <transition name="menu-pop">
        <div v-if="open" class="server-status-layer">
          <div class="server-status-scrim"></div>

          <div id="server-status-tooltip" class="server-status-tooltip" :style="panelStyle" role="tooltip">
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
      </transition>
    </Teleport>
  </div>
</template>

<style scoped>
.server-status { position: relative; }
.server-status-button { cursor: help; font-family: inherit; }
.server-status-button:focus-visible { outline: 2px solid var(--primary-cyan); outline-offset: 2px; }
.server-status-button { border: none; color: #fff; }
.status-online { background: rgba(var(--accent-emerald-rgb), 0.85); }
.status-degraded { background: rgba(var(--accent-amber-rgb), 0.85); }
.status-offline { background: rgba(var(--accent-rose-rgb), 0.85); }
.status-checking, .status-unknown { background: rgba(255, 255, 255, 0.12); }
.server-status-button .pulse-dot { background: #fff; }
.server-status-button:not(.status-online) .pulse-dot { animation: none; box-shadow: none; }
.server-status-tooltip {
  position: fixed; padding: 14px;
  border: 1px solid var(--border-subtle); border-radius: 12px;
  background: var(--bg-popover); color: var(--text-main);
  box-shadow: 0 18px 40px rgba(0, 0, 0, 0.55);
  font-size: 12px; font-weight: 500; text-transform: none; letter-spacing: normal;
}
.server-status-layer {
  position: fixed; inset: 0; z-index: 9400;
  pointer-events: none;
}
.server-status-scrim {
  position: absolute; inset: 0;
  background: rgba(12, 13, 14, 0.82);
}
.server-status-tooltip strong { font-size: 13px; font-weight: 700; }
dl { margin: 10px 0; padding: 4px 10px; border-radius: 8px; background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.06); }
dl div { display: flex; justify-content: space-between; gap: 16px; margin: 6px 0; }
dt, small { color: var(--text-dim); }
dd { margin: 0; text-align: right; overflow-wrap: anywhere; font-weight: 600; }
p { color: var(--accent-amber); margin: 8px 0; }
</style>
