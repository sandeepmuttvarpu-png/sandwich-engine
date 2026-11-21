/**
 * Sandwich Engine — Web GUI Application Orchestrator
 * Bootstraps controllers, binds UI interactions, and monitors backend connectivity
 */

import { SandwichAPI } from './api.js';
import { ChatController } from './chat.js';
import { DashboardController } from './dashboard.js';
import { SettingsController } from './settings.js';

document.addEventListener('DOMContentLoaded', () => {
  const api = new SandwichAPI();

  // DOM Elements Query
  const elements = {
    // Header & Global
    statusDot: document.getElementById('status-dot'),
    statusText: document.getElementById('status-text'),
    btnToggleSidebar: document.getElementById('btn-toggle-sidebar'),
    btnToggleTelemetry: document.getElementById('btn-toggle-telemetry'),
    btnClearChat: document.getElementById('btn-clear-chat'),
    sidebarPanel: document.getElementById('sidebar-panel'),
    telemetryPanel: document.getElementById('telemetry-panel'),

    // Chat
    messagesContainer: document.getElementById('chat-messages'),
    welcomeScreen: document.getElementById('welcome-screen'),
    textarea: document.getElementById('chat-textarea'),
    sendBtn: document.getElementById('btn-send'),
    stopBtn: document.getElementById('btn-stop'),

    // Telemetry Dashboard
    vramVal: document.getElementById('telem-vram-val'),
    vramPct: document.getElementById('telem-vram-pct'),
    vramBar: document.getElementById('telem-vram-bar'),
    ramVal: document.getElementById('telem-ram-val'),
    ramPct: document.getElementById('telem-ram-pct'),
    ramBar: document.getElementById('telem-ram-bar'),
    decodeVal: document.getElementById('telem-decode-val'),
    prefillVal: document.getElementById('telem-prefill-val'),
    latencyVal: document.getElementById('telem-latency-val'),
    moeHitVal: document.getElementById('telem-moe-hit-val'),
    ctxVal: document.getElementById('telem-ctx-val'),
    ctxBar: document.getElementById('telem-ctx-bar'),

    // Settings
    tabAuto: document.getElementById('tab-mode-auto'),
    tabManual: document.getElementById('tab-mode-manual'),
    autoSettingsGroup: document.getElementById('auto-settings-group'),
    manualSettingsGroup: document.getElementById('manual-settings-group'),
    selectModel: document.getElementById('settings-select-model'),
    inputModel: document.getElementById('settings-input-model'),
    inputCtx: document.getElementById('settings-input-ctx'),
    inputThreads: document.getElementById('settings-input-threads'),
    inputGpuLayers: document.getElementById('settings-input-gpu-layers'),
    inputDrafter: document.getElementById('settings-input-drafter'),
    inputDrafterLayers: document.getElementById('settings-input-drafter-layers'),
    inputCacheSlots: document.getElementById('settings-input-cache-slots'),
    inputWarmRam: document.getElementById('settings-input-warm-ram'),
    selectCtk: document.getElementById('settings-select-ctk'),
    checkIoUring: document.getElementById('settings-check-io-uring'),
    checkMoe2q: document.getElementById('settings-check-moe-2q'),
    checkMoeRot: document.getElementById('settings-check-moe-rot'),
    cliPreviewText: document.getElementById('cli-preview-text'),
    btnCopyCli: document.getElementById('btn-copy-cli')
  };

  // Initialize Controllers
  const chatCtrl = new ChatController(api, elements);
  const dashboardCtrl = new DashboardController(api, elements);
  const settingsCtrl = new SettingsController(api, elements);

  // Cross-wire Chat and Dashboard for live token updates
  chatCtrl.onStreamToken = () => {
    dashboardCtrl.recordStreamToken();
  };
  chatCtrl.onPromptSent = (promptTokens) => {
    dashboardCtrl.metrics.contextUsed += promptTokens;
    dashboardCtrl.render();
  };
  chatCtrl.onStreamDone = (timings) => {
    dashboardCtrl.setFinalTimings(timings);
  };
  chatCtrl.onChatCleared = () => {
    dashboardCtrl.resetStreamMetrics();
  };

  // Toggle Sidebar
  if (elements.btnToggleSidebar && elements.sidebarPanel) {
    elements.btnToggleSidebar.addEventListener('click', () => {
      elements.sidebarPanel.classList.toggle('collapsed');
      elements.btnToggleSidebar.classList.toggle('active', !elements.sidebarPanel.classList.contains('collapsed'));
    });
  }

  // Toggle Telemetry HUD
  if (elements.btnToggleTelemetry && elements.telemetryPanel) {
    elements.btnToggleTelemetry.addEventListener('click', () => {
      elements.telemetryPanel.classList.toggle('collapsed');
      elements.btnToggleTelemetry.classList.toggle('active', !elements.telemetryPanel.classList.contains('collapsed'));
    });
  }

  // Clear Chat
  if (elements.btnClearChat) {
    elements.btnClearChat.addEventListener('click', () => {
      chatCtrl.clearChat();
      dashboardCtrl.resetStreamMetrics();
    });
  }

  // Quick Prompt Cards
  document.querySelectorAll('.prompt-card').forEach(card => {
    card.addEventListener('click', () => {
      const prompt = card.dataset.prompt;
      if (prompt) {
        chatCtrl.handleSend(prompt);
      }
    });
  });

  // Model Selection Dropdown Sync
  if (elements.selectModel && elements.inputModel) {
    elements.selectModel.addEventListener('change', () => {
      elements.inputModel.value = elements.selectModel.value;
      settingsCtrl.syncFromInputs();
      dashboardCtrl.onModelChange(elements.selectModel.value);
    });
  }
  if (elements.inputModel) {
    elements.inputModel.addEventListener('change', () => {
      dashboardCtrl.onModelChange(elements.inputModel.value);
    });
    elements.inputModel.addEventListener('input', () => {
      dashboardCtrl.onModelChange(elements.inputModel.value);
    });
  }

  // Initialize Dashboard with selected model architecture
  dashboardCtrl.onModelChange(settingsCtrl.config.modelPath);

  // Connectivity Health Checker
  async function checkConnection() {
    const health = await api.checkHealth();
    if (health.status === 'ok') {
      elements.statusDot.className = 'status-dot online';
      elements.statusText.innerText = 'Connected (8080)';
    } else if (health.status === 'error' && health.code === 503) {
      elements.statusDot.className = 'status-dot loading';
      elements.statusText.innerText = 'Loading Model...';
    } else {
      elements.statusDot.className = 'status-dot offline';
      elements.statusText.innerText = 'Server Offline';
    }
  }

  checkConnection();
  setInterval(checkConnection, 4000);

  // Start Telemetry Polling
  dashboardCtrl.startPolling();

  console.log('🥪 Sandwich Engine Web GUI initialized successfully.');
});
