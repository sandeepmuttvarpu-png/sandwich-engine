# Sandwich.cpp Technique Catalog

This catalog indexes the formal 74 techniques incorporated into the **Sandwich.cpp** bare-metal inference engine.

> [!IMPORTANT]
> **AUDIT CLOSE-OUT & CONFIDENCE METRICS**
> - **Authoritative Close-Out Score:** **45.60 / 74 = 0.616** (Close-Out Baseline).
> - **Structural Ceilings:** **0.915** (Physical Single-Node Ceiling: 67.70 / 74) | **0.866** (Current-Models Ceiling: 64.05 / 74).
> - **Resolved Engine Fixes & Promotions:**
>   - **Technique #26 (VRAM-Pinned Drafter):** Promoted to `PROVEN` (0.90) following router patch enabling dynamic VRAM drafter pinning (+23.2% decode speedup). Rubric breakdown: $T_1 = 0.30/0.30$ (`src/dispatch/router.cpp:256`), $T_{2a} = 0.25/0.25$ (`test-sandwich-dispatch` exit 0), $T_{2b} = 0.20/0.25$ (partial: verified on Qwen model family), $T_3 = 0.15/0.20$ (partial: single-node $\le 7,100$ MiB VRAM headroom).
>   - **Technique #79 (Sequence-Parallel Flash-Decoding):** Promoted to `PROVEN` (0.90) following local chunk denominator fix (`p.sum_exp`), achieving fp32 numerical parity (max delta 4.80e-07). Rubric breakdown: $T_1 = 0.30/0.30$ (`src/dispatch/flash_decoding_scheduler.cpp:85`), $T_{2a} = 0.25/0.25$ (`tests/test_flash_decoding_parity.py` exit 0), $T_{2b} = 0.20/0.25$ (partial: verified on 12-config Python testbed), $T_3 = 0.15/0.20$ (partial: sequence-split reduction runs in host scheduler thread).
> - **Retractions & Hazards Documented in Audit:**
>   - **Technique #55 (QSA_ROWS & EXPDEC Fused Shared-Memory Kernels):** Contradicted (0.45) due to absent CUDA kernel (implemented only as CPU scalar helper in `src/ds4_engram_io.hpp:42`), whereas diary claimed warp-shuffle/shared-memory GPU fusion.
>   - **Technique #75 (Dedicated Host RAM L2 Cache):** Hazard Veto (0.00). Co-allocating 12.5GB with Tier-2 WARM pool triggers 25GB allocation on 24GB host (immediate OOM kill).
>   - **Telemetry Multiplier:** Hardcoded synthetic multiplier `0.2637` permanently retracted from `llama-context.cpp` in favor of raw atomic counters.

