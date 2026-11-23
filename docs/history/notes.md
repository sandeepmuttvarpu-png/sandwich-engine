# Project Documentation & Engineering Notes: sandwich-engine

This document collects ongoing observational notes, future refinement tasks, and citation clarifications identified during audit reviews.

---

## 1. Commit SHA Citations in Current State
- `current_state.md` previously cited `7fe588876` while commit `fc20f5298` synchronized the inventory. The snapshot describes the audited engine state as finalized in commit `fc20f5298` and its immediate documentation correction commit.

## 2. Claim 7 (Turbo2 Quantization Perplexity) Citation Note
- Claim 7 currently states catastrophic perplexity collapse ($PPL > 1000$) for uniform 2-bit quantization without per-channel codebooks.
- Note for next update: The paired empirical measurement on record in early ablation logs is $+5.50\%$ relative perplexity degradation under specific test corpora, whereas uncalibrated uniform quantization on larger models exhibited runaway divergence. Provide the exact raw log path or refine the statement to cite both the $+5.50\%$ measured baseline and the regime under which divergence occurs.

## 3. Claim 13 Statistical Wording Clarification
- In `claim_status.md` Claim 13, the statistical significance threshold is accurately expressed as $z = 0.70 < 2$ (standardized score under the standard normal distribution), rather than "< 2sigma", since the denominator of $z = |\Delta| / \sigma_{\text{comb}}$ already normalizes by standard deviation.

## 4. Workload Tagging for CURRENT Claims
- To avoid ambiguous generalization across disparate model architectures, future iterations will append explicit workload tags:
  - **P-Core Pinning (+12.4%):** Workload tagged as `[workload: Qwen3.6-35B-A3B, 6P-cores pinned, Raptor Lake, batch 1]`.
  - **2Q Ghost-Cache Hit Rate (>88%):** Workload tagged as `[workload: Samsung PM9B1 NVMe, 32k context MoE trace, 16 slots]`.
  - **io_uring Direct I/O Latency (<1.2 ms):** Workload tagged as `[workload: Linux io_uring O_DIRECT, 4k-aligned cold expert read]`.
  - **Dynamic YaRN Scaling (32k context):** Workload tagged as `[workload: Qwen3.6-35B, passkey retrieval, 32k window]`.

## 5. VRAM Matrix Command Lines per Row
- The VRAM and headroom matrix in `current_state.md` would benefit from expanding each configuration row to embed its full canonical execution command line inline for one-click operator reproducibility.

## 6. Understory & LM Studio Operational Protocol
- Understory MCP semantic queries require LM Studio's local server active on port 1234 (`lms server start`).
- **Operational Protocol:** Always start LM Studio server for memory maintenance and retrieval queries, but explicitly unload all resident models (`lms unload --all`) before running GPU benchmarks or generation parity tests (`verify_generation_parity.py`) to guarantee an idle GPU memory floor (2 MiB used) and eliminate VRAM contention.
