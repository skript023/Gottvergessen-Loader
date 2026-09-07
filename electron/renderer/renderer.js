// State Management
let binaries = [];
let selectedBinaryIndex = -1;
let processes = [];
let selectedProcess = null;
let currentTab = "dashboard";

// DOM Elements
const loginView = document.querySelector("#login-view");
const appView = document.querySelector("#app-view");
const loginForm = document.querySelector("#login-form");
const usernameInput = document.querySelector("#username");
const passwordInput = document.querySelector("#password");
const rememberInput = document.querySelector("#remember-me");
const loginButton = document.querySelector("#login");
const loginMessage = document.querySelector("#login-message");

// Navigation
const navItems = document.querySelectorAll(".nav-item");
const tabPanes = document.querySelectorAll(".tab-pane");
const breadcrumbCurrent = document.querySelector("#breadcrumb-current");

// Logout buttons
const sidebarLogoutBtn = document.querySelector("#sidebar-logout-btn");
const headerLogoutBtn = document.querySelector("#header-logout-btn");

// Profile elements
const profileAvatar = document.querySelector("#profile-avatar");
const profileName = document.querySelector("#profile-name");
const profileRole = document.querySelector("#profile-role");
const profileUsername = document.querySelector("#profile-username");
const profileExpiry = document.querySelector("#profile-expiry");

// Metrics
const binaryCount = document.querySelector("#binary-count");
const sidebarBinaryCount = document.querySelector("#sidebar-binary-count");
const processCount = document.querySelector("#process-count");
const targetState = document.querySelector("#target-state");
const targetStateSub = document.querySelector("#target-state-sub");

// Hero Banner
const heroTitle = document.querySelector("#hero-title");
const heroDesc = document.querySelector("#hero-desc");
const heroArch = document.querySelector("#hero-arch");
const heroVersion = document.querySelector("#hero-version");
const heroProtection = document.querySelector("#hero-protection");
const heroQuickInject = document.querySelector("#hero-quick-inject");

// Binary Lists
const binaryCardsList = document.querySelector("#binary-cards-list");
const catalogGrid = document.querySelector("#catalog-grid");
const binarySelect = document.querySelector("#binary");
const refreshBinariesBtn = document.querySelector("#refresh-binaries-btn");
const refreshCatalogBtn = document.querySelector("#refresh-catalog-btn");

// Process Manager
const processBody = document.querySelector("#processes");
const filterInput = document.querySelector("#filter");
const refreshButton = document.querySelector("#refresh");
const quickJumpProcesses = document.querySelector("#quick-jump-processes");

// Injection Engine & Settings
const selectionPill = document.querySelector("#selection");
const modeSelect = document.querySelector("#mode");
const injectButton = document.querySelector("#inject");
const btnManualTarget = document.querySelector("#btn-manual-target");
const manualTargetBox = document.querySelector("#manual-target-box");
const manualTargetInput = document.querySelector("#manual-target-input");
const btnSaveManualTarget = document.querySelector("#btn-save-manual-target");
const btnCancelManualTarget = document.querySelector("#btn-cancel-manual-target");
const cloudSyncStatus = document.querySelector("#cloud-sync-status");
const cloudSyncText = document.querySelector("#cloud-sync-text");
const progressCard = document.querySelector("#operation-progress");
const progressStage = document.querySelector("#progress-stage");
const progressValue = document.querySelector("#progress-value");
const progressFill = document.querySelector("#progress-fill");
const message = document.querySelector("#message");

// Diagnostics Console
const consoleLogs = document.querySelector("#console-logs");
const clearLogsBtn = document.querySelector("#clear-logs-btn");

// Logging Utility
function addLog(msg, type = "info") {
  if (!consoleLogs) return;
  const now = new Date();
  const timeStr = `[${String(now.getHours()).padStart(2, "0")}:${String(now.getMinutes()).padStart(2, "0")}:${String(now.getSeconds()).padStart(2, "0")}]`;
  const line = document.createElement("div");
  line.className = `log-line ${type}`;
  line.innerHTML = `<span class="log-time">${timeStr}</span> ${escapeHtml(msg)}`;
  consoleLogs.appendChild(line);
  consoleLogs.scrollTop = consoleLogs.scrollHeight;
}

