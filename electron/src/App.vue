<script setup lang="ts">
import { computed } from 'vue';
import { useRoute } from 'vue-router';
import Sidebar from './components/Sidebar.vue';
import Header from './components/Header.vue';
import KickModal from './components/KickModal.vue';

const route = useRoute();
const isAuthRoute = computed(() => route.name === 'Login');
</script>

<template>
  <div v-if="isAuthRoute" class="auth-wrapper">
    <router-view />
  </div>
  <div v-else class="app-layout">
    <Sidebar />
    <main class="main-wrapper">
      <Header />
      <div class="content-scroll">
        <router-view />
      </div>
    </main>
  </div>

  <!-- Real-time Kick / Force-Logout Notification Modal -->
  <KickModal />
</template>
