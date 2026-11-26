#include "unified_planner.hpp"

#include "ggml-backend.h"
#include "ggml-cpp.h"
#include "gguf.h"
#include "log.h"

#include <algorithm>
#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(__linux__)
#include <sys/sysinfo.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__) && defined(__MACH__)
#include <sys/sysctl.h>
#endif

namespace sandwich {
namespace dispatch {

namespace {

int64_t get_kv_i64(const struct gguf_context * ctx, int64_t key_id) {
    if (key_id < 0) return 0;
    switch (gguf_get_kv_type(ctx, key_id)) {
        case GGUF_TYPE_UINT32: return (int64_t) gguf_get_val_u32(ctx, key_id);
        case GGUF_TYPE_INT32:  return (int64_t) gguf_get_val_i32(ctx, key_id);
        case GGUF_TYPE_UINT64: return (int64_t) gguf_get_val_u64(ctx, key_id);
        case GGUF_TYPE_INT64:  return gguf_get_val_i64(ctx, key_id);
        case GGUF_TYPE_UINT16: return (int64_t) gguf_get_val_u16(ctx, key_id);
        case GGUF_TYPE_INT16:  return (int64_t) gguf_get_val_i16(ctx, key_id);
        case GGUF_TYPE_UINT8:  return (int64_t) gguf_get_val_u8(ctx, key_id);
        case GGUF_TYPE_INT8:   return (int64_t) gguf_get_val_i8(ctx, key_id);
        default: return 0;
    }
}

void detect_pcore_ids(std::vector<int> & out_pcores) {
    out_pcores.clear();
#if defined(__linux__)
    namespace fs = std::filesystem;
    std::vector<std::pair<int, int64_t>> cpu_freqs;
    int64_t highest_freq = 0;

    for (int i = 0; i < 256; ++i) {
        std::string fpath = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/cpuinfo_max_freq";
        std::ifstream f(fpath);
        if (f.is_open()) {
            int64_t freq = 0;
            if (f >> freq) {
                cpu_freqs.push_back({i, freq});
                if (freq > highest_freq) highest_freq = freq;
            }
        }
    }

    if (cpu_freqs.empty() || highest_freq <= 0) {
        return;
    }

    int64_t pcore_threshold = (int64_t) ((double) highest_freq * 0.90);
    for (const auto & cf : cpu_freqs) {
        if (cf.second >= pcore_threshold) {
            std::string sib_path = "/sys/devices/system/cpu/cpu" + std::to_string(cf.first) + "/topology/thread_siblings_list";
            std::ifstream sf(sib_path);
            bool is_primary = true;
            if (sf.is_open()) {
                int first_sib = -1;
                if (sf >> first_sib && first_sib != cf.first) {
                    is_primary = false;
                }
            }
            if (is_primary) {
                out_pcores.push_back(cf.first);
            }
        }
    }

    const int n_total_hw = (int) std::thread::hardware_concurrency();
    if (out_pcores.size() < 4 || (int) out_pcores.size() >= n_total_hw) {
        out_pcores.clear();
    }
#endif
}

std::string find_compatible_drafter(
    const std::string & model_path,
    const std::string & target_arch,
    size_t target_vocab_size,
    size_t target_embd,
    const std::string & target_tok_model) {

    namespace fs = std::filesystem;
    std::error_code ec;

    fs::path target_p(model_path);
    fs::path anchor_dir = target_p.has_parent_path() ? target_p.parent_path() : fs::current_path();
    std::string best_candidate;
    uintmax_t best_size = UINTMAX_MAX;

    std::vector<fs::path> search_roots;
    search_roots.push_back(anchor_dir);

    if (anchor_dir.has_parent_path()) {
        fs::path parent_dir = anchor_dir.parent_path();
        search_roots.push_back(parent_dir);
        if (parent_dir.has_parent_path()) {
            fs::path grand_dir = parent_dir.parent_path();
            for (const auto & entry : fs::directory_iterator(grand_dir, ec)) {
                if (entry.is_directory(ec)) {
                    std::string dname = entry.path().filename().string();
                    std::string lower_d = dname;
                    std::transform(lower_d.begin(), lower_d.end(), lower_d.begin(), ::tolower);
                    if (lower_d.find("draft") != std::string::npos || lower_d.find("unsloth") != std::string::npos) {
                        search_roots.push_back(entry.path());
                    }
                }
            }
        }
    }

    const char * explicit_roots[] = {
        "/home/sandeep/ai-models/drafters",
        "/home/sandeep/ai-models/draft"
    };
    for (const char * r : explicit_roots) {
        if (fs::exists(r, ec) && fs::is_directory(r, ec)) {
            search_roots.push_back(fs::path(r));
        }
    }

    std::vector<fs::path> unique_dirs;
    for (const auto & root : search_roots) {
        if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) continue;
        unique_dirs.push_back(root);
        for (auto it = fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied, ec);
             it != fs::recursive_directory_iterator(); ++it) {
            if (ec) { ec.clear(); continue; }
            if (it.depth() > 2) { it.disable_recursion_pending(); continue; }
            if (it->is_directory(ec)) {
                unique_dirs.push_back(it->path());
            }
        }
    }

