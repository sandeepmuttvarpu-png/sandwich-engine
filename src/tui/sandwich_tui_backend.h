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
    int32_t  ctx_used_tokens        = 0;
    bool     is_moe                 = true;
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

struct StrataChatMessage {
    std::string role;       // "user" or "assistant"
    std::string content;
    int         token_count = 0;
    double      tok_per_sec = 0.0;
    double      latency_ms  = 0.0;
};

// Model Architecture & Discovery
bool strata_is_model_moe(const std::string & model_path);
std::vector<std::string> strata_discover_models();
std::vector<std::string> strata_discover_drafters();

// Hardware Telemetry & Memory Wrappers
void strata_get_vram_usage(uint64_t & free_bytes, uint64_t & total_bytes);
uint64_t strata_get_ram_usage();
StrataTelemetry strata_get_telemetry();
void strata_reset_telemetry();

// Server Lifecycle Management (Client-Server Architecture)
bool  strata_server_is_healthy(const std::string & host = "127.0.0.1", int port = 8080);
pid_t strata_server_spawn(const StrataInferenceConfig & config, int port = 8080);
bool  strata_server_wait_healthy(const std::string & host = "127.0.0.1", int port = 8080, int timeout_sec = 60, std::function<void(int)> progress_cb = nullptr);
bool  strata_server_stop(pid_t pid, int timeout_sec = 10);
bool  strata_server_abort_slot(const std::string & host = "127.0.0.1", int port = 8080, int slot_id = 0);
void  strata_query_server_telemetry(const std::string & host, int port, StrataTelemetry & telem);

// SSE Streaming Chat Inference
bool strata_stream_chat_sse(const std::string & host,
                            int port,
                            const std::string & prompt,
                            std::function<void(const std::string &)> token_callback,
                            std::atomic<bool> & abort_flag,
                            double & out_tok_per_sec,
                            int & out_token_count,
                            double & out_latency_ms);

// CLI Command Synthesizer (must match Web GUI byte-identically)
std::string strata_construct_cli_command(const StrataInferenceConfig & config);

// Terminal Markdown-LITE formatter
std::string strata_format_markdown_lite(const std::string & text);

// Clipboard OSC52 Copy
std::string strata_make_osc52_copy_sequence(const std::string & text);

// In-process legacy helpers
struct llama_model * strata_load_model(const std::string & model_path);
bool strata_start_inference(const StrataInferenceConfig & config,
                            std::function<void(const std::string&)> token_callback,
                            std::atomic<bool> & stop_flag);

} // namespace tui
} // namespace sandwich