function escapeHtml(str) {
  return String(str)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;");
}

// Navigation Tab Switching
function switchTab(tabName) {
  currentTab = tabName;
  navItems.forEach((btn) => {
    btn.classList.toggle("active", btn.dataset.tab === tabName);
  });
  tabPanes.forEach((pane) => {
    pane.classList.toggle("active", pane.id === `tab-${tabName}`);
  });

  const titles = {
    dashboard: "Dashboard Overview",
    binaries: "Binaries Catalog",
    processes: "Process Manager",
    diagnostics: "Live Diagnostics"
  };
  if (breadcrumbCurrent) {
    breadcrumbCurrent.textContent = titles[tabName] || "Dashboard";
  }
}

navItems.forEach((btn) => {
  btn.addEventListener("click", () => {
    if (btn.dataset.tab) switchTab(btn.dataset.tab);
  });
});

if (quickJumpProcesses) {
  quickJumpProcesses.addEventListener("click", () => switchTab("processes"));
}

if (clearLogsBtn) {
  clearLogsBtn.addEventListener("click", () => {
    consoleLogs.replaceChildren();
    addLog("Console cleared.", "info");
  });
}

// Helpers for Injection Mode and Cloud Sync
function getModeName(mode) {
  switch (Number(mode)) {
    case 0: return "CreateRemoteThread";
    case 1: return "Thread Hijack";
    case 2: return "Manual Map (Kernel/Stealth)";
    case 3: return "Reflective DLL";
    default: return "Manual Map";
  }
}

function getShortModeName(mode) {
  switch (Number(mode)) {
    case 0: return "Remote Thread";
    case 1: return "Thread Hijack";
    case 2: return "Manual Map";
    case 3: return "Reflective";
    default: return "Manual Map";
  }
}

function updateSyncStatus(status, text) {
  if (!cloudSyncStatus || !cloudSyncText) return;
  cloudSyncStatus.classList.remove("saving", "error");
  if (status === "saving") cloudSyncStatus.classList.add("saving");
  else if (status === "error") cloudSyncStatus.classList.add("error");
  cloudSyncText.textContent = text;
}

let persistTimeout = null;
async function persistBinarySettings(binary) {
  if (!binary || !binary.id) return;

  updateSyncStatus("saving", "Syncing to Ellohim Cloud...");

  // Also save to localStorage fallback
  try {
    const localStore = JSON.parse(localStorage.getItem("ellohim_binary_settings") || "{}");
    localStore[binary.id] = {
      target_process: binary.target_process || "",
      injection_mode: Number(binary.injection_mode) || 0
    };
    localStorage.setItem("ellohim_binary_settings", JSON.stringify(localStore));
  } catch (_) {}

  clearTimeout(persistTimeout);
  persistTimeout = setTimeout(async () => {
    try {
      if (window.loader && typeof window.loader.saveBinarySettings === "function") {
        await window.loader.saveBinarySettings(
          binary.id,
          binary.target_process || "",
          Number(binary.injection_mode) || 0
        );
        updateSyncStatus("synced", `Synced: ${binary.target_process || "No target"} • ${getShortModeName(binary.injection_mode)}`);
        addLog(`Cloud saved [${binary.name || "Binary"}]: target=${binary.target_process || "None"}, mode=${getShortModeName(binary.injection_mode)}`, "success");
        updateActiveBinaryCardBadges();
      }
    } catch (err) {
      updateSyncStatus("error", "Local saved (Cloud error)");
      addLog(`Failed to sync binary settings to cloud: ${err.message}`, "warn");
    }
  }, 400);
}

