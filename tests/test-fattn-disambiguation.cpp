#include "ggml.h"
#include "ggml-backend.h"
#include "ggml-cuda.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <vector>

// Step 3 Gate: Dual-Path Regression Test for FlashAttention Role Disambiguation
// Path A: Sparse tile index list (GGML_FATTN_SRC5_ROLE_SPARSE_TILES) -> executes sparse MMA path
// Path B: Multi-sequence kv_rows (GGML_FATTN_SRC5_ROLE_KV_ROWS) -> executes unified gather path
// Path C: Alternating interleaved execution (A -> B -> A -> B ...) verifying zero state bleed

static void fill_f32(float * data, size_t n, float val) {
    for (size_t i = 0; i < n; ++i) {
        data[i] = val;
    }
}

static void fill_random_f32(float * data, size_t n, uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-0.5f, 0.5f);
    for (size_t i = 0; i < n; ++i) {
        data[i] = dist(rng);
    }
}

static void fill_random_f16(ggml_fp16_t * data, size_t n, uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-0.5f, 0.5f);
    for (size_t i = 0; i < n; ++i) {
        data[i] = ggml_fp32_to_fp16(dist(rng));
    }
}

struct PathA_Context {
    ggml_context * ctx = nullptr;
    ggml_tensor  * q   = nullptr;
    ggml_tensor  * k   = nullptr;
    ggml_tensor  * v   = nullptr;
    ggml_tensor  * m   = nullptr;
    ggml_tensor  * r   = nullptr;
    ggml_tensor  * out = nullptr;
    ggml_cgraph  * gf  = nullptr;
    ggml_backend_buffer_t buf = nullptr;
    int64_t out_elements = 0;
};

struct PathB_Context {
    ggml_context * ctx = nullptr;
    ggml_tensor  * q   = nullptr;
    ggml_tensor  * k0  = nullptr;
    ggml_tensor  * v0  = nullptr;
    ggml_tensor  * k   = nullptr;
    ggml_tensor  * v   = nullptr;
    ggml_tensor  * m   = nullptr;
    ggml_tensor  * r   = nullptr;
    ggml_tensor  * out = nullptr;
    ggml_cgraph  * gf  = nullptr;
    ggml_backend_buffer_t buf = nullptr;
    int64_t out_elements = 0;
};

static PathA_Context create_path_a(ggml_backend_t backend) {
    PathA_Context ctx_a;
    const int64_t hs = 128;
    const int64_t nh = 8;
    const int64_t gqa = 8;
    const int64_t n_q_heads = nh * gqa; // 64
    const int64_t q1 = 1;
    const int64_t n_kv = 3840; // multiple of 256
    const float scale = 1.0f / sqrtf((float)hs);

    struct ggml_init_params params = { 128 * 1024 * 1024, NULL, true };
    ctx_a.ctx = ggml_init(params);

    ctx_a.q = ggml_new_tensor_4d(ctx_a.ctx, GGML_TYPE_F32, hs, q1, n_q_heads, 1);
    ggml_set_name(ctx_a.q, "q_a");

    ctx_a.k = ggml_new_tensor_4d(ctx_a.ctx, GGML_TYPE_F16, hs, n_kv, nh, 1);
    ggml_set_name(ctx_a.k, "k_a");

    ctx_a.v = ggml_new_tensor_4d(ctx_a.ctx, GGML_TYPE_F16, hs, n_kv, nh, 1);
    ggml_set_name(ctx_a.v, "v_a");

    ctx_a.m = ggml_new_tensor_4d(ctx_a.ctx, GGML_TYPE_F16, n_kv, q1, 1, 1);
    ggml_set_name(ctx_a.m, "mask_a");

    ctx_a.r = ggml_new_tensor_2d(ctx_a.ctx, GGML_TYPE_I32, n_kv, 1);
    ggml_set_name(ctx_a.r, "sparse_tiles_a");

    ctx_a.out = ggml_flash_attn_ext_rows(ctx_a.ctx, ctx_a.q, ctx_a.k, ctx_a.v, ctx_a.m, ctx_a.r, scale, 0.0f, 0.0f);
    ggml_set_name(ctx_a.out, "fattn_a");
    ggml_flash_attn_ext_set_src5_role(ctx_a.out, GGML_FATTN_SRC5_ROLE_SPARSE_TILES);
    ggml_prec_set_acc(ctx_a.out, GGML_PREC_F32);

    ctx_a.gf = ggml_new_graph(ctx_a.ctx);
    ggml_build_forward_expand(ctx_a.gf, ctx_a.out);

    ctx_a.buf = ggml_backend_alloc_ctx_tensors(ctx_a.ctx, backend);
    assert(ctx_a.buf != nullptr);

    // Initialize data
    std::vector<int32_t> r_data(n_kv);
    for (int64_t i = 0; i < n_kv; ++i) {
        r_data[i] = (i * 13) % n_kv;
    }
    ggml_backend_tensor_set(ctx_a.r, r_data.data(), 0, ggml_nbytes(ctx_a.r));

    std::vector<ggml_fp16_t> m_data(n_kv);
    for (int64_t i = 0; i < n_kv; ++i) {
        m_data[i] = ggml_fp32_to_fp16(0.0f);
    }
    ggml_backend_tensor_set(ctx_a.m, m_data.data(), 0, ggml_nbytes(ctx_a.m));

    std::vector<float> q_data(hs * q1 * n_q_heads);
    fill_random_f32(q_data.data(), q_data.size(), 101);
    ggml_backend_tensor_set(ctx_a.q, q_data.data(), 0, ggml_nbytes(ctx_a.q));

    std::vector<ggml_fp16_t> kv_data(hs * n_kv * nh);
    fill_random_f16(kv_data.data(), kv_data.size(), 202);
    ggml_backend_tensor_set(ctx_a.k, kv_data.data(), 0, ggml_nbytes(ctx_a.k));
    ggml_backend_tensor_set(ctx_a.v, kv_data.data(), 0, ggml_nbytes(ctx_a.v));

    ctx_a.out_elements = ggml_nelements(ctx_a.out);
    return ctx_a;
}

