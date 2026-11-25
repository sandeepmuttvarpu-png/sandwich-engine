# Changelog

All notable changes to Sandwich Engine will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased] - UI Overhaul & Architecture Tooling

### Added
- **Sequential Batching via $nrc > 1$ (IQ3_S / IQ2_S Grid-Unpack Amortization):**
  - Unlocked $nrc \in \{2, 4, 8\}$ in fused `ggml_vec_dot_iq3_s_q8_K` and `ggml_vec_dot_iq2_s_q8_K` AVX2 kernels by hoisting 512-entry grid lookups (`iq3s_grid` / `iq2s_grid`), sign-mask extraction, and scale broadcasts out of the per-column loop.
  - Demonstrated up to $3.90\times$ kernel-level speedup on IQ3_S ($2.40$ cyc/32 vs $9.36$ cyc/32, $\Delta > 2\sigma$) and $2.89\times$ on IQ2_S ($1.75$ cyc/32 vs $5.07$ cyc/32, $\Delta > 2\sigma$).
  - Restructured `ggml_compute_forward_mul_mat_id_one_chunk` to group tokens routed to the same expert into sequential batches ($nrc \in \{8, 4, 2, 1\}$) with expert-major order within chunks (`nchunk0 = nth, nchunk1 = 1`).
  - Added NRC sequential batching telemetry instrumentation reporting runtime distribution across prompt prefill and token decode phases.
  - End-to-end verified on canonical C4 stack (`Qwen3.6-35B-A3B` + `DFlash2` with `-ngld 99`):
    * Prefill throughput increased to $484.35 \pm 5.11$ t/s ($+23.08$ t/s, $+5.0\%$, $\Delta > 2\sigma$).
    * Peak generation decode reached $31.73 \pm 1.12$ t/s ($+7.46$ t/s over baseline, $\Delta > 2\sigma$).
    * Pure-token SHA256 verified 100% bit-exact: `481b2a7aea43731e77979bca6631cfac9d32bd42d023d63c91552b245931a6aa`.
    * Clean zero-abort execution verified under `GGML_SCHED_DEBUG_REALLOC=1`.
    * Peak VRAM ($7,056$ MiB) and peak VmHWM ($13,936$ MiB) strictly preserved within platform limits.
- **Recommended Default Configuration (`-ngld 99`):**
  - Documented `-ngld 99` (VRAM-pinned speculative drafter) as the recommended default production configuration for 8 GB consumer GPUs.
  - Formally certified empirical headroom and decode throughput in `README.md` and `docs/audit/CONFIDENCE.md`: 30.73 ± 0.21 tok/s decode (+6.23 tok/s over host-injected drafter `-ngld 0`, $\Delta > 2\sigma$), peak VRAM 6,576 MiB, and 524 MiB safety headroom below the 7,100 MiB OS compositor safety ceiling.
- **VNNI IQ3_S Plugin Kernel Consumer:**
  - Implemented hardware VNNI `__dp4a` (`IDP.4A.S8.S8`) accelerated dot-product kernel in `plugins/iq3_s_vnni.cu`.
  - Compiled multi-architecture fatbin artifact `plugins/iq3_s_vnni.cubin` supporting Turing (`sm_75`) through Blackwell (`sm_120`).
  - Integrated dynamic runtime loader and execution dispatch in `src/dispatch/plugin_iq3_s.{hpp,cpp}` with weak linkage into `ggml-cpu/arch/x86/quants.c`.
  - Added unit test and micro-benchmarks in `tests/test-sandwich-plugin.cpp` proving exact bit-level parity with CPU AVX2 baseline.
- **Autotuner CLI Integration (`--autotune`):**
  - Wired `--autotune`, `--autotune-disable`, and `--autotune-invalidate` into `router.cpp` and `sandwich_auto_dispatch()`.
  - Benchmarks FlashAttention variants across representative shapes with trimmed mean and standard error gating, caching winners to `~/.cache/sandwich/kernel_cache.txt`.