    for (const auto & dir : unique_dirs) {
        for (const auto & entry : fs::directory_iterator(dir, ec)) {
            if (!entry.is_regular_file(ec)) continue;
            fs::path p = entry.path();
            if (p.extension() != ".gguf") continue;

            std::string fname = p.filename().string();
            std::string lower_f = fname;
            std::transform(lower_f.begin(), lower_f.end(), lower_f.begin(), ::tolower);

            if (lower_f.find("mmproj") != std::string::npos ||
                lower_f.find("embed")  != std::string::npos ||
                lower_f.find("imatrix") != std::string::npos) {
                continue;
            }

            std::string full_path = p.string();
            if (full_path == model_path) continue;

            uintmax_t fsize = fs::file_size(p, ec);
            if (ec || fsize < 50ULL * 1024 * 1024 || fsize > 4ULL * 1024 * 1024 * 1024) {
                continue;
            }

            struct gguf_init_params gparams = { true, nullptr };
            gguf_context_ptr cand_ctx(gguf_init_from_file(full_path.c_str(), gparams));
            if (!cand_ctx) continue;

            int64_t cand_arch_id = gguf_find_key(cand_ctx.get(), "general.architecture");
            std::string cand_arch = (cand_arch_id >= 0 && gguf_get_kv_type(cand_ctx.get(), cand_arch_id) == GGUF_TYPE_STRING)
                                    ? gguf_get_val_str(cand_ctx.get(), cand_arch_id)
                                    : "";
            if (cand_arch.empty()) continue;

            bool arch_match = (cand_arch == target_arch);
            if (!arch_match) {
                if (target_arch.find("qwen") != std::string::npos && cand_arch.find("qwen") != std::string::npos) arch_match = true;
                else if (target_arch.find("llama") != std::string::npos && cand_arch.find("llama") != std::string::npos) arch_match = true;
                else if (target_arch.find("gemma") != std::string::npos && cand_arch.find("gemma") != std::string::npos) arch_match = true;
                else if (target_arch.find("lfm2") != std::string::npos && cand_arch.find("lfm2") != std::string::npos) arch_match = true;
                else if (cand_arch.find("dflash") != std::string::npos) arch_match = true;
                else if (cand_arch.find("assistant") != std::string::npos) arch_match = true;
            }
            if (!arch_match) continue;

            if (cand_arch == "assistant" || cand_arch == "dflash") {
                std::string key_embd = cand_arch + ".embedding_length";
                int64_t cand_embd_id = gguf_find_key(cand_ctx.get(), key_embd.c_str());
                if (cand_embd_id >= 0 && target_embd > 0) {
                    size_t cand_embd = (size_t) get_kv_i64(cand_ctx.get(), cand_embd_id);
                    if (cand_embd != target_embd) {
                        continue;
                    }
                }
            }

            int64_t cand_tok_id = gguf_find_key(cand_ctx.get(), "tokenizer.ggml.tokens");
            size_t cand_vocab = (cand_tok_id >= 0) ? gguf_get_arr_n(cand_ctx.get(), cand_tok_id) : 0;
            if (cand_vocab != target_vocab_size || cand_vocab == 0) {
                continue;
            }

            if (!target_tok_model.empty()) {
                int64_t cand_tmod_id = gguf_find_key(cand_ctx.get(), "tokenizer.ggml.model");
                if (cand_tmod_id >= 0 && gguf_get_kv_type(cand_ctx.get(), cand_tmod_id) == GGUF_TYPE_STRING) {
                    std::string cand_tok_model = gguf_get_val_str(cand_ctx.get(), cand_tmod_id);
                    if (cand_tok_model != target_tok_model) {
                        continue;
                    }
                }
            }

            if (fsize < best_size) {
                best_size = fsize;
                best_candidate = full_path;
            }
        }
    }

