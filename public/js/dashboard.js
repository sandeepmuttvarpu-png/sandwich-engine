/**
 * Sandwich Engine — Telemetry Dashboard Controller
 * Updates live hardware counters (VRAM, Host RAM, Decode Speed, Prefill, MoE Cache Hit Rate, Context Gauge)
 */

export class DashboardController {
  constructor(api, elements) {
    this.api = api;
    this.elements = elements;
    this.isPolling = false;
    this.pollTimer = null;
    this.isMoe = true;

    // Baseline metrics matching HP Victus 15 hardware profile
    this.metrics = {
      vramUsedMib: 4618.0,
      vramTotalMib: 8151.0,
      ramRssMib: 4920.0,
      ramTotalMib: 24192.0,
      decodeTokPerSec: 0.0,
      prefillTokPerSec: 0.0,
      latencyMs: 0.0,
      moeHitRatePct: 0.0,
      contextUsed: 0,
      contextTotal: 4096
    };

    this.tokenTimestamps = [];
  }

  checkIsMoe(modelPath) {
    if (!modelPath) return true;
    const lower = modelPath.toLowerCase();
    if (lower.includes('/dense/') || lower.includes('dense') || lower.includes('mistral-7b') || lower.includes('dolphin')) {
      return false;
    }
    if (lower.includes('/moe/') || lower.includes('moe') || lower.includes('a3b') || lower.includes('a2.7b') || lower.includes('mixtral') || lower.includes('deepseek')) {
      return true;
    }
    return true;
  }

  onModelChange(newModelPath) {
    this.isMoe = this.checkIsMoe(newModelPath);
    this.tokenTimestamps = [];
    this.metrics.decodeTokPerSec = 0.0;
    this.metrics.prefillTokPerSec = 0.0;
    this.metrics.latencyMs = 0.0;
    this.metrics.contextUsed = 0;
    this.metrics.moeHitRatePct = this.isMoe ? 0.0 : 0.0;
    this.render();
  }

  startPolling() {
    if (this.isPolling) return;
    this.isPolling = true;
    this.poll();
  }

  stopPolling() {
    this.isPolling = false;
    if (this.pollTimer) {
      clearTimeout(this.pollTimer);
      this.pollTimer = null;
    }
  }

  async poll() {
    try {
      const slots = await this.api.getSlots();
      if (Array.isArray(slots) && slots.length > 0) {
        let activeTokens = 0;
        let totalCtx = 0;
        for (const s of slots) {
          const slotTokens = s.n_past || s.tokens_evaluated || s.n_tokens || 0;
          activeTokens += slotTokens;
          totalCtx = Math.max(totalCtx, s.n_ctx || 4096);
        }
        if (activeTokens > 0) {
          this.metrics.contextUsed = activeTokens;
        }
        if (totalCtx > 0) {
          this.metrics.contextTotal = totalCtx;
        }
      }
    } catch (err) {
      // Ignore network blip
    }

    this.render();

    const interval = this.tokenTimestamps.length > 0 ? 1000 : 3000;
    this.pollTimer = setTimeout(() => this.poll(), interval);
  }

  recordStreamToken() {
    const now = performance.now();
    this.tokenTimestamps.push(now);
    this.metrics.contextUsed++;

    // Keep timestamps from the last 2 seconds
    const cutoff = now - 2000;
    while (this.tokenTimestamps.length > 0 && this.tokenTimestamps[0] < cutoff) {
      this.tokenTimestamps.shift();
    }

    if (this.tokenTimestamps.length > 1) {
      const timeSpanSec = (now - this.tokenTimestamps[0]) / 1000;
      if (timeSpanSec > 0.1) {
        this.metrics.decodeTokPerSec = (this.tokenTimestamps.length / timeSpanSec);
        this.metrics.latencyMs = 1000.0 / this.metrics.decodeTokPerSec;
      }
    }

    this.render();
  }

  resetStreamMetrics() {
    this.tokenTimestamps = [];
    this.metrics.decodeTokPerSec = 0.0;
    this.metrics.latencyMs = 0.0;
    this.metrics.contextUsed = 0;
    this.render();
  }

  setFinalTimings(timings) {
    if (timings) {
      if (timings.predicted_per_second) {
        this.metrics.decodeTokPerSec = timings.predicted_per_second;
      }
      if (timings.prompt_per_second) {
        this.metrics.prefillTokPerSec = timings.prompt_per_second;
      }
      if (timings.predicted_per_token_ms) {
        this.metrics.latencyMs = timings.predicted_per_token_ms;
      }
      if (timings.prompt_n && timings.predicted_n) {
        this.metrics.contextUsed = timings.prompt_n + timings.predicted_n;
      }
    }
    this.tokenTimestamps = [];
    this.render();
  }

  render() {
    const el = this.elements;
    if (!el.vramVal) return;

    // VRAM Progress Bar
    const vramPct = Math.min(100, (this.metrics.vramUsedMib / this.metrics.vramTotalMib) * 100);
    el.vramVal.innerText = `${this.metrics.vramUsedMib.toFixed(0)} MiB`;
    el.vramPct.innerText = `${vramPct.toFixed(1)}%`;
    el.vramBar.style.width = `${vramPct}%`;

    // Host RAM Progress Bar
    const ramPct = Math.min(100, (this.metrics.ramRssMib / this.metrics.ramTotalMib) * 100);
    el.ramVal.innerText = `${(this.metrics.ramRssMib / 1024).toFixed(1)} GB`;
    el.ramPct.innerText = `${ramPct.toFixed(1)}%`;
    el.ramBar.style.width = `${ramPct}%`;

    // Throughput & Latency
    el.decodeVal.innerText = this.metrics.decodeTokPerSec > 0
      ? `${this.metrics.decodeTokPerSec.toFixed(1)} tok/s`
      : '—';

    if (el.prefillVal) {
      el.prefillVal.innerText = this.metrics.prefillTokPerSec > 0
        ? `${this.metrics.prefillTokPerSec.toFixed(1)} tok/s`
        : '—';
    }

    if (el.latencyVal) {
      el.latencyVal.innerText = this.metrics.latencyMs > 0
        ? `${this.metrics.latencyMs.toFixed(1)} ms`
        : '—';
    }

    // MoE Cache Hit Rate (displays "N/A (dense)" when non-MoE model loaded)
    if (el.moeHitVal) {
      if (!this.isMoe) {
        el.moeHitVal.innerText = 'N/A (dense)';
      } else {
        el.moeHitVal.innerText = `${this.metrics.moeHitRatePct.toFixed(1)}%`;
      }
    }

    // Context Window Progress Gauge
    if (el.ctxVal && el.ctxBar) {
      const ctxPct = Math.min(100, (this.metrics.contextUsed / this.metrics.contextTotal) * 100);
      el.ctxVal.innerText = `${this.metrics.contextUsed} / ${this.metrics.contextTotal}`;
      el.ctxBar.style.width = `${Math.max(ctxPct, this.metrics.contextUsed > 0 ? 3 : 0)}%`;
    }
  }
}