- **Autotuning & Persistent Kernel Cache:**
  - Added a header-only, dependency-free kernel variant cache in `ggml/src/ggml-cuda/sandwich-kernel-cache.cuh`, keyed by an FNV-1a fingerprint of device compute capability, head dimension, query count, GQA ratio, K/V dtypes, power-of-two-bucketed KV length, and mask/bias presence.
  - `ggml_cuda_get_best_fattn_kernel` now consults the cache as an *override* of the existing heuristic. The heuristic remains authoritative on a cache miss, when autotuning is disabled, or when the cached variant is invalid for the current tensor, so a cold or corrupt cache can never make the engine slower than its compiled-in default.
  - Added environment controls: `SANDWICH_KERNEL_CACHE` (cache path override), `SANDWICH_AUTOTUNE_DISABLE`, and `SANDWICH_AUTOTUNE_INVALIDATE`.
  - Persistence uses an atomic temp-file-plus-rename write with a schema guard; malformed records are skipped rather than treated as fatal.
  - Added unit test `tests/test-sandwich-autotune.cpp` covering power-of-two bucketing, fingerprint stability and sensitivity, store/lookup/persistence round-trips, the disable switch, and malformed/schema-guarded input.
- **Plugin Kernels via the CUDA Driver API:**
  - Added `src/dispatch/plugin_loader.{hpp,cpp}`, which loads `.cubin` or PTX kernel images at runtime with `cuModuleLoad` / `cuModuleLoadData`, resolves symbols with `cuModuleGetFunction`, and launches with `cuLaunchKernel`. Plugins are addressed by name and launched through a plain argument array, so there is no C++ ABI coupling and externally built kernels can be imported without a source merge.
  - `libcuda` is opened with `dlopen` rather than linked, so the engine still builds and runs on CPU-only or non-NVIDIA hosts; plugin use degrades to a `driver_unavailable` status with a diagnostic message instead of failing at link time.
  - Added a name-to-module `Registry` for runtime lookup, with replacement-on-duplicate semantics and borrowed-pointer access.
  - Added unit test `tests/test-sandwich-plugin.cpp`, which verifies the driverless degradation path, PTX JIT load, symbol resolution success and failure, kernel launch and synchronisation, rejection of malformed images, and registry lifecycle.
- **Rebuilt Web GUI (Port 8080):**
  - Rebuilt modern web GUI from scratch in `public/` matching the Strata dark-slate and teal/emerald design aesthetic.
  - Features real-time telemetry HUD displaying GPU VRAM, Host RAM RSS, decode/prefill speed (tokens/sec), latency (ms), and MoE cache hit-rate.
  - Implemented token-by-token streaming chat interface with markdown rendering, syntax-highlighted code blocks, and copy buttons.
  - Integrated dual-mode dispatch settings panel supporting Auto-Dispatch (`sandwich_auto_dispatch`) and Manual Mode (custom `-ngl`, `-t`, `-c`, `-md`, `-ngld`, `--moe-cache-slots`, `--moe-warm-ram-mb`, `-ctk`/`-ctv`, `--io-uring`).
- **Developer & User Documentation:**
  - Added Strata TUI Backend C++ API reference in `docs/api/strata_backend_api.md`.
  - Added power-user CLI cheatsheet and flag reference in `docs/guides/cli_cheatsheet.md`.
  - Authored comprehensive 3-Tier MoE Memory Architecture specification in `docs/architecture/moe_3tier_memory.md`.
  - Added comprehensive user guide and keymap documentation in `docs/tui_user_guide.md`.
- **Hardware & Validation Tooling:**
  - Added automated system diagnostic tool `scripts/check_hardware.py` auditing CPU P/E topology, GPU VRAM capacity, and Transparent HugePages status.
  - Added prompt template dry-run validator in `scripts/test_prompts.sh`.
  - Added `scripts/bench_summary.py` CLI tool for computing warm-4 vs. all-5 throughput, cold start delta, sample standard deviation σ, and 2σ significance tests.
  - Added automated environment checker and launcher script `scripts/launch_tui.sh`.
  - Added dedicated one-line launcher script `scripts/launch_web.sh`.
- **Server Web UI Auto-Mount:**
  - Enhanced `llama-server` HTTP listener to automatically search and mount pre-compiled SvelteKit assets (`build/tools/ui/dist`) when `--path` is omitted.
- **Responsive Strata TUI Terminal Handling:**
  - Added terminal size geometry checks (`cols < 70 || rows < 20`) with warning banners in `src/tui/strata_tui.cpp` to prevent text truncation or ANSI escape wrapping on small terminal windows.
- **Strata Terminal User Interface (TUI):**
  - Integrated MIT-licensed Strata TUI modernized for Sandwich Engine (`sandwich-tui` and `sandwich --tui`).
  - Added ANSI 24-bit truecolor styling, raw POSIX termios non-blocking keyboard controls, model selector, and dual-mode inference screens.
  - Implemented real-time hardware telemetry HUD polling NVIDIA VRAM via `cudaMemGetInfo()`, host RSS via `/proc/self/status`, decode/prefill tokens/sec, and MoE cache hit-rate counters.
