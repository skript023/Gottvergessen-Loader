const fs = require('fs');
const path = require('path');

const ADMIN_ROOT = 'L:\\Coding\\Ellohim\\Ellohim-Admin';

// Ensure directory exists
const clientModDir = path.join(ADMIN_ROOT, 'src', 'modules', 'client');
const clientDtoDir = path.join(clientModDir, 'dto');
fs.mkdirSync(clientDtoDir, { recursive: true });

// 1. Create client.dto.ts
const dtoContent = `export interface ClientRelease {
  id: string;
  version: string;
  release_notes?: string;
  file_name: string;
  file_size: number;
  checksum: string;
  is_mandatory: boolean;
  min_supported_version?: string;
  status: string;
  download_count: number;
  download_url?: string;
  created_at: string;
  updated_at: string;
}

export interface ClientModule {
  id: string;
  name: string;
  version: string;
  target_path: string;
  file_name: string;
  file_size: number;
  checksum: string;
  is_required: boolean;
  status: string;
  download_url?: string;
  created_at: string;
  updated_at: string;
}

export interface CheckUpdateSimulation {
  has_update: boolean;
  is_mandatory: boolean;
  current_version: string;
  latest_version: string;
  latest_release?: ClientRelease;
  modules: ClientModule[];
}
`;
fs.writeFileSync(path.join(clientDtoDir, 'client.dto.ts'), dtoContent, 'utf8');
console.log('Created client.dto.ts');

// 2. Create service.ts
const serviceContent = `import { http, type ServerResponse } from "@/util/http";
import type { ClientRelease, ClientModule, CheckUpdateSimulation } from "./dto/client.dto";

export default class ClientService {
  static async findAllReleases(params?: { page?: number; limit?: number }): Promise<ServerResponse<ClientRelease[]>> {
    try {
      const response = await http.get('/client/admin/releases', { params });
      return response.data;
    } catch (error: any) {
      return error.response?.data || { success: false, message: 'Failed to fetch client releases', data: [], total: 0 };
    }
  }

  static async uploadRelease(
    formData: FormData,
    onProgress?: (progress: number, loaded: number, total: number) => void
  ): Promise<ServerResponse<ClientRelease>> {
    try {
      const response = await http.post('/client/admin/release/upload', formData, {
        headers: { 'Content-Type': 'multipart/form-data' },
        onUploadProgress: (progressEvent) => {
          const loaded = progressEvent.loaded || 0;
          const total = progressEvent.total || 0;
          const percent = total > 0 ? Math.round((loaded * 100) / total) : 0;
          if (onProgress) onProgress(percent, loaded, total);
        }
      });
      return response.data;
    } catch (error: any) {
      return error.response?.data || { success: false, message: 'Failed to upload client release' };
    }
  }

  static async updateRelease(id: string, payload: Partial<ClientRelease>): Promise<ServerResponse<ClientRelease>> {
    try {
      const response = await http.patch(\`/client/admin/release/\${id}\`, payload);
      return response.data;
    } catch (error: any) {
      return error.response?.data || { success: false, message: 'Failed to update client release' };
    }
  }

  static async removeRelease(id: string): Promise<ServerResponse<any>> {
    try {
      const response = await http.delete(\`/client/admin/release/\${id}\`);
      return response.data;
    } catch (error: any) {
      return error.response?.data || { success: false, message: 'Failed to delete release' };
    }
  }

  static async findAllModules(): Promise<ServerResponse<ClientModule[]>> {
    try {
      const response = await http.get('/client/admin/modules');
      return response.data;
    } catch (error: any) {
      return error.response?.data || { success: false, message: 'Failed to fetch modules', data: [] };
    }
  }

  static async uploadModule(
    formData: FormData,
    onProgress?: (progress: number, loaded: number, total: number) => void
  ): Promise<ServerResponse<ClientModule>> {
    try {
      const response = await http.post('/client/admin/module/upload', formData, {
        headers: { 'Content-Type': 'multipart/form-data' },
        onUploadProgress: (progressEvent) => {
          const loaded = progressEvent.loaded || 0;
          const total = progressEvent.total || 0;
          const percent = total > 0 ? Math.round((loaded * 100) / total) : 0;
          if (onProgress) onProgress(percent, loaded, total);
        }
      });
      return response.data;
    } catch (error: any) {
      return error.response?.data || { success: false, message: 'Failed to deploy module' };
    }
  }

  static async updateModule(id: string, payload: Partial<ClientModule>): Promise<ServerResponse<ClientModule>> {
    try {
      const response = await http.patch(\`/client/admin/module/\${id}\`, payload);
      return response.data;
    } catch (error: any) {
      return error.response?.data || { success: false, message: 'Failed to update module' };
    }
  }

  static async removeModule(id: string): Promise<ServerResponse<any>> {
    try {
      const response = await http.delete(\`/client/admin/module/\${id}\`);
      return response.data;
    } catch (error: any) {
      return error.response?.data || { success: false, message: 'Failed to delete module' };
    }
  }

  static async simulateCheckUpdate(version: string): Promise<ServerResponse<CheckUpdateSimulation>> {
    try {
      const response = await http.get('/client/check-update', { params: { version } });
      return response.data;
    } catch (error: any) {
      return error.response?.data || { success: false, message: 'Simulation request failed' };
    }
  }
}
`;
fs.writeFileSync(path.join(clientModDir, 'service.ts'), serviceContent, 'utf8');
console.log('Created service.ts');