function updateActiveBinaryCardBadges() {
  if (selectedBinaryIndex < 0 || !binaries[selectedBinaryIndex]) return;
  const current = binaries[selectedBinaryIndex];
  const cards = document.querySelectorAll(".binary-card-item");
  if (cards[selectedBinaryIndex]) {
    const procBadge = cards[selectedBinaryIndex].querySelector(".badge-setting-proc");
    const modeBadge = cards[selectedBinaryIndex].querySelector(".badge-setting-mode");
    if (procBadge) procBadge.textContent = `🎯 ${current.target_process || "No Target"}`;
    if (modeBadge) modeBadge.textContent = `⚡ ${getShortModeName(current.injection_mode ?? 2)}`;
  }
  const catCards = document.querySelectorAll(".catalog-card");
  if (catCards[selectedBinaryIndex]) {
    const details = catCards[selectedBinaryIndex].querySelectorAll(".catalog-detail strong");
    if (details.length >= 4) {
      details[2].textContent = current.target_process || "Not set";
      details[3].textContent = getShortModeName(current.injection_mode ?? 2);
    }
  }
}

// Ready State Check
function updateReadyState() {
  const isRunning = Boolean(selectedProcess && selectedProcess.pid > 0);
  const isReady = isRunning && selectedBinaryIndex >= 0;
  injectButton.disabled = !isReady;
  heroQuickInject.disabled = !isReady;

  if (selectedProcess) {
    selectionPill.classList.add("has-target");
    if (selectedProcess.pid > 0) {
      selectionPill.classList.remove("waiting-process");
      selectionPill.innerHTML = `
        <span class="target-icon">🎯</span>
        <span class="target-text"><strong>${escapeHtml(selectedProcess.name)}</strong> (PID: ${selectedProcess.pid}) [${selectedProcess.arch}]</span>
      `;
      targetState.textContent = selectedProcess.name;
      targetStateSub.textContent = `PID ${selectedProcess.pid} • ${selectedProcess.arch}`;
    } else {
      selectionPill.classList.add("waiting-process");
      selectionPill.innerHTML = `
        <span class="target-icon">⏳</span>
        <span class="target-text"><strong>${escapeHtml(selectedProcess.name)}</strong> (Waiting for game to launch...)</span>
      `;
      targetState.textContent = selectedProcess.name;
      targetStateSub.textContent = "Executable configured • Offline";
    }
  } else {
    selectionPill.classList.remove("has-target", "waiting-process");
    selectionPill.innerHTML = `
      <span class="target-icon">🎯</span>
      <span class="target-text">No target process selected</span>
    `;
    targetState.textContent = "None";
    targetStateSub.textContent = "Select from process table or set manual";
  }
}

// Select Binary Payload
function selectBinary(index) {
  if (index < 0 || index >= binaries.length) return;
  selectedBinaryIndex = index;
  const binary = binaries[index];

  // Restore fallback from local store if BE didn't have it
  try {
    const localStore = JSON.parse(localStorage.getItem("ellohim_binary_settings") || "{}");
    if (localStore[binary.id]) {
      if (!binary.target_process && localStore[binary.id].target_process) {
        binary.target_process = localStore[binary.id].target_process;
      }
      if (binary.injection_mode === undefined && localStore[binary.id].injection_mode !== undefined) {
        binary.injection_mode = localStore[binary.id].injection_mode;
      }
    }
  } catch (_) {}

  // Update fallback select
  binarySelect.value = String(index);

  // Restore injection mode
  const modeVal = binary.injection_mode !== undefined && binary.injection_mode !== null ? String(binary.injection_mode) : "2";
  modeSelect.value = modeVal;

  // Restore target process
  if (binary.target_process && binary.target_process.trim()) {
    const procName = binary.target_process.trim();
    const match = processes.find((p) => p.name.toLowerCase() === procName.toLowerCase());
    if (match) {
      selectedProcess = match;
      addLog(`Auto-hooked target process ${match.name} (PID: ${match.pid}) for ${binary.name || "binary"}`, "info");
    } else {
      selectedProcess = { name: procName, pid: 0, arch: "Configured" };
      addLog(`Target configured: ${procName} (Waiting for process to launch...)`, "warn");
    }
  } else {
    selectedProcess = null;
  }

  // Update Hero Banner
  const title = binary.name || binary.game || `Binary #${index + 1}`;
  const version = binary.version || "1.0.0";
  const arch = binary.arch || "x64 Native";

  heroTitle.textContent = title;
  heroDesc.textContent = binary.description || `Authorized binary payload for ${title}. Ready to inject.`;
  heroVersion.textContent = `v${version}`;
  heroArch.textContent = arch;

  // Update sync status text
  updateSyncStatus("synced", `Active: ${binary.target_process || "No target set"} • ${getShortModeName(binary.injection_mode ?? 2)}`);

  // Highlight card items in dashboard
  document.querySelectorAll(".binary-card-item").forEach((card, idx) => {
    card.classList.toggle("selected", idx === index);
  });

  // Highlight card items in catalog
  document.querySelectorAll(".catalog-card").forEach((card, idx) => {
    card.classList.toggle("selected", idx === index);
  });

  addLog(`Selected binary: ${title} (v${version})`, "info");
  updateReadyState();
  renderProcesses();
}