- **Auto-Dispatch & Manual Inference Backend:**
  - Integrated `sandwich_auto_dispatch()` under the hood for automatic hardware tuning (Intel hybrid P-core pinning, 3-tier MoE memory hierarchy, TurboQuant Lloyd-Max KV cache compression, and VRAM-pinned speculative assistant).
  - Implemented Manual Configuration mode for power users, allowing granular customization of `-ngl`, `-t`, `-c`, `-md`, `-ngld`, KV quantization, and `--io-uring`.
- **Test Suite Extensions:**
  - Added unit test suite `test_strata_backend()` in `tests/test-sandwich-dispatch.cpp` verifying backend telemetry, model discovery, whitespace path quoting, and parameter synthesis.
  - Added configuration invariant defaults validation and boundary tests.
  - Added `test_dense_auto_dispatch_regressions()` in `tests/test-sandwich-dispatch.cpp` guarding dense full-fit (`-1 -> 99`), `<= 30%` partial offload retention, `jit_down_quant == false` preservation, and explicit `--spec-type none` handling.

### Fixed
- **Dense Auto-Dispatch Silent `Q2_K` Weight Down-Quantization (`--sandwich-auto` Correctness):**
  - Removed forced `params.jit_down_quant = true` in both dense branches of `sandwich_auto_dispatch()` (`common/common.cpp`), which previously re-quantized CPU-resident weights on load to 2-bit `Q2_K` (`src/llama-model-loader.cpp:1527-1549`) without warning or `--jit-down-quant`, silently degrading dense model generation into repetition loops (`-\n\n-` on Dolphin-2.8-Mistral-7B, `The same same...` on Gemma-4-31B-it QAT).
- **Dense Auto-Dispatch `-1` Offload Sentinel & `--spec-type none` Guard:**
  - Fixed `sandwich_auto_dispatch()` (`common/common.cpp`) treating `sandwich_calculate_optimal_layers() == -1` (100% VRAM fit) as `0%` GPU offload (`-ngl 0`), restoring full GPU offload (`-ngl 99`, `4,342 MiB` VRAM, `50.87 t/s` on Dolphin-2.8-Mistral-7B).
  - Honored explicit `--spec-type none` (`has_explicit_non_model_spec`) in the dense drafter scanner and N-gram fallback, and retained partial GPU layer offload (`optimal_layers > 0`) rather than forcing `push_to_ram` (`-ngl 0`) on `<= 30%` fit ratios (`-ngl 16`, `5,689 MiB` VRAM on Gemma-4-31B-it QAT).
- **MoE Online Warm-Up Auto-Tuning Gate (`SANDWICH_AUTOTUNE_DISABLE`):**
  - Wired `SANDWICH_AUTOTUNE_DISABLE` into `llama_context` initialization (`src/llama-context.cpp`) so `--autotune-disable` or `SANDWICH_AUTOTUNE_DISABLE=1` disables both kernel-variant autotuning and live MoE hot-expert warm-up repacking (`repack_moe_expert_cache`), keeping autotune-disabled baseline runs strictly static.
- **Multi-Shard GGUF File Size Aggregation (`--sandwich-auto` & `ModelScanner`):**
  - Added `sandwich_total_gguf_file_bytes()` / `sandwich_total_model_file_bytes()` in `common/common.cpp` and wired them into `sandwich_auto_dispatch()`, `sandwich_calculate_optimal_layers()`, and `ModelScanner::inspect_gguf_file()` (`src/dispatch/model_scanner.cpp`), eliminating shard-1 size underestimation on split checkpoints such as `Qwen3.8-Flash-Next` (`44.09 GB` shard 1 vs `70.91 GB` total).
  - Added `test_multi_shard_size_aggregation()` in `tests/test-sandwich-dispatch.cpp`.

### Removed
- **Legacy DUI/TUI Implementation:**
  - Completely purged broken legacy terminal wizard code (`tui_wizard.*`), removing dead code paths with zero residual references across `src/`, `tools/`, and `common/`.

### Verified
- Zero test exclusions under Rule W (all 76 CTest suites passing).
- 26/26 audit rules verified in forensic ledger validator (`docs/audit/validate_ledger.py`).
