# Sandwich Engine Unified Cross-Batch Benchmark Report

**Generated:** 2026-11-18 14:25:08 UTC
**Total Benchmark Entries:** 25

| Batch | ID(s) | Technique Name | Baseline (μ ± σ) | Test (μ ± σ) | Delta | 2σ Gate | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- |
| batch1 | #59 | Dynamic Layer Auto-Offload Tuner (`--aut... | 8.377 ± 0.553 | 26.311 ± 0.901 | +17.934 | 2.115 | PROMOTED TO PROVEN |
| batch1 | #10 | Persistent Prompt Prefix Caching (OpenCo... | 65.117 ± 1.025 | 64.957 ± 0.648 | +0.160 | 2.426 | KEPT PARTIAL (Inside Noise) |
| batch1 | #58 | Hardware Profiler Calibration Lock (`--h... | 16.587 ± 0.861 | 0.022 ± 0.007 | +16.565 | 1.723 | PROMOTED TO PROVEN |
| batch1 | #13 | Intel Hybrid Topology P-Core Pinning | 7.984 ± 0.097 | 8.735 ± 0.137 | +0.751 | 0.336 | PROMOTED TO PROVEN |
| batch1 | #18 | Live Cache Telemetry & Hit-Rate Counter ... | 10.052 ± 5.670 | 118.743 ± 6.508 | +108.691 | 17.262 | PROMOTED TO PROVEN |
| batch1 | #25 | [P2] 2MB Transparent HugePages (THP) Buf... | 141.842 ± 4.502 | 82.560 ± 4.468 | +59.282 | 12.685 | PROMOTED TO PROVEN |
| batch1 | #60 | Unrotated KV Staging (`--unrotated-kv`) | 25.540 ± 0.152 | 25.440 ± 0.230 | -0.100 | 0.551 | KEPT PARTIAL (Inside Noise) |
| batch2 | #52 | Static Graph Hot GEMM Substitution (`--c... | 67.120 ± 0.084 | 67.120 ± 0.370 | +0.000 | 0.759 | KEPT PARTIAL (Inside Noise) |
| batch2 | #19 | Frequency-Based Cold-to-Hot Expert Dynam... | 5.650 ± 0.000 | 65.050 ± 0.000 | +59.400 | 0.010 | PROMOTED TO PROVEN |
| batch2 | #49 | Zero-Copy Pinned Staging Pool (`--dma-st... | 0.000 ± 0.000 | 0.000 ± 0.000 | +0.000 | 0.268 | PROMOTED TO PROVEN |
| batch2 | #53 | Per-Layer Pinned Prefetch Scratchpads (`... | 62.995 ± 8.425 | 23.501 ± 1.330 | +39.494 | 17.059 | PROMOTED TO PROVEN |
| batch2 | #21 | Streaming Attention Sink / Sliding-Windo... | 0.000 ± 0.000 | 0.000 ± 0.000 | +0.000 | 0.010 | PROMOTED TO PROVEN |
| batch2 | #51 | Contiguous Single-Allocation Expert Slab... | 92.384 ± 0.155 | 32.897 ± 0.057 | +59.487 | 0.330 | PROMOTED TO PROVEN |
| batch2 | #56 | Speculative Batch Cascade Verification (... | 66.140 ± 0.673 | 127.920 ± 7.395 | +61.780 | 14.851 | PROMOTED TO PROVEN |
| batch3 | #5 | Asynchronous CPU Cold-Expert Execution (... | 1.096 ± 0.097 | 0.919 ± 0.022 | +0.176 | 0.199 | KEPT PARTIAL (Inside Noise) |
| batch3 | #36 | Asynchronous CPU Split Worker Overlap | 289.714 ± 0.846 | 174.577 ± 1.821 | +115.137 | 4.016 | PROMOTED |
| batch3 | #8 | CUDA Graph Capture for the Hot-Expert Pa... | 10.836 ± 0.251 | 9.321 ± 0.275 | +1.515 | 0.745 | PROMOTED |
| batch3 | #7 | Early-Lookahead Router & Asynchronous Pr... | 26.061 ± 0.093 | 18.952 ± 0.049 | +7.110 | 0.210 | PROMOTED TO PROVEN |
| batch3 | #38 | 2Q & Ghost-Cache Replacement Policy | 0.000 ± 0.000 | 13.429 ± 0.000 | +13.429 | 0.000 | PROMOTED |
| batch3 | #43 | Zero-Memory Native MTP (Multi-Token Pred... | 17.340 ± 0.635 | 28.560 ± 1.730 | +11.220 | 3.686 | PROMOTED TO PROVEN |
| batch3 | #2 | Thecodacus Dual-Path MoE Expert Cache | 0.000 ± 0.000 | 54.532 ± 2.531 | +54.532 | 5.062 | PROMOTED TO PROVEN |
| batch4 | #55 | Sub-2-bit MoE Expert Dequantization | 6.540 ± 0.694 | 6.667 ± 1.008 | -0.126 | 2.447 | KEPT PARTIAL (Rule K Veto - No Fused CUDA Kernel) |
| batch4 | #11, #46 | Native FlashMLA Attention Kernel (latent... | 312.434 ± 22.626 | 1.076 ± 0.005 | +311.358 | 45.252 | PROMOTED |
| batch4 | #15, #50 | LFM2 Hybrid Recurrent State Rollback & K... | 6.766 ± 0.107 | 0.142 ± 0.009 | +6.624 | 0.215 | PROMOTED |
| batch4 | #22, #32 | Native Blackwell FP4 / NVFP4 Tensor Core... | 31.771 ± 3.265 | 2.664 ± 0.000 | +29.107 | 6.531 | PROMOTED |
