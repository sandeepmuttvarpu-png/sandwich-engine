#include "ggml.h"
#include "ggml-backend.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

// Technique #11: Native FlashMLA Attention Kernel (`latent_attn.cu`)
// Technique #46: Latent FlashMLA Direct Tensor Core MMA
//
// Microbenchmark comparing standard CPU reference execution against FlashMLA GPU Tensor Core MMA.
// Uses DeepSeek V2/V3 low-rank latent dimensions: Dk=576, T=16, H=4, N=256, P=16, dv=512.

static void fill_random_f32(float * data, size_t n) {
    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (size_t i = 0; i < n; ++i) {
        data[i] = dist(rng);
    }
}

int main(int argc, char ** argv) {
    bool use_gpu = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--flash-mla" || std::string(argv[i]) == "--gpu") {
            use_gpu = true;
        }
    }

    ggml_backend_load_all();
    ggml_backend_t backend = use_gpu 
        ? ggml_backend_init_by_type(GGML_BACKEND_DEVICE_TYPE_GPU, NULL)
        : ggml_backend_init_by_type(GGML_BACKEND_DEVICE_TYPE_CPU, NULL);

    if (!backend) {
        fprintf(stderr, "Failed to initialize backend (use_gpu=%d)\n", use_gpu);
        return 1;
    }

    // DeepSeek V2/V3 low-rank latent attention parameters
    const int64_t Dk = 576;
    const int64_t T  = 16;
    const int64_t H  = 4;
    const int64_t N  = 256;
    const int64_t P  = 16;
    const int64_t dv = 512;
    const float scale = 1.0f / sqrtf((float)Dk);

    struct ggml_init_params params = { 128 * 1024 * 1024, NULL, true };
    struct ggml_context * ctx = ggml_init(params);

    struct ggml_tensor * q        = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, Dk, T, H);
    struct ggml_tensor * cache    = ggml_new_tensor_2d(ctx, GGML_TYPE_F16, Dk, N);
    struct ggml_tensor * prefix_k = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, Dk, P);
    struct ggml_tensor * prefix_v = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, P, dv);
    struct ggml_tensor * mask     = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, N, T);

    struct ggml_tensor * dst = ggml_latent_attn_prefix_ext(ctx, q, cache, prefix_k, prefix_v, mask, dv, 0, scale, 0.0f);
    struct ggml_cgraph * gf = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf, dst);

    ggml_backend_buffer_t buf = ggml_backend_alloc_ctx_tensors(ctx, backend);
    if (!buf) {
        fprintf(stderr, "Failed to allocate tensor buffer\n");
        ggml_free(ctx);
        ggml_backend_free(backend);
        return 1;
    }

    std::vector<float> q_host(ggml_nelements(q));
    fill_random_f32(q_host.data(), q_host.size());
    ggml_backend_tensor_set(q, q_host.data(), 0, ggml_nbytes(q));

    // Warmup iterations
    for (int i = 0; i < 3; ++i) {
        ggml_backend_graph_compute(backend, gf);
    }

    // Benchmark loop
    const int iters = 50;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iters; ++i) {
        ggml_backend_graph_compute(backend, gf);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double per_step_ms = total_ms / iters;

    printf("[FLASH_MLA] Mode: %s | Iters: %d | Total: %.4f ms | Per-step: %.4f ms | Status: OK\n",
           use_gpu ? "FlashMLA_TensorCore_GPU" : "CPU_Scalar_Baseline", iters, total_ms, per_step_ms);

    ggml_backend_buffer_free(buf);
    ggml_free(ctx);
    ggml_backend_free(backend);
    return 0;
}
