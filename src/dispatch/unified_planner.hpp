#pragma once

#include "hardware_profiler.hpp"
#include "common.h"

#include <cstdint>
#include <string>
#include <vector>

namespace sandwich {
namespace dispatch {

struct DispatchRequest {
    common_params_model model;
    int32_t     n_ctx                    = 0;
    int32_t     n_threads                = -1;
    std::string vram_guard;
    bool        sandwich_predict         = true;
    bool        explicit_non_model_spec  = false;
    std::string explicit_drafter_path;
    int32_t     drafter_gpu_layers       = -1;
    bool        moe_3tier                = false;
    int32_t     moe_cache_slots          = 0;
    size_t      moe_cache_size           = 0;
    int32_t     moe_warm_ram_mb          = 12288;
    int32_t     cache_ram_mib            = 0;
    bool        chunked_prefill          = false;
    int32_t     n_ubatch                 = 0;
    int32_t     prefill_chunk_size       = 0;
    int32_t     n_parallel               = -1;
    bool        split_kv                 = false;
    int32_t     logits_topk              = 0;
    bool        asym_kv_squeeze          = false;
    bool        kv_is_turbo4_k           = false;
    bool        kv_is_turbo2_v           = false;
    bool        turbo_mma                = false;
    bool        turboquant_varn          = false;
    bool        asym_prefetch            = false;
    bool        kva_prefill              = false;
    bool        native_mtp               = false;
    bool        load_mtp                 = false;
    bool        jit_down_quant           = false;
    // Optional overrides for deterministic unit-test fixtures (-1 = probe live hardware)
    int64_t     override_total_vram_mb   = -1;
    int64_t     override_free_vram_mb    = -1;
};

// Single authoritative auto-dispatch planner used by both `llama-cli --sandwich-auto`
// and `sandwich` CLI (`Router`). Always computes `plan.total_model_bytes` across all shards.
TieringPlan plan_auto_dispatch(const DispatchRequest & req);

} // namespace dispatch
} // namespace sandwich
