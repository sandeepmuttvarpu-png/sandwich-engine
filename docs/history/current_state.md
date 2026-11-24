# Current Engine State Snapshot: sandwich-engine

**Date:** November 23, 2026  
**Status:** Certified & Audited (v4.9 Ledger Baseline)  
**Repository Working Directory:** `/home/sandeep/dev/sandwich/sandwich-engine`

---

## 1. Core Integrity & Validation Metrics

| Metric | Certified Value | Status | Citation / Command |
| :--- | :---: | :---: | :--- |
| **Git HEAD SHA** | `7fe588876` (reconciled claims ledger, tracked hook installer, config matrix) | Validated | `git rev-parse HEAD` |
| **CTest Passing Suite** | **78 / 78 Passing (100%)** | Active | `ctest --test-dir build` (zero exclusions) |
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
- **CPU Governor:** `performance` (frequency locked across P-cores)
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

1. **Sequential Batching ($nrc > 1$) Full-Stack Ablation & Clean Arbitration:**
   - The micro-kernel win is proven ($2.40\text{ cyc/32}$ vs $9.36\text{ cyc/32}$, $74.4\%$ cycle reduction, bit-exact across $nrc \in \{1, 2, 3, 4, 5, 8\}$ via `test-sandwich-nrc`).
   - **Initial 4-Arm Ablation (Session Noise Identified):**
     * Arm 0 vs Arm 1 decode: $\Delta = +1.000\text{ t/s}$, $\sigma_{\text{comb}} = 1.146$, $2\sigma_{\text{comb}} = 2.292$, $z = 0.87 < 2.0$ (Not Significant).
     * Arm 1 vs Arm 2 decode: $\Delta = -1.000\text{ t/s}$, $z = 0.67 < 2.0$ (Not Significant).
     * Arm 2 vs Arm 3 decode: $\Delta = +0.400\text{ t/s}$, $z = 0.25 < 2.0$ (Not Significant).
     * Arm 2 vs Arm 1 prefill: $\Delta = \mathbf{+8.325\text{ t/s}}$, $2\sigma_{\text{comb}} = 5.248$, $\mathbf{z = 3.17 > 2.0}$ (**Significant ✓**).
   - **Clean Isolated Arbitration (Arm 0 vs Arm 1, $n=5$ Interleaved):**
     * Protocol: Matched slots 16, autotune disabled, 4s cooldown. Background interference eliminated (installer paused via SIGSTOP, `ryoku-ai-usage.timer` stopped, compositor idle at 2 MiB VRAM, >90% CPU idle).
     * **Arm 0 (Control):** Prefill `[435.0, 445.7, 448.9, 441.0, 452.6]`, Decode `[25.4, 28.7, 27.6, 29.3, 29.6]`
     * **Arm 1 (Grouping Only):** Prefill `[444.0, 447.8, 447.4, 451.2, 445.9]`, Decode `[23.4, 29.0, 29.4, 30.6, 30.2]`
     * **Arm 2 (Grouping + $n\text{chunk1}=1$):** Prefill `[439.1, 454.5, 457.4, 456.3, 457.4]`, Decode `[27.2, 29.4, 29.8, 29.1, 26.9]`
     * **Arm 3 (Full `ab022ec65`):** Prefill `[452.6, 441.4, 461.9, 449.6, 442.8]`, Decode `[24.4, 27.9, 30.2, 29.6, 29.1]`
   - **Warm-4 Rule Wording:** Discard run 1 *only if it is cold and slow*. When reporting, provide both all-5 and warm-4 with external background interference outliers explicitly flagged.
   - **Environment Guard:** Linux `scaling_governor` is `powersave` with `energy_performance_preference = performance` via `powerprofilesctl`. Clean isolated re-run of Arm 0 vs Arm 1 pending when background noise drops to $\sigma \le 0.35$.
2. **Formal Deprecation ADR for Technique #44:** Technique #44 (Turbo2 uniform 2-bit quantization) is retired in the confidence ledger, but `GGML_TYPE_TURBO2_0` type definitions and template instantiations remain in CUDA kernels. A formal ADR should govern its removal.
3. **Pruning Dead Code from Technique #55:**
   - Dead parameter `bool prefill_head_alias` in [src/llama-cparams.h#L60](file:///home/sandeep/dev/sandwich/sandwich-engine/src/llama-cparams.h#L60) is unread across the engine.
   - Header [src/ds4_engram_io.hpp](file:///home/sandeep/dev/sandwich/sandwich-engine/src/ds4_engram_io.hpp) has zero includes in any engine build target.
4. **Telemetry HUD Wiring for Techniques #78 & #79:**
   - **#78 (PagedAttention CoW Allocator):** T3 score held at 0.10 until copy-on-write event counters are piped to the interactive HUD.
   - **#79 (Flash-Decoding):** T3 score held at 0.05 because GPU-side reduction tree timings remain unmeasured in the live telemetry feed.
5. **166-Call Legacy Residual:** Documented in [CONFIDENCE.md#L480](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L480) as an unexplainable legacy telemetry trace. Retained for historical transparency while fresh runs consistently emit exact multiples of 8.