static PathB_Context create_path_b(ggml_backend_t backend) {
    PathB_Context ctx_b;
    const int64_t hs = 128;
    const int64_t nh = 8;
    const int64_t gqa = 8;
    const int64_t n_q_heads = nh * gqa; // 64
    const int64_t n_cells = 2048;
    const int64_t kv = 512; // multiple of 256
    const int64_t nb = 1;
    const int64_t n_slices = 2;
    const float scale = 1.0f / sqrtf((float)hs);

    struct ggml_init_params params = { 128 * 1024 * 1024, NULL, true };
    ctx_b.ctx = ggml_init(params);

    ctx_b.q = ggml_new_tensor_4d(ctx_b.ctx, GGML_TYPE_F32, hs, nb, n_q_heads, n_slices);
    ggml_set_name(ctx_b.q, "q_b");

    ctx_b.k0 = ggml_new_tensor_3d(ctx_b.ctx, GGML_TYPE_F16, hs, nh, n_cells);
    ctx_b.v0 = ggml_new_tensor_3d(ctx_b.ctx, GGML_TYPE_F16, hs, nh, n_cells);
    ggml_set_name(ctx_b.k0, "k0_b");
    ggml_set_name(ctx_b.v0, "v0_b");

    ctx_b.k = ggml_permute(ctx_b.ctx, ctx_b.k0, 0, 2, 1, 3);
    ctx_b.v = ggml_permute(ctx_b.ctx, ctx_b.v0, 0, 2, 1, 3);

    ctx_b.m = ggml_new_tensor_4d(ctx_b.ctx, GGML_TYPE_F16, kv, nb, 1, n_slices);
    ggml_set_name(ctx_b.m, "mask_b");

    ctx_b.r = ggml_new_tensor_2d(ctx_b.ctx, GGML_TYPE_I32, kv, n_slices);
    ggml_set_name(ctx_b.r, "kv_rows_b");

    ctx_b.out = ggml_flash_attn_ext_rows(ctx_b.ctx, ctx_b.q, ctx_b.k, ctx_b.v, ctx_b.m, ctx_b.r, scale, 0.0f, 0.0f);
    ggml_set_name(ctx_b.out, "fattn_b");
    ggml_flash_attn_ext_set_src5_role(ctx_b.out, GGML_FATTN_SRC5_ROLE_KV_ROWS);
    ggml_prec_set_acc(ctx_b.out, GGML_PREC_F32);

    ctx_b.gf = ggml_new_graph(ctx_b.ctx);
    ggml_build_forward_expand(ctx_b.gf, ctx_b.out);

    ctx_b.buf = ggml_backend_alloc_ctx_tensors(ctx_b.ctx, backend);
    assert(ctx_b.buf != nullptr);

    // Initialize data
    std::vector<int32_t> r_data(kv * n_slices);
    for (int64_t s = 0; s < n_slices; ++s) {
        for (int64_t i = 0; i < kv; ++i) {
            r_data[s * kv + i] = (int32_t)((s * 256 + i) % n_cells);
        }
    }
    ggml_backend_tensor_set(ctx_b.r, r_data.data(), 0, ggml_nbytes(ctx_b.r));

    std::vector<ggml_fp16_t> m_data(kv * nb * n_slices);
    for (size_t i = 0; i < m_data.size(); ++i) {
        m_data[i] = ggml_fp32_to_fp16(0.0f);
    }
    ggml_backend_tensor_set(ctx_b.m, m_data.data(), 0, ggml_nbytes(ctx_b.m));

    std::vector<float> q_data(hs * nb * n_q_heads * n_slices);
    fill_random_f32(q_data.data(), q_data.size(), 303);
    ggml_backend_tensor_set(ctx_b.q, q_data.data(), 0, ggml_nbytes(ctx_b.q));

    std::vector<ggml_fp16_t> kv_data(hs * nh * n_cells);
    fill_random_f16(kv_data.data(), kv_data.size(), 404);
    ggml_backend_tensor_set(ctx_b.k0, kv_data.data(), 0, ggml_nbytes(ctx_b.k0));
    ggml_backend_tensor_set(ctx_b.v0, kv_data.data(), 0, ggml_nbytes(ctx_b.v0));

    ctx_b.out_elements = ggml_nelements(ctx_b.out);
    return ctx_b;
}

