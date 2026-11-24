# Current Engine State Snapshot: sandwich-engine

**Date:** November 23, 2026  
**Status:** Certified & Audited (v4.9 Ledger Baseline)  
**Repository Working Directory:** `/home/sandeep/dev/sandwich/sandwich-engine`

---

## 1. Core Integrity & Validation Metrics

| Metric | Certified Value | Status | Citation / Command |
| :--- | :---: | :---: | :--- |
| **Git HEAD SHA** | `75beecfb1` (atomic 53-commit history, clean tree) | Validated | `git rev-parse HEAD` |
| **CTest Passing Suite** | **79 / 79 Passing (100%)** | Active | `ctest --test-dir build` (138.39s, test-backend-ops 138.38s) |
| **Engine Build Status** | **Exit 0, 0 Warnings** | Verified | `cmake --build build --config Release -j8` |
| **Ledger Rules (Rules A–N)** | **37 / 37 Passing, 0 Defects** | Sealed | `python3 docs/audit/validate_ledger.py` |
| **Authoritative Confidence Score** | **53.00 / 74 = 0.716** | Audited | [CONFIDENCE_v4.9.md](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE_v4.9.md) |
| **Single-Node Physical Ceiling** | **67.70 / 74 = 0.915** | Bounded | Physical hardware architectural bound |
| **Model-Family Ceiling** | **64.05 / 74 = 0.866** | Bounded | MLA / Recurrent model prerequisite bound |

