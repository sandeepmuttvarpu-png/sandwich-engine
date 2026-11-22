# Comprehensive Claim-Status Ledger: sandwich-engine

This document maintains the authoritative status of every quantitative and architectural claim made throughout the development of `sandwich-engine`. Every claim is classified under strict forensic standards and linked to empirical evidence, hardware configurations, and governing git commits.

---

## 1. Forensic Classification Scheme

Every quantitative claim in this ledger is categorized into one of five forensic states:

| Status | Definition |
| :--- | :--- |
| **CURRENT (VERIFIED)** | Empirically reproduced and verified on target hardware under $n \ge 5$ ablated benchmarks. Complies with all ledger validation rules. |
| **SUPERSEDED** | Historically accurate and verified at time of measurement, but replaced by a more optimal configuration, algorithm, or hardware regime. |
| **RETRACTED** | Formally withdrawn after forensic re-measurement proved the claimed effect was negligible, misunderstood, or mathematically unsound. |
| **FALSIFIED** | Disproven and rejected due to identified methodological flaws, software routing bugs, or fabricated baseline artifacts. |
| **CONTRADICTED** | Empirical hardware measurements directly oppose the theoretical claim (e.g. negative speedup or catastrophic quality degradation). |

---

## 2. Complete Claim-Status Inventory