// Render Binaries
function renderBinaries(items = []) {
  binaries = items;
  binaryCount.textContent = String(items.length);
  sidebarBinaryCount.textContent = String(items.length);

  // Fallback select
  binarySelect.replaceChildren();
  binaryCardsList.replaceChildren();
  catalogGrid.replaceChildren();

  if (items.length === 0) {
    binaryCardsList.innerHTML = `<div class="empty-state">No binaries assigned to your account.</div>`;
    catalogGrid.innerHTML = `<div class="empty-state">No binaries available.</div>`;
    selectedBinaryIndex = -1;
    updateReadyState();
    return;
  }

  items.forEach((binary, index) => {
    const title = binary.name || binary.game || `Binary #${index + 1}`;
    const filename = binary.file_name || binary.filename || "payload.dll";
    const version = binary.version || "1.0.0";
    const arch = binary.arch || "x64";
    const targetProc = binary.target_process || "";
    const modeName = getShortModeName(binary.injection_mode ?? 2);

    // 1. Fallback select option
    const opt = document.createElement("option");
    opt.value = String(index);
    opt.textContent = `${title} (v${version})`;
    binarySelect.appendChild(opt);

    // 2. Dashboard Quick Card
    const card = document.createElement("div");
    card.className = `binary-card-item ${index === selectedBinaryIndex ? "selected" : ""}`;
    card.innerHTML = `
      <div class="binary-item-left">
        <div class="binary-item-icon">⚡</div>
        <div class="binary-item-info">
          <strong>${escapeHtml(title)}</strong>
          <div class="binary-item-meta">
            <span>${escapeHtml(filename)}</span>
            <span>•</span>
            <span class="binary-tag">SAFE</span>
            <span>•</span>
            <span>v${escapeHtml(version)}</span>
          </div>
          <div class="binary-settings-summary">
            <span class="badge-setting-proc" title="Saved Target Executable">🎯 ${escapeHtml(targetProc || "No Target")}</span>
            <span class="badge-setting-mode" title="Saved Injection Mode">⚡ ${escapeHtml(modeName)}</span>
          </div>
        </div>
      </div>
      <button class="binary-item-action">${index === selectedBinaryIndex ? "Active" : "Select"}</button>
    `;
    card.addEventListener("click", () => selectBinary(index));
    binaryCardsList.appendChild(card);

    // 3. Full Catalog Grid Card
    const catCard = document.createElement("div");
    catCard.className = `catalog-card ${index === selectedBinaryIndex ? "selected" : ""}`;
    catCard.innerHTML = `
      <div class="catalog-card-header">
        <span class="catalog-game-tag">${escapeHtml(arch.toUpperCase())}</span>
        <span class="catalog-status-badge">
          <span class="status-indicator-dot"></span> UNDETECTED
        </span>
      </div>
      <h3 class="catalog-title">${escapeHtml(title)}</h3>
      <div class="catalog-details">
        <div class="catalog-detail">
          <span>PAYLOAD</span>
          <strong>${escapeHtml(filename)}</strong>
        </div>
        <div class="catalog-detail">
          <span>VERSION</span>
          <strong>${escapeHtml(version)}</strong>
        </div>
        <div class="catalog-detail">
          <span>SAVED TARGET</span>
          <strong style="color: #7dd3fc;">${escapeHtml(targetProc || "Not set")}</strong>
        </div>
        <div class="catalog-detail">
          <span>METHOD</span>
          <strong style="color: #c084fc;">${escapeHtml(modeName)}</strong>
        </div>
      </div>
      <button class="btn-select-binary">${index === selectedBinaryIndex ? "✓ Selected" : "Select & Configure"}</button>
    `;
    catCard.querySelector(".btn-select-binary").addEventListener("click", () => {
      selectBinary(index);
      switchTab("dashboard");
    });
    catalogGrid.appendChild(catCard);
  });

  // Default select first item
  if (selectedBinaryIndex < 0 || selectedBinaryIndex >= items.length) {
    selectBinary(0);
  } else {
    selectBinary(selectedBinaryIndex);
  }
}

