<script setup lang="ts">
import brandLogo from '../../../../src/logo.ico';
import { nextTick, onMounted, ref } from 'vue';
import { useRouter } from 'vue-router';
import { useAuthStore } from '../../stores/auth';

const router = useRouter();
const auth = useAuthStore();

const usernameInput = ref<HTMLInputElement | null>(null);
const username = ref('');
const password = ref('');
const rememberMe = ref(true);

// `autofocus` only fires on the initial document parse, so returning here
// after a sign-out would leave the form without focus.
onMounted(async () => {
  await nextTick();
  usernameInput.value?.focus();
});

async function handleLogin() {
  auth.kickReason = '';
  if (!username.value.trim() || !password.value) {
    auth.errorMessage = 'Username and password are required.';
    return;
  }

  const ok = await auth.login({
    username: username.value.trim(),
    password: password.value,
    rememberMe: rememberMe.value
  });

  if (ok) {
    password.value = '';
    router.push('/dashboard');
  }
}
</script>

<template>
  <div class="login-view">
    <div class="login-backdrop-glow"></div>
    <div class="login-container">
      <div class="login-card">
        <div class="login-header">
          <div class="brand-logo-large">
            <img :src="brandLogo" alt="Astra" width="40" height="40" />
          </div>
          <p class="login-eyebrow">SECURITY GATEWAY</p>
          <h1 class="login-title">Astra</h1>
          <p class="login-desc">Sign in with your license credentials to access authorized binaries.</p>
        </div>

        <!-- Kick / Disconnect Alert Banner -->
        <div v-if="auth.kickReason" class="kick-banner">
          <div class="kick-banner-icon">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/>
              <line x1="12" y1="9" x2="12" y2="13"/>
              <line x1="12" y1="17" x2="12.01" y2="17"/>
            </svg>
          </div>
          <div class="kick-banner-text">
            <strong>Session Disconnected</strong>
            <p>{{ auth.kickReason }}</p>
          </div>
        </div>

        <form class="login-form" @submit.prevent="handleLogin">
          <div class="form-group">
            <label for="username">
              <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/>
              </svg>
              Username
            </label>
            <input
              id="username"
              ref="usernameInput"
              v-model="username"
              autocomplete="username"
              placeholder="Enter username"
              required
            />
          </div>

          <div class="form-group">
            <label for="password">
              <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/>
              </svg>
              Password
            </label>
            <input
              id="password"
              v-model="password"
              type="password"
              autocomplete="current-password"
              placeholder="Enter password"
              required
            />
          </div>

          <div class="form-extra">
            <label class="checkbox-container">
              <input v-model="rememberMe" type="checkbox" />
              <span class="custom-checkbox"></span>
              Remember device session
            </label>
          </div>

          <button type="submit" class="btn-primary-glow" :disabled="auth.isLoading">
            <span v-if="auth.isLoading">Signing In...</span>
            <template v-else>
              <span>Sign In to Dashboard</span>
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
                <path d="M5 12h14M12 5l7 7-7 7"/>
              </svg>
            </template>
          </button>

          <p v-if="auth.errorMessage && !auth.kickReason" class="form-message error">
            {{ auth.errorMessage }}
          </p>
          <p v-else-if="auth.isLoading" class="form-message pending-text">
            Authenticating with security server...
          </p>
        </form>

        <div class="login-footer">
          <span>Native Engine v2.0</span>
          <span class="footer-dot">•</span>
          <span>HWID Hardware Bound</span>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.kick-banner {
  display: flex;
  align-items: flex-start;
  gap: 12px;
  background: rgba(255, 77, 95, 0.12);
  border: 1px solid rgba(255, 77, 95, 0.35);
  border-radius: 12px;
  padding: 12px 14px;
  margin-bottom: 18px;
  animation: fadeIn 0.25s ease-out;
}

.kick-banner-icon {
  color: #ff4d5f;
  flex-shrink: 0;
  margin-top: 1px;
}

.kick-banner-text strong {
  display: block;
  font-size: 13px;
  font-weight: 700;
  color: #ff8d99;
  margin-bottom: 3px;
}

.kick-banner-text p {
  font-size: 11px;
  color: #ffc0c6;
  line-height: 1.4;
  margin: 0;
  word-break: break-word;
}

@keyframes fadeIn {
  from { opacity: 0; transform: translateY(-4px); }
  to { opacity: 1; transform: translateY(0); }
}
</style>