    return best_candidate;
}

int32_t calculate_optimal_layers(
    const struct gguf_context * ctx,
    uint64_t file_bytes,
    const std::string & arch,
    int64_t vram_budget_mb) {

    std::string key_block_count = arch + ".block_count";
    int64_t block_id = gguf_find_key(ctx, key_block_count.c_str());
    int64_t total_blocks = (block_id >= 0) ? get_kv_i64(ctx, block_id) : 0;
    if (total_blocks <= 0 || file_bytes == 0) {
        return 18;
    }

    // VRAM_RESERVE_MB (351) and kv_turbo4_mb are already subtracted from vram_budget_mb;
    // only account for compute scratch (~350 MiB) + GPU-resident output/norm head (~half embd, capped at 400 MiB).
    int64_t base_overhead_mb = 350;
    int64_t tokens_id = gguf_find_key(ctx, "tokenizer.ggml.tokens");
    int64_t vocab_size = (tokens_id >= 0) ? (int64_t) gguf_get_arr_n(ctx, tokens_id) : 0;
    int64_t embd_id = gguf_find_key(ctx, (arch + ".embedding_length").c_str());
    int64_t embedding_length = (embd_id >= 0) ? get_kv_i64(ctx, embd_id) : 0;

    if (vocab_size > 0 && embedding_length > 0) {
        int64_t embd_mb = (vocab_size * embedding_length * 6ULL) / (10ULL * 1024ULL * 1024ULL);
        base_overhead_mb += std::min<int64_t>(embd_mb / 2, 400);
    } else {
        base_overhead_mb += 200;
    }

    int64_t usable_weight_mb = vram_budget_mb - base_overhead_mb;
    if (usable_weight_mb <= 0) usable_weight_mb = 1000;

    uint64_t approx_layer_bytes = file_bytes / (uint64_t) (total_blocks + 1);
    if (approx_layer_bytes == 0) return 18;

    int64_t allowed_layers = (int64_t) (((uint64_t) usable_weight_mb * 1024ULL * 1024ULL) / approx_layer_bytes);
    if (allowed_layers >= total_blocks) {
        return -1; // 100% GPU offload sentinel
    }
    if (allowed_layers <= 0) {
        return 0;
    }
    return (int32_t) allowed_layers;
}

} // namespace

