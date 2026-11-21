# Sandwich Engine: 3-Tier Asymmetric MoE Memory Architecture

**Document Version:** 1.0  
**Engine:** Sandwich Engine (`sandwich.cpp`)  
**Hardware Profile:** HP Victus 15-fa1xxx | Intel Core i7-13620H (6P+4E) | 24 GB DDR5-5200 | NVIDIA RTX 5050 Mobile 8GB (sm_120 Blackwell) | Arch Linux (Kernel 7.3.5)

---

## 1. Overview & Architecture Motivation

Running large Mixture-of-Experts (MoE) models (e.g. Qwen 35B A3B, DeepSeek 236B, or Qwen Flash Next 125B) on consumer-grade hardware with 8 GB VRAM presents a fundamental memory bandwidth and capacity bottleneck. Monolithic GPU layer offloading fails because each layer contains 64 to 256 distinct expert parameter tensors, of which only 2 to 8 are activated per token.

Sandwich Engine replaces traditional monolithic offloading with an **Asymmetric 3-Tier MoE Memory Hierarchy**:

```
 ┌────────────────────────────────────────────────────────────────────────┐
 │                      TIER 1: HOT VRAM SLAB                             │
 │  • Attention weights & shared model layers (14 layers)                 │
 │  • VRAM-Pinned Speculative Drafter (DFlash2, 100% offload -ngld 99)    │
 │  • Dynamic Expert Cache Slots (16 slots in VRAM)                       │
 │  • Polar Spherical TurboQuant KV Cache (turbo4 Lloyd-Max)              │
 └──────────────────────────────────▲─────────────────────────────────────┘
                                    │ PCIe 4.0 x8 Direct DMA
 ┌──────────────────────────────────▼─────────────────────────────────────┐
 │                     TIER 2: WARM DDR5 HOST POOL                        │
 │  • Transparent HugePages (THP) memory mapping (16.5 GB pool)           │
 │  • Two-Queue (2Q) Probation & Ghost Cache eviction policy              │
 │  • Zero-Copy Rotational Buffer Pointer Exchange (--moe-exchange-rot)   │
 └──────────────────────────────────▲─────────────────────────────────────┘
                                    │ Linux kernel io_uring Async DMA
 ┌──────────────────────────────────▼─────────────────────────────────────┐
 │                     TIER 3: COLD NVMe STORAGE                          │
 │  • Direct asynchronous disk streaming via Linux io_uring (2.15+)       │
 │  • Lookahead expert routing prefetch via router token predictions     │
 └────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Memory Tiers Breakdown

### 2.1 Tier 1: Hot VRAM Slab (GPU Accelerator)
* **Resident Layers:** Shared attention matrices ($W_q, W_k, W_v, W_o$) and dense base layers (layers 0–13) residing permanently in RTX 5050 GDDR6 VRAM.
* **Speculative Drafter Pinning:** A companion lightweight draft model (such as `Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf`) is pinned 100% in GPU VRAM (`-ngld 99`), generating speculative tokens at 180+ tok/s with zero host-to-device PCIe latency.
* **TurboQuant Lloyd-Max KV Compression:** Activates polar 4-bit spherical quantization for KV tensors (`-ctk turbo4 -ctv turbo4`), reducing context memory footprint by 75% without perplexity degradation.

### 2.2 Tier 2: Warm DDR5 Host Pool (System RAM)
* **Transparent HugePages (THP):** Pre-faulted 2 MB HugePages eliminate TLB page table walk overhead across the 24 GB single-channel DDR5 memory bus.
* **Two-Queue (2Q) Cache Replacement:** Distinguishes frequently activated expert tensors ("hot" recurring experts) from one-off exploratory activations using separate FIFO and LRU queues.
* **Rotational Pointer Swapping:** Swaps active memory pointers rather than copying large weight tensors during expert eviction cycles, avoiding memory bus saturations.

### 2.3 Tier 3: Cold NVMe Storage (`io_uring`)
* **Kernel Asynchronous DMA:** Utilizes Linux `io_uring` system calls with submission/completion queue pairs, bypassing standard POSIX synchronous `pread()` syscall overhead.
* **Asymmetric Parallel Prefetch:** While the GPU evaluates Layer $N$ attention, the host scheduler pre-fetches top-$k$ expert weights for Layer $N+1$ from NVMe storage in parallel.

---

## 3. Hardware Budgeting Allocation (HP Victus 15 Reference)

| Subsystem | Capacity | Allocation Purpose | Active Engine Flags |
| :--- | :--- | :--- | :--- |
| **GPU VRAM** | 8,151 MiB | • Base Attention Layers: 4,200 MiB<br/>• DFlash2 Drafter (100%): 1,120 MiB<br/>• TurboQuant 4K KV Cache: 780 MiB<br/>• Dynamic Expert Slots: 650 MiB<br/>• CUDA / Driver Headroom: 1,401 MiB | `-ngl 14`<br/>`-ngld 99`<br/>`-c 4096`<br/>`-ctk turbo4 -ctv turbo4`<br/>`--moe-cache-slots 16` |
| **System DDR5** | 24,192 MiB | • MoE Warm Expert Pool: 16,552 MiB<br/>• OS / Desktop Working Set: 4,500 MiB<br/>• Process RSS Headroom: 3,140 MiB | `--moe-warm-ram-mb 16552`<br/>`--moe-2q`<br/>`--moe-exchange-rot` |
| **NVMe SSD** | PCIe Gen4 | • Cold expert storage streaming | `--io-uring`<br/>`--asym-prefetch` |

---

## 4. Synthesis & Invocation Reference

To launch inference utilizing the complete 3-Tier MoE hierarchy:

```bash
# Auto-Dispatch Mode (recommended, auto-tuned via sandwich_auto_dispatch):
./build/bin/sandwich -m /path/to/moe-model.gguf --sandwich-auto

# Manual Power-User Configuration:
./build/bin/llama-cli \
  -m /home/sandeep/ai-models/moe/Qwen3.6-35B-A3B-MTP-GGUF/Qwen3.6-35B-A3B-UD-IQ3_XXS.gguf \
  -md /home/sandeep/ai-models/drafters/Qwen3.6-35B-A3B-DFlash2-GGUF/Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf \
  -ngl 14 \
  -ngld 99 \
  -t 6 \
  -c 4096 \
  --moe-cache-slots 16 \
  --moe-warm-ram-mb 16552 \
  -ctk turbo4 \
  -ctv turbo4 \
  --io-uring \
  --moe-2q \
  --moe-exchange-rot \
  --asym-prefetch
```
