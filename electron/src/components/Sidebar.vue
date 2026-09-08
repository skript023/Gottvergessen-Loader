<script setup lang="ts">
import { useRouter } from 'vue-router';
import { useAuthStore } from '../stores/auth';
import { useBinariesStore } from '../stores/binaries';

const router = useRouter();
const auth = useAuthStore();
const binariesStore = useBinariesStore();

async function handleLogout() {
  const confirmed = confirm('Are you sure you want to sign out?');
  if (!confirmed) return;
  await auth.logout();
  router.push('/login');
}
</script>

<template>
  <aside class="sidebar">
    <!-- Brand -->
    <div class="sidebar-brand">
      <div class="brand-badge-icon">
        <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5" stroke="#38bdf8" />
        </svg>
      </div>
      <div class="brand-text">
        <span class="brand-name">GOTTVERGESSEN</span>
        <span class="brand-version">CONTROL CENTER</span>
      </div>
    </div>

    <!-- Navigation Tabs -->
    <nav class="sidebar-nav">
      <div class="nav-section-label">CORE MENU</div>

      <RouterLink to="/dashboard" class="nav-item" active-class="active">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <rect x="3" y="3" width="7" height="7"/><rect x="14" y="3" width="7" height="7"/>
          <rect x="14" y="14" width="7" height="7"/><rect x="3" y="14" width="7" height="7"/>
        </svg>
        <span>Dashboard</span>
      </RouterLink>

      <RouterLink to="/catalog" class="nav-item" active-class="active">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"/>
          <polyline points="3.27 6.96 12 12.01 20.73 6.96"/><line x1="12" y1="22.08" x2="12" y2="12"/>
        </svg>
        <span>Binaries Catalog</span>
        <span class="nav-badge">{{ binariesStore.binaries.length }}</span>
      </RouterLink>

      <RouterLink to="/processes" class="nav-item" active-class="active">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <rect x="4" y="4" width="16" height="16" rx="2"/><rect x="9" y="9" width="6" height="6"/>
          <line x1="9" y1="1" x2="9" y2="4"/><line x1="15" y1="1" x2="15" y2="4"/>
          <line x1="9" y1="20" x2="9" y2="23"/><line x1="15" y1="20" x2="15" y2="23"/>
          <line x1="20" y1="9" x2="23" y2="9"/><line x1="20" y1="14" x2="23" y2="14"/>
          <line x1="1" y1="9" x2="4" y2="9"/><line x1="1" y1="14" x2="4" y2="14"/>
        </svg>
        <span>Process Manager</span>
      </RouterLink>

      <RouterLink to="/diagnostics" class="nav-item" active-class="active">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <polyline points="4 17 10 11 4 5"/><line x1="12" y1="19" x2="20" y2="19"/>
        </svg>
        <span>Console & Logs</span>
      </RouterLink>
    </nav>

    <!-- User Profile & Logout -->
    <div class="sidebar-footer">
      <div class="user-profile-box">
        <div class="user-avatar">{{ auth.avatarInitial }}</div>
        <div class="user-info">
          <span class="user-name" :title="auth.displayName">{{ auth.displayName }}</span>
          <div class="user-subline">
            <span class="user-badge">{{ auth.role }}</span>
            <span class="user-handle">{{ auth.displayHandle }}</span>
          </div>
        </div>
      </div>

      <button class="btn-logout" @click="handleLogout" title="Sign out of account">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"/>
          <polyline points="16 17 21 12 16 7"/><line x1="21" y1="12" x2="9" y2="12"/>
        </svg>
        <span>Logout</span>
      </button>
    </div>
  </aside>
</template>
