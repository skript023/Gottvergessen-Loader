<script setup lang="ts">
import { useAuthStore } from '../stores/auth';
import { useRouter } from 'vue-router';

const auth = useAuthStore();
const router = useRouter();

function handleAcknowledge() {
  auth.dismissKick();
  router.push('/login');
}
</script>

<template>
  <div v-if="auth.isKicked" class="kick-modal-overlay">
    <div class="kick-modal-box">
      <!-- Top Glow Accent -->
      <div class="kick-modal-glow"></div>

      <!-- Icon -->
      <div class="kick-icon-wrapper">
        <svg width="36" height="36" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/>
          <line x1="12" y1="9" x2="12" y2="13"/>
          <line x1="12" y1="17" x2="12.01" y2="17"/>
        </svg>
      </div>

      <!-- Header -->
      <h3 class="kick-title">SESSION TERMINATED</h3>
      <p class="kick-sub">
        Your active loader session was disconnected by the security server.
      </p>

      <!-- Reason Display -->
      <div class="kick-reason-box">
        <div class="kick-reason-label">REASON</div>
        <div class="kick-reason-text">
          {{ auth.kickReason || 'Session invalidated or device kick triggered.' }}
        </div>
      </div>

      <!-- Explanation -->
      <div class="kick-details">
        <span>Another device may have connected using your credentials or an administrator unlinked this hardware ID.</span>
      </div>

      <!-- Action Button -->
      <button class="btn-kick-ack" @click="handleAcknowledge">
        <span>Acknowledge & Sign In Again</span>
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
          <path d="M5 12h14M12 5l7 7-7 7"/>
        </svg>
      </button>
    </div>
  </div>
</template>

<style scoped>
.kick-modal-overlay {
  position: fixed;
  inset: 0;
  z-index: 9999;
  background: rgba(4, 6, 12, 0.88);
  backdrop-filter: blur(16px);
  display: grid;
  place-items: center;
  padding: 20px;
  animation: fadeIn 0.25s ease-out;
}

.kick-modal-box {
  position: relative;
  width: min(460px, 95vw);
  background: rgba(15, 23, 42, 0.95);
  border: 1px solid rgba(239, 68, 68, 0.4);
  border-radius: 20px;
  padding: 32px 28px;
  box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.8), 0 0 35px rgba(239, 68, 68, 0.2);
  display: flex;
  flex-direction: column;
  align-items: center;
  text-align: center;
  overflow: hidden;
}

.kick-modal-glow {
  position: absolute;
  top: -100px;
  left: 50%;
  transform: translateX(-50%);
  width: 250px;
  height: 200px;
  background: radial-gradient(circle, rgba(239, 68, 68, 0.35) 0%, transparent 70%);
  filter: blur(40px);
  pointer-events: none;
}

.kick-icon-wrapper {
  width: 68px;
  height: 68px;
  border-radius: 20px;
  background: rgba(239, 68, 68, 0.12);
  border: 1px solid rgba(239, 68, 68, 0.35);
  color: #ef4444;
  display: grid;
  place-items: center;
  margin-bottom: 18px;
  box-shadow: 0 8px 24px rgba(239, 68, 68, 0.25);
}

.kick-title {
  font-size: 20px;
  font-weight: 800;
  color: #fff;
  letter-spacing: 0.04em;
  margin-bottom: 6px;
}

.kick-sub {
  font-size: 13px;
  color: #94a3b8;
  line-height: 1.5;
  margin-bottom: 20px;
}

.kick-reason-box {
  width: 100%;
  background: rgba(239, 68, 68, 0.08);
  border: 1px solid rgba(239, 68, 68, 0.25);
  border-radius: 12px;
  padding: 12px 16px;
  margin-bottom: 16px;
  text-align: left;
}

.kick-reason-label {
  font-size: 10px;
  font-weight: 800;
  color: #ef4444;
  letter-spacing: 0.12em;
  margin-bottom: 4px;
}

.kick-reason-text {
  font-size: 13px;
  font-weight: 600;
  color: #fca5a5;
  word-break: break-word;
}

.kick-details {
  font-size: 11px;
  color: #64748b;
  line-height: 1.5;
  margin-bottom: 24px;
}

.btn-kick-ack {
  width: 100%;
  padding: 13px 20px;
  background: linear-gradient(135deg, #dc2626 0%, #ef4444 100%);
  border: none;
  border-radius: 12px;
  color: #fff;
  font-size: 13px;
  font-weight: 700;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  cursor: pointer;
  box-shadow: 0 4px 18px rgba(239, 68, 68, 0.4);
  transition: all 0.2s ease;
}

.btn-kick-ack:hover {
  filter: brightness(1.12);
  transform: translateY(-1px);
  box-shadow: 0 6px 22px rgba(239, 68, 68, 0.55);
}

@keyframes fadeIn {
  from { opacity: 0; transform: scale(0.96); }
  to { opacity: 1; transform: scale(1); }
}
</style>
