<script setup lang="ts">
import { useAuthStore } from '../../../stores/auth';
import { useBinariesStore } from '../../../stores/binaries';
import { useProcessStore } from '../../../stores/process';

const auth = useAuthStore();
const binariesStore = useBinariesStore();
const processStore = useProcessStore();
</script>

<template>
  <div class="metrics-grid">
    <div class="metric-card">
      <div class="metric-icon cyan">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"/>
          <polyline points="3.27 6.96 12 12.01 20.73 6.96"/><line x1="12" y1="22.08" x2="12" y2="12"/>
        </svg>
      </div>
      <div class="metric-data">
        <span class="metric-label">Assigned Binaries</span>
        <strong class="metric-value">{{ binariesStore.binaries.length }}</strong>
        <span class="metric-sub">Accessible Products</span>
      </div>
    </div>

    <div class="metric-card">
      <div class="metric-icon indigo">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <rect x="4" y="4" width="16" height="16" rx="2"/><rect x="9" y="9" width="6" height="6"/>
          <line x1="9" y1="1" x2="9" y2="4"/><line x1="15" y1="1" x2="15" y2="4"/>
          <line x1="9" y1="20" x2="9" y2="23"/><line x1="15" y1="20" x2="15" y2="23"/>
        </svg>
      </div>
      <div class="metric-data">
        <span class="metric-label">Running Processes</span>
        <strong class="metric-value">{{ processStore.processes.length }}</strong>
        <span class="metric-sub">Scanned on Host</span>
      </div>
    </div>

    <div class="metric-card">
      <div class="metric-icon emerald">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <circle cx="12" cy="12" r="10"/><line x1="22" y1="12" x2="18" y2="12"/>
          <line x1="6" y1="12" x2="2" y2="12"/><line x1="12" y1="6" x2="12" y2="2"/>
          <line x1="12" y1="22" x2="12" y2="18"/>
        </svg>
      </div>
      <div class="metric-data">
        <span class="metric-label">Selected Target</span>
        <strong class="metric-value">{{ processStore.selectedProcess?.name || 'None' }}</strong>
        <span class="metric-sub">
          {{ processStore.selectedProcess && processStore.selectedProcess.pid > 0 ? `PID ${processStore.selectedProcess.pid} • ${processStore.selectedProcess.arch}` : 'Select from process table' }}
        </span>
      </div>
    </div>

    <div class="metric-card">
      <div class="metric-icon purple">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>
          <polyline points="9 12 11 14 15 10"/>
        </svg>
      </div>
      <div class="metric-data">
        <span class="metric-label">License Expiry</span>
        <strong class="metric-value">{{ auth.expiryDate }}</strong>
        <span class="metric-sub">Subscription Status</span>
      </div>
    </div>
  </div>
</template>
