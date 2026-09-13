import { defineStore } from 'pinia';
import type { UpdateProgress, UpdateStatus } from '../types/loader';

export const useUpdaterStore = defineStore('updater', {
  state: () => ({
    isChecking: false,
    hasUpdate: false,
    isMandatory: false,
    latestVersion: '',
    releaseNotes: '',
    state: 'idle' as 'idle' | 'downloading' | 'paused' | 'verifying' | 'ready-to-install' | 'error',
    downloadedBytes: 0,
    totalBytes: 0,
    percent: 0,
    speed: 0,
    error: null as string | null,
    modalVisible: false,
    listenersAttached: false
  }),

  getters: {
    isDownloading: (state) => state.state === 'downloading',
    isPaused: (state) => state.state === 'paused',
    isReadyToInstall: (state) => state.state === 'ready-to-install',
    isVerifying: (state) => state.state === 'verifying',
    formattedDownloaded: (state) => {
      const mb = (state.downloadedBytes / (1024 * 1024)).toFixed(1);
      return `${mb} MB`;
    },
    formattedTotal: (state) => {
      const mb = (state.totalBytes / (1024 * 1024)).toFixed(1);
      return `${mb} MB`;
    },
    formattedSpeed: (state) => {
      if (!state.speed || state.speed <= 0) return '0 KB/s';
      if (state.speed > 1024 * 1024) {
        return `${(state.speed / (1024 * 1024)).toFixed(2)} MB/s`;
      }
      return `${Math.round(state.speed / 1024)} KB/s`;
    }
  },

  actions: {
    initListeners() {
      if (this.listenersAttached || !window.loader?.updater) return;
      this.listenersAttached = true;

      window.loader.updater.onProgress((data: UpdateProgress) => {
        this.downloadedBytes = data.downloadedBytes;
        this.totalBytes = data.totalBytes;
        this.percent = data.percent;
        this.speed = data.speed;
        this.state = data.state;
      });

      window.loader.updater.onStatus((data: UpdateStatus) => {
        this.hasUpdate = data.hasUpdate;
        this.isMandatory = data.isMandatory;
        this.latestVersion = data.latestVersion;
        this.releaseNotes = data.releaseNotes || '';
        this.state = data.state;
        this.downloadedBytes = data.downloadedBytes;
        this.totalBytes = data.totalBytes;
        this.percent = data.percent;
        this.speed = data.speed;
        this.error = data.error || null;

        if (data.state === 'ready-to-install' || data.isMandatory) {
          this.modalVisible = true;
        }
      });
    },

    async checkForUpdates(silent = false) {
      if (!window.loader?.updater) return;
      this.initListeners();
      this.isChecking = true;
      this.error = null;

      try {
        const res = await window.loader.updater.checkUpdate();
        this.hasUpdate = res.hasUpdate;
        this.isMandatory = res.isMandatory;
        this.latestVersion = res.latestVersion;
        this.releaseNotes = res.releaseNotes || '';
        this.state = res.state;
        this.downloadedBytes = res.downloadedBytes;
        this.totalBytes = res.totalBytes;
        this.percent = res.percent;

        if (res.hasUpdate) {
          this.modalVisible = true;
        } else if (!silent) {
          console.log('[AutoUpdater] Gottvergessen Loader is already on the latest version.');
        }

        // Sync auxiliary modules if needed
        if (window.loader.updater.syncModules) {
          window.loader.updater.syncModules().catch(() => {});
        }
      } catch (err: any) {
        this.error = err.message;
      } finally {
        this.isChecking = false;
      }
    },

    async startDownload() {
      if (!window.loader?.updater) return;
      this.initListeners();
      this.error = null;
      try {
        const res = await window.loader.updater.startDownload();
        if (!res.success && res.error) {
          this.error = res.error;
          this.state = 'error';
        }
      } catch (err: any) {
        this.error = err.message;
        this.state = 'error';
      }
    },

    async pauseDownload() {
      if (!window.loader?.updater) return;
      try {
        await window.loader.updater.pauseDownload();
        this.state = 'paused';
      } catch (err: any) {
        this.error = err.message;
      }
    },

    async cancelDownload() {
      if (!window.loader?.updater) return;
      try {
        await window.loader.updater.cancelDownload();
        if (!this.isMandatory) {
          this.modalVisible = false;
        }
      } catch (err: any) {
        this.error = err.message;
      }
    },

    async installUpdate() {
      if (!window.loader?.updater) return;
      try {
        await window.loader.updater.install();
      } catch (err: any) {
        this.error = err.message;
      }
    },

    dismissModal() {
      if (this.isMandatory) return; // Cannot dismiss mandatory update
      this.modalVisible = false;
    }
  }
});