// Render Profile
function renderProfile(profile = {}) {
  const first = typeof profile.firstname === "string" ? profile.firstname : "";
  const last = typeof profile.lastname === "string" ? profile.lastname : "";
  const username = profile.username || "user";
  const fullname = [first, last].filter(Boolean).join(" ") || profile.fullname || username;
  const role = profile.role || "VIP CLIENT";
  const expiry = profile.expired_date || profile.expiry_date || "Lifetime Active";

  profileName.textContent = fullname;
  profileUsername.textContent = `@${username}`;
  profileRole.textContent = role;
  profileExpiry.textContent = expiry;
  profileAvatar.textContent = fullname.trim().charAt(0).toUpperCase() || "U";
}

// Process Selection
function selectProcess(proc, autoSave = true) {
  selectedProcess = proc;
  addLog(`Target locked: ${proc.name} (PID: ${proc.pid}, ${proc.arch})`, "info");
  renderProcesses();
  updateReadyState();

  if (autoSave && selectedBinaryIndex >= 0 && binaries[selectedBinaryIndex]) {
    const current = binaries[selectedBinaryIndex];
    if (current.target_process !== proc.name) {
      current.target_process = proc.name;
      persistBinarySettings(current);
    }
  }
}

// Render Processes Table
function renderProcesses() {
  const query = filterInput.value.trim().toLowerCase();
  processBody.replaceChildren();

  const visible = processes.filter(
    (item) => item.name.toLowerCase().includes(query) || String(item.pid).includes(query)
  );

  for (const proc of visible) {
    const row = document.createElement("tr");
    const isSelected = selectedProcess?.pid === proc.pid;
    if (isSelected) row.classList.add("selected");

    row.innerHTML = `
      <td><strong>${proc.pid}</strong></td>
      <td>${escapeHtml(proc.name)}</td>
      <td><span class="tag-arch">${escapeHtml(proc.arch || "x64")}</span></td>
      <td>
        <span class="${proc.accessible ? "tag-access-ready" : "tag-access-denied"}">
          ${proc.accessible ? "● Ready" : "✕ Denied"}
        </span>
      </td>
      <td style="text-align: right;">
        ${proc.accessible ? `<button class="btn-target-row">${isSelected ? "Targeted" : "Target"}</button>` : '<span style="color: var(--text-dim);">N/A</span>'}
      </td>
    `;

    if (proc.accessible) {
      row.addEventListener("click", () => selectProcess(proc));
    }
    processBody.appendChild(row);
  }
}

// Refresh Process List
async function refreshProcesses() {
  refreshButton.disabled = true;
  addLog("Scanning running processes...", "info");
  try {
    processes = await window.loader.listProcesses();
    processCount.textContent = String(processes.length);

    // Auto-hook check for active binary
    if (selectedBinaryIndex >= 0 && binaries[selectedBinaryIndex]) {
      const active = binaries[selectedBinaryIndex];
      if (active.target_process && active.target_process.trim()) {
        const match = processes.find(
          (p) => p.name.toLowerCase() === active.target_process.trim().toLowerCase()
        );
        if (match) {
          if (!selectedProcess || selectedProcess.pid !== match.pid) {
            selectProcess(match, false);
            addLog(`Auto-hooked running target: ${match.name} (PID: ${match.pid})`, "success");
          }
        }
      }
    }

    renderProcesses();
    updateReadyState();
    addLog(`Process scan complete. ${processes.length} processes detected.`, "success");
  } catch (error) {
    addLog(`Process enumeration error: ${error.message}`, "error");
  } finally {
    refreshButton.disabled = false;
  }
}

