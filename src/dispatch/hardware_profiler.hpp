#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace sandwich {
namespace dispatch {

// Invariant Constants
constexpr uint64_t DGPU_VRAM_CEILING_MB  = 7800; // Single dGPU VRAM ceiling (351 MiB reserve below 8,151 MiB physical; compositor runs on iGPU)
constexpr uint64_t VRAM_RESERVE_MB       = 351;  // Runtime reserve subtracted from cudaMemGetInfo free VRAM
constexpr double   RAM_RULE_42_RATIO     = 0.70; // Hard invariant: Technique #42 (70% Max Host RAM = 16,552 MB)
constexpr uint64_t HOST_RAM_CEILING_MB   = 16552;
constexpr int      FALLBACK_THREADS      = 6;    // Optimal physical P-cores on i7-13620H
constexpr uint64_t FALLBACK_VRAM_MB      = 7800;
constexpr uint64_t FALLBACK_RAM_MB       = 16552;

inline uint64_t effective_vram_ceiling_mb(uint64_t total_vram_mb, uint64_t free_vram_mb) {
    const uint64_t avail = (free_vram_mb > 0) ? free_vram_mb : total_vram_mb;
    if (avail == 0) {
        return 0;
    }
    const uint64_t runtime_cap = (avail > VRAM_RESERVE_MB) ? (avail - VRAM_RESERVE_MB) : (avail / 2);
    return std::min<uint64_t>(DGPU_VRAM_CEILING_MB, runtime_cap);
}

struct CPUProfile {
    std::string model_name           = "Generic x86_64";
    int         total_logical_cores  = 8;
    int         p_cores              = 4;  // Performance cores
    int         e_cores              = 0;  // Efficiency cores
    int         optimal_threads      = 4;  // Bound to physical P-cores
    double      max_freq_ghz         = 2.5;
};

struct RAMProfile {
    uint64_t total_mb               = FALLBACK_RAM_MB;
    uint64_t available_mb           = FALLBACK_RAM_MB / 2;
    uint64_t max_safe_allocation_mb = static_cast<uint64_t>(FALLBACK_RAM_MB * RAM_RULE_42_RATIO);
    double   est_bandwidth_gbps     = 64.0; // DDR5 typical dual-channel bandwidth
};

struct GPUProfile {
    std::string device_name            = "NVIDIA GPU (Autodetected)";
    uint64_t    total_vram_mb          = FALLBACK_VRAM_MB;
    uint64_t    free_vram_mb           = FALLBACK_VRAM_MB;
    uint64_t    wayland_guard_vram_mb  = std::min(FALLBACK_VRAM_MB, DGPU_VRAM_CEILING_MB);
    double      est_bandwidth_gbps     = 224.0; // GDDR6 mobile bus bandwidth
    bool        cuda_available         = false;
};

struct StorageProfile {
    std::string nvme_model             = "Standard NVMe / SSD";
    std::string pcie_link_speed        = "16.0 GT/s PCIe";
    int         pcie_link_width        = 4;
    double      est_read_bandwidth_mbps = 5000.0; // Gen4 x4 sequential
};

struct OSProfile {
    std::string pretty_name            = "Linux";
    std::string id                     = "linux";
    std::string kernel_release         = "";
    bool        is_wayland             = false;
};

struct HardwareProfile {
    std::string    timestamp;
    OSProfile      os;
    CPUProfile     cpu;
    RAMProfile     ram;
    GPUProfile     gpu;
    StorageProfile storage;

    std::string to_json() const;
    static HardwareProfile from_json(const std::string & json_str);
    static HardwareProfile get_conservative_fallback();
};

struct TieringPlan {
    uint64_t total_model_bytes    = 0; // Authoritative sum across ALL shards
    uint64_t model_total_mb       = 0;
    uint64_t tier1_hot_vram_mb    = 0; // GPU VRAM (capped by DGPU_VRAM_CEILING_MB)
    uint64_t tier2_warm_ddr5_mb   = 0; // Host RAM (capped by 70% RAM Rule)
    uint64_t tier3_cold_nvme_mb   = 0; // Direct NVMe mmap paging
    int      gpu_layers           = 0;
    int      total_layers         = 0;
    double   est_tokens_sec_best  = 0.0;
    double   est_tokens_sec_hybrid= 0.0;
    double   est_tokens_sec_worst = 0.0;
    std::string recommended_flags;

