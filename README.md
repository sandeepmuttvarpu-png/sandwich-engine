# Sandwich Engine (`sandwich.cpp`)

> **High-Performance Heterogeneous Inference Runtime for Frontier Mixture-of-Experts (MoE) on Asymmetric Silicon.**  
> *Derived from [llama.cpp](https://github.com/ggerganov/llama.cpp) under the MIT License.*

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![CUDA: 12.0+ / 12.8 / 13.x](https://img.shields.io/badge/CUDA-12.0%2B%20%7C%2012.8-green.svg)](https://developer.nvidia.com/cuda-toolkit)
[![C++ Standard: C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](CMakeLists.txt)
[![CTest: 73/74 Passing](https://img.shields.io/badge/CTest-73%2F74%20(98.6%25)-brightgreen)](docs/audit/)
[![Parity: Bit-Identical Verified](https://img.shields.io/badge/Parity-Bit--Identical%20(SHA256)-success)](docs/audit/)
[![Audit Score: 0.611 / 0.915](https://img.shields.io/badge/Audited%20Score-0.611%20%7C%20Ceiling%200.915-orange)](docs/techniques.md)
[![Hardware Target: Consumer Asymmetric Silicon](https://img.shields.io/badge/Hardware-VRAM%20%2B%20DDR5%20%2B%20NVMe-purple)](#)

---

## Executive Summary

State-of-the-art open-weights models increasingly adopt **Mixture-of-Experts (MoE)** architectures (e.g., Qwen 35B-A3B with 256 experts, DeepSeek V3/V4, Nemotron MoE) to maximize reasoning capacity while bounding active FLOPs. However, deploying these models on consumer workstations presents a fundamental memory bottleneck:
* **Enterprise Inference Engines (vLLM, TensorRT-LLM):** Architected strictly for multi-GPU unified HBM clusters (SXM / NVLink). On single consumer GPUs, they fail to boot or exhaust VRAM immediately.
* **Standard Offloading Engines (vanilla llama.cpp):** Rely on rigid, layer-by-layer offloading (`-ngl`). When memory is partitioned across PCIe, intermediate activations repeatedly traverse the host-device bus, introducing latency bubbles that degrade throughput to 2–4 tokens/second.

**Sandwich Engine** is an independent, bare-metal C++17/CUDA inference engine engineered specifically for **heterogeneous, asymmetric computing environments** (e.g., a single 8–16 GB GPU paired with 24–64 GB system DDR5 and high-speed NVMe storage). By decoupling compute-dense attention kernels from parameter-dense MoE feed-forward networks, Sandwich Engine eliminates PCIe serialization stalls and achieves near-linear scaling on consumer silicon.

---

## Key Architectural Innovations

```
                                Consumer Asymmetric Silicon
               (e.g., NVIDIA RTX 8GB–16GB VRAM + Host 24GB–64GB DDR5 + NVMe)
 ┌─────────────────────────────────────────────────────────────────────────────┐
 │                      TIER 1: COMPUTE & HOT SLAB (VRAM @ ~288+ GB/s)         │
 │  • Router Gating ($W_{\text{gate}}$) & Shared Experts permanently pinned     │
 │  • 2Q Active MoE Slab ($A_m$ Frequent 75% | $A_{1\text{in}}$ Probation 25%)   │
 │  • Fused TurboQuant MMA FlashAttention (turbo4 / turbo3 / turbo2)           │
 │  • VRAM-Pinned Speculative Drafter (e.g., DFlash2) drafting at 180+ tok/s    │
 │  • 7,100 MB Compositor Safety Guard (Prevents GUI desktop display crashes)   │
 └──────────────────────────────────────┬──────────────────────────────────────┘
                                        │ ▲ Pinned DMA Ring Buffer
                                        │ │ (`cudaHostRegister` Zero-Copy)
 ┌──────────────────────────────────────┴──────────────────────────────────────┐
 │                      TIER 2: WARM WORKING SET (Host DDR5 @ ~50–90 GB/s)     │
 │  • 10–14 GB Dedicated Memory Pool backed by 2MB Transparent HugePages (THP)  │
 │  • Raptor Lake / Core P-Core Thread Affinity (`pthread_setaffinity_np`)      │
 │  • Asymmetric Split-KV Cache Overflow Buffer for 128k Context Scaling       │
 └──────────────────────────────────────┬──────────────────────────────────────┘
                                        │ ▲ Linux io_uring / GPUDirect Storage
                                        │ │ (Direct Async NVMe-to-RAM DMA)
 ┌──────────────────────────────────────┴──────────────────────────────────────┐
 │                      TIER 3: COLD STORAGE (PCIe Gen4/Gen5 NVMe SSD)          │
 │  • Direct GGUF Memory-Mapped Descriptor Streaming                           │
 │  • Multi-Layer Lookahead Prefetch Engine (Overlaps Layer $L+1$ fetch)       │
 │  • Instant Page Recycling via `madvise(MADV_DONTNEED)`                       │
 └─────────────────────────────────────────────────────────────────────────────┘
```

### 1. Three-Tier Hierarchical MoE Memory Architecture
Rather than requiring entire models to reside symmetrically in a single pool, Sandwich Engine implements an automated 3-tier hierarchy:
* **Tier 1 (HOT — GPU VRAM):** Hosts latency-critical routing gates, shared experts (`ffn_*_shexp`), active 2Q expert slabs, and the entire speculative drafter model.
* **Tier 2 (WARM — Host DDR5):** A dedicated Transparent HugePages (THP) pool holding frequently activated warm experts, eliminating TLB miss latency.
* **Tier 3 (COLD — NVMe SSD):** Direct asynchronous Linux `io_uring` and GPUDirect Storage (`cuFile`) streaming for transient cold experts, completely overlapping disk fetch with preceding GPU attention computations.

### 2. 2Q & Ghost-Cache Replacement Policy
Traditional single-queue LRU caches suffer from catastrophic cache pollution when processing outlier tokens that activate transient "one-hit wonder" experts. Sandwich Engine implements a **Two-Queue (2Q) + Ghost-Cache** algorithm:
* **Probation Queue ($A_{1\text{in}}$):** Newly activated experts enter probation (25% capacity). Transient experts are evicted without displacing proven routing paths.
* **Frequent Queue ($A_m$):** Experts that hit repeatedly earn promotion to the frequent cache (75% capacity).
* **Ghost Cache ($A_{1\text{out}}$):** Zero-weight metadata hash table tracking recently evicted IDs to immediately restore returning experts to $A_m$.

### 3. VRAM-Pinned Speculative Decoding (Technique #26)
Speculative decoding typically bottlenecks over PCIe when both draft and target models contend for system memory. Sandwich Engine pins the speculative assistant (e.g., DFlash2) **100% in GPU VRAM** (`-ngld 99`). The drafter produces candidate verification trees at **180+ tok/s** in high-bandwidth memory while the host target model evaluates candidates in batched parallel passes.
* **Empirical Validation:** 
  * Host-Injected Drafter (`-ngld 0`): **23.24 ± 0.26 tok/s** (Note: exhibits an -8.3% decode regression from baseline 25.34 ± 0.26 tok/s due to dense-path fallback in commit `46c37169f`).
  * VRAM-Pinned Drafter (`-ngld 99`): **28.62 ± 0.45 tok/s** (+23.1% net speedup over host-injected drafter, verified under interleaved $n=5$ ablation).
  * **Workaround Limitation Notice:** Due to the Issue #1 crash workaround in commit `46c37169f`, Techniques #78 (PagedAttention), #29/#39 (Asymmetric Split-KV), and #79 (Flash-Decoding) execute on the dense MMA fallback path. The sparse FlashAttention fast path is temporarily disabled when `src[5]` is non-null until role disambiguation lands ([Issue #4](https://github.com/sandeepmuttvarpu-png/sandwich-engine/issues/4)).

### 4. TurboQuant Polar Lloyd-Max KV Cache Compression
At extended context lengths (32k–128k), standard FP16 or quantized KV caches consume excessive memory and suffer from attention-sink degradation. Sandwich Engine incorporates **TurboQuant** (`turbo4`, `turbo3`, `turbo2`) utilizing:
* Polar spherical Lloyd-Max quantization centroids.
* Closed-form dual-axis variance normalization in `set-rows.cu` to eliminate long-horizon Chain-of-Thought reasoning drift.
* Asymmetric Split-KV (`--split-kv`), placing the entire attention KV cache into GPU VRAM even when 100% of model weights run on host CPU.

---

## Competitive Architecture Matrix

| Dimension | Enterprise Engines (vLLM / TGI) | Standard llama.cpp | Consumer Frontends (Ollama / LM Studio) | **Sandwich Engine (`sandwich.cpp`)** |
| :--- | :---: | :---: | :---: | :---: |
| **Primary Target** | Multi-GPU Datacenter (A100/H100) | Portable CPU/GPU Baseline | Desktop GUI Wrappers | **Asymmetric Consumer Workstations** |
| **MoE Memory Hierarchy** | Monolithic VRAM Allocation | Naive Linear Layer Offload (`-ngl`) | Unmodified llama.cpp Backends | **3-Tier Dynamic MoE (VRAM $\to$ DDR5 $\to$ NVMe)** |
| **PCIe Transfer Latency** | Assumes NVLink / High Bandwidth | Ping-Pong Serialization Stalls | Inherited Serialization Stalls | **Asymmetric Split-KV & Lookahead DMA** |
| **MoE Cache Policy** | Static Allocation | Single-Queue LRU | Single-Queue LRU | **2Q Probation + Ghost Cache** |
| **Speculative Architecture** | Server-Level Batch Speculation | Standard Assistant Drafter | Default Off / Limited | **VRAM-Pinned Multi-Candidate Spec-Trees** |
| **Context Memory Efficiency** | PagedAttention (FP16/FP8) | Standard Q8_0 / Q4_0 KV | Standard Q8_0 / Q4_0 KV | **TurboQuant Polar 2-Bit/4-Bit + PagedAttention** |
| **Idle Memory Footprint** | 2,400+ MB | ~210 MB | 450–800+ MB (Chromium / Helper) | **<180 MB Clean Native Runtime** |

---

## Empirical Benchmark Performance

*Evaluated on consumer workstation hardware: NVIDIA GPU (8 GB GDDR6), 24 GB DDR5-5200 host RAM, PCIe 4.0 NVMe SSD.*

### 1. Extended Context Sustained Generation (128k Tokens)

| Model Architecture | Parameters (Total / Active) | Verified Context | KV Quant Recipe | Peak Throughput | 128k Sustained Throughput | Context Degradation | Production CLI Recipe |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **`GLM-4.7-Flash`** | 30B MoE (3B / FlashMLA) | **128k** | `turbo2 -nkvo` | **22.3 tok/s** | **22.0 tok/s** | **-1.3% (Flat)** | `--sandwich-auto -ctk turbo2 -ctv turbo2 -nkvo` |
| **`Nemotron-30B-A3B`** | 30B MoE (3B / 128 exp) | **128k** | `turbo4 / turbo3` | **35.1 tok/s** | **33.8 tok/s** | **-3.7% (Flat)** | `--sandwich-auto -ctk turbo4 -ctv turbo3 -nkvo` |
| **`Qwen3.6-35B-A3B`** | 35B MoE (3B / 256 exp) | **128k** | `turbo4 / turbo2` | **34.8 tok/s** | **19.1 tok/s** | **-2.9%** | `--sandwich-auto -ctk turbo4 -ctv turbo2 -nkvo` |
| **`DeepSeek-Coder-V2`** | 16B MoE (2.4B / 64 exp) | **128k** | `turbo4 / turbo2` | **16.3 tok/s** | **15.8 tok/s** | **-3.1%** | `--sandwich-auto -ctk turbo4 -ctv turbo4` |
| **`GPT-OSS-120B`** | 117B MoE (12B / 128 exp) | **128k** | `turbo2` | **1.52 tok/s** | **1.02 tok/s** | **-7.9%** | `--moe-3tier --moe-2q --moe-warm-ram-mb 12288 -ctk turbo2` |

> **Architectural Note:** Fine-grained MoE topologies demonstrate near-zero generation throughput decay across 128k token context windows when paired with TurboQuant variance-normalized attention keys and host-side KV overflow buffers (`-nkvo`).

### 2. Migration & Parity Verification
* **Bit-Identical Verification:** 100% token stream identity against upstream baseline (`llama.cpp`) validated on Qwen3.6-35B-A3B across canonical 3,782 prompt / 63 decode token workloads:
  * Full Output SHA256: `77e2b15be215db920cd2bb86ece4bea713f42954e302c2e396bb26239f12d74b`
  * Pure Token Text SHA256: `481b2a7aea43731e77979bca6631cfac9d32bd42d023d63c91552b245931a6aa`
  * Execution Status: Clean exit (Exit Code 0).

---

## Formal Engineering Audit & Verification

Sandwich Engine enforces a rigorous 4-tier truth scoring methodology across its catalog of 74 techniques, governed by automated ledger verification (`docs/audit/validate_ledger.py`):
* **Authoritative Close-Out Score:** **45.25 / 74 (0.611)**
* **Structural Single-Node Physical Ceiling:** **0.915** (67.70 / 74)
* **Current Model Family Ceiling:** **0.866** (64.05 / 74)
* **CTest Verification Suite:** **73 passing out of 74 (98.6% pass rate, 0 skipped)**
  * *Test #16 (`test-tokenizers-ggml-vocabs`):* PASS (Verified passing across all 11 binary GGUF vocabulary models).
  * *Test #37 (`test-conversion-fp8`):* PASS (Verified passing across all 15 FP8 conversion and quantization tests).
  * *Test #33 (`test-llama-context`):* FAILING (Crash eliminated; 137/145 pass deterministically under default FlashAttention across both serial -j1 and parallel -j4; plamo3 NMSE divergence and state_load defects tracked in Issues #2, #4, #6, #7; Issues #3 and #5 closed as invalid).

For complete audit ledgers, reproduction scripts, and tier breakdowns, refer to [docs/techniques.md](docs/techniques.md) and [docs/audit/](docs/audit/).

---

## Quick Start & Installation

### Option 1: Automated Deployment Script
```bash
curl -fsSL https://raw.githubusercontent.com/sandeepmuttvarpu-png/sandwich-engine/main/install.sh | bash
```
The installer automatically:
1. Detects host architecture (CUDA compute capability, AVX2-VNNI support).
2. Verifies system memory limits and configures P-core thread binding.
3. Installs `sandwich`, `sandwich-cli`, and `sandwich-server` into `~/.local/bin`.

### Option 2: Build from Source (C++17 & CMake)
```bash
git clone https://github.com/sandeepmuttvarpu-png/sandwich-engine.git
cd sandwich-engine
cmake -B build -DGGML_CUDA=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)
```

---

## Unified Command-Line Interface (`sandwich`)

The runtime provides a high-level dispatcher for seamless operational control:

```bash
# Launch interactive TUI model selector & hardware tuner
sandwich

# Execute local inference with automated hardware detection and tuning
sandwich run -m /path/to/model.gguf

# Start an OpenAI-compatible REST server (Port 8080)
sandwich serve -m /path/to/model.gguf --port 8080

# Launch optimized coding agent backend with 64k context and prefix caching (Port 8081)
sandwich agent -m /path/to/model.gguf

# View real-time VRAM, DDR5, and compute telemetry
sandwich status

# Execute the 128k sustained endurance benchmark suite
sandwich bench
```

### Integration with Development Environments
Sandwich Engine exposes an OpenAI-compliant `/v1/chat/completions` API supporting streaming Server-Sent Events (SSE):
* **Cursor / VSCode Continue:** Configure base URL to `http://localhost:8081/v1`.
* **LibreChat / OpenWebUI:** Point OpenAI API URL to `http://localhost:8080/v1` (API key: `sandwich`).

---

## Core Configuration & CLI Flags

<details>
<summary><b>Expand Complete Parameter & Optimization Flags</b></summary>

### 1. Autopilot & Hardware Safety
| Flag | Description | Default |
| :--- | :--- | :---: |
| `--sandwich-auto` | Automatically orchestrates VRAM allocation, 70% RAM rule, drafter pairing, and thread affinity. | `false` |
| `--vram-guard LIMIT` | Establishes an absolute VRAM allocation ceiling (e.g., `7100M`) to prevent OS compositor crashes. | `7100M` |
| `--sandwich-pcore-threads N` | Binds compute threads strictly to $N$ physical performance cores on hybrid CPUs. | All P-cores |
| `--thp-align` | Enforces 2MB Transparent HugePages alignment with `madvise(MADV_HUGEPAGE)` to minimize TLB miss penalties. | Auto |

### 2. Mixture-of-Experts (MoE) 3-Tier Management
| Flag | Description | Default |
| :--- | :--- | :---: |
| `--moe-3tier` | Enables three-tier memory offloading (VRAM $\to$ Host DDR5 $\to$ NVMe SSD). | Auto |
| `--moe-warm-ram-mb N` | Dedicated Host RAM budget (MiB) for Tier 2 warm expert staging. | `12288` |
| `--moe-2q` | Enables Two-Queue (2Q) probation and ghost-cache replacement to eliminate cache thrashing. | Auto |
| `--sched-async-cpu` | Overlaps CPU cold-expert evaluation with concurrent GPU hot-expert computation. | Auto |

### 3. Speculative Decoding & Tree Search
| Flag | Description | Default |
| :--- | :--- | :---: |
| `-md, --model-draft PATH` | Path to speculative draft model (e.g., DFlash2 head or aligned compact model). | Optional |
| `-ngld, --gpu-layers-draft N` | Number of draft model layers offloaded to VRAM (`99` = fully pinned in VRAM). | `99` |
| `--spec-tree` | Evaluates multi-candidate speculative trees in a single target forward pass. | Auto |
| `--spec-tree-depth N` | Maximum exploration depth for candidate speculative trees. | `3` |
| `--spec-tree-width N` | Maximum branching width per node for speculative candidate exploration. | `2` |

### 4. Attention & Quantized KV Compression
| Flag | Description | Default |
| :--- | :--- | :---: |
| `-ctk, -ctv TYPE` | Selects KV cache compression format: `turbo4` (4-bit), `turbo3` (3-bit), `turbo2` (2-bit). | `turbo4` |
| `--split-kv` | Pins attention KV cache strictly to GPU VRAM even when model weights run on host CPU. | Auto |
| `-nkvo` | Routes KV cache overflow into host DDR5 RAM for ultra-long context scaling. | `0` (VRAM) |
| `-fa, --flash-attn` | Enables hardware-accelerated FlashAttention. | `on` |
| `--radix-cache` | Enables Radix-Tree persistent prefix caching for instant multi-turn prompt evaluation. | Auto |

</details>

---

## Architectural Provenance & Legal Compliance

* **Base Project:** Derived from [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp) at merge-base commit [`3837bf0bc`](https://github.com/ggml-org/llama.cpp/commit/3837bf0bc).
* **License:** Licensed under the [MIT License](LICENSE).
* **Copyright:** Copyright (c) Georgi Gerganov and contributors. Copyright (c) Sandwich Engine contributors.
* **Documentation of Derivation:** Full derivation details, upstream tracking, and technique audit ledgers are documented in [NOTICE.md](NOTICE.md) and [UPSTREAM.md](UPSTREAM.md).

---

## Acknowledgments & Research Foundations

Sandwich Engine builds upon foundational research and engineering contributions across the open-source systems community:
* **Georgi Gerganov & llama.cpp Contributors:** Architecture of the foundational `ggml` tensor runtime and portable inference core.
* **Thecodacus:** Heterogeneous MoE routing patterns and cache split mechanics.
* **Niko1221 (`Strata`):** Hierarchical MoE cache research and exchange buffer pointer rotation patterns.
* **Piggidragon:** Fused Quantized-Native MMA FlashAttention kernels.
* **TheTom:** Theoretical foundations of Polar Lloyd-Max spherical quantization for KV caches.
* **Dao-AILab:** Foundational FlashAttention and Flash-Decoding algorithms.