// 3. Create ClientManager.vue
const vueContent = `<template>
  <Navigation title="Client Management & Updates">
    <div class="space-y-6">
      <!-- Header Banner -->
      <div class="flex flex-col md:flex-row justify-between items-start md:items-center bg-base-100 p-6 rounded-2xl border border-base-300 shadow-sm gap-4">
        <div>
          <div class="flex items-center gap-3">
            <div class="w-10 h-10 rounded-xl bg-primary/10 flex items-center justify-center text-primary">
              <i class="ph ph-app-window-bold text-2xl"></i>
            </div>
            <div>
              <h1 class="text-2xl font-bold tracking-tight text-base-content flex items-center gap-2">
                Client Management & Auto-Update
                <span class="badge badge-primary badge-sm font-mono">Astra</span>
              </h1>
              <p class="text-xs text-base-content/70 mt-0.5">
                Distribute versioned portable client executables, enforce mandatory updates, and hot-deploy dynamic binary modules.
              </p>
            </div>
          </div>
        </div>
        <div class="flex items-center gap-2 w-full md:w-auto">
          <button v-if="activeTab === 'releases'" class="btn btn-primary gap-2 btn-sm md:btn-md flex-1 md:flex-initial" @click="openUploadReleaseModal">
            <i class="ph ph-upload-simple-bold"></i>
            Upload Client Release (.exe)
          </button>
          <button v-if="activeTab === 'modules'" class="btn btn-secondary gap-2 btn-sm md:btn-md flex-1 md:flex-initial" @click="openUploadModuleModal">
            <i class="ph ph-cube-bold"></i>
            Deploy Module (.dll / .bin)
          </button>
          <button class="btn btn-outline btn-sm md:btn-md gap-2" @click="refreshCurrent">
            <i class="ph ph-arrows-clockwise" :class="{ 'animate-spin': loading }"></i>
            Refresh
          </button>
        </div>
      </div>

      <!-- Quick Metrics -->
      <div class="grid grid-cols-1 md:grid-cols-4 gap-4">
        <div class="stat bg-base-100 rounded-xl border border-base-300 shadow-sm">
          <div class="stat-figure text-primary">
            <i class="ph ph-git-branch text-3xl"></i>
          </div>
          <div class="stat-title text-xs font-semibold uppercase tracking-wider">Total Releases</div>
          <div class="stat-value text-2xl text-primary">{{ releases.length }}</div>
          <div class="stat-desc">Versioned client builds</div>
        </div>

        <div class="stat bg-base-100 rounded-xl border border-base-300 shadow-sm">
          <div class="stat-figure text-success">
            <i class="ph ph-check-circle text-3xl"></i>
          </div>
          <div class="stat-title text-xs font-semibold uppercase tracking-wider">Latest Version</div>
          <div class="stat-value text-2xl text-success font-mono">{{ latestReleaseVersion || 'None' }}</div>
          <div class="stat-desc">{{ latestReleaseDate || 'No builds uploaded' }}</div>
        </div>

        <div class="stat bg-base-100 rounded-xl border border-base-300 shadow-sm">
          <div class="stat-figure text-secondary">
            <i class="ph ph-download-simple text-3xl"></i>
          </div>
          <div class="stat-title text-xs font-semibold uppercase tracking-wider">Total Downloads</div>
          <div class="stat-value text-2xl text-secondary">{{ totalDownloads.toLocaleString() }}</div>
          <div class="stat-desc">Resumable chunk downloads</div>
        </div>

        <div class="stat bg-base-100 rounded-xl border border-base-300 shadow-sm">
          <div class="stat-figure text-warning">
            <i class="ph ph-cube text-3xl"></i>
          </div>
          <div class="stat-title text-xs font-semibold uppercase tracking-wider">Active Modules</div>
          <div class="stat-value text-2xl text-warning">{{ modules.length }}</div>
          <div class="stat-desc">Hot-deploy payload modules</div>
        </div>
      </div>

      <!-- Navigation Tabs -->
      <div class="tabs tabs-boxed bg-base-100 p-1 border border-base-300 w-fit">
        <button
          class="tab tab-md gap-2"
          :class="{ 'tab-active': activeTab === 'releases' }"
          @click="activeTab = 'releases'"
        >
          <i class="ph ph-app-window"></i>
          Client Releases (.exe)
          <span class="badge badge-sm" :class="activeTab === 'releases' ? 'badge-primary' : 'badge-ghost'">{{ releases.length }}</span>
        </button>
        <button
          class="tab tab-md gap-2"
          :class="{ 'tab-active': activeTab === 'modules' }"
          @click="activeTab = 'modules'"
        >
          <i class="ph ph-cube"></i>
          Dynamic Modules (.dll)
          <span class="badge badge-sm" :class="activeTab === 'modules' ? 'badge-secondary' : 'badge-ghost'">{{ modules.length }}</span>
        </button>
        <button
          class="tab tab-md gap-2"
          :class="{ 'tab-active': activeTab === 'simulator' }"
          @click="activeTab = 'simulator'"
        >
          <i class="ph ph-terminal-window"></i>
          Update Gate Simulator
        </button>
      </div>

      <!-- TAB 1: Client Releases -->
      <div v-if="activeTab === 'releases'" class="card bg-base-100 border border-base-300 shadow-sm">
        <div class="card-body p-6">
          <div class="flex justify-between items-center mb-4">
            <div>
              <h2 class="text-base font-bold text-base-content">Versioned Executable Builds</h2>
              <p class="text-xs text-base-content/60">Upload portable .exe files for Astra auto-update system.</p>
            </div>
          </div>

          <div class="overflow-x-auto">
            <table class="table table-zebra w-full text-xs">
              <thead>
                <tr class="text-base-content/70">
                  <th>Version</th>
                  <th>Executable & Size</th>
                  <th>SHA-256 Checksum</th>
                  <th>Mandatory Gate</th>
                  <th>Min Version</th>
                  <th>Downloads</th>
                  <th>Status</th>
                  <th>Uploaded</th>
                  <th class="text-right">Actions</th>
                </tr>
              </thead>
              <tbody>
                <tr v-if="loading && releases.length === 0">
                  <td colspan="9" class="text-center py-8 text-base-content/50">
                    <span class="loading loading-spinner loading-md"></span>
                    <p class="mt-2">Loading client releases...</p>
                  </td>
                </tr>
                <tr v-else-if="releases.length === 0">
                  <td colspan="9" class="text-center py-8 text-base-content/40">
                    <i class="ph ph-folder-open text-4xl block mb-2 opacity-50"></i>
                    No client releases uploaded yet. Click "Upload Client Release" to deploy your first version.
                  </td>
                </tr>
                <tr v-for="rel in releases" :key="rel.id" class="hover">
                  <td>
                    <div class="flex items-center gap-2">
                      <span class="font-bold font-mono text-sm text-primary">v{{ rel.version }}</span>
                      <span v-if="rel.version === latestReleaseVersion" class="badge badge-success badge-xs font-bold uppercase">Latest</span>
                    </div>
                    <div v-if="rel.release_notes" class="text-[11px] text-base-content/60 max-w-xs truncate" :title="rel.release_notes">
                      {{ rel.release_notes }}
                    </div>
                  </td>
                  <td>
                    <div class="font-medium font-mono text-xs">{{ rel.file_name }}</div>
                    <div class="text-[11px] text-base-content/50">{{ formatBytes(rel.file_size) }}</div>
                  </td>
                  <td>
                    <div class="flex items-center gap-1.5">
                      <span class="font-mono text-[10px] bg-base-200 px-2 py-0.5 rounded max-w-[140px] truncate" :title="rel.checksum">
                        {{ rel.checksum }}
                      </span>
                      <button class="btn btn-ghost btn-xs btn-square" @click="copyText(rel.checksum)" title="Copy SHA-256">
                        <i class="ph ph-copy text-xs"></i>
                      </button>
                    </div>
                  </td>
                  <td>
                    <label class="cursor-pointer flex items-center gap-2">
                      <input
                        type="checkbox"
                        class="toggle toggle-primary toggle-sm"
                        :checked="rel.is_mandatory"
                        @change="toggleMandatory(rel)"
                      />
                      <span class="text-xs" :class="rel.is_mandatory ? 'text-error font-bold' : 'text-base-content/60'">
                        {{ rel.is_mandatory ? 'MANDATORY' : 'Optional' }}
                      </span>
                    </label>
                  </td>
                  <td>
                    <span v-if="rel.min_supported_version" class="badge badge-outline badge-sm font-mono">
                      &gt;= v{{ rel.min_supported_version }}
                    </span>
                    <span v-else class="text-base-content/40 italic">None</span>
                  </td>
                  <td>
                    <span class="font-mono font-bold">{{ rel.download_count }}</span>
                  </td>
                  <td>
                    <span class="badge badge-sm" :class="rel.status === 'active' ? 'badge-success' : 'badge-ghost'">
                      {{ rel.status }}
                    </span>
                  </td>
                  <td>
                    <span class="text-[11px] text-base-content/60">{{ formatDate(rel.created_at) }}</span>
                  </td>
                  <td class="text-right">
                    <div class="flex justify-end gap-1">
                      <button
                        class="btn btn-ghost btn-xs btn-square text-info"
                        @click="copyDirectDownload(rel.id)"
                        title="Copy Download URL"
                      >
                        <i class="ph ph-link text-sm"></i>
                      </button>
                      <button
                        class="btn btn-ghost btn-xs btn-square text-error"
                        @click="confirmDeleteRelease(rel)"
                        title="Delete Release"
                      >
                        <i class="ph ph-trash text-sm"></i>
                      </button>
                    </div>
                  </td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>

      <!-- TAB 2: Dynamic Modules -->
      <div v-if="activeTab === 'modules'" class="card bg-base-100 border border-base-300 shadow-sm">
        <div class="card-body p-6">
          <div class="flex justify-between items-center mb-4">
            <div>
              <h2 class="text-base font-bold text-base-content">Dynamic Binary Modules (.dll / .bin)</h2>
              <p class="text-xs text-base-content/60">Hot-deploy auxiliary modules, anti-cheat bypasses, and game plugins directly to client installations.</p>
            </div>
          </div>

          <div class="overflow-x-auto">
            <table class="table table-zebra w-full text-xs">
              <thead>
                <tr class="text-base-content/70">
                  <th>Module Name</th>
                  <th>Version</th>
                  <th>File & Size</th>
                  <th>Target Directory</th>
                  <th>SHA-256 Checksum</th>
                  <th>Required</th>
                  <th>Status</th>
                  <th>Deployed</th>
                  <th class="text-right">Actions</th>
                </tr>
              </thead>
              <tbody>
                <tr v-if="loading && modules.length === 0">
                  <td colspan="9" class="text-center py-8 text-base-content/50">
                    <span class="loading loading-spinner loading-md"></span>
                    <p class="mt-2">Loading dynamic modules...</p>
                  </td>
                </tr>
                <tr v-else-if="modules.length === 0">
                  <td colspan="9" class="text-center py-8 text-base-content/40">
                    <i class="ph ph-cube text-4xl block mb-2 opacity-50"></i>
                    No dynamic modules deployed yet. Click "Deploy Module" to publish your first DLL.
                  </td>
                </tr>
                <tr v-for="mod in modules" :key="mod.id" class="hover">
                  <td>
                    <div class="font-bold text-sm text-base-content flex items-center gap-1.5">
                      <i class="ph ph-cube-fill text-secondary"></i>
                      {{ mod.name }}
                    </div>
                  </td>
                  <td>
                    <span class="badge badge-secondary badge-outline font-mono text-xs">v{{ mod.version }}</span>
                  </td>
                  <td>
                    <div class="font-mono text-xs">{{ mod.file_name }}</div>
                    <div class="text-[11px] text-base-content/50">{{ formatBytes(mod.file_size) }}</div>
                  </td>
                  <td>
                    <span class="font-mono text-[11px] bg-base-200 px-2 py-0.5 rounded text-base-content/80">
                      {{ mod.target_path }}
                    </span>
                  </td>
                  <td>
                    <div class="flex items-center gap-1.5">
                      <span class="font-mono text-[10px] bg-base-200 px-2 py-0.5 rounded max-w-[130px] truncate" :title="mod.checksum">
                        {{ mod.checksum }}
                      </span>
                      <button class="btn btn-ghost btn-xs btn-square" @click="copyText(mod.checksum)" title="Copy SHA-256">
                        <i class="ph ph-copy text-xs"></i>
                      </button>
                    </div>
                  </td>
                  <td>
                    <span class="badge badge-sm" :class="mod.is_required ? 'badge-primary' : 'badge-ghost'">
                      {{ mod.is_required ? 'Mandatory' : 'Optional' }}
                    </span>
                  </td>
                  <td>
                    <span class="badge badge-sm" :class="mod.status === 'active' ? 'badge-success' : 'badge-ghost'">
                      {{ mod.status }}
                    </span>
                  </td>
                  <td>
                    <span class="text-[11px] text-base-content/60">{{ formatDate(mod.created_at) }}</span>
                  </td>
                  <td class="text-right">
                    <div class="flex justify-end gap-1">
                      <button
                        class="btn btn-ghost btn-xs btn-square text-error"
                        @click="confirmDeleteModule(mod)"
                        title="Delete Module"
                      >
                        <i class="ph ph-trash text-sm"></i>
                      </button>
                    </div>
                  </td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>

      <!-- TAB 3: Gate Simulator -->
      <div v-if="activeTab === 'simulator'" class="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <div class="card bg-base-100 border border-base-300 shadow-sm">
          <div class="card-body p-6">
            <h2 class="text-base font-bold text-base-content flex items-center gap-2">
              <i class="ph ph-terminal-window text-primary text-xl"></i>
              Simulate Client Version Check
            </h2>
            <p class="text-xs text-base-content/60 mb-4">
              Enter any client version string to verify how the backend auto-update engine evaluates release gates and module synchronization.
            </p>

            <div class="form-control">
              <label class="label">
                <span class="label-text font-semibold text-xs">Test Client Version</span>
              </label>
              <div class="join w-full">
                <input
                  v-model="simulationVersion"
                  type="text"
                  placeholder="e.g. 1.0.0, 0.9.8, 1.2.0"
                  class="input input-bordered join-item font-mono text-sm w-full"
                  @keyup.enter="runSimulation"
                />
                <button class="btn btn-primary join-item gap-2" :disabled="simulating" @click="runSimulation">
                  <i class="ph ph-play-bold" :class="{ 'animate-spin': simulating }"></i>
                  Evaluate
                </button>
              </div>
            </div>

            <!-- Quick presets -->
            <div class="flex items-center gap-2 mt-4 text-xs text-base-content/70">
              <span>Quick tests:</span>
              <button class="btn btn-ghost btn-xs font-mono" @click="simulationVersion = '0.9.0'; runSimulation()">v0.9.0</button>
              <button class="btn btn-ghost btn-xs font-mono" @click="simulationVersion = '1.0.0'; runSimulation()">v1.0.0</button>
              <button class="btn btn-ghost btn-xs font-mono" @click="simulationVersion = latestReleaseVersion || '1.1.0'; runSimulation()">
                v{{ latestReleaseVersion || '1.1.0' }} (Latest)
              </button>
            </div>
          </div>
        </div>

        <!-- Simulation Result -->
        <div class="card bg-base-100 border border-base-300 shadow-sm">
          <div class="card-body p-6">
            <h2 class="text-base font-bold text-base-content mb-2">Gate Decision Result</h2>

            <div v-if="!simulationResult" class="py-12 text-center text-base-content/40">
              <i class="ph ph-magnifying-glass text-4xl block mb-2 opacity-50"></i>
              Run a test evaluation to see client gate outcome.
            </div>

            <div v-else class="space-y-4">
              <div
                class="alert"
                :class="simulationResult.has_update
                  ? (simulationResult.is_mandatory ? 'alert-error' : 'alert-warning')
                  : 'alert-success'"
              >
                <i
                  class="ph text-2xl"
                  :class="simulationResult.has_update
                    ? (simulationResult.is_mandatory ? 'ph-prohibit' : 'ph-arrow-circle-up')
                    : 'ph-check-circle'"
                ></i>
                <div>
                  <div class="font-bold text-sm">
                    {{ simulationResult.has_update
                      ? (simulationResult.is_mandatory ? 'MANDATORY UPDATE REQUIRED' : 'OPTIONAL UPDATE AVAILABLE')
                      : 'CLIENT IS UP TO DATE' }}
                  </div>
                  <div class="text-xs opacity-85">
                    Client Version: <span class="font-mono font-bold">v{{ simulationResult.current_version }}</span>
                    &nbsp;|&nbsp;
                    Latest: <span class="font-mono font-bold">v{{ simulationResult.latest_version }}</span>
                  </div>
                </div>
              </div>

              <div class="bg-base-200 rounded-xl p-4 font-mono text-xs overflow-x-auto">
                <div class="text-base-content/60 font-semibold mb-2">JSON Response:</div>
                <pre class="text-[11px]">{{ JSON.stringify(simulationResult, null, 2) }}</pre>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Upload Release Modal -->
      <dialog ref="uploadReleaseModalRef" class="modal">
        <div class="modal-box max-w-xl">
          <h3 class="font-bold text-lg text-base-content flex items-center gap-2">
            <i class="ph ph-upload-simple-bold text-primary"></i>
            Upload Client Executable Release
          </h3>
          <p class="text-xs text-base-content/60 mt-1">
            Upload the compiled portable .exe (e.g. Astra-1.0.1-portable.exe).
          </p>

          <form class="space-y-4 mt-4" @submit.prevent="submitReleaseUpload">
            <!-- File input -->
            <div class="form-control">
              <label class="label">
                <span class="label-text font-semibold text-xs">Executable File (.exe) *</span>
              </label>
              <input
                type="file"
                accept=".exe"
                class="file-input file-input-bordered file-input-primary w-full text-xs"
                @change="handleReleaseFileSelect"
                required
              />
              <span v-if="releaseFile" class="text-[11px] text-base-content/60 mt-1">
                Selected: <span class="font-mono font-bold">{{ releaseFile.name }}</span> ({{ formatBytes(releaseFile.size) }})
              </span>
            </div>

            <div class="grid grid-cols-2 gap-4">
              <div class="form-control">
                <label class="label">
                  <span class="label-text font-semibold text-xs">Release Version *</span>
                </label>
                <input
                  v-model="releaseForm.version"
                  type="text"
                  placeholder="e.g. 1.0.1"
                  class="input input-bordered font-mono text-xs"
                  required
                />
              </div>

              <div class="form-control">
                <label class="label">
                  <span class="label-text font-semibold text-xs">Min Supported Version</span>
                </label>
                <input
                  v-model="releaseForm.min_supported_version"
                  type="text"
                  placeholder="e.g. 1.0.0 (older will force update)"
                  class="input input-bordered font-mono text-xs"
                />
              </div>
            </div>

            <div class="form-control">
              <label class="label">
                <span class="label-text font-semibold text-xs">Release Notes / Changelog</span>
              </label>
              <textarea
                v-model="releaseForm.release_notes"
                rows="3"
                placeholder="List features, bugfixes, and enhancements in this release..."
                class="textarea textarea-bordered text-xs"
              ></textarea>
            </div>

            <div class="form-control">
              <label class="cursor-pointer label justify-start gap-3">
                <input
                  v-model="releaseForm.is_mandatory"
                  type="checkbox"
                  class="checkbox checkbox-primary checkbox-sm"
                />
                <span class="label-text text-xs font-semibold">
                  Mandatory Update Gate (Clients below this version will be forced to update before playing)
                </span>
              </label>
            </div>

            <!-- Upload Progress -->
            <div v-if="uploadingRelease" class="space-y-1">
              <div class="flex justify-between text-xs font-semibold">
                <span>Uploading Executable...</span>
                <span>{{ releaseUploadProgress }}%</span>
              </div>
              <progress class="progress progress-primary w-full" :value="releaseUploadProgress" max="100"></progress>
            </div>

            <div class="modal-action">
              <button type="button" class="btn btn-ghost btn-sm" :disabled="uploadingRelease" @click="closeUploadReleaseModal">
                Cancel
              </button>
              <button type="submit" class="btn btn-primary btn-sm gap-2" :disabled="uploadingRelease || !releaseFile">
                <span v-if="uploadingRelease" class="loading loading-spinner loading-xs"></span>
                <i v-else class="ph ph-upload-simple-bold"></i>
                Publish Release
              </button>
            </div>
          </form>
        </div>
        <form method="dialog" class="modal-backdrop">
          <button :disabled="uploadingRelease">close</button>
        </form>
      </dialog>

      <!-- Upload Module Modal -->
      <dialog ref="uploadModuleModalRef" class="modal">
        <div class="modal-box max-w-lg">
          <h3 class="font-bold text-lg text-base-content flex items-center gap-2">
            <i class="ph ph-cube-bold text-secondary"></i>
            Deploy Dynamic Client Module
          </h3>
          <p class="text-xs text-base-content/60 mt-1">
            Deploy runtime libraries (.dll / .bin) to client machines during update checks.
          </p>

          <form class="space-y-4 mt-4" @submit.prevent="submitModuleUpload">
            <div class="form-control">
              <label class="label">
                <span class="label-text font-semibold text-xs">Module File (.dll / .bin) *</span>
              </label>
              <input
                type="file"
                accept=".dll,.bin"
                class="file-input file-input-bordered file-input-secondary w-full text-xs"
                @change="handleModuleFileSelect"
                required
              />
              <span v-if="moduleFile" class="text-[11px] text-base-content/60 mt-1">
                Selected: <span class="font-mono font-bold">{{ moduleFile.name }}</span> ({{ formatBytes(moduleFile.size) }})
              </span>
            </div>

            <div class="grid grid-cols-2 gap-4">
              <div class="form-control">
                <label class="label">
                  <span class="label-text font-semibold text-xs">Module Identifier *</span>
                </label>
                <input
                  v-model="moduleForm.name"
                  type="text"
                  placeholder="e.g. native-core, dxgi_hook"
                  class="input input-bordered font-mono text-xs"
                  required
                />
              </div>

              <div class="form-control">
                <label class="label">
                  <span class="label-text font-semibold text-xs">Module Version *</span>
                </label>
                <input
                  v-model="moduleForm.version"
                  type="text"
                  placeholder="e.g. 1.0.0"
                  class="input input-bordered font-mono text-xs"
                  required
                />
              </div>
            </div>

            <div class="form-control">
              <label class="label">
                <span class="label-text font-semibold text-xs">Client Target Folder</span>
              </label>
              <input
                v-model="moduleForm.target_path"
                type="text"
                placeholder="e.g. modules, native (relative to client appData)"
                class="input input-bordered font-mono text-xs"
              />
            </div>

            <div class="form-control">
              <label class="cursor-pointer label justify-start gap-3">
                <input
                  v-model="moduleForm.is_required"
                  type="checkbox"
                  class="checkbox checkbox-secondary checkbox-sm"
                />
                <span class="label-text text-xs font-semibold">
                  Required Module (Client will automatically download before launching)
                </span>
              </label>
            </div>

            <div v-if="uploadingModule" class="space-y-1">
              <div class="flex justify-between text-xs font-semibold">
                <span>Deploying Module...</span>
                <span>{{ moduleUploadProgress }}%</span>
              </div>
              <progress class="progress progress-secondary w-full" :value="moduleUploadProgress" max="100"></progress>
            </div>

            <div class="modal-action">
              <button type="button" class="btn btn-ghost btn-sm" :disabled="uploadingModule" @click="closeUploadModuleModal">
                Cancel
              </button>
              <button type="submit" class="btn btn-secondary btn-sm gap-2" :disabled="uploadingModule || !moduleFile">
                <span v-if="uploadingModule" class="loading loading-spinner loading-xs"></span>
                <i v-else class="ph ph-upload-simple-bold"></i>
                Deploy Module
              </button>
            </div>
          </form>
        </div>
        <form method="dialog" class="modal-backdrop">
          <button :disabled="uploadingModule">close</button>
        </form>
      </dialog>

      <!-- Delete Confirmation -->
      <ConfirmDialog
        ref="confirmDialogRef"
        title="Confirm Deletion"
        :message="confirmMessage"
        @confirm="executeDelete"
      />

      <!-- Toast Notification -->
      <Notification ref="notificationRef" />
    </div>
  </Navigation>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import Navigation from '@/components/Navigation.vue'
import ConfirmDialog from '@/components/ConfirmDialog.vue'
import Notification from '@/components/Notification.vue'
import ClientService from './service'
import type { ClientRelease, ClientModule, CheckUpdateSimulation } from './dto/client.dto'

const activeTab = ref<'releases' | 'modules' | 'simulator'>('releases')
const loading = ref(false)

const releases = ref<ClientRelease[]>([])
const modules = ref<ClientModule[]>([])

const confirmDialogRef = ref()
const notificationRef = ref()
const uploadReleaseModalRef = ref()
const uploadModuleModalRef = ref()

const confirmMessage = ref('')
let deleteTargetType: 'release' | 'module' | null = null
let deleteTargetId = ''

// Metrics
const latestRelease = computed(() => releases.value[0] || null)
const latestReleaseVersion = computed(() => latestRelease.value?.version || '')
const latestReleaseDate = computed(() => latestRelease.value ? formatDate(latestRelease.value.created_at) : '')
const totalDownloads = computed(() => releases.value.reduce((sum, r) => sum + (r.download_count || 0), 0))

// Release upload state
const releaseFile = ref<File | null>(null)
const uploadingRelease = ref(false)
const releaseUploadProgress = ref(0)
const releaseForm = ref({
  version: '',
  release_notes: '',
  is_mandatory: false,
  min_supported_version: ''
})

// Module upload state
const moduleFile = ref<File | null>(null)
const uploadingModule = ref(false)
const moduleUploadProgress = ref(0)
const moduleForm = ref({
  name: '',
  version: '',
  target_path: 'modules',
  is_required: true
})

// Simulator state
const simulationVersion = ref('1.0.0')
const simulating = ref(false)
const simulationResult = ref<CheckUpdateSimulation | null>(null)

function formatBytes(bytes: number): string {
  if (!bytes || bytes <= 0) return '0 B'
  const k = 1024
  const sizes = ['B', 'KB', 'MB', 'GB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i]
}

function formatDate(dateStr?: string): string {
  if (!dateStr) return '-'
  const d = new Date(dateStr)
  return d.toLocaleDateString('en-US', { month: 'short', day: 'numeric', year: 'numeric' })
}

async function fetchReleases() {
  loading.value = true
  try {
    const res = await ClientService.findAllReleases()
    if (res.success && Array.isArray(res.data)) {
      releases.value = res.data
    }
  } catch (err: any) {
    notificationRef.value?.show(err.message || 'Failed to fetch releases', 'error')
  } finally {
    loading.value = false
  }
}

async function fetchModules() {
  loading.value = true
  try {
    const res = await ClientService.findAllModules()
    if (res.success && Array.isArray(res.data)) {
      modules.value = res.data
    }
  } catch (err: any) {
    notificationRef.value?.show(err.message || 'Failed to fetch modules', 'error')
  } finally {
    loading.value = false
  }
}

function refreshCurrent() {
  if (activeTab.value === 'releases') fetchReleases()
  else if (activeTab.value === 'modules') fetchModules()
}

// Releases actions
function openUploadReleaseModal() {
  releaseForm.value = {
    version: '',
    release_notes: '',
    is_mandatory: false,
    min_supported_version: ''
  }
  releaseFile.value = null
  releaseUploadProgress.value = 0
  uploadReleaseModalRef.value?.showModal()
}

function closeUploadReleaseModal() {
  uploadReleaseModalRef.value?.close()
}

function handleReleaseFileSelect(e: Event) {
  const target = e.target as HTMLInputElement
  if (target.files && target.files[0]) {
    releaseFile.value = target.files[0]
    // Auto-detect version from filename like Astra-1.0.1-portable.exe
    const match = releaseFile.value.name.match(/(\\d+\\.\\d+\\.\\d+)/)
    if (match && !releaseForm.value.version) {
      releaseForm.value.version = match[1]
    }
  }
}

async function submitReleaseUpload() {
  if (!releaseFile.value) return
  uploadingRelease.value = true
  releaseUploadProgress.value = 0

  const formData = new FormData()
  formData.append('file', releaseFile.value)
  formData.append('version', releaseForm.value.version)
  formData.append('release_notes', releaseForm.value.release_notes)
  formData.append('is_mandatory', releaseForm.value.is_mandatory ? 'true' : 'false')
  if (releaseForm.value.min_supported_version) {
    formData.append('min_supported_version', releaseForm.value.min_supported_version)
  }

  try {
    const res = await ClientService.uploadRelease(formData, (percent) => {
      releaseUploadProgress.value = percent
    })
    if (res.success) {
      notificationRef.value?.show('Client release uploaded successfully!', 'success')
      closeUploadReleaseModal()
      fetchReleases()
    } else {
      notificationRef.value?.show(res.message || 'Failed to upload release', 'error')
    }
  } catch (err: any) {
    notificationRef.value?.show(err.message || 'Upload error', 'error')
  } finally {
    uploadingRelease.value = false
  }
}

async function toggleMandatory(rel: ClientRelease) {
  const newVal = !rel.is_mandatory
  try {
    const res = await ClientService.updateRelease(rel.id, { is_mandatory: newVal })
    if (res.success) {
      rel.is_mandatory = newVal
      notificationRef.value?.show(\`Updated release v\${rel.version} mandatory status to \${newVal}\`, 'success')
    } else {
      notificationRef.value?.show('Failed to toggle status', 'error')
    }
  } catch (err: any) {
    notificationRef.value?.show(err.message || 'Error updating status', 'error')
  }
}

function confirmDeleteRelease(rel: ClientRelease) {
  deleteTargetType = 'release'
  deleteTargetId = rel.id
  confirmMessage.value = \`Are you sure you want to delete release v\${rel.version} (\${rel.file_name})? This file will be removed permanently from the server.\`
  confirmDialogRef.value?.open()
}

// Module actions
function openUploadModuleModal() {
  moduleForm.value = {
    name: '',
    version: '',
    target_path: 'modules',
    is_required: true
  }
  moduleFile.value = null
  moduleUploadProgress.value = 0
  uploadModuleModalRef.value?.showModal()
}

function closeUploadModuleModal() {
  uploadModuleModalRef.value?.close()
}

function handleModuleFileSelect(e: Event) {
  const target = e.target as HTMLInputElement
  if (target.files && target.files[0]) {
    moduleFile.value = target.files[0]
    const baseName = moduleFile.value.name.replace(/\\.[^/.]+$/, '')
    if (!moduleForm.value.name) moduleForm.value.name = baseName
  }
}

async function submitModuleUpload() {
  if (!moduleFile.value) return
  uploadingModule.value = true
  moduleUploadProgress.value = 0

  const formData = new FormData()
  formData.append('file', moduleFile.value)
  formData.append('name', moduleForm.value.name)
  formData.append('version', moduleForm.value.version)
  formData.append('target_path', moduleForm.value.target_path)
  formData.append('is_required', moduleForm.value.is_required ? 'true' : 'false')

  try {
    const res = await ClientService.uploadModule(formData, (percent) => {
      moduleUploadProgress.value = percent
    })
    if (res.success) {
      notificationRef.value?.show('Dynamic module deployed successfully!', 'success')
      closeUploadModuleModal()
      fetchModules()
    } else {
      notificationRef.value?.show(res.message || 'Failed to deploy module', 'error')
    }
  } catch (err: any) {
    notificationRef.value?.show(err.message || 'Deployment error', 'error')
  } finally {
    uploadingModule.value = false
  }
}

function confirmDeleteModule(mod: ClientModule) {
  deleteTargetType = 'module'
  deleteTargetId = mod.id
  confirmMessage.value = \`Are you sure you want to delete module "\${mod.name}" v\${mod.version}? Clients will no longer receive this dynamic module.\`
  confirmDialogRef.value?.open()
}

async function executeDelete() {
  if (!deleteTargetId || !deleteTargetType) return
  try {
    if (deleteTargetType === 'release') {
      const res = await ClientService.removeRelease(deleteTargetId)
      if (res.success) {
        notificationRef.value?.show('Client release deleted successfully', 'success')
        fetchReleases()
      } else {
        notificationRef.value?.show(res.message || 'Failed to delete release', 'error')
      }
    } else if (deleteTargetType === 'module') {
      const res = await ClientService.removeModule(deleteTargetId)
      if (res.success) {
        notificationRef.value?.show('Module deleted successfully', 'success')
        fetchModules()
      } else {
        notificationRef.value?.show(res.message || 'Failed to delete module', 'error')
      }
    }
  } catch (err: any) {
    notificationRef.value?.show(err.message || 'Delete error', 'error')
  } finally {
    deleteTargetType = null
    deleteTargetId = ''
  }
}

// Simulator
async function runSimulation() {
  if (!simulationVersion.value) return
  simulating.value = true
  try {
    const res = await ClientService.simulateCheckUpdate(simulationVersion.value)
    if (res.success && res.data) {
      simulationResult.value = res.data
    } else {
      notificationRef.value?.show(res.message || 'Simulation returned no data', 'warning')
    }
  } catch (err: any) {
    notificationRef.value?.show(err.message || 'Simulation failed', 'error')
  } finally {
    simulating.value = false
  }
}

// Helpers
function copyText(text: string) {
  navigator.clipboard.writeText(text)
  notificationRef.value?.show('Copied to clipboard!', 'info')
}

function copyDirectDownload(id: string) {
  const url = \`\${window.location.origin}/client/download/release/\${id}\`
  copyText(url)
}

onMounted(() => {
  fetchReleases()
  fetchModules()
})
</script>
`;
fs.writeFileSync(path.join(clientModDir, 'ClientManager.vue'), vueContent, 'utf8');
console.log('Created ClientManager.vue');

