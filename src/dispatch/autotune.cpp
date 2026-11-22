#include "autotune.hpp"
#include "hardware_profiler.hpp"
#include "model_scanner.hpp"
#include "ggml.h"
#include "gguf.h"
#include "ggml-backend.h"

#ifdef GGML_USE_CUDA
#include "ggml-cuda.h"
#include <cuda_runtime.h>
#endif

#include "../../ggml/src/ggml-cuda/sandwich-kernel-cache.cuh"

#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>

namespace sandwich {
namespace autotune {

static int64_t get_kv_i64(const struct gguf_context * ctx, int64_t key_id) {
    if (key_id < 0) return 0;
    const enum gguf_type type = gguf_get_kv_type(ctx, key_id);
    switch (type) {
        case GGUF_TYPE_UINT8:   return gguf_get_val_u8(ctx, key_id);
        case GGUF_TYPE_INT8:    return gguf_get_val_i8(ctx, key_id);
        case GGUF_TYPE_UINT16:  return gguf_get_val_u16(ctx, key_id);
        case GGUF_TYPE_INT16:   return gguf_get_val_i16(ctx, key_id);
        case GGUF_TYPE_UINT32:  return gguf_get_val_u32(ctx, key_id);
        case GGUF_TYPE_INT32:   return gguf_get_val_i32(ctx, key_id);
        case GGUF_TYPE_UINT64:  return (int64_t) gguf_get_val_u64(ctx, key_id);
        case GGUF_TYPE_INT64:   return gguf_get_val_i64(ctx, key_id);
        default:                return 0;
    }
}

TuneResult run_autotune(const TuneOptions & opts) {
    if (sandwich::autotune::env_flag("SANDWICH_AUTOTUNE_DISABLE")) {
        std::cout << "🥪 [Autotune] Autotuning is disabled via environment.\n";
        return {0, 0, 0.0};
    }

    if (opts.force || sandwich::autotune::env_flag("SANDWICH_AUTOTUNE_INVALIDATE")) {
        Cache::instance().clear();
    }

#ifdef GGML_USE_CUDA
    if (ggml_backend_cuda_get_device_count() == 0) {
        std::cout << "🥪 [Autotune] No CUDA devices available for autotuning.\n";
        return {0, 0, 0.0};
    }

    ggml_backend_t backend = ggml_backend_cuda_init(0);
    if (!backend) {
        std::cout << "🥪 [Autotune] Failed to initialize CUDA backend device 0.\n";
        return {0, 0, 0.0};
    }

    cudaDeviceProp prop;
    if (cudaGetDeviceProperties(&prop, 0) != cudaSuccess) {
        std::cout << "🥪 [Autotune] Failed to read CUDA device properties.\n";
        ggml_backend_free(backend);
        return {0, 0, 0.0};
    }
    const int cc_major = prop.major;
    const int cc_minor = prop.minor;

    int head_dim = 128;
    int gqa_ratio = 4;
    int nh = 8;
    int k_type = GGML_TYPE_F16;
    int v_type = GGML_TYPE_F16;

    namespace fs = std::filesystem;
    std::error_code ec;
    if (!opts.model_path.empty() && fs::exists(opts.model_path, ec)) {
        struct gguf_init_params gparams = { true, nullptr };
        struct gguf_context * gctx = gguf_init_from_file(opts.model_path.c_str(), gparams);
        if (gctx) {
            std::string arch = "unknown";
            int64_t aid = gguf_find_key(gctx, "general.architecture");
            if (aid >= 0 && gguf_get_kv_type(gctx, aid) == GGUF_TYPE_STRING) {
                arch = gguf_get_val_str(gctx, aid);
            }
            if (!arch.empty() && arch != "unknown") {
                int64_t kid = gguf_find_key(gctx, (arch + ".attention.key_length").c_str());
                if (kid >= 0) head_dim = (int)get_kv_i64(gctx, kid);
                int64_t h_id = gguf_find_key(gctx, (arch + ".attention.head_count").c_str());
                int64_t hkv_id = gguf_find_key(gctx, (arch + ".attention.head_count_kv").c_str());
                int64_t h = (h_id >= 0) ? get_kv_i64(gctx, h_id) : 32;
                int64_t hkv = (hkv_id >= 0) ? get_kv_i64(gctx, hkv_id) : 8;
                if (hkv > 0) {
                    nh = (int)hkv;
                    gqa_ratio = (int)(h / hkv);
                }
            }
            gguf_free(gctx);
        }
    }

    struct Shape {
        int n_queries;
        int kv_len;
    };
    const std::vector<Shape> shapes = {
        {1, 512},
        {1, 1024},
        {1, 2048},
        {1, 4096},
        {4, 512},
        {4, 1024},
        {4, 2048},
        {4, 4096},
        {7, 512},
        {7, 1024},
        {7, 2048},
        {7, 4096}
    };

    cudaEvent_t ev_start, ev_stop;
    cudaEventCreate(&ev_start);
    cudaEventCreate(&ev_stop);

    auto start_time = std::chrono::steady_clock::now();
    int shapes_tuned = 0;
    int winners_cached = 0;

    for (const auto & s : shapes) {
        const int n_queries = s.n_queries;
        const int kv_len    = s.kv_len;
        const int n_q_heads = nh * gqa_ratio;
        const float scale   = 1.0f / std::sqrt((float)head_dim);

        struct ggml_init_params p = { 64 * 1024 * 1024, NULL, true };
        ggml_context * ctx = ggml_init(p);
        if (!ctx) continue;

        ggml_tensor * q = ggml_new_tensor_4d(ctx, GGML_TYPE_F32, head_dim, n_queries, n_q_heads, 1);
        ggml_tensor * k = ggml_new_tensor_4d(ctx, (ggml_type)k_type, head_dim, kv_len, nh, 1);
        ggml_tensor * v = ggml_new_tensor_4d(ctx, (ggml_type)v_type, head_dim, kv_len, nh, 1);
        ggml_tensor * m = ggml_new_tensor_4d(ctx, GGML_TYPE_F16, kv_len, n_queries, 1, 1);

        ggml_tensor * out = ggml_flash_attn_ext(ctx, q, k, v, m, scale, 0.0f, 0.0f);
        ggml_cgraph * gf = ggml_new_graph(ctx);
        ggml_build_forward_expand(gf, out);

        ggml_backend_buffer_t buf = ggml_backend_alloc_ctx_tensors(ctx, backend);
        if (!buf) {
            ggml_free(ctx);
            continue;
        }
        ggml_backend_buffer_clear(buf, 0);

        const uint64_t fp = sandwich::autotune::fingerprint_fattn(
            cc_major, cc_minor,
            head_dim, n_queries, gqa_ratio,
            k_type, v_type,
            kv_len, true, false
        );

        const int ITERS_PER_TRIAL = 200;

        // Helper lambda to compute trimmed mean and sigma over 7 trials (dropping min and max)
        auto compute_stats = [](std::vector<double> vals, double & out_mu, double & out_sigma) {
            if (vals.size() < 3) {
                out_mu = 0.0; out_sigma = 0.0;
                for (double v : vals) out_mu += v;
                if (!vals.empty()) out_mu /= vals.size();
                return;
            }
            std::sort(vals.begin(), vals.end());
            // drop 1 lowest and 1 highest outlier
            const size_t start = 1;
            const size_t end = vals.size() - 1;
            const size_t count = end - start;
            out_mu = 0.0;
            for (size_t i = start; i < end; ++i) out_mu += vals[i];
            out_mu /= count;
            double var = 0.0;
            for (size_t i = start; i < end; ++i) {
                double diff = vals[i] - out_mu;
                var += diff * diff;
            }
            out_sigma = std::sqrt(var / (count > 1 ? count - 1 : 1)) / std::sqrt((double)count);
        };

        // 1. Measure heuristic baseline (trial override = 0)
        Cache::instance().set_override(family::fattn, fp, 0);
        for (int w = 0; w < 3; ++w) {
            for (int rep = 0; rep < ITERS_PER_TRIAL; ++rep) {
                ggml_backend_graph_compute(backend, gf);
            }
        }
        cudaDeviceSynchronize();

        std::vector<double> heur_times;
        for (int t = 0; t < 7; ++t) {
            cudaEventRecord(ev_start, 0);
            for (int rep = 0; rep < ITERS_PER_TRIAL; ++rep) {
                ggml_backend_graph_compute(backend, gf);
            }
            cudaEventRecord(ev_stop, 0);
            cudaEventSynchronize(ev_stop);
            float ms = 0.0f;
            cudaEventElapsedTime(&ms, ev_start, ev_stop);
            heur_times.push_back(((double)ms * 1000.0) / ITERS_PER_TRIAL);
        }

        double mu_heur = 0.0;
        double sigma_heur = 0.0;
        compute_stats(heur_times, mu_heur, sigma_heur);

        // 2. Measure candidate variants
        const int candidate_variants[] = {
            100, // BEST_FATTN_KERNEL_VEC
            200, // BEST_FATTN_KERNEL_TILE
            400, // BEST_FATTN_KERNEL_MMA_F16
            500  // BEST_FATTN_KERNEL_MMA_NATIVE
        };

        int best_variant = 0;
        double best_cand_mu = 1e9;
        double best_cand_sigma = 0.0;

        for (int var_id : candidate_variants) {
            Cache::instance().set_override(family::fattn, fp, var_id);

            for (int w = 0; w < 3; ++w) {
                for (int rep = 0; rep < ITERS_PER_TRIAL; ++rep) {
                    ggml_backend_graph_compute(backend, gf);
                }
            }
            cudaDeviceSynchronize();

            std::vector<double> cand_times;
            for (int t = 0; t < 7; ++t) {
                cudaEventRecord(ev_start, 0);
                for (int rep = 0; rep < ITERS_PER_TRIAL; ++rep) {
                    ggml_backend_graph_compute(backend, gf);
                }
                cudaEventRecord(ev_stop, 0);
                cudaEventSynchronize(ev_stop);
                float ms = 0.0f;
                cudaEventElapsedTime(&ms, ev_start, ev_stop);
                cand_times.push_back(((double)ms * 1000.0) / ITERS_PER_TRIAL);
            }

            double mu_c = 0.0;
            double sigma_c = 0.0;
            compute_stats(cand_times, mu_c, sigma_c);

            if (mu_c < best_cand_mu) {
                best_cand_mu = mu_c;
                best_cand_sigma = sigma_c;
                best_variant = var_id;
            }
        }

        Cache::instance().set_override(family::fattn, fp, 0);

        // 3. Store winner only when it beats the heuristic by >2σ
        const double sigma_comb = std::sqrt(best_cand_sigma * best_cand_sigma + sigma_heur * sigma_heur);
        if (best_cand_mu < mu_heur && (mu_heur - best_cand_mu) > (2.0 * sigma_comb)) {
            entry e;
            e.fingerprint = fp;
            e.variant     = best_variant;
            e.ncols1      = 0;
            e.ncols2      = 0;
            e.us_winner   = best_cand_mu;
            e.us_baseline = mu_heur;
            Cache::instance().store(family::fattn, e);
            winners_cached++;
        }

        shapes_tuned++;
        ggml_backend_buffer_free(buf);
        ggml_free(ctx);
    }

    cudaEventDestroy(ev_start);
    cudaEventDestroy(ev_stop);
    ggml_backend_free(backend);

    auto end_time = std::chrono::steady_clock::now();
    double elapsed_sec = std::chrono::duration<double>(end_time - start_time).count();

    std::cout << "Autotuned " << shapes_tuned << " shapes in "
              << std::fixed << std::setprecision(1) << elapsed_sec << "s, cached "
              << winners_cached << " winners\n";

    return {shapes_tuned, winners_cached, elapsed_sec};
#else
    std::cout << "Autotuned 0 shapes in 0.0s, cached 0 winners (CUDA not enabled)\n";
    return {0, 0, 0.0};
#endif
}

void ensure_autotuned(const std::string & model_path, int ctx_size) {
    if (!Cache::instance().enabled()) {
        return;
    }
    if (Cache::instance().size() > 0) {
        return;
    }
    TuneOptions opts;
    opts.model_path = model_path;
    opts.ctx_size = ctx_size;
    run_autotune(opts);
}

} // namespace autotune
} // namespace sandwich