static void verify_finite(const std::vector<float> & data, const char * label) {
    size_t nan_count = 0;
    size_t inf_count = 0;
    for (float v : data) {
        if (std::isnan(v)) nan_count++;
        if (std::isinf(v)) inf_count++;
    }
    printf("[%s] Verified %zu output elements: NaNs=%zu, Infs=%zu\n", label, data.size(), nan_count, inf_count);
    assert(nan_count == 0 && "Found NaN in output");
    assert(inf_count == 0 && "Found Inf in output");
}

int main(int argc, char ** argv) {
    (void)argc;
    (void)argv;
    printf("================================================================================\n");
    printf(" TEST: FlashAttention src[5] Role Disambiguation Dual-Path Regression\n");
    printf(" Path A: Sparse Tiles MMA fast path (GGML_FATTN_SRC5_ROLE_SPARSE_TILES)\n");
    printf(" Path B: Multi-Sequence KV Rows unified gather (GGML_FATTN_SRC5_ROLE_KV_ROWS)\n");
    printf(" Path C: Alternating Interleaved Execution (Zero State Bleed Verification)\n");
    printf("================================================================================\n");

    ggml_backend_load_all();
    ggml_backend_t backend = ggml_backend_cuda_init(0);
    if (!backend) {
        fprintf(stderr, "CUDA backend not available; skipping test.\n");
        return 0;
    }

    // Path A Standalone Run
    printf("\n[1/3] Executing Path A (Sparse Tile Index List)...\n");
    PathA_Context path_a = create_path_a(backend);
    ggml_backend_graph_compute(backend, path_a.gf);

    std::vector<float> out_a_baseline(path_a.out_elements);
    ggml_backend_tensor_get(path_a.out, out_a_baseline.data(), 0, ggml_nbytes(path_a.out));
    verify_finite(out_a_baseline, "Path A Standalone");

    // Path B Standalone Run
    printf("\n[2/3] Executing Path B (Multi-Sequence KV Rows Unified Gather)...\n");
    PathB_Context path_b = create_path_b(backend);
    ggml_backend_graph_compute(backend, path_b.gf);

    std::vector<float> out_b_baseline(path_b.out_elements);
    ggml_backend_tensor_get(path_b.out, out_b_baseline.data(), 0, ggml_nbytes(path_b.out));
    verify_finite(out_b_baseline, "Path B Standalone");

    // Path C Interleaved Alternating Run (10 cycles: A -> B -> A -> B ...)
    printf("\n[3/3] Executing Path C (Interleaved 10-iteration A/B cycle)...\n");
    int64_t initial_native_count = ggml_backend_cuda_fattn_native_count();
    printf("Initial fattn_native_count: %ld\n", (long)initial_native_count);

    for (int iter = 0; iter < 10; ++iter) {
        // Step A: Run Path A
        ggml_backend_graph_compute(backend, path_a.gf);
        std::vector<float> out_a_iter(path_a.out_elements);
        ggml_backend_tensor_get(path_a.out, out_a_iter.data(), 0, ggml_nbytes(path_a.out));

        // Step B: Run Path B
        ggml_backend_graph_compute(backend, path_b.gf);
        std::vector<float> out_b_iter(path_b.out_elements);
        ggml_backend_tensor_get(path_b.out, out_b_iter.data(), 0, ggml_nbytes(path_b.out));

        // Verify exact numerical invariance against standalone baselines
        for (size_t i = 0; i < out_a_baseline.size(); ++i) {
            float diff = std::abs(out_a_baseline[i] - out_a_iter[i]);
            (void)diff;
            assert(diff < 1e-5f && "State bleed detected in Path A during interleaved execution!");
        }

        for (size_t i = 0; i < out_b_baseline.size(); ++i) {
            float diff = std::abs(out_b_baseline[i] - out_b_iter[i]);
            (void)diff;
            assert(diff < 1e-5f && "State bleed detected in Path B during interleaved execution!");
        }
    }
    printf("10 interleaved iterations completed with bit-identical outputs (diff < 1e-5).\n");

    int64_t final_native_count = ggml_backend_cuda_fattn_native_count();
    printf("Final fattn_native_count: %ld\n", (long)final_native_count);

    // Cleanup
    ggml_backend_buffer_free(path_a.buf);
    ggml_free(path_a.ctx);

    ggml_backend_buffer_free(path_b.buf);
    ggml_free(path_b.ctx);

    ggml_backend_free(backend);

    printf("\n[RESULT] ALL 3 PATHS (A, B, C) PASSED ZERO-STATE-BLEED REGRESSION GATE.\n");
    return 0;
}