refreshButton.addEventListener("click", refreshProcesses);
filterInput.addEventListener("input", renderProcesses);

// Refresh Catalog
async function refreshBinariesCatalog() {
  try {
    addLog("Syncing binary catalog from server...", "info");
    const items = await window.loader.refreshBinaries();
    renderBinaries(items);
    addLog(`Catalog synced. ${items.length} binaries available.`, "success");
  } catch (error) {
    addLog(`Failed to refresh catalog: ${error.message}`, "error");
  }
}

if (refreshBinariesBtn) refreshBinariesBtn.addEventListener("click", refreshBinariesCatalog);
if (refreshCatalogBtn) refreshCatalogBtn.addEventListener("click", refreshBinariesCatalog);

// Submit Login
async function submitLogin() {
  const username = usernameInput.value.trim();
  const password = passwordInput.value;
  if (!username || !password) {
    loginMessage.className = "form-message error";
    loginMessage.textContent = "Username and password are required.";
    return;
  }

  loginButton.disabled = true;
  loginButton.innerHTML = `<span>Signing In...</span>`;
  loginMessage.className = "form-message pending-text";
  loginMessage.textContent = "Authenticating with security server...";

  try {
    const session = await window.loader.login({
      username,
      password,
      rememberMe: rememberInput.checked
    });
    passwordInput.value = "";
    loginMessage.textContent = "";

    renderProfile(session.profile);
    renderBinaries(session.binaries);

    loginView.classList.add("hidden");
    appView.classList.remove("hidden");

    addLog(`Authenticated as ${username}. Welcome back!`, "success");
    await refreshProcesses();
  } catch (error) {
    loginMessage.className = "form-message error";
    loginMessage.textContent = error.message;
    passwordInput.select();
  } finally {
    loginButton.disabled = false;
    loginButton.innerHTML = `
      <span>Sign In to Dashboard</span>
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
    `;
  }
}

loginForm.addEventListener("submit", (e) => {
  e.preventDefault();
  submitLogin();
});

// Logout Feature
async function handleLogout() {
  const confirmed = confirm("Are you sure you want to sign out?");
  if (!confirmed) return;

  sidebarLogoutBtn.disabled = true;
  headerLogoutBtn.disabled = true;
  addLog("Signing out and clearing session...", "warn");

  try {
    if (window.loader.logout) {
      await window.loader.logout();
    }
  } catch (error) {
    console.error("Logout error:", error);
  } finally {
    // Reset state
    binaries = [];
    selectedBinaryIndex = -1;
    processes = [];
    selectedProcess = null;

    passwordInput.value = "";
    loginMessage.className = "form-message success";
    loginMessage.textContent = "Logged out successfully.";

    // Switch view
    appView.classList.add("hidden");
    loginView.classList.remove("hidden");

    sidebarLogoutBtn.disabled = false;
    headerLogoutBtn.disabled = false;
    usernameInput.focus();
  }
}

sidebarLogoutBtn.addEventListener("click", handleLogout);
headerLogoutBtn.addEventListener("click", handleLogout);

// Restore Session on App Launch
async function restoreSession() {
  try {
    addLog("Checking for active device session...", "info");
    const session = await window.loader.restoreSession();
    if (!session) {
      addLog("No active session found. Sign in required.", "info");
      return;
    }
    renderProfile(session.profile);
    renderBinaries(session.binaries);

    loginView.classList.add("hidden");
    appView.classList.remove("hidden");

    addLog("Active device session restored successfully.", "success");
    await refreshProcesses();
  } catch (error) {
    addLog(`Session restore notice: ${error.message}`, "warn");
  }
}

