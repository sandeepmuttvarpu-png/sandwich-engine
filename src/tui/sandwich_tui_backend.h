#pragma once

#include "llama.h"
#include "common.h"

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <cstdint>
#include <memory>

namespace sandwich {
namespace tui {

struct StrataTelemetry {
    uint64_t vram_used_bytes        = 0;
    uint64_t vram_total_bytes       = 0;
    double   vram_used_mib          = 0.0;
    double   vram_total_mib         = 0.0;
    uint64_t ram_rss_bytes          = 0;
    double   ram_rss_mib            = 0.0;
    double   decode_tokens_per_sec  = 0.0;
    double   prefill_tokens_per_sec = 0.0;
    double   latency_ms             = 0.0;
    double   moe_cache_hit_rate_pct = 0.0;
    uint64_t hot_hits               = 0;
    uint64_t cold_misses            = 0;
    uint64_t total_evals            = 0;
    int32_t  n_gpu_layers           = 14;
    int32_t  n_threads              = 6;
    int32_t  n_ctx                  = 4096;
    bool     drafter_active         = false;
    bool     drafter_pinned         = false;
    std::string drafter_path;
    bool     io_uring_active        = false;
    double   pcie_bandwidth_gbps    = 0.0;
};

struct StrataInferenceConfig {
    std::string model_path;
    bool        auto_mode           = true;
    int32_t     n_ctx               = 4096;
    int32_t     n_threads           = 6;
    int32_t     n_gpu_layers        = 14;

    // Advanced manual flags (for power users)
    std::string drafter_path;
    int32_t     drafter_gpu_layers  = 99; // 0 = host, 99 = VRAM-pinned
    int32_t     moe_cache_slots     = 0;  // 0, 16, 32
    int32_t     moe_warm_ram_mb     = 0;
    std::string ctk                 = "turbo4";
    std::string ctv                 = "turbo4";
    bool        io_uring            = true;
    bool        moe_2q              = true;
    bool        moe_exchange_rot    = true;

    std::string prompt              = "Introduce yourself and explain the benefits of asymmetric MoE offload.";
};

// Strata TUI Abstraction API Layer
struct llama_model * strata_load_model(const std::string & model_path);
bool strata_start_inference(const StrataInferenceConfig & config,
                            std::function<void(const std::string&)> token_callback,
                            std::atomic<bool> & stop_flag);
StrataTelemetry strata_get_telemetry();
void strata_get_vram_usage(uint64_t & free_bytes, uint64_t & total_bytes);
uint64_t strata_get_ram_usage();
std::vector<std::string> strata_discover_models();
std::vector<std::string> strata_discover_drafters();
std::string strata_construct_cli_command(const StrataInferenceConfig & config);

} // namespace tui
} // namespace sandwich
