# Strata TUI Backend C++ API Reference

**Header:** `src/tui/sandwich_tui_backend.h`  
**Namespace:** `sandwich::tui`  
**Library:** `libsandwich-dispatch.a`

The Strata TUI Backend provides a decoupled abstraction layer connecting the Strata terminal user interface with Sandwich Engine's inference execution, memory estimators, and hardware telemetry HUD.

---

## Data Structures

### `struct StrataTelemetry`
Real-time hardware, cache, and inference throughput metrics sampled at 20 Hz.

```cpp
struct StrataTelemetry {
    uint64_t vram_used_bytes        = 0;      // Physical VRAM allocated (bytes)
    uint64_t vram_total_bytes       = 0;      // Physical VRAM capacity (bytes)
    double   vram_used_mib          = 0.0;    // VRAM allocated in MiB
    double   vram_total_mib         = 0.0;    // VRAM capacity in MiB
    uint64_t ram_rss_bytes          = 0;      // Host Resident Set Size (bytes)
    double   ram_rss_mib            = 0.0;    // Host Resident Set Size (MiB)
    double   decode_tokens_per_sec  = 0.0;    // Generation throughput (tok/s)
    double   prefill_tokens_per_sec = 0.0;    // Prompt processing speed (tok/s)
    double   latency_ms             = 0.0;    // Single-token decode latency (ms)
    double   moe_cache_hit_rate_pct = 0.0;    // Dynamic expert cache hit rate (%)
    uint64_t hot_hits               = 0;      // Cumulative hot expert hits
    uint64_t cold_misses            = 0;      // Cumulative cold expert misses
    uint64_t total_evals            = 0;      // Total expert evaluations
    int32_t  n_gpu_layers           = 14;     // Active GPU layer count
    int32_t  n_threads              = 6;      // Physical worker threads
    int32_t  n_ctx                  = 4096;   // Context window size
    bool     drafter_active         = false;  // Whether speculative drafter is enabled
    bool     drafter_pinned         = false;  // Whether drafter is 100% in VRAM (-ngld 99)
    std::string drafter_path;                 // Path to active drafter .gguf
    bool     io_uring_active        = false;  // Whether Linux io_uring DMA is active
    double   pcie_bandwidth_gbps    = 0.0;    // Active host-to-device PCIe bandwidth
};
```

### `struct StrataInferenceConfig`
Configuration parameters for launching inference in Auto-Dispatch or Manual Mode.

```cpp
struct StrataInferenceConfig {
    std::string model_path;                   // Absolute path to primary model .gguf
    bool        auto_mode           = true;   // If true, invokes sandwich_auto_dispatch()
    int32_t     n_ctx               = 4096;   // Context window size (default 4096)
    int32_t     n_threads           = 6;      // Compute worker threads (default 6 P-cores)
    int32_t     n_gpu_layers        = 14;     // Base model GPU layers offloaded
    std::string drafter_path;                 // Companion speculative drafter path
    int32_t     drafter_gpu_layers  = 99;     // Drafter GPU layers (99 = pinned in VRAM)
    int32_t     moe_cache_slots     = 0;      // Expert VRAM slots (0, 16, 32)
    int32_t     moe_warm_ram_mb     = 0;      // Host DDR5 warm pool capacity in MiB
    std::string ctk                 = "turbo4"; // Key quantization type
    std::string ctv                 = "turbo4"; // Value quantization type
    bool        io_uring            = true;   // Enable Linux io_uring direct NVMe DMA
    bool        moe_2q              = true;   // Two-Queue cache replacement policy
    bool        moe_exchange_rot    = true;   // Rotational zero-copy buffer pointer swap
    std::string prompt;                       // Input inference prompt string
};
```

---

## API Functions

### `strata_get_telemetry`
```cpp
StrataTelemetry strata_get_telemetry();
```
Polls live hardware counters from NVIDIA CUDA runtime (`cudaMemGetInfo`), host RSS from `/proc/self/status` (`VmRSS`), and atomic cache counters from Technique #18 (`sandwich_telemetry_get_stats()`).

### `strata_get_vram_usage`
```cpp
void strata_get_vram_usage(uint64_t & free_bytes, uint64_t & total_bytes);
```
Queries physical CUDA memory directly from the active GPU driver. If no CUDA driver is available, returns 0 gracefully.

### `strata_get_ram_usage`
```cpp
uint64_t strata_get_ram_usage();
```
Parses the current process's `VmRSS` in bytes directly from `/proc/self/status`.

### `strata_discover_models` / `strata_discover_drafters`
```cpp
std::vector<std::string> strata_discover_models();
std::vector<std::string> strata_discover_drafters();
```
Scans well-known model directories (`/home/sandeep/ai-models/`, `./models`, `~/.gguf`) for compatible `.gguf` checkpoints and categorizes speculative drafters (e.g. `DFlash2`).

### `strata_construct_cli_command`
```cpp
std::string strata_construct_cli_command(const StrataInferenceConfig & config);
```
Synthesizes a complete, copy-pasteable CLI execution command from the given configuration, properly quoting paths with whitespace and applying `sandwich_auto_dispatch()` auto-tuning when `config.auto_mode == true`.
