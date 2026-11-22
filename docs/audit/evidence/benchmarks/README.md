# Historical Evidentiary Benchmarks & Scaffolding

This directory contains one-off forensic ablation scripts, microbenchmarks, and empirical telemetry generated during the Phase 4/5 Sandwich Engine validation audits.

## Archive Context & Lineage
- **Origin:** Formerly located at `tests/benchmarks/`.
- **CTest Status:** Unregistered in `tests/CMakeLists.txt` (CTest maintains 78/78 registered unit and integration tests under `tests/`).
- **Purpose:** These scripts provided the empirical evidence ($t$-statistic, $2\sigma$ thresholds, and speedup ratios) for technique promotions and retractions in `technique_ledger_v4.9.json` and `BATCH_SUMMARY_v4.9.md`.

## Contents
- `batch1/` through `batch5/`: Standalone Python and C++/CUDA benchmarks for Techniques #5..#60 (THP, P-core pinning, DMA staging, slab alloc, Flash MLA, LFM2 rollback, NVFP4, etc.).
- `measure_fattn_step1.*`: FlashAttention step 1 crossover and tile sizing microbenchmarks.
- `aggregate_summary.md`: Historical ablation run summaries.