| ID | Claimed Feature / Hypothesis | Claimed Metric / Behavior | Status | Verified Reality & Empirical Resolution | Test Environment | Governing Commit & Citation |
| :---: | :--- | :--- | :---: | :--- | :--- | :--- |
| **1** | **Universal MoE Speedup** | $3.8\times$ universal speedup across all MoE models | **FALSIFIED** | Uncovered that claimed speedup originated from a hardcoded `0.2637` scaling constant in an early mock baseline. True speedup ranges between $1.15\times$ and $1.31\times$. | i7-13620H, RTX 5050 Mobile | `4c5880c5c` ([CONFIDENCE.md#L420](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L420)) |
| **2** | **Tensor Alignment Padding** | Eliminating tensor alignment padding saves **1.4 GB** VRAM | **RETRACTED** | Exhaustive byte-level audit proved total alignment padding across all layers is only **5.4 MB** ($<0.08\%$ of 8GB VRAM). | RTX 5050 Mobile (8GB), Linux 7.3.5 | `85f4a3c09` ([CONFIDENCE.md#L435](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L435)) |
| **3** | **Speculative Drafting Feasibility** | Speculative drafting on MoE is a net-negative dead-end | **RETRACTED** | The benchmark script passed the drafter GGUF to `-m` instead of `-md`, running target model twice. Speculative drafting is a net positive when pinned to VRAM. | RTX 5050 Mobile, Qwen3.6-35B | `176746cf6` ([CONFIDENCE.md#L580](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L580)) |
| **4** | **Initial Technique Confidence** | 100% of claimed techniques (74/74) verified and production ready | **RETRACTED** | Forensic audit reduced score to 44.35 (v4.8). Promotion batches 1–5 subsequently raised verified score to **53.00 / 74 = 0.716** (v4.9). | HP Victus 15, Ubuntu/Arch | `85f4a3c09` ([CONFIDENCE_v4.9.md#L1](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE_v4.9.md#L1)) |
| **5** | **Pure CPU Offload Decode** | Pure CPU MoE decode with P-core pinning reaches interactive 20+ t/s | **SUPERSEDED** | Standalone CPU decode achieves $11.2\text{ t/s}$. Superseded by hybrid GPU/CPU offload (`-ngl 99 -ngld 99`), which reaches $31.73\text{ t/s}$. | i7-13620H (6P+4E), DDR5-5200 | `176746cf6` ([CONFIDENCE.md#L595](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L595)) |
| **6** | **Turbo2 (2-bit KV Quant)** | 2-bit uniform quantization maintains acceptable output quality | **RETRACTED (RETIRED)** | Uniform 2-bit quantization without per-channel codebooks suffers catastrophic perplexity collapse ($PPL > 1000$). Formally retired under Rule N. | Qwen3.6-35B, RTX 5050 | `12cbd14e8` ([CONFIDENCE.md#L450](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L450)) |
| **7** | **P-Core Affinity Pinning** | Pinning worker threads to Raptor Lake P-cores eliminates jitter | **CURRENT (VERIFIED)** | $+12.4\%$ sustained throughput gain over OS scheduler default; thread migration overhead reduced to zero. | i7-13620H, performance governor | `7752c6c32` ([CONFIDENCE.md#L210](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L210)) |
| **8** | **70% Host RAM Guard** | Capping WARM host RAM pool at 16,552 MB prevents kernel OOM | **CURRENT (VERIFIED)** | Zero kernel OOM-killer invocations across 500+ test cycles under concurrent desktop workloads. | 24GB DDR5 host, Linux 7.3.5 | `86c44d476` ([CONFIDENCE.md#L235](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L235)) |
| **9** | **3-Tier Hierarchical MoE** | VRAM $\to$ RAM $\to$ NVMe tiered offload maintains interactive decode | **CURRENT (VERIFIED)** | Direct I/O via io_uring loads cold expert slices with $<1.2\text{ ms}$ latency; hot/warm cache hit rate $>88\%$. | Samsung NVMe, io_uring | `51360748d` ([CONFIDENCE.md#L280](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L280)) |
| **10** | **Dynamic YaRN Scaling** | Closed-form variance normalization extends context to 32k tokens | **CURRENT (VERIFIED)** | Passes passkey retrieval and perplexity sanity checks across 32k context windows without numerical divergence. | Qwen3.6-35B, RTX 5050 | `03acc353c` ([CONFIDENCE.md#L310](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L310)) |
| **11** | **DFlash2 MMA Routing** | DFlash2 candidate verify runs through `mma_turbo` | **FALSIFIED** | $N_{\text{draft}} = 6 \implies Q = 7$. Microarchitectural threshold `Q->ne[1] <= 4` in `fattn.cu:1266` routes to `BEST_FATTN_KERNEL_MMA_F16`. `fattn_native_count` was 0. | RTX 5050, Turing MMA | `85f4a3c09` ([CONFIDENCE.md#L510](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L510)) |
| **12** | **DFlash2 MMA Fix** | Adjusting MMA kernel dispatch to support $Q > 4$ restores acceleration | **CURRENT (VERIFIED)** | Dispatched verification kernels properly route to optimized tensor core paths, eliminating fallback overhead. | RTX 5050 Mobile, Blackwell `sm_120` | `85f4a3c09` ([CONFIDENCE.md#L525](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L525)) |
| **13** | **`-ngld 99` VRAM Pinning** | VRAM drafter pinning achieves **$28.80 \pm 1.42\text{ t/s}$** ($30.73\text{ t/s}$ warm-4) | **CURRENT (VERIFIED)** | $+6.23\text{ t/s}$ speedup over `-ngld 0` ($22.58\text{ t/s}$); peak VRAM 6,576 MiB with **524 MiB safe headroom** below compositor guard. | RTX 5050 8GB, driver 620.73.04 | `176746cf6` ([CONFIDENCE.md#L590](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L590)) |
| **14** | **Sequential Batching $nrc > 1$** | IQ3_S/IQ2_S grid unpack amortization yields up to **$3.90\times$ kernel speedup** | **CURRENT (VERIFIED)** | Prefill throughput: **$484.35 \pm 5.11\text{ t/s}$** ($+23.08\text{ t/s}$); Decode: **$31.73 \pm 1.12\text{ t/s}$** ($+7.46\text{ t/s}$); peak VRAM 7,056 MiB; bit-identical SHA256 match. | i7-13620H (P-core 2), RTX 5050 | `ed16af597` ([CONFIDENCE.md#L620](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L620)) |

---

## 3. Methodological Standards & Experimental Guardrails

To prevent regression to unverified claims, all future performance claims must adhere to the following standards:

1. **$n \ge 5$ Ablation Standard:** Every performance metric must report mean and standard deviation across at least five independent warm runs under locked CPU/GPU frequencies.
2. **Deterministic Token Parity:** Output token streams must match canonical reference generation token-for-token (verified via pure-token SHA256 hashing).
3. **Hardware Boundary Compliance:**
   - Host allocations must strictly honor the **16,552 MB WARM pool ceiling** (70% Host RAM rule).
   - GPU allocations must strictly honor the **7,100 MiB Wayland compositor safety ceiling**.
4. **Automated Ledger Enforcement:** Any proposed score change must pass all 26 rules in `python3 docs/audit/validate_ledger.py` with 0 defects before merge.
