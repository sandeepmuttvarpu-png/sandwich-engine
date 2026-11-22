# Archived One-Off Benchmark Scaffolding

This directory holds one-off forensic benchmark scaffolding that has **zero
references** from the frozen audit ledgers (`technique_ledger_v4.9.json`,
`evidence_ledger_v4.9.md`, `claims_ledger_v4.9.csv`). It is kept as evidence,
not as runnable test assets.

## Why these files moved

* None of the files here is registered with CTest. The registered suite is the
  78 tests in `tests/CMakeLists.txt`.
* No ledger row cites any path in this directory, so moving them cannot break a
  frozen citation.
* The scripts produced one-off ablation telemetry during the Phase 4/5 audits
  (deltas, 2-sigma thresholds, speedup ratios). The numbers they produced were
  transcribed into the ledgers; the scripts themselves have no live consumer.

## What stayed in `tests/benchmarks/`

Every benchmark family that a ledger row cites stayed at its original path so
the frozen citations remain resolvable. This covers the `bench_*` scripts, their
`.cpp`/`.cu` sources and their `result_*.json` telemetry for Techniques #2, #5,
#7, #8, #10, #13, #15, #18, #19, #21, #25, #32, #36, #38, #43, #49, #51, #53,
#55, #56, #58, #59 and #60.

Reference check (2026-11-22): all 24 ledger-cited `tests/benchmarks/...` paths
exist on disk.

## Archived contents

| Path | Role |
| :--- | :--- |
| `measure_fattn_step1.cpp`, `measure_fattn_step1.py` | FlashAttention step-1 crossover microbenchmark and its driver |
| `measure_fattn_step1_results.json`, `measure_fattn_crossover_results.json` | Captured crossover telemetry |
| `run_all_benchmarks.py` | One-off batch runner for the archived ablation results |
| `aggregate_summary.md` | Batch 1-5 ablation summary report |
| `batch2/bench_cuda_graph_52.py`, `batch2/result_cuda_graph_52.json` | Uncited CUDA graph ablation driver and telemetry |
| `batch5/bench_dma_pinned_6.cu`, `batch5/bench_dma_pinned_6.py`, `batch5/result_dma_pinned_6.json` | Uncited pinned DMA ablation and telemetry |

## Evidentiary role

These files support audit claims only. They are not part of the build, are not
run by CTest, and must not be cited as passing tests. Re-running them requires
the original model and hardware; the captured JSON is the reproducible record.
