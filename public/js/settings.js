/**
 * Sandwich Engine — Settings & Dispatch Configuration Controller
 * Manages Auto-Dispatch vs. Manual Mode parameter tuning and CLI command synthesis
 */

export class SettingsController {
  constructor(api, elements) {
    this.api = api;
    this.elements = elements;
    this.isAutoMode = true;

    this.config = {
      modelPath: '/home/sandeep/ai-models/dense/dolphin-2.8-mistral-7b-v02-GGUF/dolphin-2.8-mistral-7b-v02-Q4_K_S.gguf',
      nCtx: 4096,
      nThreads: 6,
      nGpuLayers: 14,
      drafterPath: '/home/sandeep/ai-models/drafters/Qwen3.6-35B-A3B-DFlash2-GGUF/Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf',
      drafterGpuLayers: 99,
      moeCacheSlots: 16,
      moeWarmRamMb: 16552,
      ctk: 'turbo4',
      ctv: 'turbo4',
      ioUring: true,
      moe2q: true,
      moeExchangeRot: true
    };

    this.initEventListeners();
    this.populateModels();
    this.updateCommandPreview();
  }

  initEventListeners() {
    const el = this.elements;

    // Mode tabs
    if (el.tabAuto && el.tabManual) {
      el.tabAuto.addEventListener('click', () => this.setMode(true));
      el.tabManual.addEventListener('click', () => this.setMode(false));
    }

    // Input changes trigger command preview update
    const formInputs = [
      el.inputModel, el.inputCtx, el.inputThreads, el.inputGpuLayers,
      el.inputDrafter, el.inputDrafterLayers, el.inputCacheSlots,
      el.inputWarmRam, el.selectCtk, el.checkIoUring, el.checkMoe2q, el.checkMoeRot
    ];

    formInputs.forEach(input => {
      if (input) {
        input.addEventListener('change', () => this.syncFromInputs());
        input.addEventListener('input', () => this.syncFromInputs());
      }
    });

    // Copy synthesized CLI command
    if (el.btnCopyCli) {
      el.btnCopyCli.addEventListener('click', () => {
        const cmd = this.generateCliCommand();
        navigator.clipboard.writeText(cmd).then(() => {
          const original = el.btnCopyCli.innerText;
          el.btnCopyCli.innerText = '✓ Copied';
          setTimeout(() => { el.btnCopyCli.innerText = original; }, 1800);
        });
      });
    }
  }

  setMode(isAuto) {
    this.isAutoMode = isAuto;
    const el = this.elements;

    if (el.tabAuto) el.tabAuto.classList.toggle('active', isAuto);
    if (el.tabManual) el.tabManual.classList.toggle('active', !isAuto);

    if (el.autoSettingsGroup) el.autoSettingsGroup.style.display = isAuto ? 'block' : 'none';
    if (el.manualSettingsGroup) el.manualSettingsGroup.style.display = isAuto ? 'none' : 'block';

    this.updateCommandPreview();
  }

  async populateModels() {
    const el = this.elements;
    if (!el.selectModel) return;

    try {
      const models = await this.api.getModels();
      if (models.length > 0) {
        el.selectModel.innerHTML = '';
        models.forEach(m => {
          const opt = document.createElement('option');
          opt.value = m.id;
          opt.innerText = m.id.split('/').pop();
          el.selectModel.appendChild(opt);
        });
      }
    } catch (err) {
      console.warn('Using default model paths');
    }
  }

  syncFromInputs() {
    const el = this.elements;
    if (el.inputModel) this.config.modelPath = el.inputModel.value;
    if (el.inputCtx) this.config.nCtx = parseInt(el.inputCtx.value, 10) || 4096;
    if (el.inputThreads) this.config.nThreads = parseInt(el.inputThreads.value, 10) || 6;
    if (el.inputGpuLayers) this.config.nGpuLayers = parseInt(el.inputGpuLayers.value, 10) || 14;

    if (el.inputDrafter) this.config.drafterPath = el.inputDrafter.value;
    if (el.inputDrafterLayers) this.config.drafterGpuLayers = parseInt(el.inputDrafterLayers.value, 10) || 99;
    if (el.inputCacheSlots) this.config.moeCacheSlots = parseInt(el.inputCacheSlots.value, 10) || 0;
    if (el.inputWarmRam) this.config.moeWarmRamMb = parseInt(el.inputWarmRam.value, 10) || 0;
    if (el.selectCtk) {
      this.config.ctk = el.selectCtk.value;
      this.config.ctv = el.selectCtk.value;
    }
    if (el.checkIoUring) this.config.ioUring = el.checkIoUring.checked;
    if (el.checkMoe2q) this.config.moe2q = el.checkMoe2q.checked;
    if (el.checkMoeRot) this.config.moeExchangeRot = el.checkMoeRot.checked;

    this.updateCommandPreview();
  }

  generateCliCommand() {
    if (this.isAutoMode) {
      return `./build/bin/sandwich -m "${this.config.modelPath}" --sandwich-auto -t ${this.config.nThreads} -ngl ${this.config.nGpuLayers} -c ${this.config.nCtx}`;
    }

    let cmd = `./build/bin/llama-cli -m "${this.config.modelPath}" -ngl ${this.config.nGpuLayers} -t ${this.config.nThreads} -c ${this.config.nCtx}`;
    if (this.config.drafterPath) {
      cmd += ` -md "${this.config.drafterPath}" -ngld ${this.config.drafterGpuLayers}`;
    }
    if (this.config.moeCacheSlots > 0) {
      cmd += ` --moe-cache-slots ${this.config.moeCacheSlots}`;
    }
    if (this.config.moeWarmRamMb > 0) {
      cmd += ` --moe-warm-ram-mb ${this.config.moeWarmRamMb}`;
    }
    if (this.config.ctk) {
      cmd += ` -ctk ${this.config.ctk} -ctv ${this.config.ctv}`;
    }
    if (this.config.ioUring) cmd += ` --io-uring`;
    if (this.config.moe2q) cmd += ` --moe-2q`;
    if (this.config.moeExchangeRot) cmd += ` --moe-exchange-rot`;

    return cmd;
  }

  updateCommandPreview() {
    if (this.elements.cliPreviewText) {
      this.elements.cliPreviewText.innerText = this.generateCliCommand();
    }
  }
}
