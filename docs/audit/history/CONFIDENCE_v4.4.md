# FORENSIC AUDIT BASELINE MEASUREMENT REPORT v4.4

> **AUDIT STATUS: UNSEALED — BASELINE MEASUREMENT REPORT**  
> **Hardware Platform:** HP Victus 15 (RTX 5050 Mobile `sm_120`, 8,151 MiB GDDR6; i7-13620H 6 P-cores + 4 E-cores; 24 GB DDR5-5200; Wayland; PCIe Gen4 NVMe).  
> **System Power Governor:** Pinned `performance` via `powerprofilesctl`.  
> **Evaluated Techniques:** Exactly 74 Canonical Headings (#1–60 + #67–80).  
> **Aggregate Baseline Score:** **46.15 / 74 = 0.624**  

## 1. Executive Forensic Summary & Status Reclassification

- **PROVEN:** 12 techniques
- **PARTIAL (Empirical / Simulated):** 57 techniques
- **CONTRADICTED:** 1 techniques (#26 VRAM-Pinned Drafter forces -ngld 0)
- **HAZARD VETO:** 1 technique (#75 `--ram-cache` clamped to 0.00)
- **SUPERSEDED / PRUNED:** 2 techniques (#3 BeeLlama superseded by #37; #17 Web UI excised)
- **UNVERIFIED:** 1 techniques (#20 `--moe-hot-only`)
- **FALSIFIED / RETRACTED CLAIMS:** 3 claims (16.73 GB/s GDS line rate -> 2.51–3.16 GB/s; VRAM-pinned drafter -> host DDR5 injection; Standalone MoE decode acceleration -> retracted at 0.21 sigma).

### Per-Technique Score Reconciliation & Diff Table

| Technique ID | Technique Name | Score v4.1 | Score v4.2 | Score v4.4 | Net Delta | Forensic Justification |
|---|---|---|---|---|---|---|
| **#13** | Intel Hybrid Topology P-Core Pinning | 0.70 | 0.65 | 0.65 | +0.00 | Strict rubric weights: 0.30 T1 + 0.00 T2a + 0.20 T2b + 0.15 T3 = 0.65 (reconciled in v4.2). |
| **#35** | Fused TurboQuant MMA FlashAttention | 0.50 | 0.75 | 0.75 | +0.00 | Upgraded in v4.2 after Defect B1 patch (fattn.cu:905 & fattn-mma-f16.cuh:2389); 0 sanitizer errors. |
| **#37** | 3-Tier Hierarchical MoE Memory System Engine | 0.75 | 0.75 | 1.00 | +0.25 | Upgraded to PROVEN: Precedence Defect B2 resolved with user_set guard; 12.5GB THP pool verified (30.0% collapse). |
| **#38** | 2Q & Ghost-Cache Replacement Policy | 1.00 | 0.75 | 0.75 | +0.00 | Demoted in v4.2 due to missing Tier 3 io_uring zero-copy NVMe bypass. |
| **#40** | NIAH Semantic Fidelity Matrix | 0.75 | 0.75 | 0.75 | +0.00 | Rescoped to PARTIAL (0.75) against diary 128k claim (PROVEN for tested 4k/32k envelope). |
| **#55** | QSA_ROWS & EXPDEC Fused Shared Kernels | 0.45 | 0.45 | 0.45 | +0.00 | Reverted to PARTIAL (0.45): CPU reference in test-ds4-engram; fused CUDA kernel absent. |
| **TOTAL** | Canonical 74 Aggregate Score | 45.90 | 45.90 | 46.15 | +0.25 | Net sum: v4.1 (45.90) -> v4.2 (45.90) -> v4.4 (46.15, +0.25 from v4.2 and v4.1). |

## 2. Statistical Controlled A/B Ablation Matrix ($n=5$ Interleaved Repetitions)

> **Protocol:** 5 interleaved rounds across C1..C5 with 4s thermal cooling delays under pinned `performance` governor. Exact prompt length: 3,782 tokens; decode length: 63 tokens. Wall-time is instrumented in-engine with zero-residual arithmetic consistency ($T_{\text{total}} = \sum T_i$).

| Config | Description | Prefill (t/s) [$\mu \pm \sigma$] | Decode (t/s) [$\mu \pm \sigma$] | Load (s) | Init (s) | Prefill (s) | Decode (s) | Teardown (s) | Total Wall (s) |
|---|---|---|---|---|---|---|---|---|---|
| **C1** (Config_1_Pure_Baseline) | Baseline (f16 KV, no MoE cache, no drafter) | 520.90 ± 2.63 | 15.62 ± 0.15 | 1.46 | 0.00 | 7.26 | 4.03 | 0.18 | 12.92s |
| **C2** (Config_2_Turbo4_KV) | Baseline + Turbo4 KV + FlashAttention | 508.36 ± 2.73 | 16.66 ± 0.23 | 1.44 | 0.00 | 7.43 | 3.79 | 0.18 | 12.84s |
| **C3** (Config_3_Turbo4_MoECache) | Turbo4 KV + MoE Hot-Pack (32 slots) + Warm DDR5 Pool + 2Q | 478.42 ± 2.20 | 16.72 ± 0.16 | 1.66 | 0.00 | 7.90 | 3.77 | 0.18 | 13.51s |
| **C4** (Config_4_FullStack_With_Drafter) | Full Stack: Config 3 + Speculative Drafter (-md DFlash2 -ngld 0) | 459.10 ± 1.97 | 25.34 ± 0.26 | 1.87 | 0.00 | 8.23 | 2.49 | 0.18 | 12.77s |
| **C5** (Config_5_Drafter_Alone_On_Baseline) | Baseline + Speculative Drafter (-md DFlash2 -ngld 0, f16 KV, no MoE cache) | 493.48 ± 2.84 | 21.18 ± 0.55 | 1.61 | 0.00 | 7.66 | 2.97 | 0.18 | 12.42s |

### Rigorous $2\sigma$ Statistical Significance & Retraction Analysis

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
   - **RETRACTION STATEMENT (MANDATED BY DIRECTIVE 1):** The $+0.06$ t/s difference between C3 and C2 represents only $0.21\sigma < 2.0\sigma$, strictly inside measurement noise. All claims of standalone MoE cache decode acceleration without speculative execution are **RETRACTED**. Without speculation, DDR5 synchronous transfer latency dominates decode token generation regardless of static hot-pack residency.

## 3. Patched Attention Correctness Verification (NIAH & Perplexity)

### Perplexity ($\Delta\text{PPL}$) Comparison on wikitext-2-raw (chunks=4, 8,192 tokens)

| Configuration | KV Cache Type | Perplexity (PPL) | $\Delta\text{PPL}$ vs FP16 | Degradation % | Runtime |
|---|---|---|---|---|---|
| **FP16_Baseline** | `f16` | 6.4815 ± 0.24678 | +0.0000 | +0.00% | 20.31s |
| **Q8_0_KV** | `q8_0` | 6.4878 ± 0.24720 | +0.0063 | +0.10% | 20.20s |
| **Turbo4_Patched** | `turbo4` | 6.5313 ± 0.24943 | +0.0498 | +0.77% | 20.05s |
| **Turbo2_Polar** | `turbo2` | 6.8367 ± 0.26349 | +0.3552 | +5.48% | 19.99s |

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

> **Resolution of $Q\text{->ne}[1] \le 4$ Prefill Gate:** During prompt prefill ($Q\text{->ne}[1] = 3,782 > 4$), execution routes to `launch_fattn<DV, ncols1, ncols2>` in `fattn-mma-f16.cuh`, staging Turbo4 KV cache into CUDA SRAM and dequantizing to FP16 before executing Tensor Core MMA operations. This eliminates the CPU fallback while maintaining full numerical precision.

## 4. Post-Fix MoE Cache Slots Sweep & 7,100 MB Wayland Guard

| Slots | Peak VRAM | Peak RSS | Peak AnonHugePages | Cumulative Invocations | Hot-Pack Hits | Hot-Pack Hit Rate % | Guard Status (<= 7,100 MB) |
|---|---|---|---|---|---|---|---|
| **32** | 5,766 MB | 13,928.61 MB | 494.0 MB | 854,112 | 226,037 | 26.46% | **PASS (<= 7,100 MB)** |
| **64** | 6,726 MB | 13,928.70 MB | 492.0 MB | 854,112 | 226,722 | 26.54% | **PASS (<= 7,100 MB)** |
| **72** | 6,964 MB | 13,928.59 MB | 494.0 MB | 854,112 | 226,910 | 26.57% | **PASS (<= 7,100 MB)** |
| **76** | 7,084 MB | 13,928.61 MB | 494.0 MB | 854,112 | 227,010 | 26.58% | **PASS (<= 7,100 MB)** |

### Model Topology & Expert Call Arithmetic Reconciliation

- **Model Architecture:** `qwen35moe`
- **Total Layers (`block_count`):** 41 blocks (blocks 0 to 40)
- **MoE vs Dense Map:** All 41 blocks are MoE; 0 dense blocks.
- **Experts per Block (`expert_count`):** 256 experts
- **Routed Experts per Token (`expert_used_count`):** 8 experts
- **Bytes per Expert:** 1,122,304 bytes (1.0703 MiB / 1.122 MB)
  - `ffn_down_exps`: 450,560 bytes
  - `ffn_gate_exps`: 335,872 bytes
  - `ffn_up_exps`: 335,872 bytes
- **Layer Partitioning under `-ngl 14`:**
  - GPU Offloaded Layers: 13 blocks (blocks 0..12)
  - Host DDR5 MoE Cache Layers: 28 blocks (blocks 13..40)
- **Invocation Dissection:**
  - Prefill Invocations: 3,776 tokens * 28 layers * 8 routed = 845,824 calls
  - Decode Invocations: 37 tokens * 28 layers * 8 routed = 8,288 calls
  - Total Cumulative Evaluations: 845,824 + 8,288 = 854,112 calls

## 5. Architectural Reconciliation & Telemetry Units

1. **Dual-Cache Architectural Separation:**
   - **Cache 1 (Thecodacus, #2 & #19):** Contiguous tensor VRAM hot pack in `llama-model.cpp:2153`. Statically allocates top-S experts into GPU VRAM (32 to 76 slots within 7,100 MB Wayland guard).
   - **Cache 2 (Phase 12, #37 & #38):** Host-RAM THP pool and LRU cache in `llama-moe-cache.cpp:960`. Under `-ngl 14`, layers 13..40 (28 blocks) are host-resident; `groups` in `llama-moe-cache.cpp:695` is empty because Phase 12 manages host DDR5 memory, not GPU VRAM slots.

2. **Hardware Memory Residency & Telemetry:**
   - **Tier 1 (GPU Hot Pack Buffer):** 226,037 hot hits (26.46% hit rate across 854,112 evals); physical VRAM footprint for 32 slots: 35.91 MB.
   - **Tier 2 (Host DDR5 WARM Pool):** 1,811 MB actively cached slabs within 12,500 MB pre-allocated pool; OS anonymous memory resident (`smaps_rollup`): 8,246,644 kB (8,053.36 MB / 7.86 GiB) with 2,476,032 kB (2.36 GiB) in `AnonHugePages` (30.0% THP collapse ratio).
   - **Tier 3 (NVMe COLD Storage):** 0 MB active NVMe paging.
   - **Peak System Memory (`VmHWM`):** 14.60 GiB <= 16,552 MiB system headroom limit.

## 6. Structural Ceiling & Reachability Classification (0.93 Target)

Every technique is assigned an explicit reachability class on this hardware stack:

| Reachability Class | Technique Count | Techniques Included | Max Achievable Score | Inevitable Score Deficit |
|---|:---:|---|:---:|:---:|
| **BY-DESIGN-ZERO** | 3 | #3 (superseded), #17 (pruned via git rm), #75 (hazard veto) | 0.00 each | -3.00 |
| **BLOCKED-HARDWARE** | 7 | #42 (Metal/ROCm on NVIDIA), #67-#72 (multi-node distributed) | 0.70 max (#42: 0.50) | -2.30 |
| **MODEL-FAMILY-CONSTRAINED** | 5 | #11 (MLA), #15 (LFM2 SSM), #20 (hot-only), #22 (FP4), #26 (VRAM drafter) | 0.15 - 0.75 | -2.65 |
| **REACHABLE-ON-THIS-STACK** | 59 | All remaining 59 techniques on Qwen 35B + HP Victus | 1.00 each | 0.00 |
| **STRUCTURAL CEILING** | **74** | **Theoretical Hardware/Model Maximum** | **66.05 / 74** | **0.893 (Baseline) -> 0.930 (Optimized)** |
