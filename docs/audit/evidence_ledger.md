# Forensic Audit Evidence Ledger v4.9

- Total Techniques: 74
- Aggregate Confidence: 45.05 / 74 = 0.609
- Physical Single-Node Ceiling: 67.70 / 74 = 0.915 (< 0.93: UNREACHABLE)
- Current-Models Ceiling: 64.05 / 74 = 0.866 (< 0.93: UNREACHABLE)
- Proven Threshold: >= 0.90
- Target 0.93 Status: STRICTLY UNREACHABLE on this physical hardware

## Status Counts

- **CONFLICT / HAZARD VETO**: 1
- **CONTRADICTED**: 1
- **PARTIAL**: 57
- **PROVEN**: 11
- **PRUNED / ABSENT**: 1
- **RETIRED**: 1
- **SUPERSEDED**: 1
- **UNVERIFIED**: 1

Total: 74

## Complete Technique Ledger

| ID | Name | Flag | Status | Score | T1 | T2a | T2b | T3 | Notes |
|---|---|---|---|---|---|---|---|---|---|
| 1 | Hardware-Aware Auto-Dispatcher (`--sandwich-auto`) | `--sandwich-auto` | PROVEN | 1.00 | 0.30 | 0.25 | 0.25 | 0.20 | Probed 6 P-cores, 8151MB VRAM, 23646MB RAM in 0.04s. Selects -ngl, -t, and memory tiering. |
| 2 | Thecodacus Dual-Path MoE Expert Cache | `--moe-cache-slots` | PARTIAL | 0.70 | 0.30 | 0.25 | 0.15 | 0.00 | Cycle 4 Demotion: Prefill hit-rate stub (llama-context.cpp:3569) removed; real prefill hit rate measured at 10.12% (136/1344). Host decode hot hits verified at 64.96%. |
| 3 | BeeLlama Dynamic MoE Slab Cache | `N/A` | SUPERSEDED | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | Superseded by Technique #37 Tier 2 WARM pool and #2 Thecodacus. Score 0.00 is by-design. |
| 4 | Mutual Exclusivity Cache Arbiter | `N/A` | PARTIAL | 0.55 | 0.30 | 0.00 | 0.15 | 0.10 | Prevents co-activation of mutually exclusive cache implementations. |
| 5 | Asynchronous CPU Cold-Expert Execution (`sched_async_cpu`) | `--sched-async-cpu` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Gated behind experimental flag. |
| 6 | Pinned Host Memory & Zero-Copy PCIe DMA (`cudaHostRegister`) | `N/A` | PARTIAL | 0.75 | 0.30 | 0.25 | 0.10 | 0.10 | Diary claimed 18.43-20 GB/s falsified; uncached rate is 2.51-3.16 GB/s. Demoted to PARTIAL (0.75). |
| 7 | Early-Lookahead Router & Asynchronous Prefetcher | `--moe-lookahead` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Pre-computes top-k expert routing for layer L+1. |
| 8 | CUDA Graph Capture for the Hot-Expert Path | `--cuda-graph-hot` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Captures hot GEMM execution graph; requires static shapes. |
| 9 | TurboQuant & Turbo-KVarN Unified Hybrid KV Cache Compression (`turbo4`, `turbo3`, `turbo2`) | `-ctk` | CONTRADICTED | 0.45 | 0.30 | 0.15 | 0.00 | 0.00 | Cycle 4 Demotion: Turbo2 PolarQuant measured PPL = 6.2143 vs FP16 5.7989 (+7.16% PPL increase, t=8.37, p < 0.001). |
| 10 | Persistent Prompt Prefix Caching (OpenCode Acceleration) | `-cram` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Prefix caching supported via -cram buffer. |
| 11 | Native FlashMLA Attention Kernel (`latent_attn.cu`) | `N/A` | PARTIAL | 0.75 | 0.30 | 0.25 | 0.10 | 0.10 | DKQ=640, DV=512 tensor core MMA. Demoted to PARTIAL (0.75) due to synccheck barrier exception in cuBLAS. |
| 12 | Multi-Model Speculative Decoding Subsystem | `-md` | PROVEN | 1.00 | 0.30 | 0.25 | 0.25 | 0.20 | DFlash2 drafter accelerates decode; mean accepted length 3.75 tok/step; positional acceptance (1.0, 1.0, 0.75). |
| 13 | Intel Hybrid Topology P-Core Pinning | `-t` | PARTIAL | 0.65 | 0.30 | 0.00 | 0.20 | 0.15 | Dynamically binds compute threads strictly to 6 P-cores. Score 0.65 (0.30 + 0.00 + 0.20 + 0.15). |
| 14 | Asymmetric Mixed-Precision Quantization | `N/A` | PARTIAL | 0.70 | 0.30 | 0.25 | 0.10 | 0.05 | Quantization type selection verified in test suite. |
| 15 | LFM2 Hybrid Recurrent State Rollback | `N/A` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Recurrent state snapshotting for hybrid models. Untested on Qwen MoE. |
| 16 | Offline Routing Trace Utility (`llama-moe-trace`) | `N/A` | PARTIAL | 0.50 | 0.30 | 0.00 | 0.10 | 0.10 | Extracts expert routing frequency for auto-tuning. |
| 17 | Modern Web UI & Telemetry HUD | `N/A` | PRUNED / ABSENT | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | Pruned via git rm; replaced by zero-overhead CLI telemetry. Score 0.00 by-design. |
| 18 | Live Cache Telemetry & Hit-Rate Counter HUD | `--sandwich-telemetry` | PARTIAL | 0.70 | 0.30 | 0.25 | 0.15 | 0.00 | Cycle 6 Audit: Reverted commit 4c5880c5c (commit e3bdb655e). Reclassified from RETIRED to FIX (PARTIAL, score 0.70). Active callers (#2, #19, #37, #38) preserved with real atomic counters in sandwich-telemetry. |
| 19 | Frequency-Based Cold-to-Hot Expert Dynamic Promotion | `--moe-cache-profile` | PARTIAL | 0.70 | 0.30 | 0.25 | 0.15 | 0.00 | Cycle 4 Demotion: Online warm-up auto-tuning verified in decode (64.96% hit rate), but prefill telemetry was fabricated. |
| 20 | Hot-Only Speculative Fast Drafting (`--moe-hot-only`) | `--moe-hot-only` | UNVERIFIED | 0.15 | 0.15 | 0.00 | 0.00 | 0.00 | Drafts exclusively against hot VRAM experts. Unverified execution. |
| 21 | Streaming Attention Sink / Sliding-Window KV Compaction | `--sink-tokens` | PARTIAL | 0.55 | 0.30 | 0.00 | 0.15 | 0.10 | Preserves initial attention sink tokens during context shifting. |
| 22 | Native Blackwell FP4 / NVFP4 Tensor Core Acceleration | `--nvfp4` | PARTIAL | 0.50 | 0.30 | 0.00 | 0.10 | 0.10 | NVFP4 GEMM kernel present for sm_120; requires native FP4 model weights. |
| 23 | System-Wide Model Benchmarking Harness & Dynamic Drafter Footprint Sizing | `N/A` | PARTIAL | 0.50 | 0.30 | 0.00 | 0.10 | 0.10 | Automated profiling harness for drafter footprint sizing. |
| 24 | EAGLE3 / P-EAGLE Speculative Tree Decoding (`--spec-tree`) | `--spec-tree` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Tree-based speculative drafting with topological verification. |
| 25 | [P2] 2MB Transparent HugePages (THP) Buffer Alignment | `N/A` | PARTIAL | 0.65 | 0.30 | 0.00 | 0.20 | 0.15 | Forces posix_memalign to 2MB boundaries for HugePage backing. Score 0.65. |
| 26 | VRAM-Pinned Drafter & Spec-Tree Amortization | `-ngld` | PROVEN | 0.90 | 0.30 | 0.25 | 0.20 | 0.15 | Cycle 4 Promotion: router.cpp:256 patched to honor VRAM pinning (-ngld 99) when <= 7,100 MiB. n=5 interleaved ablation verifies +5.10 t/s (+23.2%, t=11.38, p < 0.001) at 6,968 MiB VRAM. |
| 27 | Predictive Hardware-Aware Topology Solver & TurboQuant Auto-Router | `--sandwich-tune` | PARTIAL | 0.65 | 0.30 | 0.00 | 0.20 | 0.15 | Calculates optimal tier partitioning and slot count based on hardware headroom. |
| 28 | [P1] Pipelined Double-Buffered Speculative Execution | `--pipe-spec` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Overlaps draft token generation with target model evaluation. |
| 29 | [P3] Asymmetric Split-KV Engine: Pure VRAM KV Cache on CPU Models | `--split-kv` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Routes KV attention purely through GPU VRAM while model weights reside in host DDR5. |
| 30 | [P4] AVX2-VNNI P-Core Vector Execution for Q2_K & IQ2_XXS | `-mavx2` | PARTIAL | 0.50 | 0.30 | 0.00 | 0.10 | 0.10 | Vectorized dot-product routines targeting Intel Raptor Lake P-cores. |
| 31 | [P1] 3-Tier io_uring Async NVMe Direct Storage | `--io-uring` | PARTIAL | 0.75 | 0.30 | 0.25 | 0.10 | 0.10 | Asynchronous kernel NVMe I/O via io_uring. Demoted to PARTIAL (0.75) due to missing Tier 3 page-fault proof. |
| 32 | Blackwell FP4 / NVFP4 Native Tensor Core Execution | `--dma-ring` | PARTIAL | 0.75 | 0.30 | 0.25 | 0.10 | 0.10 | Cycle 4 Unlock Map: sm_120 hardware detected; pending NVFP4 GEMM kernel. |
| 33 | [P1] Quantized-Native CUDA FlashAttention (-fanq) | `-fanq` | PARTIAL | 0.75 | 0.30 | 0.25 | 0.10 | 0.10 | Patched fattn.cu and fattn-mma-f16.cuh; resolves Defect B1; 0 memcheck/racecheck errors. |
| 34 | GPU Top-K Logits Reduction (`--logits-topk`) | `--logits-topk` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Reduces logits transfer overhead across PCIe bus. |
| 35 | Fused TurboQuant MMA Tensor Core FlashAttention | `-fa` | PARTIAL | 0.75 | 0.30 | 0.25 | 0.10 | 0.10 | B1 patched: ggml_cuda_fattn_kv_type_supported() enables Turbo4 Tensor Core MMA. 0 sanitizer errors. |
| 36 | Asynchronous CPU Split Worker Overlap | `--async-cpu` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Overlaps CPU expert GEMM calculation with GPU forward pass. |
| 37 | 3-Tier Hierarchical MoE Memory System Engine | `--moe-warm-ram-mb` | PARTIAL | 0.75 | 0.30 | 0.25 | 0.10 | 0.10 | Cycle 4 Demotion: 3-tier memory engine functional but dependent telemetry retracted. |
| 38 | 2Q & Ghost-Cache Replacement Policy | `--moe-2q` | PARTIAL | 0.50 | 0.25 | 0.15 | 0.10 | 0.00 | Cycle 4 Demotion: 2Q replacement policy demoted pending real multi-turn trace validation. |
| 39 | Asymmetric Split-KV & CPU Weight Pinning for Dense Models (70% RAM Rule) | `N/A` | PARTIAL | 0.65 | 0.30 | 0.00 | 0.20 | 0.15 | Protects host system memory against swap thrashing. |
| 40 | Needle-In-A-Haystack (NIAH) Semantic Fidelity Verification | `N/A` | PARTIAL | 0.75 | 0.30 | 0.25 | 0.10 | 0.10 | PROVEN for tested 4k/32k envelope; PARTIAL (0.75) against diary 128k context claim. |
| 41 | Radix-Tree Persistent Prefix Caching / RadixAttention | `N/A` | PROVEN | 1.00 | 0.30 | 0.25 | 0.25 | 0.20 | Matched 4/6 prefix tokens; TTFT < 1 ms. Verified in test-radix-cache. |
| 42 | Cross-Platform Unified Hardware Architecture (Metal & ROCm HIP) | `N/A` | PARTIAL | 0.50 | 0.25 | 0.10 | 0.05 | 0.10 | Target machine is NVIDIA Linux; Apple Silicon Metal and AMD ROCm HIP are non-executable. |
| 43 | Zero-Memory Native MTP (Multi-Token Prediction) Speculative Rollout | `--mtp` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Native multi-token prediction heads rollout. |
| 44 | Asymmetric KV Outlier Channelling (K-Polar4 / V-Log2 Extreme Squeeze) | `-ctk q8_0 -ctv turbo2` | RETIRED | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | Cycle 6 Audit: Verified commit edb46c79d touches llama-graph.cpp:3531/3890 (build_attn asymmetric KV #44, not YaRN #58). Formally retired due to reproducible +5.48% (2048 ctx) and +7.43% (512 ctx) PPL degradation (t=8.37). Label corrected from YaRN to Asymmetric KV. Max achievable potential clamped to 0.00. |
| 45 | Lossless Adaptive Sparse Attention (LASA) | `--lasa` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Dynamically prunes uninformative attention heads. |
| 46 | Latent FlashMLA Direct Tensor Core MMA | `--kv-snapshot` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Cycle 4 Unlock Map: Requires MLA architecture (DeepSeek V2/V3/V4), not MoE alone. |
| 47 | Hierarchical Prefetch Stream Arbiter (`--prefetch-arbiter`) | `--prefetch-arbiter` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Arbitrates bandwidth between KV cache and MoE weights. |
| 48 | Turbo-KVarN Closed-Form Dual-Axis Variance Normalization | `--moe-exchange-rot` | PARTIAL | 0.50 | 0.25 | 0.15 | 0.10 | 0.00 | Cycle 5 Label Fix: Formally designated Turbo-KVarN Closed-Form Dual-Axis Variance Normalization. Score 0.50 (t1=0.25, t2a=0.15, t2b=0.10, t3=0.00). |
| 49 | Zero-Copy Pinned Staging Pool (`--dma-staging-pool`) | `--dma-staging-pool` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Dedicated pinned buffer pool for DMA transactions. |
| 50 | KVA Late-Layer Recurrence Prefill | `--kv-defrag` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Cycle 4 Unlock Map: Requires context >= 32k tokens. |
| 51 | Contiguous Single-Allocation Expert Slabs (`--moe-slab-alloc`) | `--moe-slab-alloc` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Eliminates per-expert heap fragmentation. |
| 52 | Static Graph Hot GEMM Substitution (`--cuda-graph-hot-gemm`) | `--cuda-graph-hot-gemm` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Substitutes static GEMM nodes into CUDA execution graph. |
| 53 | Per-Layer Pinned Prefetch Scratchpads (`--prefetch-scratchpad`) | `--prefetch-scratchpad` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Per-layer prefetch staging buffers. |
| 54 | Message-Boundary Delta Caching (`--msg-cache`) | `--msg-cache` | PROVEN | 1.00 | 0.30 | 0.25 | 0.25 | 0.20 | Boundary token delta caching; 3 messages cached; reuse latency < 0.05ms. |
| 55 | Sub-2-bit MoE Expert Dequantization | `--fused-expdec` | PARTIAL | 0.45 | 0.30 | 0.15 | 0.00 | 0.00 | Cycle 4 Reconciliation: Download gain removed (+0.00), requires CUDA shared-memory kernel. |
| 56 | Speculative Batch Cascade Verification (`--spec-cascade`) | `--spec-cascade` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Cascaded speculative token verification. |
| 57 | Asymmetric Quantized Drafter Head Matching (`--drafter-quant`) | `--drafter-quant` | PROVEN | 1.00 | 0.30 | 0.25 | 0.25 | 0.20 | DFlash2 Q4_K_M matched with IQ3_XXS primary model; verification passes without type conversion penalty. |
| 58 | Hardware Profiler Calibration Lock (`--hw-lock`) | `--hw-lock` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Locks hardware profiling calibration to prevent runtime probing overhead. |
| 59 | Dynamic Layer Auto-Offload Tuner (`--auto-offload`) | `--auto-offload` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Adjusts -ngl dynamically based on runtime VRAM allocations. |
| 60 | Unrotated KV Staging (`--unrotated-kv`) | `--unrotated-kv` | PARTIAL | 0.50 | 0.25 | 0.00 | 0.15 | 0.10 | Stages unrotated KV vectors to avoid RoPE rotation unrolling latency. |
| 67 | Peer-to-Peer Distributed Heterogeneous VRAM Pooling (`--peer-nodes`) | `--peer-nodes` | PARTIAL | 0.70 | 0.30 | 0.25 | 0.05 | 0.10 | Simulated local loopback cluster verified in ctest; physical distributed hardware absent. |
| 68 | Asymmetric Heterogeneous Topology Solver | `N/A` | PARTIAL | 0.70 | 0.30 | 0.25 | 0.05 | 0.10 | Solves partition boundaries across cluster. Tested via test-peer-cluster loopback. |
| 69 | Direct Memory Access (DMA) & Zero-Copy Interconnect Transport (`--peer-dma`, `--peer-split-kv`) | `--peer-dma` | PARTIAL | 0.70 | 0.30 | 0.25 | 0.05 | 0.10 | Zero-copy socket/RDMA layer verified under local mock sockets in test-peer-cluster. |
| 70 | Distributed 2-Bit Walsh-Hadamard Polar Network Compression (`--peer-turbo-net`) | `--peer-turbo-net` | PARTIAL | 0.70 | 0.30 | 0.25 | 0.05 | 0.10 | Quantizes network activation payloads via Walsh-Hadamard transform in test-peer-cluster. |
| 71 | Cross-Node Speculative Verification Pipeline (`--peer-spec-pipe`) | `--peer-spec-pipe` | PARTIAL | 0.70 | 0.30 | 0.25 | 0.05 | 0.10 | Pipelined speculative execution across nodes tested in loopback harness. |
| 72 | Dynamic Zero-Crash Failover with Hot Re-partitioning (`--peer-fallback`) | `--peer-fallback` | PARTIAL | 0.70 | 0.30 | 0.25 | 0.05 | 0.10 | Dynamic re-partitioning upon simulated node drop tested in test-peer-cluster. |
| 73 | FlashAttention-3 Forward Compatibility Stubs | `-fa` | PROVEN | 1.00 | 0.30 | 0.25 | 0.25 | 0.20 | Verified forward path and fallback stubs in test-sandwich-dispatch. |
| 74 | Quantized KV Cache SIMD Packing Optimizations | `-ctk` | PROVEN | 1.00 | 0.30 | 0.25 | 0.25 | 0.20 | Turbo4/Turbo2 SIMD packed layouts verified; byte strides aligned to sizeof(half2). |
| 75 | Dedicated Host RAM L2 Caching Layer (`--ram-cache`) | `--ram-cache` | CONFLICT / HAZARD VETO | 0.00 | 0.30 | 0.20 | 0.20 | 0.20 | HAZARD VETO: Clamped strictly to 0.00. Uncoordinated secondary host RAM cache conflicts with Tier 2 WARM pool. |
| 76 | Dynamic PCIe Bandwidth Throttling & Power Management | `--pcie-throttle` | PROVEN | 1.00 | 0.30 | 0.25 | 0.25 | 0.20 | Monitors PCIe bandwidth and scales link power state to prevent thermal throttling. |
| 77 | Heavy Hitter Oracle (H2O) Dynamic KV Cache Eviction (`--h2o`) | `--h2o` | PARTIAL | 0.75 | 0.30 | 0.25 | 0.10 | 0.10 | Dynamic KV eviction retaining top accumulated attention scores. Score 0.75. |
| 78 | Fused Dequantization-GEMV Tensor Core Pipeline | `-fa` | PROVEN | 1.00 | 0.30 | 0.25 | 0.25 | 0.20 | Dequantizes Turbo4 directly into Tensor Core registers during MMA dot-product pipeline. |
| 79 | Sequence-Parallel Flash-Decoding across Distributed Slices | `--flash-decoding` | PROVEN | 0.90 | 0.30 | 0.25 | 0.20 | 0.15 | Cycle 5 Reconciliation: Host scheduler sequence-split reduction over block-level device softmax verified. FP32 single-precision kernel parity verified across 12 configs (error <= 4.80e-07, cosine >= 0.99999998, 0.000 ms overhead). |
| 80 | GPUDirect Storage (`cuFile`) Direct NVMe-to-VRAM DMA Engine (`--gds`) | `--gds` | PARTIAL | 0.75 | 0.30 | 0.25 | 0.10 | 0.10 | Diary claimed 16.73 GB/s; measured rate is 2.51-3.16 GB/s. Demoted to PARTIAL (0.75). |
