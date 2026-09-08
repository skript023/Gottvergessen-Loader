<script setup lang="ts">
import { computed } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { useAuthStore } from '../stores/auth';

const route = useRoute();
const router = useRouter();
const auth = useAuthStore();

const pageTitle = computed(() => {
  return (route.meta.title as string) || 'Dashboard';
});

async function handleLogout() {
  const confirmed = confirm('Are you sure you want to sign out?');
  if (!confirmed) return;
  await auth.logout();
  router.push('/login');
}
</script>

<template>
  <header class="top-bar">
    <div class="breadcrumb-container">
      <span class="breadcrumb-root">Control Center</span>
      <span class="breadcrumb-sep">/</span>
      <span class="breadcrumb-active">{{ pageTitle }}</span>
    </div>

    <div class="top-bar-right">
      <div class="system-pills">
        <div class="sys-pill pill-online">
          <span class="pulse-dot"></span>
          <span>API: ONLINE</span>
        </div>
        <div class="sys-pill pill-secure">
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>
          </svg>
          <span>ENGINE: READY (x64)</span>
        </div>
        <div class="sys-pill pill-hwid">
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <rect x="3" y="11" width="18" height="11" rx="2" ry="2"/>
            <path d="M7 11V7a5 5 0 0 1 10 0v4"/>
          </svg>
          <span>HWID: LOCKED</span>
        </div>
      </div>

      <button class="btn-header-logout" @click="handleLogout" title="Logout">
        <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"/>
          <polyline points="16 17 21 12 16 7"/><line x1="21" y1="12" x2="9" y2="12"/>
        </svg>
        <span>Sign Out</span>
      </button>
    </div>
  </header>
</template>
