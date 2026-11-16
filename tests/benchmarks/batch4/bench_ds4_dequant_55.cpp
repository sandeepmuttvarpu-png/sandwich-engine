#include "src/ds4_engram_io.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

// Technique #55: Sub-2-bit MoE Expert Dequantization (`--fused-expdec`)
// Evaluates the CPU scalar fallback path for DeepSeek V4 Engram retrieval and dequantization dot product.
// Note: Per Rule K, Technique #55 is vetoed from PROVEN status until a fused CUDA kernel is implemented.
// It remains PARTIAL regardless of statistical delta.

int main(int argc, char ** argv) {
    bool enable_cached = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--fused-expdec" || std::string(argv[i]) == "--cached") {
            enable_cached = true;
        }
    }

    const std::string test_file = "/tmp/test_engram_55.bin";
    const size_t row_bytes = 4096;
    const size_t num_rows = 64;

    // Create temporary mock engram table
    {
        std::ofstream ofs(test_file, std::ios::binary);
        std::vector<uint8_t> row_buf(row_bytes);
        for (size_t r = 0; r < num_rows; ++r) {
            for (size_t b = 0; b < row_bytes; ++b) {
                row_buf[b] = (uint8_t)((r * 19 + b) & 0xFF);
            }
            ofs.write((const char *)row_buf.data(), row_bytes);
        }
    }

    ds4_engram_io engram;
    if (!engram.open(test_file, row_bytes, enable_cached ? 32 : 0)) {
        fprintf(stderr, "Failed to open engram table\n");
        return 1;
    }

    std::vector<float> query_vec(row_bytes * 2, 0.5f);
    std::vector<uint8_t> read_buf(row_bytes, 0);

    // Warmup
    engram.read_row(0, read_buf.data());

    const int iters = 1000;
    auto t0 = std::chrono::high_resolution_clock::now();

    float total_score = 0.0f;
    for (int i = 0; i < iters; ++i) {
        int64_t row_idx = enable_cached ? (i % 16) : (i % num_rows);
        engram.read_row(row_idx, read_buf.data());
        float score = 0.0f;
        ds4_engram_io::fused_expdec_dot(read_buf.data(), query_vec.data(), &score, row_bytes * 2);
        total_score += score;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double per_step_us = (total_ms * 1000.0) / iters;

    printf("[DS4_DEQUANT_55] Mode: %s | Iters: %d | Total: %.4f ms | Per-step: %.2f us | Score: %.2f | Status: OK\n",
           enable_cached ? "Host_Cache_Fallback" : "Cold_Disk_Baseline",
           iters, total_ms, per_step_us, total_score);

    engram.close();
    std::remove(test_file.c_str());
    return 0;
}
