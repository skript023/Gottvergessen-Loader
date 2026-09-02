const processBody = document.querySelector("#processes");
const filterInput = document.querySelector("#filter");
const refreshButton = document.querySelector("#refresh");
const injectButton = document.querySelector("#inject");
const modeSelect = document.querySelector("#mode");
const binarySelect = document.querySelector("#binary");
const usernameInput = document.querySelector("#username");
const passwordInput = document.querySelector("#password");
const rememberInput = document.querySelector("#remember-me");
const loginButton = document.querySelector("#login");
const loginView = document.querySelector("#login-view");
const appView = document.querySelector("#app-view");
const loginMessage = document.querySelector("#login-message");
const nativeStatus = document.querySelector("#native-status");
const selection = document.querySelector("#selection");
const message = document.querySelector("#message");

let processes = [];
let selectedProcess = null;

function updateReadyState() {
  injectButton.disabled = !selectedProcess || binarySelect.value === "";
  selection.textContent = selectedProcess
    ? `${selectedProcess.name} - PID ${selectedProcess.pid} - ${selectedProcess.arch}`
    : "No target selected";
}

function renderProcesses() {
  const query = filterInput.value.trim().toLowerCase();
  processBody.replaceChildren();
  const visible = processes.filter((item) =>
    item.name.toLowerCase().includes(query) || String(item.pid).includes(query));

  for (const process of visible) {
    const row = document.createElement("tr");
    if (!process.accessible) row.classList.add("muted");
    if (selectedProcess?.pid === process.pid) row.classList.add("selected");

    for (const value of [process.pid, process.name, process.arch, process.accessible ? "Ready" : "Denied"]) {
      const cell = document.createElement("td");
      cell.textContent = value;
      row.appendChild(cell);
    }

    if (process.accessible) {
      row.addEventListener("click", () => {
        selectedProcess = process;
        updateReadyState();
        renderProcesses();
      });
    }
    processBody.appendChild(row);
  }
}

async function refreshProcesses() {
  refreshButton.disabled = true;
  message.textContent = "Loading processes...";
  try {
    processes = await window.loader.listProcesses();
    renderProcesses();
    message.textContent = `${processes.length} processes found.`;
  } catch (error) {
    message.textContent = `Failed to load processes: ${error.message}`;
  } finally {
    refreshButton.disabled = false;
  }
}

function renderBinaries(items) {
  binarySelect.replaceChildren();
  for (const [index, binary] of items.entries()) {
    const option = document.createElement("option");
    option.value = String(index);
    option.textContent = `${binary.name || binary.game || "Binary"} - ${binary.version || "unknown version"}`;
    binarySelect.appendChild(option);
  }
  binarySelect.disabled = items.length === 0;
  if (items.length === 0) {
    const option = document.createElement("option");
    option.value = "";
    option.textContent = "No binaries assigned";
    binarySelect.appendChild(option);
  }
  updateReadyState();
}

async function submitLogin() {
  const username = usernameInput.value.trim();
  const password = passwordInput.value;
  if (!username || !password) {
    loginMessage.className = "form-message error";
    loginMessage.textContent = "Username and password are required.";
    return;
  }

  loginButton.disabled = true;
  usernameInput.disabled = true;
  passwordInput.disabled = true;
  loginButton.textContent = "Signing in...";
  loginMessage.className = "form-message pending-text";
  loginMessage.textContent = "Checking your account...";

  try {
    const items = await window.loader.login({ username, password, rememberMe: rememberInput.checked });
    passwordInput.value = "";
    renderBinaries(items);
    nativeStatus.className = "badge";
    nativeStatus.textContent = "Authenticated";
    loginView.classList.add("hidden");
    appView.classList.remove("hidden");
    message.textContent = `Login successful. ${items.length} server binaries loaded.`;
    await refreshProcesses();
  } catch (error) {
    renderBinaries([]);
    loginMessage.className = "form-message error";
    loginMessage.textContent = `Login failed: ${error.message}`;
    passwordInput.select();
  } finally {
    loginButton.disabled = false;
    usernameInput.disabled = false;
    passwordInput.disabled = false;
    loginButton.textContent = "Sign in";
  }
}

refreshButton.addEventListener("click", refreshProcesses);
filterInput.addEventListener("input", renderProcesses);
loginButton.addEventListener("click", submitLogin);
passwordInput.addEventListener("keydown", (event) => {
  if (event.key === "Enter") submitLogin();
});
binarySelect.addEventListener("change", updateReadyState);

async function restoreSession() {
  loginButton.disabled = true;
  loginMessage.className = "form-message pending-text";
  loginMessage.textContent = "Restoring saved session...";
  try {
    const items = await window.loader.restoreSession();
    if (!items) {
      loginMessage.textContent = "";
      return;
    }
    renderBinaries(items);
    nativeStatus.className = "badge";
    nativeStatus.textContent = "Authenticated";
    loginView.classList.add("hidden");
    appView.classList.remove("hidden");
    message.textContent = `Session restored. ${items.length} server binaries loaded.`;
    await refreshProcesses();
  } catch (error) {
    loginMessage.className = "form-message error";
    loginMessage.textContent = `Saved session could not be restored: ${error.message}`;
  } finally {
    loginButton.disabled = false;
  }
}

restoreSession();

injectButton.addEventListener("click", async () => {
  if (!selectedProcess || binarySelect.value === "") return;
  injectButton.disabled = true;
  message.textContent = "Downloading, decrypting, and injecting...";
  try {
    const success = await window.loader.inject({
      pid: selectedProcess.pid,
      processName: selectedProcess.name,
      binaryIndex: Number(binarySelect.value),
      mode: Number(modeSelect.value)
    });
    message.textContent = success ? "Injection completed successfully." : "Injection failed. Check the native log.";
  } catch (error) {
    message.textContent = `Injection error: ${error.message}`;
  } finally {
    updateReadyState();
  }
});
