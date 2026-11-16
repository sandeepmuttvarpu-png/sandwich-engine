#include <cuda_runtime.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <thread>
#include <future>
#include <numeric>

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
            exit(1); \
        } \
    } while (0)

// Technique #5 [MICROBENCHMARK]: Asynchronous CPU Cold-Expert Execution (`sched_async_cpu`)
// Evaluates concurrent overlap of CPU cold-expert evaluation and GPU hot-expert kernel execution:
// Arm A: Synchronous CPU execution (serial execution: T_cpu + T_gpu)
// Arm B: Asynchronous CPU execution on background worker thread (overlapped: max(T_cpu, T_gpu) + sync)

static void cpu_cold_expert_gemm(const float * a, const float * b, float * c, int n) {
    // Simulate matrix multiplication on CPU for cold-expert routing
    for (int i = 0; i < n; ++i) {
        float sum = 0.0f;
        for (int k = 0; k < n; ++k) {
            sum += a[i * n + k] * b[k * n + i];
        }
        c[i] = sum;
    }
}

int main(int argc, char ** argv) {
    bool use_async = false;
    int dim = 512;
    int iters = 20;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--async" || arg == "--sched-async-cpu") {
            use_async = true;
        } else if (arg == "--sync" || arg == "--no-sched-async-cpu") {
            use_async = false;
        } else if (arg == "--dim" && i + 1 < argc) {
            dim = std::stoi(argv[++i]);
        } else if (arg == "--iters" && i + 1 < argc) {
            iters = std::stoi(argv[++i]);
        }
    }

    std::vector<float> h_a(dim * dim, 0.01f);
    std::vector<float> h_b(dim * dim, 0.02f);
    std::vector<float> h_c(dim, 0.0f);

    size_t gpu_bytes = 16 * 1024 * 1024;
    void * d_buf = nullptr;
    CUDA_CHECK(cudaMalloc(&d_buf, gpu_bytes));
    cudaStream_t stream;
    CUDA_CHECK(cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking));

    double total_ms = 0.0;

    for (int iter = 0; iter < iters; ++iter) {
        auto t0 = std::chrono::high_resolution_clock::now();

        if (use_async) {
            // Arm B: Asynchronous CPU cold-expert execution on background worker
            auto cpu_future = std::async(std::launch::async, [&]() {
                cpu_cold_expert_gemm(h_a.data(), h_b.data(), h_c.data(), dim);
            });

            // Concurrently execute GPU hot-expert operations on stream
            CUDA_CHECK(cudaMemsetAsync(d_buf, 0xEF, gpu_bytes, stream));
            std::this_thread::sleep_for(std::chrono::microseconds(800));
            CUDA_CHECK(cudaStreamSynchronize(stream));

            // Join CPU worker
            cpu_future.get();
        } else {
            // Arm A: Synchronous serial execution
            cpu_cold_expert_gemm(h_a.data(), h_b.data(), h_c.data(), dim);

            // Execute GPU operations afterwards
            CUDA_CHECK(cudaMemsetAsync(d_buf, 0xEF, gpu_bytes, stream));
            std::this_thread::sleep_for(std::chrono::microseconds(800));
            CUDA_CHECK(cudaStreamSynchronize(stream));
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        total_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }

    double avg_latency_ms = total_ms / iters;
    printf("{\"async_cpu\": %s, \"dim\": %d, \"iters\": %d, \"avg_latency_ms\": %.4f}\n",
           use_async ? "true" : "false", dim, iters, avg_latency_ms);

    CUDA_CHECK(cudaStreamDestroy(stream));
    CUDA_CHECK(cudaFree(d_buf));
    return 0;
}
