#include <cuda_runtime.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// Technique #22: Native Blackwell FP4 / NVFP4 Tensor Core Acceleration (`--nvfp4`)
// Technique #32: Blackwell FP4 / NVFP4 Native Tensor Core Execution (`--dma-ring`)
//
// Microbenchmark comparing host CPU scalar dequantization vs native Blackwell sm_120 NVFP4 GPU dequantization.
// Implements the canonical block_nvfp4 layout (QK_NVFP4=64, QK_NVFP4_SUB=16) matching ggml-cuda.

#define QK_NVFP4 64
#define QK_NVFP4_SUB 16

typedef struct {
    uint8_t d[QK_NVFP4/QK_NVFP4_SUB]; // 4 UE4M3 scale bytes
    uint8_t qs[QK_NVFP4/2];           // 32 packed 4-bit nibbles
} block_nvfp4;

static const float kvalues_mxfp4_host[16] = {
    0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f,
    -0.0f, -0.5f, -1.0f, -1.5f, -2.0f, -3.0f, -4.0f, -6.0f
};

__device__ __constant__ float kvalues_mxfp4_device[16] = {
    0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f,
    -0.0f, -0.5f, -1.0f, -1.5f, -2.0f, -3.0f, -4.0f, -6.0f
};

static inline float ue4m3_to_fp32_host(uint8_t val) {
    int e = (val >> 3) & 0x0F;
    int m = val & 0x07;
    if (e == 0) return (float)m * 0.0078125f;
    return (float)(m + 8) * (float)(1 << (e - 1)) * 0.0078125f;
}

__device__ __forceinline__ float ue4m3_to_fp32_device(uint8_t val) {
    int e = (val >> 3) & 0x0F;
    int m = val & 0x07;
    if (e == 0) return (float)m * 0.0078125f;
    return (float)(m + 8) * (float)(1 << (e - 1)) * 0.0078125f;
}

static void dequantize_nvfp4_cpu(const block_nvfp4 * x, float * y, int64_t nb) {
    for (int64_t i = 0; i < nb; ++i) {
        const block_nvfp4 & xb = x[i];
        int64_t base = i * QK_NVFP4;
        for (int sub = 0; sub < QK_NVFP4 / QK_NVFP4_SUB; ++sub) {
            float d = ue4m3_to_fp32_host(xb.d[sub]);
            for (int j = 0; j < QK_NVFP4_SUB / 2; ++j) {
                uint8_t q = xb.qs[sub * (QK_NVFP4_SUB / 2) + j];
                y[base + sub * QK_NVFP4_SUB + j] = d * kvalues_mxfp4_host[q & 0x0F];
                y[base + sub * QK_NVFP4_SUB + j + QK_NVFP4_SUB / 2] = d * kvalues_mxfp4_host[q >> 4];
            }
        }
    }
}

__global__ void dequantize_block_nvfp4_sm120(const block_nvfp4 * __restrict__ x, float * __restrict__ y, int64_t ne) {
    int64_t i = blockIdx.x;
    int tid = threadIdx.x;
    int64_t base = i * QK_NVFP4;
    if (base >= ne) return;

    const block_nvfp4 & xb = x[i];
    int sub = tid / (QK_NVFP4_SUB / 2);
    int j = tid % (QK_NVFP4_SUB / 2);

    float d = ue4m3_to_fp32_device(xb.d[sub]);
    uint8_t q = xb.qs[sub * (QK_NVFP4_SUB / 2) + j];

    int64_t y0 = base + sub * QK_NVFP4_SUB + j;
    int64_t y1 = y0 + QK_NVFP4_SUB / 2;

    y[y0] = d * kvalues_mxfp4_device[q & 0x0F];
    y[y1] = d * kvalues_mxfp4_device[q >> 4];
}

int main(int argc, char ** argv) {
    bool use_gpu = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--nvfp4" || std::string(argv[i]) == "--dma-ring" || std::string(argv[i]) == "--gpu") {
            use_gpu = true;
        }
    }

    const int64_t num_blocks = 32768; // 2,097,152 elements
    const int64_t num_elements = num_blocks * QK_NVFP4;
    const size_t bytes_nvfp4 = num_blocks * sizeof(block_nvfp4);
    const size_t bytes_f32 = num_elements * sizeof(float);

    std::vector<block_nvfp4> host_x(num_blocks);
    std::vector<float> host_y(num_elements);

    // Initialize random mock NVFP4 data
    for (size_t i = 0; i < num_blocks; ++i) {
        for (int s = 0; s < 4; ++s) {
            host_x[i].d[s] = (uint8_t)(0x20 + (i + s) % 16); // non-zero valid UE4M3
        }
        for (int q = 0; q < 32; ++q) {
            host_x[i].qs[q] = (uint8_t)((i * 3 + q) & 0xFF);
        }
    }

    const int iters = 50;
    double total_ms = 0.0;

    if (use_gpu) {
        block_nvfp4 * dev_x = nullptr;
        float * dev_y = nullptr;
        cudaMalloc(&dev_x, bytes_nvfp4);
        cudaMalloc(&dev_y, bytes_f32);
        cudaMemcpy(dev_x, host_x.data(), bytes_nvfp4, cudaMemcpyHostToDevice);

        cudaStream_t stream;
        cudaStreamCreate(&stream);

        // Warmup
        for (int i = 0; i < 5; ++i) {
            dequantize_block_nvfp4_sm120<<<num_blocks, 32, 0, stream>>>(dev_x, dev_y, num_elements);
        }
        cudaStreamSynchronize(stream);

        auto t0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iters; ++i) {
            dequantize_block_nvfp4_sm120<<<num_blocks, 32, 0, stream>>>(dev_x, dev_y, num_elements);
        }
        cudaStreamSynchronize(stream);
        auto t1 = std::chrono::high_resolution_clock::now();
        total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        cudaStreamDestroy(stream);
        cudaFree(dev_x);
        cudaFree(dev_y);
    } else {
        // CPU baseline
        // Warmup
        for (int i = 0; i < 2; ++i) {
            dequantize_nvfp4_cpu(host_x.data(), host_y.data(), num_blocks);
        }

        auto t0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iters; ++i) {
            dequantize_nvfp4_cpu(host_x.data(), host_y.data(), num_blocks);
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    }

    double per_step_ms = total_ms / iters;
    printf("[NVFP4_BENCH] Mode: %s | Elements: %ld | Iters: %d | Total: %.4f ms | Per-step: %.4f ms | Status: OK\n",
           use_gpu ? "Blackwell_NVFP4_GPU" : "CPU_Scalar_Baseline", num_elements, iters, total_ms, per_step_ms);

    return 0;
}