TieringPlan plan_auto_dispatch(const DispatchRequest & req) {
    TieringPlan plan;
    if (req.model.path.empty()) {
        return plan;
    }

    struct gguf_init_params gguf_params = {
        /* .no_alloc = */ true,
        /* .ctx      = */ nullptr,
    };

    gguf_context_ptr gguf_ctx(gguf_init_from_file(req.model.path.c_str(), gguf_params));
    if (!gguf_ctx) {
        LOG_WRN("🥪 [Sandwich Auto-Dispatch] Failed to open GGUF file: %s\n", req.model.path.c_str());
        return plan;
    }

    // 1. P-core affinity
    detect_pcore_ids(plan.p_core_ids);
    plan.threads = !plan.p_core_ids.empty()
        ? (int32_t) plan.p_core_ids.size()
        : req.n_threads;

    // 2. Topology metadata
    int64_t key_expert_count = gguf_find_key(gguf_ctx.get(), "general.expert_count");
    if (key_expert_count >= 0) {
        plan.expert_count = get_kv_i64(gguf_ctx.get(), key_expert_count);
    }

    int64_t arch_id = gguf_find_key(gguf_ctx.get(), "general.architecture");
    if (arch_id >= 0 && gguf_get_kv_type(gguf_ctx.get(), arch_id) == GGUF_TYPE_STRING) {
        plan.model_arch = gguf_get_val_str(gguf_ctx.get(), arch_id);
    }

    if (plan.expert_count == 0 && !plan.model_arch.empty() && plan.model_arch != "unknown") {
        int64_t key_exp = gguf_find_key(gguf_ctx.get(), (plan.model_arch + ".expert_count").c_str());
        if (key_exp >= 0) {
            plan.expert_count = get_kv_i64(gguf_ctx.get(), key_exp);
        }
        if (plan.expert_count == 0) {
            int64_t key_used = gguf_find_key(gguf_ctx.get(), (plan.model_arch + ".expert_used_count").c_str());
            if (key_used >= 0) {
                plan.expert_count = get_kv_i64(gguf_ctx.get(), key_used);
            }
        }
    }

    int64_t tokens_id = gguf_find_key(gguf_ctx.get(), "tokenizer.ggml.tokens");
    if (tokens_id >= 0) {
        plan.vocab_size = gguf_get_arr_n(gguf_ctx.get(), tokens_id);
    }

    if (!plan.model_arch.empty() && plan.model_arch != "unknown") {
        int64_t embd_id = gguf_find_key(gguf_ctx.get(), (plan.model_arch + ".embedding_length").c_str());
        if (embd_id >= 0) {
            plan.embedding_length = (size_t) get_kv_i64(gguf_ctx.get(), embd_id);
        }
    }

    std::string tok_model;
    int64_t tok_model_id = gguf_find_key(gguf_ctx.get(), "tokenizer.ggml.model");
    if (tok_model_id >= 0 && gguf_get_kv_type(gguf_ctx.get(), tok_model_id) == GGUF_TYPE_STRING) {
        tok_model = gguf_get_val_str(gguf_ctx.get(), tok_model_id);
    }

    plan.is_moe = (plan.expert_count > 0);
    plan.is_fine_grained = plan.is_moe && (
        plan.expert_count >= 256 ||
        plan.model_arch == "qwen3next" ||
        plan.model_arch == "deepseek2" ||
        plan.model_arch == "deepseek3"
    );
    plan.is_mla = (
        plan.model_arch == "deepseek2" ||
        plan.model_arch == "deepseek"  ||
        plan.model_arch == "deepseek3" ||
        plan.model_arch == "deepseek4"
    );

    // 3. GPU VRAM discovery & dynamic ceiling: min(DGPU_VRAM_CEILING_MB=7800, free_mib - VRAM_RESERVE_MB=351)
    size_t gpu_free_vram_b  = 0;
    size_t gpu_total_vram_b = 0;
    if (req.override_total_vram_mb >= 0 && req.override_free_vram_mb >= 0) {
        gpu_total_vram_b = (size_t) req.override_total_vram_mb * 1024ULL * 1024ULL;
        gpu_free_vram_b  = (size_t) req.override_free_vram_mb * 1024ULL * 1024ULL;
    } else {
        for (size_t i = 0; i < ggml_backend_dev_count(); ++i) {
            auto * dev = ggml_backend_dev_get(i);
            if (ggml_backend_dev_type(dev) == GGML_BACKEND_DEVICE_TYPE_GPU) {
                size_t free_b = 0, total_b = 0;
                ggml_backend_dev_memory(dev, &free_b, &total_b);
                gpu_free_vram_b  += free_b;
                gpu_total_vram_b += total_b;
            }
        }
    }
    plan.gpu_free_vram_b   = gpu_free_vram_b;
    plan.gpu_total_vram_mb = (gpu_total_vram_b > 0) ? (int64_t) (gpu_total_vram_b / (1024 * 1024)) : 8192;

    const int64_t static_ceiling_mb = (int64_t) DGPU_VRAM_CEILING_MB; // 7,800 MiB (351 MiB reserve below 8,151 MiB)
    const int64_t free_mib = (gpu_free_vram_b > 0)
        ? (int64_t) (gpu_free_vram_b / (1024ULL * 1024ULL))
        : plan.gpu_total_vram_mb;
    const int64_t runtime_available_mb = (free_mib > (int64_t) VRAM_RESERVE_MB)
        ? std::min<int64_t>(static_ceiling_mb, free_mib - (int64_t) VRAM_RESERVE_MB)
        : std::max<int64_t>(0, free_mib / 2);

    int64_t safe_vram_cap_mb = runtime_available_mb;
    if (!req.vram_guard.empty()) {
        const std::string & vg = req.vram_guard;
        if (vg == "none" || vg == "uncapped" || vg == "max" || vg == "0") {
            safe_vram_cap_mb = std::min<int64_t>(static_ceiling_mb, std::max<int64_t>(0, free_mib - 80));
        } else {
            try {
                size_t end_idx = 0;
                double val = std::stod(vg, &end_idx);
                std::string unit = vg.substr(end_idx);
                int64_t parsed_mb = (unit == "G" || unit == "g" || unit == "GB" || unit == "gb" || unit == "GiB")
                    ? (int64_t) (val * 1024.0)
                    : (int64_t) val;
                safe_vram_cap_mb = std::min<int64_t>(parsed_mb, runtime_available_mb > 0 ? runtime_available_mb : parsed_mb);
            } catch (...) {
            }
        }
    }
    plan.vram_ceiling_mb           = static_ceiling_mb;
    plan.vram_runtime_available_mb = runtime_available_mb;
    plan.safe_vram_cap_mb          = safe_vram_cap_mb;

    // 4. Layer / attention / shard size metadata (authoritative multi-shard sum!)
    int64_t block_id = gguf_find_key(gguf_ctx.get(), (plan.model_arch + ".block_count").c_str());
    int64_t total_blocks = (block_id >= 0) ? get_kv_i64(gguf_ctx.get(), block_id) : 0;
    plan.total_layers = (int) total_blocks;

    int64_t hkv_id = gguf_find_key(gguf_ctx.get(), (plan.model_arch + ".attention.head_count_kv").c_str());
    if (hkv_id >= 0) {
        plan.head_count_kv = get_kv_i64(gguf_ctx.get(), hkv_id);
    } else {
        int64_t h_id = gguf_find_key(gguf_ctx.get(), (plan.model_arch + ".attention.head_count").c_str());
        if (h_id >= 0) plan.head_count_kv = get_kv_i64(gguf_ctx.get(), h_id);
    }
    if (plan.head_count_kv <= 0) plan.head_count_kv = 8;

    int64_t klen_id = gguf_find_key(gguf_ctx.get(), (plan.model_arch + ".attention.key_length").c_str());
    if (klen_id >= 0) {
        plan.head_dim = get_kv_i64(gguf_ctx.get(), klen_id);
    } else {
        int64_t h_id = gguf_find_key(gguf_ctx.get(), (plan.model_arch + ".attention.head_count").c_str());
        int64_t head_count = (h_id >= 0) ? get_kv_i64(gguf_ctx.get(), h_id) : 0;
        if (head_count > 0 && plan.embedding_length > 0) {
            plan.head_dim = (int64_t) (plan.embedding_length / (size_t) head_count);
        } else {
            plan.head_dim = 128;
        }
    }

    plan.total_model_bytes = sandwich_total_model_file_bytes(req.model, gguf_ctx.get());
    plan.model_total_mb    = plan.total_model_bytes / (1024ULL * 1024ULL);
    const uint64_t approx_layer_bytes = (total_blocks > 0 && plan.total_model_bytes > 0)
        ? (plan.total_model_bytes / (uint64_t) (total_blocks + 1))
        : 0;

    plan.n_ctx_eval = (req.n_ctx > 0) ? req.n_ctx : 4096;
    const uint64_t kv_elems_per_token = 2ULL * (uint64_t) (total_blocks > 0 ? total_blocks : 32) * (uint64_t) plan.head_count_kv * (uint64_t) plan.head_dim;
    const int64_t kv_turbo4_mb = (int64_t) ((uint64_t) (kv_elems_per_token * (double) plan.n_ctx_eval * 0.5) / (1024ULL * 1024ULL));

    int64_t ctx_id = gguf_find_key(gguf_ctx.get(), (plan.model_arch + ".context_length").c_str());
    if (ctx_id >= 0) {
        plan.n_ctx_train = get_kv_i64(gguf_ctx.get(), ctx_id);
    }

    // Never silently force Q2_K down-quantization
    plan.jit_down_quant = req.jit_down_quant;
    plan.thp_align      = true;
    plan.flash_attn     = true;
    plan.parallel_decoding = true;
    plan.n_parallel     = (req.n_parallel < 0) ? 4 : req.n_parallel;
    plan.cont_batching  = true;

    // Preserve explicit `--spec-type none` or explicit drafter path
    plan.drafter_path       = req.explicit_drafter_path;
    plan.drafter_gpu_layers = req.drafter_gpu_layers;
    plan.moe_3tier          = req.moe_3tier;
    plan.moe_cache_slots    = req.moe_cache_slots;
    plan.moe_cache_size     = req.moe_cache_size;
    plan.moe_warm_ram_mb    = req.moe_warm_ram_mb;
    plan.cache_ram_mib      = req.cache_ram_mib;
    plan.n_ubatch           = req.n_ubatch;
    plan.prefill_chunk_size = req.prefill_chunk_size;
    plan.chunked_prefill    = req.chunked_prefill;
    plan.turbo_mma          = req.turbo_mma;
    plan.turboquant_varn    = req.turboquant_varn;
    plan.asym_prefetch      = req.asym_prefetch;
    plan.kva_prefill        = req.kva_prefill;

    auto resolve_dense_offload = [&](int32_t opt_layers) -> int32_t {
        if (opt_layers < 0) {
            return std::max<int32_t>(99, (int32_t) total_blocks + 1);
        }
        if (opt_layers > 0) {
            return opt_layers;
        }
        return 0;
    };

    bool moe_full_fit = false;
    if (plan.is_moe) {
        const size_t total_free_vram = gpu_free_vram_b;
        const int64_t model_mb = (int64_t) (plan.total_model_bytes / (1024ULL * 1024ULL));
        const int64_t moe_compute_buf_mb = 350;
        const bool fits_in_vram = (safe_vram_cap_mb > 0 && plan.total_model_bytes > 0 &&
                                   (model_mb + kv_turbo4_mb + moe_compute_buf_mb) <= safe_vram_cap_mb);

        if (fits_in_vram && plan.moe_cache_size == 0 && plan.moe_cache_slots <= 0) {
            // MoE Full-Fit: 100% GPU offload; disable Tier 2/3 paging, hot-pack, and warm pool
            moe_full_fit              = true;
            plan.gpu_layers           = 99;
            plan.moe_cpu_override     = false;
            plan.moe_3tier            = false;
            plan.moe_2q               = false;
            plan.moe_cache_slots      = 0;
            plan.moe_cache_size       = 0;
            plan.moe_warm_ram_mb      = 0;
            plan.cache_ram_mib        = 0;
            plan.sched_async_cpu      = false;
            plan.asym_prefetch        = false;
            plan.moe_prefetch_threads = 0;
        } else {
#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGE_SIZE)
            int64_t total_host_ram_mb = ((int64_t) sysconf(_SC_PHYS_PAGES) * (int64_t) sysconf(_SC_PAGE_SIZE)) / (1024 * 1024);
            if (total_host_ram_mb >= 22000 && plan.moe_warm_ram_mb <= 14336) {
                plan.moe_warm_ram_mb = 14336;
            }
            if (plan.moe_warm_ram_mb > 16552) {
                plan.moe_warm_ram_mb = 16384;
            }
#endif
            plan.gpu_layers       = 99;
            plan.moe_cpu_override = true;

            const size_t free_vram_mib = total_free_vram / (1024 * 1024);
            if (plan.moe_3tier) {
                plan.moe_cache_slots = 0;
                if (plan.moe_cache_size == 0) {
                    // Fill Tier-1 MoE expert slab up to the dynamic VRAM ceiling after backbone + KV + compute scratch
                    const double backbone_frac = (plan.expert_count >= 256) ? 0.06 : ((plan.expert_count >= 64) ? 0.20 : 0.25);
                    const int64_t est_backbone_mb = std::max<int64_t>(600, (int64_t) ((double) model_mb * backbone_frac));
                    int64_t slab_mb = safe_vram_cap_mb - est_backbone_mb - kv_turbo4_mb - 450;
                    slab_mb = std::max<int64_t>(1024, std::min<int64_t>(slab_mb, 5600));
                    slab_mb = (slab_mb / 64) * 64;
                    plan.moe_cache_size = (size_t) slab_mb * 1024ULL * 1024ULL;
                }
            } else if (plan.moe_cache_slots <= 0 && plan.moe_cache_size == 0) {
                if (plan.expert_count >= 256) {
                    plan.moe_cache_slots = (free_vram_mib > 6000) ? 24 : 16;
                } else if (plan.expert_count >= 64) {
                    plan.moe_cache_slots = 36;
                } else {
                    plan.moe_cache_slots = std::min<int32_t>(32, (int32_t) plan.expert_count);
                }
                plan.moe_cache_size = 0;
            } else if (plan.moe_cache_slots > 0) {
                plan.moe_cache_size = 0;
            } else {
                if (free_vram_mib > 2000)      plan.moe_cache_size = (free_vram_mib - 800) * 1024 * 1024;
                else if (free_vram_mib > 1000) plan.moe_cache_size = (free_vram_mib - 500) * 1024 * 1024;
                else if (free_vram_mib > 400)  plan.moe_cache_size = (free_vram_mib - 200) * 1024 * 1024;
                else                           plan.moe_cache_size = 512ULL * 1024 * 1024;
            }

            plan.moe_2q          = true;
            plan.moe_3tier       = true;
            plan.sched_async_cpu = true;
            if (plan.cache_ram_mib <= 0) {
                plan.cache_ram_mib = 6144;
            }
            plan.moe_prefetch_threads = 4;
        }

        if (req.sandwich_predict && plan.n_ctx_eval >= 48000) {
            plan.no_kv_offload = true;
            plan.kv_type_k     = "turbo2";
            plan.kv_type_v     = "turbo2";
        } else if (req.sandwich_predict && plan.n_ctx_eval >= 32768) {
            plan.no_kv_offload = true;
            plan.kv_type_k     = "turbo3";
            plan.kv_type_v     = "turbo3";
        } else {
            plan.no_kv_offload = false;
            plan.kv_type_k     = "turbo4";
            plan.kv_type_v     = "turbo4";
        }

        if (plan.drafter_path.empty() && !req.explicit_non_model_spec) {
            plan.drafter_path = find_compatible_drafter(req.model.path, plan.model_arch, plan.vocab_size, plan.embedding_length, tok_model);
        }
        if (!plan.drafter_path.empty()) {
            std::string dl = plan.drafter_path;
            std::transform(dl.begin(), dl.end(), dl.begin(), ::tolower);
            const bool is_mtp = (dl.find("mtp") != std::string::npos);
            plan.spec_type          = "draft";
            plan.spec_n_max         = is_mtp ? 1 : 6;
            plan.spec_p_min         = 0.75f;
            plan.drafter_kv_type    = "turbo4";
            if (plan.drafter_gpu_layers < 0) {
                plan.drafter_gpu_layers = (plan.gpu_total_vram_mb >= 6000) ? 99 : 0;
            }
        }
    } else {
        // DENSE TRANSFORMER
        plan.moe_cache_size = 0;
        plan.moe_prefetch_threads = 0;

        if (plan.drafter_path.empty() && !req.explicit_non_model_spec) {
            plan.drafter_path = find_compatible_drafter(req.model.path, plan.model_arch, plan.vocab_size, plan.embedding_length, tok_model);
        }

        if (!plan.drafter_path.empty()) {
            std::string dl = plan.drafter_path;
            std::transform(dl.begin(), dl.end(), dl.begin(), ::tolower);
            const bool is_assistant = (dl.find("assistant") != std::string::npos || dl.find("0.8b") != std::string::npos || dl.find("1b") != std::string::npos);
            const bool is_dflash    = (dl.find("dflash") != std::string::npos || dl.find("mtp") != std::string::npos);

            std::error_code d_ec;
            const size_t drafter_bytes = std::filesystem::file_size(plan.drafter_path, d_ec);
            const int64_t drafter_mb   = (!d_ec) ? (int64_t) (drafter_bytes / (1024ULL * 1024ULL)) : 600;
            const int64_t drafter_headroom_mb = drafter_mb + (is_dflash ? 650 : (is_assistant ? 350 : 450));
            const int64_t main_budget_mb = std::max<int64_t>(0, safe_vram_cap_mb - drafter_headroom_mb - kv_turbo4_mb);

            const int32_t opt_layers = (main_budget_mb > 500)
                ? calculate_optimal_layers(gguf_ctx.get(), plan.total_model_bytes, plan.model_arch, main_budget_mb)
                : 0;
            const int32_t target_gpu_layers = resolve_dense_offload(opt_layers);
            const bool predict_asymmetric_split = (target_gpu_layers == 0);

            int64_t offloaded_blocks = (total_blocks > 0) ? std::min<int64_t>(target_gpu_layers, total_blocks) : target_gpu_layers;
            int64_t target_gpu_weight_mb = 0;
            if (offloaded_blocks > 0 && approx_layer_bytes > 0) {
                target_gpu_weight_mb = (int64_t) ((offloaded_blocks * approx_layer_bytes) / (1024ULL * 1024ULL));
                if (plan.vocab_size > 0 && plan.embedding_length > 0) {
                    target_gpu_weight_mb += (int64_t) ((plan.vocab_size * plan.embedding_length * 6ULL) / (10ULL * 1024ULL * 1024ULL));
                }
            }
            const int64_t total_vram_proj_mb = target_gpu_weight_mb + drafter_headroom_mb + kv_turbo4_mb;

            bool route_kv_to_host = false;
            std::string kv_t = "turbo4";
            if (is_assistant) {
                kv_t = "f16";
                route_kv_to_host = false;
            } else if (req.sandwich_predict) {
                if (plan.n_ctx_eval >= 48000) {
                    route_kv_to_host = true;
                    kv_t = "turbo2";
                } else if (plan.n_ctx_eval >= 32768 || total_vram_proj_mb > safe_vram_cap_mb) {
                    route_kv_to_host = true;
                    kv_t = "turbo3";
                }
            }
            if (predict_asymmetric_split) {
                route_kv_to_host = false;
                plan.use_split_kv = true;
            }

            plan.gpu_layers         = target_gpu_layers;
            plan.no_kv_offload      = route_kv_to_host;
            plan.kv_type_k          = kv_t;
            plan.kv_type_v          = kv_t;
            plan.drafter_gpu_layers = (total_vram_proj_mb <= safe_vram_cap_mb) ? 99 : 0;
            plan.spec_type          = "draft";
            plan.spec_n_max         = 5;
            plan.spec_p_min         = 0.75f;
            plan.spec_tree          = true;
            plan.drafter_kv_type    = is_assistant ? "f16" : "turbo4";
            plan.early_exit_thresh  = predict_asymmetric_split ? 0.0f : 0.92f;
        } else {
            const int64_t uncapped_budget_mb = std::max<int64_t>(0, safe_vram_cap_mb - kv_turbo4_mb);
            const int32_t opt_layers = (uncapped_budget_mb > 500)
                ? calculate_optimal_layers(gguf_ctx.get(), plan.total_model_bytes, plan.model_arch, uncapped_budget_mb)
                : 0;
            const int32_t target_gpu_layers = resolve_dense_offload(opt_layers);
            const bool push_to_ram = (target_gpu_layers == 0);

            int64_t offloaded_blocks = (total_blocks > 0) ? std::min<int64_t>(target_gpu_layers, total_blocks) : target_gpu_layers;
            int64_t target_gpu_weight_mb = 0;
            if (offloaded_blocks > 0 && approx_layer_bytes > 0) {
                target_gpu_weight_mb = (int64_t) ((offloaded_blocks * approx_layer_bytes) / (1024ULL * 1024ULL));
                if (plan.vocab_size > 0 && plan.embedding_length > 0) {
                    target_gpu_weight_mb += (int64_t) ((plan.vocab_size * plan.embedding_length * 6ULL) / (10ULL * 1024ULL * 1024ULL));
                }
            }
            const int64_t total_vram_proj_mb = target_gpu_weight_mb + 400 + kv_turbo4_mb;

            bool route_kv_to_host = false;
            std::string kv_t = "turbo4";
            if (push_to_ram) {
                route_kv_to_host = false;
                plan.use_split_kv = true;
            } else if (req.sandwich_predict) {
                if (plan.n_ctx_eval >= 48000) {
                    route_kv_to_host = true;
                    kv_t = "turbo2";
                } else if (plan.n_ctx_eval >= 32768 || total_vram_proj_mb > safe_vram_cap_mb) {
                    route_kv_to_host = true;
                    kv_t = "turbo3";
                }
            }

            plan.gpu_layers    = target_gpu_layers;
            plan.no_kv_offload = route_kv_to_host;
            plan.kv_type_k     = kv_t;
            plan.kv_type_v     = kv_t;
            if (!req.explicit_non_model_spec) {
                plan.spec_type  = "ngram-simple";
                plan.spec_n_max = 5;
            } else {
                plan.spec_type  = "none";
            }
        }
    }

    // 5. Shared post-branch pipeline & defensive guards
    if (plan.use_split_kv || (!plan.is_moe && plan.gpu_layers == 0)) {
        plan.use_split_kv        = true;
        plan.no_kv_offload       = false;
        plan.spec_pipe           = true;
        plan.logits_topk_default = (req.logits_topk <= 0);
    }

    if (gpu_free_vram_b > 0) {
        plan.flash_attn_native_quants = true;
        if (plan.turboquant_varn || plan.kv_type_k == "turbo4" || plan.kv_type_k == "turbo3" || plan.kv_type_k == "turbo2") {
            plan.turboquant_varn = true;
        }
    }

    if (req.asym_kv_squeeze || (req.kv_is_turbo4_k && req.kv_is_turbo2_v)) {
        plan.kv_type_k = "turbo4";
        plan.kv_type_v = "turbo2";
    }

    if (plan.chunked_prefill || plan.n_ctx_eval >= 32768 || (plan.is_moe && !moe_full_fit)) {
        plan.chunked_prefill = true;
        if (plan.prefill_chunk_size > 0) {
            plan.n_ubatch = plan.prefill_chunk_size;
        } else if (plan.is_moe) {
            plan.n_ubatch = (plan.expert_count >= 256) ? 64 : 128;
            plan.prefill_chunk_size = plan.n_ubatch;
        } else if (plan.n_ubatch > 512 || plan.n_ubatch == 0) {
            plan.n_ubatch = 512;
        }
    }

    if (plan.kva_prefill || (plan.n_ctx_eval >= 32768 && (plan.is_moe || plan.is_mla))) {
        plan.kva_prefill = true;
    }

    if (plan.asym_prefetch || (plan.is_moe && !moe_full_fit && plan.expert_count >= 256)) {
        plan.asym_prefetch = true;
    }

    if (plan.turbo_mma && (plan.kv_type_k == "turbo4" || plan.kv_type_k == "turbo3" || plan.kv_type_k == "turbo2") && plan.head_dim >= 256) {
        plan.turbo_mma = false;
    }

    if (plan.drafter_gpu_layers >= 99 && plan.gpu_total_vram_mb < 6000) {
        plan.drafter_gpu_layers = 0;
    }

    if (plan.moe_warm_ram_mb > 16552) {
        plan.moe_warm_ram_mb = 16384;
    }

    if (plan.moe_cache_slots > 0 && plan.moe_cache_size > 0) {
        plan.moe_cache_size = 0;
    }

    plan.valid = true;
    return plan;
}

} // namespace dispatch
} // namespace sandwich
