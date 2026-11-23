// Unit test for sandwich-engine fused vec_dot nrc bit-exactness.
//
// Verifies that for IQ3_S and IQ2_S, fused execution with nrc in {1, 2, 3, 4, 5, 8}
// produces bit-identical results (difference strictly 0.0) compared to invoking
// the scalar nrc=1 path across each column individually.
//
// Exercises both power-of-two fused fast paths (nrc=2, 4, 8) and odd-column
// fallback loops (nrc=3, 5).

#include "ggml.h"
#include "ggml-cpu.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define NRC_CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::fprintf(stderr, "TEST ASSERTION FAILED: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while (0)

static void test_type_nrc(ggml_type type, const char * type_name) {
    ggml_quantize_init(type);
    const auto * qfns_cpu = ggml_get_type_traits_cpu(type);
    NRC_CHECK(qfns_cpu != nullptr);
    NRC_CHECK(qfns_cpu->vec_dot != nullptr);

    const ggml_type vdot_type = qfns_cpu->vec_dot_type;
    const auto * vdot_fns_cpu = ggml_get_type_traits_cpu(vdot_type);
    NRC_CHECK(vdot_fns_cpu != nullptr);
    NRC_CHECK(vdot_fns_cpu->from_float != nullptr);

    const int k_sizes[] = {256, 512, 1024, 4096};
    const int nrc_values[] = {1, 2, 3, 4, 5, 8};
    const size_t bs_values[] = {1, 4, 16};

    for (int k : k_sizes) {
        const size_t row_size_x = ggml_row_size(type, k);
        const size_t row_size_y = ggml_row_size(vdot_type, k);

        std::vector<float> f_data_x(k);
        for (int i = 0; i < k; ++i) {
            f_data_x[i] = 0.1f + 2.0f * std::cos((float)i * 0.13f);
        }

        std::vector<uint8_t> x_bytes(row_size_x);
        auto from_float_x = qfns_cpu->from_float;
        if (!from_float_x) {
            const auto * qfns = ggml_get_type_traits(type);
            if (qfns) from_float_x = qfns->from_float_ref;
        }
        if (from_float_x) {
            from_float_x(f_data_x.data(), x_bytes.data(), k);
        } else {
            for (size_t i = 0; i < row_size_x; ++i) {
                x_bytes[i] = (uint8_t)((i * 37 + 13) & 0xff);
            }
            for (int b = 0; b < k / 256; ++b) {
                uint16_t * d_ptr = (uint16_t *)(x_bytes.data() + b * (row_size_x / (k / 256)));
                *d_ptr = 0x3c00;
            }
        }

        const int max_nrc = 8;
        std::vector<uint8_t> y_bytes(row_size_y * max_nrc);
        for (int c = 0; c < max_nrc; ++c) {
            std::vector<float> f_col(k);
            for (int i = 0; i < k; ++i) {
                f_col[i] = 0.05f + 1.8f * std::sin((float)(i * 0.17f + c * 23));
            }
            vdot_fns_cpu->from_float(f_col.data(), y_bytes.data() + c * row_size_y, k);
        }

        for (int nrc : nrc_values) {
            for (size_t bs : bs_values) {
                std::vector<float> ref(nrc, 0.0f);
                for (int c = 0; c < nrc; ++c) {
                    qfns_cpu->vec_dot(k, &ref[c], 0, x_bytes.data(), 0, y_bytes.data() + c * row_size_y, 0, 1);
                }

                std::vector<float> fused(bs * nrc, -999.0f);
                qfns_cpu->vec_dot(k, fused.data(), bs, x_bytes.data(), 0, y_bytes.data(), row_size_y, nrc);

                for (int c = 0; c < nrc; ++c) {
                    float val_fused = fused[c * bs];
                    float val_ref   = ref[c];
                    float diff = std::fabs(val_fused - val_ref);
                    if (diff != 0.0f) {
                        std::fprintf(stderr,
                            "MISMATCH [%s]: k=%d, nrc=%d, bs=%zu, col=%d: fused=%e, ref=%e, diff=%e\n",
                            type_name, k, nrc, bs, c, val_fused, val_ref, diff);
                        NRC_CHECK(diff == 0.0f);
                    }
                }
            }
        }
    }
    std::printf("  [PASS] %s bit-exact across k in {256,512,1024,4096}, nrc in {1,2,3,4,5,8}, bs in {1,4,16}\n", type_name);
}

int main() {
    std::printf("[test-sandwich-nrc] Verifying fused vec_dot bit-exactness...\n");
    test_type_nrc(GGML_TYPE_IQ3_S, "IQ3_S");
    test_type_nrc(GGML_TYPE_IQ2_S, "IQ2_S");
    std::printf("[test-sandwich-nrc] ALL TESTS PASSED (diff strictly 0.0).\n");
    return 0;
}
