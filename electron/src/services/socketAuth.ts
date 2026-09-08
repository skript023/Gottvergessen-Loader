import { useDiagnosticsStore } from '../stores/diagnostics';

type KickCallback = (reason: string) => void;
type CloseCallback = () => void;

class SocketAuthService {
  private ws: WebSocket | null = null;
  private pingInterval: number | null = null;
  private reconnectTimeout: number | null = null;
  private isConnecting: boolean = false;
  private shouldReconnect: boolean = true;
  private currentToken: string = '';
  private currentHwid: string = '';
  private currentDeviceName: string = '';
  private currentBackendUrl: string = '';
  private kickCallbacks: KickCallback[] = [];
  private closeCallbacks: CloseCallback[] = [];

  public onKick(callback: KickCallback): () => void {
    this.kickCallbacks.push(callback);
    return () => {
      this.kickCallbacks = this.kickCallbacks.filter((cb) => cb !== callback);
    };
  }

  public onClose(callback: CloseCallback): () => void {
    this.closeCallbacks.push(callback);
    return () => {
      this.closeCallbacks = this.closeCallbacks.filter((cb) => cb !== callback);
    };
  }

  private triggerKick(reason: string) {
    this.shouldReconnect = false;
    this.cleanup();
    for (const cb of this.kickCallbacks) {
      try {
        cb(reason);
      } catch (err) {
        console.error('[SocketAuth] Error in kick callback:', err);
      }
    }
  }

  private triggerClose() {
    for (const cb of this.closeCallbacks) {
      try {
        cb();
      } catch (err) {
        console.error('[SocketAuth] Error in close callback:', err);
      }
    }
  }

  public connect(
    token: string,
    hwid: string = '',
    backendUrl: string = 'https://apie.rena.my.id',
    deviceName: string = ''
  ): void {
    if (!token) {
      console.warn('[SocketAuth] connect() called with empty token');
      return;
    }

    // Clean token from any "Bearer " prefix so Drogon / Drogon JWT can verify cleanly
    const cleanToken = token.replace(/^Bearer\s+/i, '').trim();
    if (!cleanToken) return;

    // If already connected with the same token and hwid, do not reconnect
    if (
      this.ws &&
      (this.ws.readyState === WebSocket.OPEN || this.ws.readyState === WebSocket.CONNECTING) &&
      this.currentToken === cleanToken &&
      this.currentHwid === hwid
    ) {
      return;
    }

    // Close any previous stale connection
    if (this.ws) {
      try {
        this.ws.close();
      } catch (_) {}
      this.ws = null;
    }

    this.currentToken = cleanToken;
    this.currentHwid = hwid;
    this.currentDeviceName = deviceName;
    this.currentBackendUrl = backendUrl || 'https://apie.rena.my.id';
    this.shouldReconnect = true;
    this.isConnecting = true;

    try {
      const wsBase = this.currentBackendUrl
        .replace(/^http:/i, 'ws:')
        .replace(/^https:/i, 'wss:');

      const params = new URLSearchParams();
      params.set('token', cleanToken);
      params.set('client', 'desktop');
      if (this.currentHwid) {
        params.set('hwid', this.currentHwid);
      }
      if (this.currentDeviceName) {
        params.set('device_name', this.currentDeviceName);
      }

      const wsUrl = `${wsBase}/ws/auth?${params.toString()}`;
      console.log('[SocketAuth] Connecting to real-time security websocket...');

      this.ws = new WebSocket(wsUrl);

      this.ws.onopen = () => {
        this.isConnecting = false;
        console.log('[SocketAuth] Connected to auth socket (Real-time Kick Protection Active)');
        try {
          useDiagnosticsStore().addLog('Connected to real-time security socket (Kick protection active).', 'success');
        } catch (_) {}
        this.startPing();
      };

      this.ws.onmessage = (event) => {
        if (event.data === 'pong') {
          return;
        }

        const raw = typeof event.data === 'string' ? event.data : '';
        console.log('[SocketAuth] Received socket message:', raw);

        // 1. Plain text unauthorized or termination from server
        const lower = raw.toLowerCase();
        if (
          lower.includes('unauthorized') ||
          lower.includes('session terminated') ||
          lower.includes('force_logout') ||
          lower.includes('revoked')
        ) {
          console.warn('[SocketAuth] Termination plain text message:', raw);
          this.triggerKick(raw || 'Session terminated or unlinked by administrator.');
          return;
        }

        // 2. Structured JSON kick event
        try {
          const data = JSON.parse(raw);
          const ev = String(data?.event || data?.type || data?.action || '').toUpperCase();
          if (
            ev === 'FORCE_LOGOUT' ||
            ev === 'KICK' ||
            ev === 'USER_KICKED' ||
            ev === 'LOGOUT' ||
            ev === 'DISCONNECTED'
          ) {
            const reason =
              data.message ||
              data.reason ||
              'Your session was terminated or device unlinked by administrator.';
            console.warn('[SocketAuth] FORCE_LOGOUT received from server:', reason);
            this.triggerKick(reason);
          }
        } catch (_) {
          // Plain message like "Connected to server successfully"
        }
      };

      this.ws.onerror = (err) => {
        console.warn('[SocketAuth] Socket error:', err);
        this.isConnecting = false;
        this.triggerClose();
      };

      this.ws.onclose = (event) => {
        console.log('[SocketAuth] Socket connection closed, code:', event.code, 'reason:', event.reason);
        this.cleanup();

        // Immediately notify listeners to verify active HTTP session
        this.triggerClose();

        if (this.shouldReconnect) {
          this.scheduleReconnect();
        }
      };
    } catch (err) {
      console.error('[SocketAuth] Failed to initialize websocket:', err);
      this.isConnecting = false;
      this.triggerClose();
      if (this.shouldReconnect) {
        this.scheduleReconnect();
      }
    }
  }

  public disconnect(): void {
    this.shouldReconnect = false;
    if (this.reconnectTimeout !== null) {
      clearTimeout(this.reconnectTimeout);
      this.reconnectTimeout = null;
    }
    if (this.ws) {
      try {
        this.ws.close();
      } catch (_) {}
    }
    this.cleanup();
  }

  private startPing(): void {
    this.stopPing();
    this.sendPing();

    // Ping every 10 seconds to maintain heartbeat and detect disconnections
    this.pingInterval = window.setInterval(() => {
      this.sendPing();
    }, 10000);
  }

  private sendPing(): void {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send('ping');
    }
  }

  private stopPing(): void {
    if (this.pingInterval !== null) {
      clearInterval(this.pingInterval);
      this.pingInterval = null;
    }
  }

  private scheduleReconnect(): void {
    if (this.reconnectTimeout !== null) {
      clearTimeout(this.reconnectTimeout);
    }
    this.reconnectTimeout = window.setTimeout(() => {
      if (this.shouldReconnect && this.currentToken) {
        console.log('[SocketAuth] Reconnecting security socket...');
        this.connect(this.currentToken, this.currentHwid, this.currentBackendUrl, this.currentDeviceName);
      }
    }, 3000);
  }

  private cleanup(): void {
    this.stopPing();
    this.ws = null;
    this.isConnecting = false;
  }
}

export const socketAuth = new SocketAuthService();
export default socketAuth;
