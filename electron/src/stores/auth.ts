import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import type { UserProfile } from '../types/loader';
import { useDiagnosticsStore } from './diagnostics';
import { useBinariesStore } from './binaries';
import { useProcessStore } from './process';
import socketAuth from '../services/socketAuth';
import router from '../router';

export const useAuthStore = defineStore('auth', () => {
  const diagnostics = useDiagnosticsStore();
  const binariesStore = useBinariesStore();
  const processStore = useProcessStore();

  const profile = ref<UserProfile | null>(null);
  const isAuthenticated = ref(false);
  const isRestoring = ref(true);
  const isLoading = ref(false);
  const errorMessage = ref('');
  const isKicked = ref(false);
  const kickReason = ref('');

  let heartbeatInterval: any = null;

  // Wire up real-time kick callback from socket
  socketAuth.onKick((reason) => {
    handleKick(reason);
  });

  // When socket closes (e.g. server terminated connection), immediately verify HTTP session
  socketAuth.onClose(async () => {
    if (isAuthenticated.value) {
      await verifySession();
    }
  });

  function stopHeartbeat() {
    if (heartbeatInterval !== null) {
      clearInterval(heartbeatInterval);
      heartbeatInterval = null;
    }
  }

  async function verifySession(): Promise<boolean> {
    if (!isAuthenticated.value) return false;
    try {
      if (window.loader?.refreshBinaries) {
        await window.loader.refreshBinaries();
        return true;
      }
    } catch (err: any) {
      const msg = String(err?.message || '').toLowerCase();
      if (
        msg.includes('401') ||
        msg.includes('unauthorized') ||
        msg.includes('login is required') ||
        msg.includes('expired') ||
        msg.includes('terminated') ||
        msg.includes('revoked')
      ) {
        handleKick('Session terminated or unlinked by security server.');
        return false;
      }
    }
    return true;
  }

  function startHeartbeat() {
    stopHeartbeat();
    // High-frequency 3-second heartbeat to ensure real-time kick detection under any proxy/network conditions
    heartbeatInterval = setInterval(async () => {
      await verifySession();
    }, 3000);
  }

  function handleKick(reason: string) {
    const finalReason = reason || 'Session terminated by server or administrator.';
    isKicked.value = true;
    kickReason.value = finalReason;
    errorMessage.value = `Disconnected: ${finalReason}`;
    diagnostics.addLog(`[SECURITY KICK] Server terminated session: ${finalReason}`, 'error');

    // Stop background socket and heartbeat
    socketAuth.disconnect();
    stopHeartbeat();

    // Clear local native session
    if (window.loader?.logout) {
      window.loader.logout().catch(() => {});
    }

    // Reset store states
    profile.value = null;
    isAuthenticated.value = false;
    binariesStore.binaries = [];
    binariesStore.selectedBinaryIndex = -1;
    processStore.processes = [];
    processStore.selectedProcess = null;

    // Immediately redirect to /login
    router.replace('/login');
  }

  function dismissKick() {
    isKicked.value = false;
  }

  const displayName = computed(() => {
    if (!profile.value) return 'User';
    const first = profile.value.firstname || '';
    const last = profile.value.lastname || '';
    const full = [first, last].filter(Boolean).join(' ') || profile.value.fullname;
    return full || profile.value.username || 'User';
  });

  const displayHandle = computed(() => {
    return profile.value?.username ? `@${profile.value.username}` : '@user';
  });

  const role = computed(() => {
    return profile.value?.role || 'VIP CLIENT';
  });

  const expiryDate = computed(() => {
    return profile.value?.expired_date || profile.value?.expiry_date || 'Lifetime Active';
  });

  const avatarInitial = computed(() => {
    return displayName.value.trim().charAt(0).toUpperCase() || 'U';
  });

  async function resolveSessionConnection(sessionData?: { token?: string; hwid?: string; backendUrl?: string; deviceName?: string }) {
    let token = sessionData?.token || '';
    let hwid = sessionData?.hwid || '';
    let backendUrl = sessionData?.backendUrl || '';
    let deviceName = sessionData?.deviceName || '';

    // Fallback: Query getSessionInfo from native core
    if (!token && window.loader?.getSessionInfo) {
      try {
        const info = await window.loader.getSessionInfo();
        token = info.token || '';
        hwid = info.hwid || hwid;
        backendUrl = info.backendUrl || backendUrl;
        deviceName = (info as any).deviceName || deviceName;
      } catch (_) {}
    }

    if (token) {
      socketAuth.connect(token, hwid, backendUrl || 'https://apie.rena.my.id', deviceName);
      startHeartbeat();
    }
  }

  async function login(credentials: { username: string; password: string; rememberMe: boolean }) {
    isLoading.value = true;
    errorMessage.value = '';
    isKicked.value = false;
    kickReason.value = '';

    try {
      if (!window.loader?.login) {
        throw new Error('Loader native login API is unavailable');
      }

      const result = await window.loader.login(credentials);
      profile.value = result.profile;
      isAuthenticated.value = true;

      binariesStore.setBinaries(result.binaries || []);
      diagnostics.addLog(`Authenticated as ${credentials.username}. Welcome back!`, 'success');

      // Connect real-time socket and start session heartbeat
      await resolveSessionConnection(result);

      // Auto-scan processes upon login
      await processStore.scanProcesses();
      return true;
    } catch (err: any) {
      errorMessage.value = err.message || 'Authentication failed';
      diagnostics.addLog(`Login failed: ${err.message}`, 'error');
      return false;
    } finally {
      isLoading.value = false;
    }
  }

  async function restoreSession() {
    isRestoring.value = true;
    diagnostics.addLog('Checking for active device session...', 'info');

    try {
      if (window.loader?.restoreSession) {
        const session = await window.loader.restoreSession();
        if (session && session.profile) {
          profile.value = session.profile;
          isAuthenticated.value = true;
          binariesStore.setBinaries(session.binaries || []);
          diagnostics.addLog('Active device session restored successfully.', 'success');

          // Connect real-time socket and start session heartbeat
          await resolveSessionConnection(session);

          await processStore.scanProcesses();
          return true;
        }
      }
      diagnostics.addLog('No active session found. Sign in required.', 'info');
      return false;
    } catch (err: any) {
      diagnostics.addLog(`Session restore notice: ${err.message}`, 'warn');
      return false;
    } finally {
      isRestoring.value = false;
    }
  }

  async function logout() {
    socketAuth.disconnect();
    stopHeartbeat();
    try {
      diagnostics.addLog('Signing out and clearing session...', 'warn');
      if (window.loader?.logout) {
        await window.loader.logout();
      }
    } catch (err: any) {
      console.error('Logout error:', err);
    } finally {
      profile.value = null;
      isAuthenticated.value = false;
      binariesStore.binaries = [];
      binariesStore.selectedBinaryIndex = -1;
      processStore.processes = [];
      processStore.selectedProcess = null;
    }
  }

  return {
    profile,
    isAuthenticated,
    isRestoring,
    isLoading,
    errorMessage,
    isKicked,
    kickReason,
    displayName,
    displayHandle,
    role,
    expiryDate,
    avatarInitial,
    handleKick,
    dismissKick,
    login,
    restoreSession,
    logout
  };
});