    // Unified auto-dispatch plan fields (shared by sandwich_auto_dispatch & Router)
    bool        valid                    = false;
    std::string model_arch               = "unknown";
    bool        is_moe                   = false;
    bool        is_fine_grained          = false;
    bool        is_mla                   = false;
    int64_t     expert_count             = 0;
    size_t      vocab_size               = 0;
    size_t      embedding_length         = 0;
    int64_t     head_count_kv            = 8;
    int64_t     head_dim                 = 128;
    int32_t     n_ctx_eval               = 4096;
    int64_t     n_ctx_train              = 0;
    int64_t     gpu_total_vram_mb        = 8192;
    size_t      gpu_free_vram_b          = 0;
    int64_t     vram_ceiling_mb          = (int64_t) DGPU_VRAM_CEILING_MB;
    int64_t     vram_runtime_available_mb = 0;
    int64_t     safe_vram_cap_mb         = 0;
    int32_t     threads                  = 0;
    std::vector<int> p_core_ids;
    bool        use_split_kv             = false;
    bool        no_kv_offload            = false;
    std::string kv_type_k                = "turbo4";
    std::string kv_type_v                = "turbo4";
    bool        flash_attn               = true;
    bool        flash_attn_native_quants = false;
    bool        turboquant_varn          = false;
    bool        turbo_mma                = false;
    bool        moe_3tier                = false;
    bool        moe_2q                   = true;
    int32_t     moe_cache_slots          = 0;
    size_t      moe_cache_size           = 0;
    int32_t     moe_warm_ram_mb          = 12288;
    bool        moe_cpu_override         = false;
    int32_t     cache_ram_mib            = 0;
    bool        chunked_prefill          = false;
    int32_t     n_ubatch                 = 0;
    int32_t     prefill_chunk_size       = 0;
    bool        sched_async_cpu          = false;
    bool        asym_prefetch            = false;
    int32_t     moe_prefetch_threads     = 0;
    bool        parallel_decoding        = true;
    int32_t     n_parallel               = 4;
    bool        cont_batching            = true;
    bool        thp_align                = true;
    float       early_exit_thresh        = 0.0f;
    bool        logits_topk_default      = false;
    bool        spec_pipe                = false;
    std::string drafter_path;
    int32_t     drafter_gpu_layers       = -1;
    std::string spec_type                = "none"; // "none", "draft", "ngram-simple"
    int32_t     spec_n_max               = 5;
    float       spec_p_min               = 0.75f;
    bool        spec_tree                = false;
    std::string drafter_kv_type          = "turbo4";
    bool        kva_prefill              = false;
    bool        jit_down_quant           = false;
};

class HardwareProfiler {
public:
    static std::string get_config_dir();
    static bool profile_exists();
    static std::string get_profile_path();
    static std::string get_flag_path();
    static HardwareProfile get_conservative_fallback();

    // Runs full hardware profiling and writes to ~/.config/sandwich/hardware_profile.json
    static HardwareProfile run_profiling(bool force = false);

    // Loads cached profile, or runs profiling if missing or corrupted
    static HardwareProfile load_or_profile();

    // Calculates 3-tier memory allocation & throughput simulation for a given model
    static TieringPlan calculate_tiering(
        const HardwareProfile & profile,
        uint64_t model_bytes,
        int total_layers,
        bool is_moe,
        int expert_count = 0,
        int active_experts = 0,
        int context_length = 4096,
        const std::string & kv_quant = "turbo4"
    );

private:
    static OSProfile      detect_os();
    static CPUProfile     detect_cpu();
    static RAMProfile     detect_ram();
    static GPUProfile     detect_gpu();
    static StorageProfile detect_storage();
};

} // namespace dispatch
} // namespace sandwich