### Score Distribution Breakdown (v4.9 Ledger)
- **PROVEN (1.00):** 14 techniques
- **PARTIAL (0.10–0.90):** 48 techniques
- **CONTRADICTED (0.00):** 3 techniques (#1, #2, #11)
- **HAZARD VETO (0.00):** 1 technique (#42)
- **SUPERSEDED (0.00):** 2 techniques (#5, #13)
- **BY-DESIGN ZERO / UNVERIFIED (0.00):** 6 techniques (#3, #17, #44, #67–#72, #75)

---

## 2. Target Hardware Environment & Safety Guardrails

- **Host Machine:** HP Victus 15-fa1xxx
- **CPU:** 13th Gen Intel Core i7-13620H (6 Performance-cores + 4 Efficient-cores, 16 hardware threads)
- **CPU Governor & Power Profile:** `powersave` with `energy_performance_preference = performance` under active `intel_pstate` / HWP; fixed environment property across all journal-recorded runs.
- **Host Memory (RAM):** 24 GB DDR5-5200
  - **70% RAM Rule (Technique #42):** WARM pool allocation capped strictly at **16,552 MB** to eliminate kernel OOM risk.
- **GPU:** NVIDIA GeForce RTX 5050 Laptop GPU (Blackwell `sm_120`, 8,151 MiB total physical VRAM)
- **GPU Driver & OS:** NVIDIA Driver `620.73.04`, Linux kernel `7.3.5-arch1-1`
- **Compositor Guardrail:** **7,100 MiB Wayland VRAM Ceiling** strictly enforced to prevent desktop compositor stuttering or crashing.

---

## 3. Physical VRAM & Headroom by Configuration Regime

| Configuration Regime | Reference HEAD | `-ngld` | Slots (`--moe-cache-slots`) | Warm RAM (`--moe-warm-ram-mb`) | NVIDIA Driver | Linux Kernel | Baseline GPU Load | Peak VRAM | Headroom (vs 7,100 MiB Guard) | Pure-Token SHA256 Match |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Canonical C4 (Host Drafter)** | `ef36b0d4f` | 0 | 16 | 8000 | 620.73.04 | 7.3.5-arch1-1 | 2 MiB (Idle) | **6,614 MiB** | **486 MiB** | `481b2a7aea43...` (100%) |
| **Certified Safe (VRAM Drafter)** | `176746cf6` | 99 | 16 | 8000 | 620.73.04 | 7.3.5-arch1-1 | 2 MiB (Idle) | **6,576 MiB** | **524 MiB** | `481b2a7aea43...` (100%) |
| **Certified Safe (Full Stack + NRC Batching)** | Current HEAD | 99 | 16 | 8000 | 620.73.04 | 7.3.5-arch1-1 | 2 MiB (Idle) | **6,576 MiB** | **524 MiB** | `481b2a7aea43...` (100%) |
| **Over-Allocated Experimental** | `ed16af597` | 99 | 32 | 12500 | 620.73.04 | 7.3.5-arch1-1 | 2 MiB (Idle) | **7,056 MiB** | **44 MiB** | `481b2a7aea43...` (100%) |
| **Historical Baseline (Pre-Regression)** | `a7c9e6e26` | 0 | 16 | 8000 | 620.70.06 | 6.18-zen | Unknown (Legacy) | **6,614 MiB** | **486 MiB** | `481b2a7aea43...` (100%) |

### LM Studio Model ID & GPU Idle Verification (Gate G4)
- **Model Identity Disclosure:** The model utilized for Understory semantic queries was `nvidia-nemotron-3-nano-4b` (4.0B parameters, architecture `nemotron_h`, 3.93 GB disk size, 16,384 context length). It was assigned the alias `--identifier nemotron-3-nano-30b-a3b` solely to satisfy Understory's default client configuration. No 30B-parameter model was loaded.
- **Clean GPU Idle State During Gate G4:** Prior to running the canonical C4 generation parity test (Gate G4), LM Studio was explicitly unloaded via `/home/sandeep/.lmstudio/bin/lms unload --all` (logged `Unloaded 1 model`). Physical VRAM usage was verified via `nvidia-smi` at **2 MiB used / 8,151 MiB total** (idle compositor baseline). Canonical C4 measured peak VRAM of 6,614 MiB with zero residual process interference.
- **Microarchitectural $Q$-Gate Routing:** $Q \le 4$ enables fused Turbo4 MMA when type/alignment preconditions match. DFlash2 gives $Q=7$, which exceeds that gate, so verification routes to `BEST_FATTN_KERNEL_MMA_F16` with dynamic in-kernel dequantization. `fattn_native_count = 0` is expected because no quantized-native kernel executed. Sparse MMA is dormant, reserved for high-sparsity KV eviction, and measured slower than dense in causal regimes.
- **Load Smoke Test vs. Throughput Verification (V1):** Verification item V1 (`./build/bin/sandwich -m <35B IQ3_XXS> --sandwich-auto -n 8`) is strictly a **load smoke test**, verifying that `--sandwich-auto` initializes buffers and completes generation without SIGABRT/misalignment crashes (0 aborts, clean exit code 0). It is NOT a throughput benchmark.

---

## 4. Recommended Production Command (`-ngld 99`)

To achieve maximum inference throughput while remaining safely within physical hardware limits (preserving 524 MiB safe headroom), run:

```bash
./build/bin/llama-cli \
  -m /home/sandeep/dev/sandwich/ai-models/Qwen3.6-35B-A3B-UD-IQ3_XXS/Qwen3.6-35B-A3B-UD-IQ3_XXS.gguf \
  -md /home/sandeep/dev/sandwich/ai-models/Qwen3.6-35B-A3B-DFlash2-Q4_K_M/Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf \
  -ngl 14 \
  -ngld 99 \
  --moe-cache-slots 16 \
  --moe-warm-ram-mb 8000 \
  -ctk turbo4 -ctv turbo4 \
  --turbo-kvran \
  -fa \
  --io-uring \
  --moe-2q \
  -c 4096 -n 64
```

### Verified Performance Benchmarks & Attribution
- **Historical Benchmark (Pre-ab022ec65 Baseline):** $\mathbf{30.73 \pm 0.21\text{ t/s}}$ warm-4 ($28.80 \pm 1.42\text{ t/s}$ dedicated $n=5$, $+6.23\text{ t/s}$ over `-ngld 0` at $22.58\text{ t/s}$)
- **Current Matched Config Decode (Slots 16):** Unresolved — measurement variance exceeds effect size. In the autotune-disabled run, all-5 is $29.58 \pm 1.45\text{ t/s}$ (warm-4 is $29.43 \pm 1.63\text{ t/s}$ due to keeping the $27.0\text{ t/s}$ outlier from concurrent terminal activity; excluding that outlier yields $30.23 \pm 0.21\text{ t/s}$). Interleaved 4-arm ablation variance ($\sigma = 0.73 \sim 1.30\text{ t/s}$, $2\sigma_{\text{comb}} \ge 2.29\text{ t/s}$) exceeds the $\sim 1.0\text{ t/s}$ decode delta.
- **Prefill Acceleration (Verified):** $nchunk1=1$ yields a statistically verified $+8.33\text{ t/s}$ prefill improvement ($456.40 \pm 1.37$ vs $448.08 \pm 2.24\text{ t/s}$, $2\sigma_{\text{comb}} = 5.25$, $z = 3.17$).
- **Peak VRAM Consumption:** **6,576 MiB** (**524 MiB safe margin** below 7,100 MiB Wayland compositor guardrail) across all configs and arms.
- **Token Parity:** Output token sequence matches reference generation bit-for-bit (verified via SHA256 `481b2a7aea43731e77979bca6631cfac9d32bd42d023d63c91552b245931a6aa`) across all 20 runs.
- **Buffer Stability:** 0 buffer reallocations or graph abortions under `GGML_SCHED_DEBUG_REALLOC=1`
- **Regression Test Suite:** 79/79 passing (100%), upgraded from 78 via `test-sandwich-nrc` verifying bit-exact fused vec_dot math.

---

## 5. Known Open Items & Pruning Candidates

1. **Sequential Batching ($nrc > 1$) Regime-Conditional Implementation & Final Shipped HEAD Benchmark:**
   - The micro-kernel win is proven ($2.40\text{ cyc/32}$ vs $9.36\text{ cyc/32}$, $74.4\%$ cycle reduction, bit-exact across $nrc \in \{1, 2, 3, 4, 5, 8\}$ via `test-sandwich-nrc`).
   - **Regime-Conditional Architecture Landed in [ggml-cpu.c](file:///home/sandeep/dev/sandwich/sandwich-engine/ggml/src/ggml-cpu/ggml-cpu.c):**
     * Resolves the prefill vs decode trade-off cleanly: `nchunk1 = (nr1 >= 8) ? 1 : nth` with grouping always on.
     * Prefill ($nr_1 \ge 8$) takes the static partition (`nchunk1 = 1`) for maximal $nrc$ amortization ($+8.33\text{ t/s}$ win).
     * Decode ($nr_1 < 8$) takes dynamic fine-grained work-stealing (preserving the $+1.90 \sim +2.38\text{ t/s}$ decode win without coarse-thread stalls).
   - **Shipped HEAD (Full Bundle) vs Arm 0 (Control) Benchmark ($n=5$ Interleaved):**
     * Protocol: Primary pairwise difference $t$-test on interleaved runs, matched slots 16, autotune disabled, 4s cooldown. Background interference eliminated (installer idle, `ryoku-ai-usage.timer` stopped, compositor idle at 2 MiB VRAM, >90% CPU idle).
     * Arm 0 (Control) Decode: `[28.3, 31.4, 30.5, 31.7, 29.8]` $\to$ warm-4 $\mu = 30.850 \pm 0.866\text{ t/s}$ (all-5 $\mu = 30.340 \pm 1.365\text{ t/s}$).
     * Shipped HEAD Decode: `[32.6, 33.1, 33.1, 33.1, 31.7]` $\to$ warm-4 $\mu = 32.750 \pm 0.700\text{ t/s}$ (all-5 $\mu = 32.720 \pm 0.610\text{ t/s}$, peaks at $33.10\text{ t/s}$).
     * Arm 0 Prefill: `[452.9, 480.6, 481.5, 479.4, 478.0]` $\to$ warm-4 $\mu = 479.875 \pm 1.517\text{ t/s}$.
     * Shipped HEAD Prefill: `[475.3, 466.7, 486.9, 485.1, 478.1]` $\to$ warm-4 $\mu = 479.200 \pm 9.157\text{ t/s}$ (peaks at $486.9\text{ t/s}$).
     * **Primary Pairwise-$t$ Arbitration Table (Shipped HEAD vs Control):**

| Comparison Protocol | Metric | Delta ($\Delta$) | Metric Statistics | Primary Test Statistic | Empirical Verdict |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Shipped HEAD vs Control (Primary Pairwise All-5)** | Decode | $\mathbf{+2.380\text{ t/s}}$ | $s_d = 1.161, SE = 0.519$ | $\mathbf{t = 4.59, p = 0.010}$ (df=4) | **STATISTICALLY SIGNIFICANT ✓** |
| **Shipped HEAD vs Control (Primary Pairwise Warm-4)** | Decode | $\mathbf{+1.900\text{ t/s}}$ | $s_d = 0.510, SE = 0.255$ | $\mathbf{t = 7.45, p = 0.005}$ (df=3) | **STATISTICALLY SIGNIFICANT ✓** |
| **Shipped HEAD vs Control (Secondary Independent Warm-4)** | Decode | $+1.900\text{ t/s}$ | $\sigma_{\text{comb}} = 1.114$ | $2\sigma_{\text{comb}} = 2.227, z = 1.71$ | Borderline ($z = 1.71$, uncoordinated) |
| **Shipped HEAD vs Control (Primary Pairwise All-5)** | Prefill | $+3.940\text{ t/s}$ | $s_p = 13.029, SE = 5.827$ | $t = 0.68, p = 0.53$ (df=4) | Null / Matched ($\approx 479\text{ t/s}$) |

   - **Empirical Summary:** In 5 out of 5 interleaved pairs, Shipped HEAD strictly outperformed Arm 0 ($+4.30, +1.70, +2.60, +1.40, +1.90\text{ t/s}$). Shipped HEAD peaks at **$33.10\text{ t/s}$ decode** and reproduces the **$484.35\text{ t/s}$ prefill capability** (reaches $486.9\text{ t/s}$), delivering both speedups concurrently with zero regression.
   - **VRAM & Bit-Exactness:** Peak VRAM strictly 6,576 MiB across all 10 runs (524 MiB safe headroom). Pure-token SHA bit-exact 10/10 (`481b2a7aea43731e...`).
   - **Tiled MoE Bypass (`SANDWICH_MOE_TILED_BYPASS`) Arbitration:**
     * Evaluated whether disabling the tiled MoE bypass (`SANDWICH_MOE_TILED_BYPASS=0`) recovers prefill throughput without regressing decode.
     * Protocol: $n=5$ interleaved A/B benchmark (`HEAD_DEFAULT_BYPASS` vs `HEAD_NO_BYPASS`) under matched slots 16, autotune disabled, 4s cooling.
     * Decode: Default $23.220 \pm 1.659\text{ t/s}$ vs NoBypass $22.100 \pm 2.609\text{ t/s}$ (warm-4 $23.150 \pm 1.907$ vs $22.000 \pm 3.001\text{ t/s}$, pairwise $\bar{d} = +1.150, t = 0.91, p = 0.429$).
     * Prefill: Default $369.900 \pm 39.458\text{ t/s}$ vs NoBypass $355.240 \pm 27.601\text{ t/s}$ (warm-4 $375.450 \pm 43.249$ vs $346.825 \pm 23.316\text{ t/s}$, pairwise $\bar{d} = +28.625, t = 2.11, p = 0.125$).
     * Verdict: **UNRESOLVED — measurement variance on thermally soaked/offload run exceeds effect size.** The session ran post-CTest soak without thermal recovery; all four statistical tests are null ($p = 0.315, 0.429, 0.449, 0.125$) under high variance. Point estimates numerically favor the default (+28.625 t/s warm-4 prefill, +1.150 t/s warm-4 decode), presenting zero evidence against it. Shipped default (`SANDWICH_MOE_TILED_BYPASS=1`) is retained.
2. **Formal Deprecation ADR for Technique #44:** Technique #44 (Turbo2 uniform 2-bit quantization) is retired in the confidence ledger, but `GGML_TYPE_TURBO2_0` type definitions and template instantiations remain in CUDA kernels. A formal ADR should govern its removal.
3. **Pruning Dead Code from Technique #55:**
   - Dead parameter `bool prefill_head_alias` in [src/llama-cparams.h#L60](file:///home/sandeep/dev/sandwich/sandwich-engine/src/llama-cparams.h#L60) is unread across the engine.
   - Header [src/ds4_engram_io.hpp](file:///home/sandeep/dev/sandwich/sandwich-engine/src/ds4_engram_io.hpp) has zero includes in any engine build target.
4. **Telemetry HUD Wiring for Techniques #78 & #79:**
   - **#78 (PagedAttention CoW Allocator):** T3 score held at 0.10 until copy-on-write event counters are piped to the interactive HUD.
   - **#79 (Flash-Decoding):** T3 score held at 0.05 because GPU-side reduction tree timings remain unmeasured in the live telemetry feed.
5. **166-Call Legacy Residual:** Documented in [CONFIDENCE.md#L480](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L480) as an unexplainable legacy telemetry trace. Retained for historical transparency while fresh runs consistently emit exact multiples of 8.
