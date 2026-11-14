# FORENSIC CONFIDENCE & BASELINE TELEMETRY CERTIFICATE (UNSEALED BASELINE v4.6)

**Audit Phase:** Cycle 3 of 6 (GPU Telemetry, Paired Statistics, Tooling)
**Canonical Scope:** Exactly 74 Headings (#1–60, #67–80 in `dairy.md`)
**Target System:** HP Victus 15-fa1xxx | Intel Core i7-13620H (6 P-cores + 4 E-cores) | 23,646 MiB DDR5-5200 | NVIDIA RTX 5050 Mobile 8,151 MiB GDDR6 (sm_120) | Wayland Compositor | Linux 6.18-zen
**Authoritative Aggregate Confidence Score:** **46.15 / 74 = 0.624**

---

## 1. Executive Summary & Confidence Score Dissection

- **Scope:** Exactly 74 canonical techniques (12 PROVEN, 57 PARTIAL, 1 CONTRADICTED, 1 HAZARD VETO, 2 SUPERSEDED/PRUNED, 1 UNVERIFIED).
- **Baseline Aggregate Confidence:** **46.15 / 74 = 0.624** (62.4% verified confidence on current stack).
- **Structural Ceilings:** Current-models ceiling: **65.05 / 74 = 0.879**; Physical single-node ceiling: **68.70 / 74 = 0.9284 ≈ 0.930**.

### Per-Technique Score Reconciliation & Diff Table

| Technique ID | Technique Name | Score v4.5 | Score v4.6 | Net Delta | Forensic Justification |
|---|---|---|---|---|---|
| **#26** | VRAM-Pinned Drafter & Spec-Tree Amortization | 0.45 | 0.45 | +0.00 | Retains PARTIAL/CONTRADICTED in baseline ablation table; VRAM-pinned experiment verified viable (+31.3% decode gain at 6,968 MiB). Score unchanged pending Cycle 4 engine patch. |
| **#55** | Sub-2-bit MoE Expert Dequantization | 0.45 | 0.45 | +0.00 | Scope corrected to sub-2-bit MoE expert dequant (IQ1_M, IQ2_XXS, GSQ_RCO). Score 0.45 unchanged pending model execution. |
| **TOTAL** | Canonical 74 Aggregate Score | 46.15 | 46.15 | +0.00 | Net sum: v4.5 (46.15) -> v4.6 (46.15), Δ = +0.00. Engine patches deferred to Cycles 4-6. |

## 2. Statistical Controlled A/B Ablation Matrix ($n=5$ Interleaved Repetitions)

> **Protocol:** 5 interleaved rounds across C1..C5 with 4s thermal cooling delays under pinned `performance` governor. Exact prompt length: 3,782 tokens; decode length: 63 tokens. Single canonical run ID token pair: (3782, 63). Wall-time is instrumented in-engine with zero-residual arithmetic consistency ($T_{\text{total}} = \sum T_i$).

| Config | Description | Prefill (t/s) [$\mu \pm \sigma$] | Decode (t/s) [$\mu \pm \sigma$] | Load (s) | Init (s) | Prefill (s) | Decode (s) | Teardown (s) | Total Wall (s) |
|---|---|---|---|---|---|---|---|---|---|
| **C1** (Config_1_Pure_Baseline) | Baseline (f16 KV, no MoE cache, no drafter) | 520.90 ± 2.63 | 15.62 ± 0.15 | 1.46 | 0.00 | 7.26 | 4.03 | 0.18 | 12.92s |
| **C2** (Config_2_Turbo4_KV) | Baseline + Turbo4 KV + FlashAttention | 508.36 ± 2.73 | 16.66 ± 0.23 | 1.44 | 0.00 | 7.43 | 3.79 | 0.18 | 12.84s |
| **C3** (Config_3_Turbo4_MoECache) | Turbo4 KV + MoE Hot-Pack (32 slots) + Warm DDR5 Pool + 2Q | 478.42 ± 2.20 | 16.72 ± 0.16 | 1.66 | 0.00 | 7.90 | 3.77 | 0.18 | 13.51s |
| **C4** (Config_4_FullStack_With_Drafter) | Full Stack: Config 3 + Speculative Drafter (-md DFlash2 -ngld 0) | 459.10 ± 1.97 | 25.34 ± 0.26 | 1.87 | 0.00 | 8.23 | 2.49 | 0.18 | 12.77s |
| **C5** (Config_5_Drafter_Alone_On_Baseline) | Baseline + Speculative Drafter (-md DFlash2 -ngld 0, f16 KV, no MoE cache) | 493.48 ± 2.84 | 21.18 ± 0.55 | 1.61 | 0.00 | 7.66 | 2.97 | 0.18 | 12.42s |

### Rigorous Statistical Significance & Synergy Propagation Analysis

1. **Speculative Drafter Decode Acceleration (C4 vs C1):**
   - Delta: $25.34 - 15.62 = +9.72$ t/s (+62.2% speedup).
   - Combined $\sigma_{\text{comb}} = \sqrt{0.26^2 + 0.15^2} = 0.300$ t/s; $2\sigma$ threshold: $0.600$ t/s.
   - **Verdict:** Statistically Significant at **32.4\sigma** ($p < 10^{-9}$). Wall-time decode reduced from 4.03s to 2.49s.

2. **Turbo4 KV Cache Decode Bandwidth Savings (C2 vs C1):**
   - Delta: $16.66 - 15.62 = +1.04$ t/s (+6.7% speedup).
   - Combined $\sigma_{\text{comb}} = \sqrt{0.23^2 + 0.15^2} = 0.275$ t/s; $2\sigma$ threshold: $0.549$ t/s.
   - **Verdict:** Statistically Significant at **3.8\sigma** ($p < 0.001$).

3. **MoE Hot-Pack Standalone Decode Impact (C3 vs C2):**
   - Delta: $16.72 - 16.66 = +0.06$ t/s.
   - Combined $\sigma_{\text{comb}} = \sqrt{0.16^2 + 0.23^2} = 0.280$ t/s; $2\sigma$ threshold: **0.560** t/s.
   - **RETRACTION STATEMENT:** The $+0.06$ t/s difference between C3 and C2 represents only $0.21\sigma < 2.0\sigma$, strictly inside measurement noise. Standalone MoE cache without speculation yields zero statistically verifiable acceleration.

4. **Synergy Interaction & Explicit 4-Config Variance Propagation:**
   - Additive component isolation:
     - Baseline C1: $15.62 \pm 0.26$ t/s
     - Isolated KV effect (C2 - C1): $+1.04$ t/s
     - Isolated Drafter effect (C5 - C1): $+5.56$ t/s
     - Additive Expected Throughput: $15.62 + 1.04 + 5.56 = 22.22$ t/s
     - Measured Full-Stack Throughput (C4): $25.34 \pm 0.26$ t/s
   - Synergy Delta (Interaction Term): $\Delta_{\text{synergy}} = 25.34 - 22.22 = \mathbf{+3.12\text{ t/s}}$
   - Explicit 4-Config Error Propagation:
     $$\sigma_{\text{synergy}} = \sqrt{\sigma_1^2 + \sigma_2^2 + \sigma_3^2 + \sigma_4^2} = \sqrt{0.26^2 + 0.15^2 + 0.23^2 + 0.55^2} = \sqrt{0.4455} = \mathbf{0.6675\text{ t/s}}$$
   - Statistical Significance: $t = +3.12 / 0.6675 = \mathbf{4.67\sigma}$ ($p < 10^{-5}$).
   - Physical Mechanism Designation: Multi-token speculative burst evaluation interacts with reduced KV bandwidth. The claim that this reduces DRAM memory traffic by ~40% is designated as **[HYPOTHESIS — UNMEASURED]** pending hardware counter profiling.

## 3. Paired Perplexity & NIAH Retrieval Verification

### Paired Per-Chunk Perplexity Statistics ($n=32$ Chunks, 16,384 Tokens, wikitext-2-raw)

> **Paired Methodology:** Per-chunk differences $\Delta_k = \text{PPL}_k - \text{PPL}_{\text{f16}}$ eliminate inter-chunk text difficulty variation, reducing standard error by 80%. A difference is statistically significant when $|\Delta| > 2 \cdot \text{SE}(\Delta)$ ($t > 2.0$, $p < 0.05$). Unchunked full-text baseline shifted from 6.4815 to $5.7989$ due to chunking context.

| Configuration | KV Cache Type | PPL [$\mu \pm \sigma$] | Mean Paired $\Delta$ | $\text{SE}(\Delta)$ | Effect % | $t$-statistic | $p$-value | Empirical Verdict |
|---|---|---|---|---|---|---|---|---|
| **FP16_Baseline** | `f16` | 5.7989 ± 0.1537 | +0.0000 | — | 0.00% | — | — | **Reference Baseline** |
| **Q8_0_KV** | `q8_0` | 5.8156 ± 0.1544 | +0.0207 | 0.0080 | +0.32% | $t = 2.60$ | $p = 0.014$ | **Degradation Detected (Minimal)** |
| **Turbo4_Patched** | `turbo4` | 5.8397 ± 0.1553 | +0.0457 | 0.0133 | +0.71% | $t = 3.43$ | $p = 0.0017$ | **Degradation Detected (Modest)** |
| **Turbo2_Polar** | `turbo2` | 6.2143 ± 0.1680 | +0.4795 | 0.0573 | +7.43% | $t = 8.37$ | $p = 1.6 \times 10^{-9}$ | **Degradation Detected (Significant)** |

> **Interpretation:** Paired statistics permanently refute the claim that quantized KV has zero degradation. All quantized formats cause statistically detectable perplexity increases ($t > 2.0$). However, Turbo4 degradation is modest (+0.71%, $t=3.43$) while Turbo2 degrades significantly (+7.43%, $t=8.37$).

### Full-Matrix Needle-In-A-Haystack (NIAH) Retrieval Verification (19/19 PASS)

| Context Length | Depth Fraction | KV Cache Type | Needle Retrieved | Test Status | Latency |
|---|---|---|---|---|---|
| 4,096 (Control) | 50% | FP16 (Dolphin-7B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 4.62s |
| 4,096 | 25% | FP16 (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 46.48s |
| 4,096 | 50% | FP16 (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 25.04s |
| 4,096 | 75% | FP16 (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 23.37s |
| 4,096 | 25% | Q8_0 (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 23.01s |
| 4,096 | 50% | Q8_0 (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 23.10s |
| 4,096 | 75% | Q8_0 (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 22.99s |
| 4,096 | 25% | **Turbo4** (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 22.86s |
| 4,096 | 50% | **Turbo4** (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 22.99s |
| 4,096 | 75% | **Turbo4** (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 22.95s |
| 4,096 | 25% | Turbo2 (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 22.82s |
| 4,096 | 50% | Turbo2 (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 22.63s |
| 4,096 | 75% | Turbo2 (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 22.92s |
| 32,768 | 25% | **Turbo4** (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 63.39s |
| 32,768 | 50% | **Turbo4** (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 63.54s |
| 32,768 | 75% | **Turbo4** (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 64.09s |
| 32,768 | 25% | Q8_0 (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 64.76s |
| 32,768 | 50% | Q8_0 (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 64.25s |
| 32,768 | 75% | Q8_0 (Qwen-35B) | `[SANDWICH-128K-RETRIEVAL-VERIFIED]` | **PASS** | 64.26s |

## 4. Unified Workload Sweep & MoE Cache Instrumentation

### Flat Prefill Hit-Rate Explanation & Source Root-Cause

- **Defect:** Prior sweep tables reported an invariant 26.37% prefill hit rate across all slot counts.
- **Root Cause:** In `src/llama-context.cpp:3568-3571`, prefill cache hit accounting was hardcoded:
  ```cpp
  // src/llama-context.cpp:3569
  uint64_t n_hits = (uint64_t)(n_evals * 0.2637);
  ```
- **Status:** Telemetry for prefill hit rate is a static stub. True dynamic cache hit accounting occurs only during the decode phase.

### Unified Workload Sweep (Identical Prompt: 3,782 tokens, Decode: 63 tokens)

| Slots | Peak VRAM (MiB) | Peak VRAM (MB) | Hot-Pack Footprint [per-layer] | Hot-Pack Footprint [whole-model] | Prefill Hit % (Stub) | Decode Hit % (Pre-Repack) | Decode Hit % (Post-Repack) | Decode Speed (t/s) | VRAM Headroom to 7,100 MiB Guard |
|---|---|---|---|---|---|---|---|---|---|
| **32** | 5,766 MiB | 6,046.07 MB | 35.91 MB [per-layer] | 1,005.58 MB [whole-model] | 26.37% | 26.34% | 27.88% | 13.3 ± 0.23 | **+1,334 MiB Headroom (PASS)** |
| **64** | 6,726 MiB | 7,052.79 MB | 71.83 MB [per-layer] | 2,011.17 MB [whole-model] | 26.37% | 36.56% | 37.75% | 13.2 ± 0.25 | **+374 MiB Headroom (PASS)** |
| **72** | 6,964 MiB | 7,302.26 MB | 80.81 MB [per-layer] | 2,262.56 MB [whole-model] | 26.37% | 39.37% | 40.53% | 14.1 ± 0.28 | **+136 MiB Headroom (PASS)** |
| **76** | 7,084 MiB | 7,428.11 MB | 85.30 MB [per-layer] | 2,388.26 MB [whole-model] | 26.37% | 41.03% | 42.29% | 12.6 ± 0.31 | **+16 MiB Headroom (PASS)** |
| **80** | 7,204 MiB | 7,553.96 MB | 89.79 MB [per-layer] | 2,513.96 MB [whole-model] | — | — | — | CRASH | **-104 MiB Deficit (OOM KILLED)** |

> **32-Slot Operating Point Recommendation:** 32 slots delivers $13.3 \pm 0.23$ t/s, which is statistically indistinguishable from 64 slots ($13.2 \pm 0.25$ t/s, $\Delta = +0.1$ t/s, $t = 0.4\sigma$). However, 32 slots leaves **+1,334 MiB of VRAM headroom** below the 7,100 MiB Wayland compositor guardrail (vs only 374 MiB at 64 slots, 136 MiB at 72 slots, and a hazardous 16 MiB margin at 76 slots). 32 slots is the operationally robust recommendation.

### GPU Expert Counter Coverage & Full-Model Call Arithmetic

- **Model Topology:** 41 blocks, all MoE, `expert_used_count` = 8 (328 expert evaluations per token across whole model).
- **Partitioning:** 13 GPU-resident layers (layers 0..12) via `ggml_cuda_mul_mat_id`; 28 host CPU layers (layers 13..40) via `sandwich-telemetry`.
- **Missing GPU Calls Explained:** The 13 GPU layers are executed entirely on GPU CUDA cores without host telemetry instrumentation. Host telemetry captures only 28/41 layers (**68.3% coverage**).
- **Full-Model Call Arithmetic Breakdown:**
  - **Prefill (3,782 tokens):**
    - Host (layers 13..40): $3,782 \times 28 \times 8 = 847,168$ calls (Tier 1 Hot-Pack: 223,398 hits [26.37% stub]; Tier 2 WARM: 623,770 calls; Tier 3 NVMe: 0 calls).
    - GPU (layers 0..12): $3,782 \times 13 \times 8 = 393,328$ calls (100% GPU VRAM resident).
  - **Decode (63 tokens):**
    - Host (layers 13..40): $63 \times 28 \times 8 = 14,112$ calls (Tier 1 Hot-Pack: 3,717 hits [26.34%]; Tier 2 WARM: 10,395 calls; Tier 3 NVMe: 0 calls).
    - GPU (layers 0..12): $63 \times 13 \times 8 = 6,552$ calls (100% GPU VRAM resident).
  - **Total Whole-Model Expert Evaluations:**
    - Host Telemetry Total: $847,168 + 14,112 = \mathbf{861,280\text{ calls}}$ (verified by formula: $(3782 + 63) \times 224 = 3845 \times 224 = \mathbf{861,280\text{ calls}}$).
    - GPU Uninstrumented Total: $393,328 + 6,552 = \mathbf{399,880\text{ calls}}$ (13 layers $\times$ 8 experts $\times$ 3,845 tokens).
    - Full-Model Grand Total: $861,280 + 399,880 = \mathbf{1,261,160\text{ total calls}}$.
  - **Canonical Token Pairs Declared:**
    - **Sweep Run Canonical Pair:** $(P=3776, D=37)$, total host calls $= 854,112\text{ calls}$.
    - **Benchmark Run Canonical Pair:** $(P=3782, D=63)$, total host calls $= 861,280\text{ calls}$.

## 5. Systemic Memory Unit Reconciliations (MB vs MiB)

### Byte-Level Unit Conversion & Product Cross-Check Table

| Subsystem / Field | Exact Bytes ($B$) | Binary Value (MiB / GiB) | Decimal Value (MB / GB) | Stated Unit in Prior Reports | Reconciled Status |
|---|---|---|---|---|---|
| **GPU Physical VRAM** | 8,546,979,840 B | 8,151.00 MiB (7.96 GiB) | 8,546.98 MB (8.55 GB) | `nvidia-smi` reports MiB | **Reconciled (MiB)** |
| **Wayland Compositor Guard** | 7,444,889,600 B | 7,100.00 MiB (6.93 GiB) | 7,444.89 MB (7.44 GB) | Stated as 7,100 MB | **Reconciled (7,100 MiB)** |
| **Host Physical RAM** | 24,794,660,864 B | 23,646.00 MiB (23.09 GiB) | 24,794.66 MB (24.79 GB) | Sysinfo reports MiB | **Reconciled (MiB)** |
| **Tier 1 Hot Pack (32 slots)** | 1,005,584,384 B | 958.99 MiB | 1,005.58 MB [whole-model] | 1,005.58 MB vs 960 MB | **Exact: 958.99 MiB == 1,005.58 MB** |
| **Tier 1 Hot Pack (64 slots)** | 2,011,168,768 B | 1,917.98 MiB | 2,011.17 MB [whole-model] | 2,011.16 MB | **Exact: 1,917.98 MiB == 2,011.17 MB** |
| **Tier 1 Hot Pack (72 slots)** | 2,262,564,864 B | 2,157.73 MiB | 2,262.56 MB [whole-model] | 2,262.56 MB | **Exact: 2,157.73 MiB == 2,262.56 MB** |
| **Tier 1 Hot Pack (76 slots)** | 2,388,262,912 B | 2,277.60 MiB | 2,388.26 MB [whole-model] | 2,388.25 MB | **Exact: 2,277.60 MiB == 2,388.26 MB** |
| **Tier 2 Host WARM Pool** | 12,500,000,000 B | 11,920.93 MiB (11.64 GiB) | 12,500.00 MB (12.50 GB) | 12500 MB flag argument | **Exact: 12,500 MB (11,921 MiB)** |
| **Tier 2 Anonymous Resident** | 8,444,563,456 B | 8,053.36 MiB (7.86 GiB) | 8,444.56 MB (8.44 GB) | smaps: 8,246,644 kB | **Exact: 8,053 MiB (8,445 MB)** |
| **Peak VRAM (32 slots)** | 6,046,072,832 B | 5,766.00 MiB | 6,046.07 MB | Stated as 5,766 MB | **Reconciled (5,766 MiB)** |
| **Peak VRAM (64 slots)** | 7,052,787,712 B | 6,726.00 MiB | 7,052.79 MB | Stated as 6,726 MB | **Reconciled (6,726 MiB)** |
| **Peak VRAM (72 slots)** | 7,302,262,784 B | 6,964.00 MiB | 7,302.26 MB | Stated as 6,964 MB | **Reconciled (6,964 MiB)** |
| **Peak VRAM (76 slots)** | 7,428,108,288 B | 7,084.00 MiB | 7,428.11 MB | Stated as 7,084 MB | **Reconciled (7,084 MiB)** |
| **Peak VRAM (Pinned Drafter)** | 7,306,457,088 B | 6,968.00 MiB | 7,306.46 MB | Measured in experiment | **Reconciled (6,968 MiB)** |

- **Reconciliation of the 960 MB vs 1,005.58 MB Delta:**
  $32\text{ slots} \times 1,122,304\text{ B/slot} \times 28\text{ layers} = 1,005,584,384\text{ B}$.
  Dividing by $10^6$ yields $1,005.58\text{ MB}$ (decimal).
  Dividing by $2^{20}$ yields $958.99\text{ MiB} \approx 960\text{ MiB}$ (binary).
  The 960 MB delta reported in prior audits was in fact 960 MiB! All memory terms now declare explicit units.

## 6. Informing Experiments & Corrected Decisions

### Experiment 1: VRAM-Pinned Speculative Drafter Viability (Technique #26)

- **Context:** Technique #26 was marked CONTRADICTED because baseline ablation scripts forced `-ngld 0` (host-injected) due to an allocator bug.
- **Empirical Verification (`exp_VRAM_Pinned_DFlash2.log` vs `exp_Host_Injected_DFlash2.log`):**
  - Host-Injected Drafter (`-ngld 0`): $24.3$ t/s decode, Peak VRAM: $6,526$ MiB.
  - VRAM-Pinned Drafter (`-ngld 99`): $31.9$ t/s decode, Peak VRAM: $6,968$ MiB.
  - Performance Delta: $\mathbf{+7.6\text{ t/s}\ (+31.3\%\text{ speedup})}$.
  - Guard Headroom: $6,968\text{ MiB} \le 7,100\text{ MiB}$ compositor guard (**+132 MiB safe margin**).
- **Conclusion:** Pinning the drafter in VRAM is physically safe and highly effective. The falsification was an artifact of bad default flags.

### Corrected Decisions Restated with Ground Truth Scope

1. **Decision 1 (MoE Cache Slot Budget):** 32 slots is recommended. Throughput is $13.3 \pm 0.23$ t/s (vs $13.2 \pm 0.25$ t/s at 64 slots, $t=0.4\sigma$, no significant throughput penalty), while leaving $+1,334$ MiB of VRAM headroom below the 7,100 MiB guard (vs only 374 MiB at 64 slots, 136 MiB at 72 slots, and 16 MiB at 76 slots).
2. **Decision 2 (Technique #55 Scope):** Correct #55 scope from 'DeepSeek-V2 / V3 Fused Linear Attention' to **'Sub-2-bit MoE Expert Dequantization (`IQ1_M`, `IQ2_XXS`, `GSQ_RCO`)'**. The diary claim of a fused CUDA kernel was falsified on current stack (CPU reference only), but sub-2-bit MoE weights are present on disk (1.8 GB) and can be evaluated for CPU offload dequantization.

## 7. Tooling, Sanitizers, and Profiler Governance

### Sudo Block & Retraction of Unmeasured Profiler Claims

- **Sudo Block:** `sudo pacman -S perf linux-tools` is blocked on root password authorization. Linux hardware performance counters (`perf`) cannot be gathered non-interactively.
- **Formal Retraction:** Prior claims citing top-3 CPU bottlenecks (`ggml_compute_forward_mul_mat` 41.2%, `moe_expert_route` 18.4%, `io_uring_enter` 12.1%) and top-3 GPU bottlenecks (`fattn_mma_f16` 54.2%, `mmvq_q4_0` 28.1%, `cudaMemcpyAsync` 11.5%) were static AST estimates, not measured counter profiles. Formally retracted.

### Comprehensive Memory Sanitizer Verification (0 Errors)

1. **CUDA Initcheck on Live Warm Pool (`sanitizer_initcheck_warmpool_live.log`):**
   - Tested on active Qwen-35B inference with warm pool allocation.
   - Result: `========= ERROR SUMMARY: 0 errors`

2. **CUDA Synccheck on Live Warm Pool (`sanitizer_synccheck_warmpool_live.log`):**
   - Tested barrier and stream synchronization during live expert dispatch.
   - Result: `========= ERROR SUMMARY: 0 errors`

3. **AddressSanitizer / UndefinedBehaviorSanitizer (Host Targets):**
   - Target `test-moe-cache-shares`: `sanitizer_asan_moe_cache.log` -> **PASS (0 errors, exit 0)**.
   - Target `test-sandwich-dispatch`: `sanitizer_asan_dispatch.log` -> **PASS (0 errors, exit 0)**.

## 8. Structural Ceilings & Model Unlock Map

### Model Acquisition & Technique Unlock Map

| Model Identifier | Size on Disk | Status | Unlocked Techniques | Impact on Aggregate Score |
|---|---|---|---|:---:|
| **DeepSeek-Coder-V2-Lite** | 8.9 GB | Present on NVMe | #11 (FlashMLA Attention), #46 (KV Snapshot) | +0.50 |
| **LFM2-8B** | 2.6 GB | Present on NVMe | #15 (Recurrent State Rollback), #50 (KV Block Defragmenter) | +0.50 |
| **Qwen-MTP** | 0.4 GB | Present on NVMe | #43 (Native MTP Speculative Rollout) | +0.25 |
| **Sub-2-bit MoE Weights** | 1.8 GB | Present on NVMe | #55 (Sub-2-bit MoE Expert Dequantization) | +0.55 |
| **NVFP4 Quantized Weights** | 4.5 GB | Download Required | #22 (Blackwell FP4 MMA), #32 (Lock-Free Ring Buffer) | +0.50 |
| **DeepSeek-V4.1 (Full)** | 239 GB | Download Required | All DeepSeek-V3 / V4 MoE architectures | Scale test |

- **NVMe Disk Feasibility:** NVMe root volume `/home` has **417 GB available** (49% used). Storing the 239 GB DeepSeek-V4.1 model is completely feasible with >170 GB free space remaining.

### Recomputed Structural Ceilings

- **Current Baseline:** $46.15 / 74 = \mathbf{0.624}$
- **Current Models Ceiling (Models Present on Disk):**
  $$46.15 + 0.50 (\#11,\#46) + 0.50 (\#15,\#50) + 0.25 (\#43) + 0.55 (\#55) + 0.25 (\#40\text{ full}) + \dots = \mathbf{65.05 / 74 = 0.879}$$
- **Physical Single-Node Ceiling (Single-Node Upper Bound with External Models):**
  $$\mathbf{68.70 / 74 = 0.9284 \approx 0.930}$$
  *(Structural Deficit: -3.00 from BY-DESIGN-ZERO [#3, #17, #75], and -2.30 from BLOCKED-HARDWARE multi-node/Metal [#42, #67..#72]).*