// 4. Update router/index.ts
const routerPath = path.join(ADMIN_ROOT, 'src', 'router', 'index.ts');
let routerContent = fs.readFileSync(routerPath, 'utf8');

if (!routerContent.includes('ClientManager')) {
  // Add import
  routerContent = routerContent.replace(
    "import UserMap from '@/modules/map/UserMap.vue'",
    "import UserMap from '@/modules/map/UserMap.vue'\nimport ClientManager from '@/modules/client/ClientManager.vue'"
  );

  // Add route before 404
  const routeDef = `  { 
    path: '/client-management', 
    name: 'ClientManagement',
    meta: { requiresAuth: true },
    component: ClientManager 
  },
`;
  routerContent = routerContent.replace(
    '  {\n    path: "/:pathMatch(.*)*",',
    routeDef + '  {\n    path: "/:pathMatch(.*)*",'
  );

  fs.writeFileSync(routerPath, routerContent, 'utf8');
  console.log('Updated router/index.ts');
} else {
  console.log('router/index.ts already contains ClientManager');
}

// 5. Update stores/menu.ts
const menuPath = path.join(ADMIN_ROOT, 'src', 'stores', 'menu.ts');
let menuContent = fs.readFileSync(menuPath, 'utf8');

if (!menuContent.includes('/client-management')) {
  const menuItem = `            {
                label: 'Client Management',
                icon: 'ph ph-app-window',
                route: '/client-management'
            },
`;
  menuContent = menuContent.replace(
    "            {\n                label: 'Binary Software',",
    menuItem + "            {\n                label: 'Binary Software',"
  );
  fs.writeFileSync(menuPath, menuContent, 'utf8');
  console.log('Updated stores/menu.ts');
} else {
  console.log('stores/menu.ts already contains /client-management');
}

console.log('Stage 2 files created and configured successfully!');
