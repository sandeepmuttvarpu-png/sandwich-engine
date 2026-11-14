# Notice

This project, **Sandwich.cpp** (`sandwich.cpp`), is derived from [llama.cpp](https://github.com/ggerganov/llama.cpp), created by Georgi Gerganov and the ggml authors.

## Upstream Base
- **Upstream Repository:** https://github.com/ggerganov/llama.cpp
- **Merge-Base Commit:** `3837bf0bc02ea5d08f8ee3585fc8c7784a81bce8` (2026-11-10)
- **Upstream License:** MIT License (see `LICENSE`)

## Sandwich.cpp Enhancements
Sandwich.cpp extends llama.cpp with specialized bare-metal MoE memory tiering, compressed KV caches, and low-latency speculative decoding designed for constrained consumer GPUs:
1. **Dynamic MoE Caching & Memory Tiering:** O(1) LRU VRAM slab caching, 3-tier hierarchical engine (HOT VRAM -> WARM DDR5 -> COLD NVMe via kernel `io_uring`), 2Q ghost-cache replacement, and lookahead asynchronous prefetching.
2. **TurboQuant KV Cache:** Fused 4-bit / 3-bit / 2-bit vector quantization with Variance-Adaptive Residual Normalization (VARN).
3. **Speculative Decoding Engines:** Multi-Token Prediction (MTP) sidecar support, EAGLE3 tree speculative decoding, DFlash2 integration, and zero-overhead N-gram lookup.
4. **Attention Optimizations:** Native FlashMLA latent attention kernels and sequence-parallel Flash-Decoding across GPU streaming multiprocessors.
5. **System & Dispatch Infrastructure:** Architecture-aware auto-dispatcher (`--sandwich-auto`), Intel hybrid P-core CPU affinity pinning, and dynamic VRAM safety guardrails (`--vram-guard`).

All original upstream copyright notices and licenses remain in effect and are preserved in all relevant files.
