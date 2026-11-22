# Current Engine State Snapshot: sandwich-engine

**Date:** November 22, 2026  
**Status:** Certified & Audited (v4.9 Ledger Baseline)  
**Repository Working Directory:** `/home/sandeep/dev/sandwich/sandwich-engine`

---

## 1. Core Integrity & Validation Metrics

| Metric | Certified Value | Status | Citation / Command |
| :--- | :---: | :---: | :--- |
| **Git HEAD SHA** | `ed16af5970c406839b8bc14e1cba50b07977eb71` | Validated | `git rev-parse HEAD` |
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

## 3. Recommended Production Command (`-ngld 99`)

To achieve maximum inference throughput while remaining safely within physical hardware limits, run:

```bash
./build/bin/llama-cli \
  -m /home/sandeep/dev/sandwich/ai-models/Qwen3.6-35B-A3B-UD-IQ3_XXS/Qwen3.6-35B-A3B-UD-IQ3_XXS.gguf \
  -md /home/sandeep/dev/sandwich/ai-models/Qwen3.6-35B-A3B-DFlash2-Q4_K_M/Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf \
  -ngl 99 \
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

### Verified Performance Benchmarks
- **Prefill Throughput:** $\mathbf{484.35 \pm 5.11\text{ t/s}}$ ($+23.08\text{ t/s}$ over baseline $461.27\text{ t/s}$)
- **Decode Throughput:** $\mathbf{31.73 \pm 1.12\text{ t/s}}$ ($+7.46\text{ t/s}$ over baseline $24.27\text{ t/s}$)
- **Peak VRAM Consumption:** $7,056\text{ MiB}$ ($44\text{ MiB}$ safety margin below 7,100 MiB guard; $6,576\text{ MiB}$ / $524\text{ MiB}$ margin when running standard row grouping)
- **Token Parity:** Output token sequence matches reference generation bit-for-bit (verified via SHA256 hashing across $n=5$ runs)
- **Buffer Stability:** 0 buffer reallocations or graph abortions under `GGML_SCHED_DEBUG_REALLOC=1`

---

## 4. Known Open Items & Pruning Candidates

1. **Formal Deprecation ADR for Technique #44:** Technique #44 (Turbo2 uniform 2-bit quantization) is retired in the confidence ledger, but `GGML_TYPE_TURBO2_0` type definitions and template instantiations remain in CUDA kernels. A formal ADR should govern its removal.
2. **Pruning Dead Code from Technique #55:**
   - Dead parameter `bool prefill_head_alias` in [src/llama-cparams.h#L60](file:///home/sandeep/dev/sandwich/sandwich-engine/src/llama-cparams.h#L60) is unread across the engine.
   - Header [src/ds4_engram_io.hpp](file:///home/sandeep/dev/sandwich/sandwich-engine/src/ds4_engram_io.hpp) has zero includes in any engine build target.
3. **Telemetry HUD Wiring for Techniques #78 & #79:**
   - **#78 (PagedAttention CoW Allocator):** T3 score held at 0.10 until copy-on-write event counters are piped to the interactive HUD.
   - **#79 (Flash-Decoding):** T3 score held at 0.05 because GPU-side reduction tree timings remain unmeasured in the live telemetry feed.
4. **166-Call Legacy Residual:** Documented in [CONFIDENCE.md#L480](file:///home/sandeep/dev/sandwich/sandwich-engine/docs/audit/CONFIDENCE.md#L480) as an unexplainable legacy telemetry trace. Retained for historical transparency while fresh runs consistently emit exact multiples of 8.
