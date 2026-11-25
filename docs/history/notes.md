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

## 7. Resolved Defect: `SANDWICH_AUTOTUNE_DISABLE=1` Does Not Gate MoE Online Warm-Up Auto-Tuning Repack
- **Symptom:** Running `llama-cli` with `SANDWICH_AUTOTUNE_DISABLE=1` (e.g. certified benchmarks in `spec_arm0_vs_head.json` and Universality Pass 2) still triggers `Online Warm-Up Auto-Tuning: Re-packed 28 layers into VRAM in ~70–80 ms` on `stderr` during Rep 1 / cold prefill.
- **Root Cause:** Two independent tuning subsystems share the "autotune" concept, but only kernel-variant tuning checks `SANDWICH_AUTOTUNE_DISABLE`:
  - `src/dispatch/autotune.cpp:43` (`run_autotune`) and `ggml/src/ggml-cuda/sandwich-kernel-cache.cuh:198` (`Cache::enabled()`) check `SANDWICH_AUTOTUNE_DISABLE`.
  - `src/llama-context.cpp:3588` (`if (!model.has_moe_cache_profile() && model.has_moe_expert_cache() && !m_warmup_repacked)`) performs the live hot-expert `repack_moe_expert_cache(live_freq)` without checking `SANDWICH_AUTOTUNE_DISABLE` (or a dedicated `SANDWICH_WARMUP_REPACK_DISABLE` gate).
- **Impact:** Any ablation or regression arm intended to run with "autotune disabled" still mutates MoE slot placement after 500 evaluations when hit rate < 65%, complicating cold-vs-warm latency attribution.
- **Resolution:** Wired `SANDWICH_AUTOTUNE_DISABLE` (matching `env_flag` semantics: non-empty and not `"0"`, `"false"`, or `"off"`) into `llama_context` constructor (`src/llama-context.cpp`) to pre-mark `m_warmup_repacked = true` and skip live warm-up repacking whenever autotuning is disabled.

## 8. Open Item: Gemma-4-31B-it QAT Nondeterministic `VmHWM` Spike
- **Symptom:** In the verified 4-model Pass 2 re-run (`scripts/benchmarks/universality/throughput_v2_verified.json`), `Gemma-4-31B-it QAT` (`-ngl 16`, `5,600–5,689 MiB` VRAM, `--spec-type none`) exhibited a `2,815 MiB` `VmHWM` spread across three identical reps:
  - Rep 1: `VmHWM = 14,539 MiB` ✓ (`tg = 2.4 t/s`)
  - Rep 2: `VmHWM = 17,354 MiB` (**+802 MiB** over 16,552 guard; `tg = 1.4 t/s`)
  - Rep 3: `VmHWM = 16,324 MiB` ✓ (`tg = 2.3 t/s`)
- **Action:** Investigate host buffer / `mmap` page-in / `ram_cache` allocation nondeterminism causing the Rep 2 `VmHWM` jump and decode drop.

## 9. Open Item: Re-Examination of `Muse-Glimmer-30B`, `C4AI Command-R 08-2024`, and `Flint Preview`
- **Scope:** In the pre-fix sweep (`coverage.json`, `throughput.json` marked `INSTRUMENT-INVALID`), **every dense model row** was impacted by the `common/common.cpp` `push_to_ram` (`-ngl 0`) and silent `jit_down_quant = true` (`Q2_K`) defects, while `Flint Preview` ran the MoE branch:
  - `Muse-Glimmer-30B` (`muse-glimmer`, 27.85B, dense): Phase 1 & invalid Pass 2 (`3.20 t/s`, `17,762 MiB` VmHWM) ran with `-ngl 0` and silent `Q2_K` weights; needs re-test under fixed hybrid GPU offload (`resolve_dense_offload`).
  - `C4AI Command-R 08-2024` (`command-r`, 32.30B, dense): Timed out at `180s` (`TIMEOUT — RE-RUN AT 600s`) under pre-fix dense CPU-fallback + `12`-thread E-core pool; needs a `600s` re-run under fixed dispatch (`-t 6`, hybrid GPU offload, no silent `Q2_K`).
  - `Flint Preview` (`flint`, 39.79B, MoE): Took the MoE branch (`12.70 t/s`, `4,192 MiB` VRAM), but breached `VmHWM` (`16,775` in Phase 1; `18,229 MiB` in Pass 2) — inspect Tier 2 `moe_warm_ram_mb` + `mmap` sizing.
