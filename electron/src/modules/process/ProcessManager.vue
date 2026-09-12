<script setup lang="ts">
import { useProcessStore } from '../../stores/process';
import type { ProcessItem } from '../../types/loader';

const processStore = useProcessStore();

function handleScan() {
  processStore.scanProcesses();
}

function handleSelect(proc: ProcessItem) {
  if (proc.accessible) {
    processStore.selectProcess(proc, true);
  }
}
</script>

<template>
  <div class="tab-pane active">
    <div class="pane-header">
      <div>
        <h2 class="pane-title">System Process Inspector</h2>
        <p class="pane-desc">Filter running processes to attach memory hooks.</p>
      </div>
      <div class="pane-actions">
        <button class="btn-action" :disabled="processStore.isScanning" @click="handleScan">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <polyline points="23 4 23 10 17 10"/>
            <polyline points="1 20 1 14 7 14"/>
            <path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"/>
          </svg>
          <span>{{ processStore.isScanning ? 'Scanning Host...' : 'Scan Processes' }}</span>
        </button>
      </div>
    </div>

    <!-- Search bar -->
    <div class="search-bar-wrap">
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
        <circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/>
      </svg>
      <input
        v-model="processStore.searchQuery"
        type="search"
        placeholder="Search by PID, executable name (e.g., GTA5.exe, FiveM)..."
      />
    </div>

    <!-- Table -->
    <div class="table-container">
      <table class="cyber-table">
        <thead>
          <tr>
            <th style="width: 100px;">PID</th>
            <th>Executable Name</th>
            <th style="width: 130px;">Architecture</th>
            <th style="width: 130px;">Access Rights</th>
            <th style="width: 120px; text-align: right;">Action</th>
          </tr>
        </thead>
        <tbody>
          <!-- Active Scanning State Indicator -->
          <tr v-if="processStore.isScanning">
            <td colspan="5" class="process-scanning-row">
              <div class="scanning-flex">
                <span class="scanning-spinner"></span>
                <span>Scanning active host processes & memory hooks...</span>
              </div>
            </td>
          </tr>

          <tr v-else-if="processStore.filteredProcesses.length === 0">
            <td colspan="5" class="empty-state">
              {{ processStore.processes.length === 0 ? 'No processes detected. Click "Scan Processes" to refresh.' : 'No processes match your search filter.' }}
            </td>
          </tr>

          <tr
            v-for="proc in processStore.filteredProcesses"
            :key="proc.pid"
            :class="{ selected: processStore.selectedProcess?.pid === proc.pid }"
            @click="handleSelect(proc)"
          >
            <td><strong>{{ proc.pid }}</strong></td>
            <td>{{ proc.name }}</td>
            <td><span class="tag-arch">{{ proc.arch || 'x64' }}</span></td>
            <td>
              <span :class="proc.accessible ? 'tag-access-ready' : 'tag-access-denied'">
                {{ proc.accessible ? '● Ready' : '✕ Denied' }}
              </span>
            </td>
            <td style="text-align: right;">
              <button
                v-if="proc.accessible"
                class="btn-target-row"
                @click.stop="handleSelect(proc)"
              >
                {{ processStore.selectedProcess?.pid === proc.pid ? 'Targeted' : 'Target' }}
              </button>
              <span v-else style="color: var(--text-dim);">N/A</span>
            </td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>