restoreSession();

// Progress HUD
function renderProgress(status) {
  const value = Math.max(0, Math.min(100, Number(status.progress) || 0));
  progressStage.textContent = status.stage || "Executing operation...";
  progressValue.textContent = `${value}%`;
  progressFill.style.width = `${value}%`;
}

// Injection Routine
async function executeInjection() {
  if (!selectedProcess || selectedBinaryIndex < 0) return;

  injectButton.disabled = true;
  heroQuickInject.disabled = true;
  progressCard.classList.remove("hidden");
  renderProgress({ progress: 5, stage: "Connecting to secure payload vault..." });
  message.textContent = "Injection operation in progress...";
  addLog(
    `Initiating injection: Mode ${modeSelect.value} -> ${selectedProcess.name} (PID: ${selectedProcess.pid})`,
    "info"
  );

  const progressTimer = setInterval(async () => {
    try {
      const status = await window.loader.operationStatus();
      renderProgress(status);
    } catch (_) {}
  }, 120);

  try {
    const success = await window.loader.inject({
      pid: selectedProcess.pid,
      processName: selectedProcess.name,
      binaryIndex: selectedBinaryIndex,
      mode: Number(modeSelect.value)
    });

    renderProgress({ progress: success ? 100 : 0, stage: success ? "Injection Complete" : "Failed" });
    if (success) {
      message.style.color = "#34d399";
      message.textContent = `Payload injected into ${selectedProcess.name} successfully!`;
      addLog(`Injection succeeded into ${selectedProcess.name} [PID: ${selectedProcess.pid}]`, "success");
    } else {
      message.style.color = "#f87171";
      message.textContent = "Injection failed. Check the native log.";
      addLog("Native engine reported injection failure.", "error");
    }
  } catch (error) {
    renderProgress({ progress: 0, stage: "Error" });
    message.style.color = "#f87171";
    message.textContent = `Error: ${error.message}`;
    addLog(`Injection error: ${error.message}`, "error");
  } finally {
    clearInterval(progressTimer);
    updateReadyState();
  }
}

injectButton.addEventListener("click", executeInjection);
heroQuickInject.addEventListener("click", executeInjection);

// Mode Selection Change Listener
modeSelect.addEventListener("change", () => {
  if (selectedBinaryIndex >= 0 && binaries[selectedBinaryIndex]) {
    const current = binaries[selectedBinaryIndex];
    current.injection_mode = parseInt(modeSelect.value, 10);
    persistBinarySettings(current);
  }
});

// Manual Target Executable Controls
if (btnManualTarget) {
  btnManualTarget.addEventListener("click", () => {
    manualTargetBox.classList.toggle("hidden");
    if (!manualTargetBox.classList.contains("hidden")) {
      const active = selectedBinaryIndex >= 0 ? binaries[selectedBinaryIndex] : null;
      manualTargetInput.value = active?.target_process || selectedProcess?.name || "";
      manualTargetInput.focus();
    }
  });
}

if (btnCancelManualTarget) {
  btnCancelManualTarget.addEventListener("click", () => {
    manualTargetBox.classList.add("hidden");
  });
}

if (btnSaveManualTarget) {
  btnSaveManualTarget.addEventListener("click", () => {
    const val = manualTargetInput.value.trim();
    if (!val) return;
    manualTargetBox.classList.add("hidden");
    if (selectedBinaryIndex >= 0 && binaries[selectedBinaryIndex]) {
      const current = binaries[selectedBinaryIndex];
      current.target_process = val;
      const match = processes.find((p) => p.name.toLowerCase() === val.toLowerCase());
      if (match) {
        selectProcess(match, false);
      } else {
        selectedProcess = { name: val, pid: 0, arch: "Configured" };
        updateReadyState();
      }
      persistBinarySettings(current);
    }
  });
}

if (manualTargetInput) {
  manualTargetInput.addEventListener("keydown", (e) => {
    if (e.key === "Enter") {
      btnSaveManualTarget?.click();
    } else if (e.key === "Escape") {
      btnCancelManualTarget?.click();
    }
  });
}