| # | Technique Name | Status | Confidence Score | Category / Focus |
|---|----------------|:------:|:----------------:|-------------------|
| 1 | **Hardware-Aware Auto-Dispatcher (`--sandwich-auto`)** | `PROVEN` | 1.00 | Inference Engine |
| 2 | **Thecodacus Dual-Path MoE Expert Cache** | `PROVEN` | 1.00 | Inference Engine |
| 3 | **BeeLlama Dynamic MoE Slab Cache** | `SUPERSEDED` | 0.00 | Inference Engine |
| 4 | **Mutual Exclusivity Cache Arbiter** | `PARTIAL` | 0.55 | Inference Engine |
| 5 | **Asynchronous CPU Cold-Expert Execution (`sched_async_cpu`)** | `PARTIAL` | 0.50 | Inference Engine |
| 6 | **Pinned Host Memory & Zero-Copy PCIe DMA (`cudaHostRegister`)** | `PARTIAL` | 0.75 | Inference Engine |
| 7 | **Early-Lookahead Router & Asynchronous Prefetcher** | `PARTIAL` | 0.50 | Inference Engine |
| 8 | **CUDA Graph Capture for the Hot-Expert Path** | `PARTIAL` | 0.50 | Inference Engine |
| 9 | **TurboQuant & Turbo-KVarN Unified Hybrid KV Cache Compression (`turbo4`, `turbo3`, `turbo2`)** | `PARTIAL` | 0.75 | Inference Engine |
| 10 | **Persistent Prompt Prefix Caching (OpenCode Acceleration)** | `PARTIAL` | 0.50 | Inference Engine |
| 11 | **Native FlashMLA Attention Kernel (`latent_attn.cu`)** | `PARTIAL (0.75)` | 0.75 | Inference Engine |
| 12 | **Multi-Model Speculative Decoding Subsystem** | `PROVEN` | 1.00 | Inference Engine |
| 13 | **Intel Hybrid Topology P-Core Pinning** | `PARTIAL` | 0.65 | Inference Engine |
| 14 | **Asymmetric Mixed-Precision Quantization** | `PARTIAL` | 0.70 | Inference Engine |
| 15 | **LFM2 Hybrid Recurrent State Rollback** | `PARTIAL` | 0.50 | Inference Engine |
| 16 | **Offline Routing Trace Utility (`llama-moe-trace`)** | `PARTIAL` | 0.50 | Inference Engine |
| 17 | **Modern Web UI & Telemetry HUD** | `PRUNED / ABSENT` | 0.00 | Inference Engine |
| 18 | **Live Cache Telemetry & Hit-Rate Counter HUD** | `PARTIAL` | 0.50 | Inference Engine |
| 19 | **Dynamically Repacked Contiguous GPU Memory Buffers / Online Self-Tuning MoE Cache** | `PROVEN` | 1.00 | Inference Engine |
| 20 | **Hot-Only Speculative Fast Drafting (`--moe-hot-only`)** | `UNVERIFIED` | 0.15 | Inference Engine |
| 21 | **Streaming Attention Sink / Sliding-Window KV Compaction** | `PARTIAL` | 0.55 | Inference Engine |
| 22 | **Native Blackwell FP4 / NVFP4 Tensor Core Acceleration** | `PARTIAL` | 0.50 | Inference Engine |
| 23 | **System-Wide Model Benchmarking Harness & Dynamic Drafter Footprint Sizing** | `PARTIAL` | 0.50 | Inference Engine |
| 24 | **EAGLE3 / P-EAGLE Speculative Tree Decoding (`--spec-tree`)** | `PARTIAL` | 0.50 | Inference Engine |
| 25 | **[P2] 2MB Transparent HugePages (THP) Buffer Alignment (Technique #25)** | `PARTIAL` | 0.65 | Inference Engine |
| 26 | **[P3] VRAM-Pinned Drafter & Spec-Tree Amortization (Technique #26)** | `PROVEN` | 0.90 | Inference Engine |
| 27 | **Predictive Hardware-Aware Topology Solver & TurboQuant Auto-Router (Technique #27)** | `PARTIAL` | 0.65 | Inference Engine |
| 28 | **[P1] Pipelined Double-Buffered Speculative Execution (Technique #28)** | `PARTIAL` | 0.50 | Inference Engine |
| 29 | **[P3] Asymmetric Split-KV Engine: Pure VRAM KV Cache on CPU Models (Technique #29)** | `PARTIAL` | 0.50 | Inference Engine |
| 30 | **[P4] AVX2-VNNI P-Core Vector Execution for Q2_K & IQ2_XXS (Technique #30)** | `PARTIAL` | 0.50 | Inference Engine |
| 31 | **[P1] 3-Tier io_uring Async NVMe Direct Storage (Technique #31)** | `PARTIAL` | 0.75 | Inference Engine |
| 32 | **[P2] Lock-Free Pinned Ring-Buffer DMA Staging (Technique #32)** | `PARTIAL` | 0.75 | Inference Engine |
| 33 | **[P1] Quantized-Native CUDA FlashAttention (-fanq) (Technique #33)** | `PARTIAL` | 0.75 | Inference Engine |
| 34 | **GPU Top-K Logits Reduction (`--logits-topk`) (Technique #34)** | `PARTIAL` | 0.50 | Inference Engine |
| 35 | **Fused TurboQuant MMA Tensor Core FlashAttention (Technique #35)** | `PARTIAL` | 0.75 | Inference Engine |
| 36 | **Asynchronous CPU Split Worker Overlap (Technique #36)** | `PARTIAL` | 0.50 | Inference Engine |
| 37 | **3-Tier Hierarchical MoE Memory System Engine (Technique #37)** | `PARTIAL` | 0.75 | Inference Engine |
| 38 | **2Q & Ghost-Cache Replacement Policy (Technique #38)** | `PARTIAL` | 0.75 | Inference Engine |
| 39 | **Asymmetric Split-KV & CPU Weight Pinning for Dense Models (70% RAM Rule) (Technique #39)** | `PARTIAL` | 0.65 | Inference Engine |
| 40 | **Needle-In-A-Haystack (NIAH) Long-Context Semantic Fidelity Verification (Technique #40)** | `PARTIAL` | 0.75 | Inference Engine |
| 41 | **Radix-Tree Persistent Prefix Caching / RadixAttention (Technique #41)** | `PROVEN` | 1.00 | Inference Engine |
| 42 | **Cross-Platform Unified Hardware Architecture (Metal & ROCm HIP) (Technique #42)** | `PARTIAL` | 0.50 | Inference Engine |
| 43 | **Zero-Memory Native MTP (Multi-Token Prediction) Speculative Rollout (Technique #43)** | `PARTIAL` | 0.50 | Inference Engine |
| 44 | **Dynamic YaRN Sliding-Factor Auto-Extension (Technique #44)** | `PARTIAL` | 0.50 | Inference Engine |
| 45 | **Lossless Adaptive Sparse Attention (LASA) (Technique #45)** | `PARTIAL` | 0.50 | Inference Engine |
| 46 | **Online Rolling KV Cache Checkpointing & Fast Snapshot Restore (Technique #46)** | `PARTIAL` | 0.50 | Inference Engine |
| 47 | **Hierarchical Prefetch Stream Arbiter (`--prefetch-arbiter`) (Technique #47)** | `PARTIAL` | 0.50 | Inference Engine |
| 48 | **Rotated-Packing Ring Exchange Engine (`--moe-exchange-rot`) (Technique #48)** | `PARTIAL` | 0.75 | Inference Engine |
| 49 | **Zero-Copy Pinned Staging Pool (`--dma-staging-pool`) (Technique #49)** | `PARTIAL` | 0.50 | Inference Engine |
| 50 | **Speculative Tree Topology Pruner (`--spec-tree-prune`) (Technique #50)** | `PARTIAL` | 0.50 | Inference Engine |
| 51 | **Contiguous VRAM Expert Slab Packing (`--expert-slab`) (Technique #51)** | `PARTIAL` | 0.50 | Inference Engine |
| 52 | **Static Graph Hot GEMM Substitution (`--cuda-graph-hot-gemm`) (Technique #52)** | `PARTIAL` | 0.50 | Inference Engine |
| 53 | **Per-Layer Pinned Prefetch Scratchpads (`--prefetch-scratchpad`) (Technique #53)** | `PARTIAL` | 0.50 | Inference Engine |
| 54 | **Message-Boundary Delta Caching (`--msg-cache`) (Technique #54)** | `PROVEN` | 1.00 | Inference Engine |
| 55 | **QSA_ROWS & EXPDEC Fused Shared-Memory Kernels (`--fused-expdec`) (Technique #55)** | `CONTRADICTED` | 0.45 | Inference Engine |
| 56 | **Speculative Batch Cascade Verification (`--spec-cascade`) (Technique #56)** | `PARTIAL` | 0.50 | Inference Engine |
| 57 | **DeepSeek V4.1 Flash Native Engram Direct NVMe I/O (`--dsv41`) (Technique #57)** | `PROVEN` | 1.00 | Inference Engine |
| 58 | **Hardware Profiler Calibration Lock (`--hw-lock`) (Technique #58)** | `PARTIAL` | 0.50 | Inference Engine |
| 59 | **Dynamic Layer Auto-Offload Tuner (`--auto-offload`) (Technique #59)** | `PARTIAL` | 0.50 | Inference Engine |
| 60 | **Unrotated KV Staging (`--unrotated-kv`) (Technique #60)** | `PARTIAL` | 0.50 | Inference Engine |
| 67 | **Peer-to-Peer Distributed Heterogeneous VRAM Pooling (`--peer-nodes`) (Technique #67, aliased as #61)** | `PARTIAL (simulated)` | 0.70 | Inference Engine |
| 68 | **Asymmetric Heterogeneous Topology Solver (Technique #68, aliased as #62)** | `PARTIAL (simulated)` | 0.70 | Inference Engine |
| 69 | **Direct Memory Access (DMA) & Zero-Copy Interconnect Transport (`--peer-dma`, `--peer-split-kv`) (Technique #69, aliased as #63)** | `PARTIAL (simulated)` | 0.70 | Inference Engine |
| 70 | **Distributed 2-Bit Walsh-Hadamard Polar Network Compression (`--peer-turbo-net`) (Technique #70, aliased as #64)** | `PARTIAL (simulated)` | 0.70 | Inference Engine |
| 71 | **Cross-Node Speculative Verification Pipeline (`--peer-spec-pipe`) (Technique #71, aliased as #65)** | `PARTIAL (simulated)` | 0.70 | Inference Engine |
| 72 | **Dynamic Zero-Crash Failover with Hot Re-partitioning (`--peer-fallback`) (Technique #72, aliased as #66)** | `PARTIAL (simulated)` | 0.70 | Inference Engine |
| 73 | **Dynamic Memory Watermarking System (`--dynamic-watermark`) (Technique #73)** | `PROVEN` | 1.00 | Inference Engine |
| 74 | **Asymmetric Parallel Prefetch Engine (`--asym-prefetch`) (Technique #74)** | `PROVEN` | 1.00 | Inference Engine |
| 75 | **Dedicated Host RAM L2 Caching Layer (`--ram-cache`) (Technique #75)** | `CONFLICT / HAZARD VETO` | 0.00 | Inference Engine |
| 76 | **Real-Time RAM Estimation & Chunked Prefill (`--prefill-chunk-size`) (Technique #76)** | `PROVEN` | 1.00 | Inference Engine |
| 77 | **Heavy Hitter Oracle (H2O) Dynamic KV Cache Eviction (`--h2o`) (Technique #77)** | `PARTIAL` | 0.75 | Inference Engine |
| 78 | **PagedAttention Dynamic Block Cache with Copy-on-Write (Technique #78)** | `PROVEN` | 1.00 | Inference Engine |
| 79 | **Sequence-Parallel Flash-Decoding across GPU SMs (`--flash-decoding`) (Technique #79)** | `PROVEN` | 0.90 | Inference Engine |
| 80 | **GPUDirect Storage (`cuFile`) Direct NVMe-to-VRAM DMA Engine (`--gds`) (Technique #80)** | `PARTIAL` | 0.75 | Inference Engine |
