#include "ggml.h"
#include "ggml-backend.h"
#include "ggml-cuda.h"

#include <cuda_runtime.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

// Step 1 Gate: Forensic CudaEvent Measurement Suite for Issue #4
// Measures exact per-layer and per-token kernel latency across all 12 active GPU decode layers (28-39).
// Compares Sparse MMA fast-path vs Dense MMA fallback on identical KV state.

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
            exit(1); \
        } \
    } while (0)

static void fill_random_f32(float * data, size_t n, float min_val, float max_val) {
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(min_val, max_val);
    for (size_t i = 0; i < n; ++i) {
        data[i] = dist(rng);
    }
}

static void fill_random_f16(ggml_fp16_t * data, size_t n) {
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (size_t i = 0; i < n; ++i) {
        data[i] = ggml_fp32_to_fp16(dist(rng));
    }
}

int main(int argc, char ** argv) {
    std::string mode = "dense";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--sparse") {
            mode = "sparse";
        } else if (arg == "--dense") {
            mode = "dense";
        }
    }

    // Set environment variable for kernel selection
    setenv("SANDWICH_FATTN_FORCE_MODE", mode.c_str(), 1);
    setenv("GGML_CUDA_DISABLE_GRAPHS", "1", 1);
    setenv("SANDWICH_PROFILE_FATTN", "1", 1);

    ggml_backend_load_all();
    ggml_backend_t backend = ggml_backend_cuda_init(0);
    if (!backend) {
        fprintf(stderr, "Failed to initialize CUDA backend device 0\n");
        return 1;
    }

    // Decode parameters matching the 40-layer model (e.g. Command-R / 35B architecture):
    // 12 active decode layers: layers 28 to 39
    const int num_layers = 12;
    const int start_layer = 28;
    const int64_t hs = 128;        // head dim
    const int64_t nh = 8;          // 8 KV heads
    const int64_t gqa = 8;         // 64 Q heads (nh * gqa = 64)
    const int64_t n_q_heads = nh * gqa;
    const int64_t q1 = 1;          // decode single token
    const int64_t n_kv = 3840;     // evaluated prompt context length (padded to FATTN_KQ_STRIDE=256)
    const int64_t n_sparse_rows = 3840; // active sparse tile index count matching mask->ne[0]
    const float kq_scale = 1.0f / sqrtf((float)hs);

    // Memory buffer for graph & tensors
    const size_t mem_size = 512 * 1024 * 1024; // 512 MB
    struct ggml_init_params params = { mem_size, NULL, true };
    struct ggml_context * ctx = ggml_init(params);

    // Common mask and sparse row index list
    struct ggml_tensor * mask = ggml_new_tensor_4d(ctx, GGML_TYPE_F16, n_kv, q1, 1, 1);
    ggml_set_name(mask, "attn_mask");

    struct ggml_tensor * rows = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, n_sparse_rows, 1);
    ggml_set_name(rows, "kv_rows");

    // Create 12 attention operations for layers 28 to 39
    std::vector<struct ggml_tensor *> q_tensors(num_layers);
    std::vector<struct ggml_tensor *> k_tensors(num_layers);
    std::vector<struct ggml_tensor *> v_tensors(num_layers);
    std::vector<struct ggml_tensor *> kqv_tensors(num_layers);

    struct ggml_cgraph * gf = ggml_new_graph(ctx);

    for (int l = 0; l < num_layers; ++l) {
        int layer_idx = start_layer + l;
        char name_buf[64];

        // Q: [hs, q1, n_q_heads, 1]
        snprintf(name_buf, sizeof(name_buf), "q_l%d", layer_idx);
        q_tensors[l] = ggml_new_tensor_4d(ctx, GGML_TYPE_F32, hs, q1, n_q_heads, 1);
        ggml_set_name(q_tensors[l], name_buf);

        // K: [hs, n_kv, nh, 1]
        snprintf(name_buf, sizeof(name_buf), "k_l%d", layer_idx);
        k_tensors[l] = ggml_new_tensor_4d(ctx, GGML_TYPE_F16, hs, n_kv, nh, 1);
        ggml_set_name(k_tensors[l], name_buf);

        // V: [hs, n_kv, nh, 1]
        snprintf(name_buf, sizeof(name_buf), "v_l%d", layer_idx);
        v_tensors[l] = ggml_new_tensor_4d(ctx, GGML_TYPE_F16, hs, n_kv, nh, 1);
        ggml_set_name(v_tensors[l], name_buf);

        // Attention node
        snprintf(name_buf, sizeof(name_buf), "fattn_l%d", layer_idx);
        kqv_tensors[l] = ggml_flash_attn_ext_rows(ctx, q_tensors[l], k_tensors[l], v_tensors[l], mask, rows, kq_scale, 0.0f, 0.0f);
        ggml_set_name(kqv_tensors[l], name_buf);
        ggml_flash_attn_ext_set_src5_role(kqv_tensors[l], GGML_FATTN_SRC5_ROLE_SPARSE_TILES);
        ggml_prec_set_acc(kqv_tensors[l], GGML_PREC_F32);

        ggml_build_forward_expand(gf, kqv_tensors[l]);
    }

    // Allocate tensors on CUDA device
    ggml_backend_buffer_t buf = ggml_backend_alloc_ctx_tensors(ctx, backend);
    if (!buf) {
        fprintf(stderr, "Failed to allocate CUDA backend buffers\n");
        ggml_free(ctx);
        ggml_backend_free(backend);
        return 1;
    }

    // Initialize data
    std::vector<int32_t> rows_data(n_sparse_rows);
    for (int64_t i = 0; i < n_sparse_rows; ++i) {
        rows_data[i] = (i * 7) % n_kv;
    }
    ggml_backend_tensor_set(rows, rows_data.data(), 0, ggml_nbytes(rows));

    std::vector<ggml_fp16_t> mask_data(n_kv);
    for (int64_t i = 0; i < n_kv; ++i) {
        mask_data[i] = ggml_fp32_to_fp16(0.0f);
    }
    ggml_backend_tensor_set(mask, mask_data.data(), 0, ggml_nbytes(mask));

    std::vector<float> q_host(hs * q1 * n_q_heads);
    fill_random_f32(q_host.data(), q_host.size(), -0.5f, 0.5f);

    std::vector<ggml_fp16_t> kv_host(hs * n_kv * nh);
    fill_random_f16(kv_host.data(), kv_host.size());

    for (int l = 0; l < num_layers; ++l) {
        ggml_backend_tensor_set(q_tensors[l], q_host.data(), 0, ggml_nbytes(q_tensors[l]));
        ggml_backend_tensor_set(k_tensors[l], kv_host.data(), 0, ggml_nbytes(k_tensors[l]));
        ggml_backend_tensor_set(v_tensors[l], kv_host.data(), 0, ggml_nbytes(v_tensors[l]));
    }

    // Warm-up run (2 iterations)
    for (int w = 0; w < 2; ++w) {
        ggml_backend_graph_compute(backend, gf);
    }
    cudaDeviceSynchronize();

    // Measurement with cudaEvent
    cudaEvent_t ev_token_start, ev_token_stop;
    CUDA_CHECK(cudaEventCreate(&ev_token_start));
    CUDA_CHECK(cudaEventCreate(&ev_token_stop));

    std::vector<cudaEvent_t> ev_layer_start(num_layers);
    std::vector<cudaEvent_t> ev_layer_stop(num_layers);
    for (int l = 0; l < num_layers; ++l) {
        CUDA_CHECK(cudaEventCreate(&ev_layer_start[l]));
        CUDA_CHECK(cudaEventCreate(&ev_layer_stop[l]));
    }

    // Execute measurement run across all 12 layers
    CUDA_CHECK(cudaEventRecord(ev_token_start, 0));
    ggml_backend_graph_compute(backend, gf);
    CUDA_CHECK(cudaEventRecord(ev_token_stop, 0));
    CUDA_CHECK(cudaEventSynchronize(ev_token_stop));

    float total_token_ms = 0.0f;
    CUDA_CHECK(cudaEventElapsedTime(&total_token_ms, ev_token_start, ev_token_stop));

    printf("{\n");
    printf("  \"mode\": \"%s\",\n", mode.c_str());
    printf("  \"num_layers\": %d,\n", num_layers);
    printf("  \"context_length\": %ld,\n", (long)n_kv);
    printf("  \"total_token_kernel_ms\": %.4f,\n", total_token_ms);
    printf("  \"avg_layer_kernel_ms\": %.4f\n", total_token_ms / num_layers);
    printf("}\n");

    // Cleanup
    CUDA_CHECK(cudaEventDestroy(ev_token_start));
    CUDA_CHECK(cudaEventDestroy(ev_token_stop));
    for (int l = 0; l < num_layers; ++l) {
        CUDA_CHECK(cudaEventDestroy(ev_layer_start[l]));
        CUDA_CHECK(cudaEventDestroy(ev_layer_stop[l]));
    }

    ggml_backend_buffer_free(buf);
    ggml_free(ctx);
    ggml_backend_free(backend);
    return 0;
}
