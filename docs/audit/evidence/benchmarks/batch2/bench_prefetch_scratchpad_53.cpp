#include <cuda_runtime.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <thread>

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
            exit(1); \
        } \
    } while (0)

// Technique #53: Per-Layer Pinned Prefetch Scratchpads (`--prefetch-scratchpad`)
// Evaluates double-buffered prefetch pipeline across N transformer layers:
// Arm A: Synchronous on-demand staging without pinned prefetch scratchpad (pipeline stalls)
// Arm B: Double-buffered pinned prefetch scratchpad staging (prefetches layer L+1 concurrently with layer L execution)

int main(int argc, char ** argv) {
    bool use_scratchpad = false;
    int num_layers = 32;
    size_t layer_size_bytes = 16 * 1024 * 1024; // 16 MiB per layer scratchpad as in ledger telemetry
    int iters = 10;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--scratchpad") {
            use_scratchpad = true;
        } else if (arg == "--synchronous") {
            use_scratchpad = false;
        } else if (arg == "--layers" && i + 1 < argc) {
            num_layers = std::stoi(argv[++i]);
        } else if (arg == "--iters" && i + 1 < argc) {
            iters = std::stoi(argv[++i]);
        }
    }

    cudaStream_t compute_stream, prefetch_stream;
    CUDA_CHECK(cudaStreamCreateWithFlags(&compute_stream, cudaStreamNonBlocking));
    CUDA_CHECK(cudaStreamCreateWithFlags(&prefetch_stream, cudaStreamNonBlocking));

    void * d_compute = nullptr;
    void * d_prefetch = nullptr;
    CUDA_CHECK(cudaMalloc(&d_compute, layer_size_bytes));
    CUDA_CHECK(cudaMalloc(&d_prefetch, layer_size_bytes));

    void * h_pinned_0 = nullptr;
    void * h_pinned_1 = nullptr;
    CUDA_CHECK(cudaHostAlloc(&h_pinned_0, layer_size_bytes, cudaHostAllocPortable));
    CUDA_CHECK(cudaHostAlloc(&h_pinned_1, layer_size_bytes, cudaHostAllocPortable));
    std::memset(h_pinned_0, 0x11, layer_size_bytes);
    std::memset(h_pinned_1, 0x22, layer_size_bytes);

    void * h_unpinned = std::malloc(layer_size_bytes);
    std::memset(h_unpinned, 0x33, layer_size_bytes);

    double total_pipeline_ms = 0.0;

    for (int iter = 0; iter < iters; ++iter) {
        auto t_start = std::chrono::high_resolution_clock::now();

        if (use_scratchpad) {
            // Arm B: Double-buffered asynchronous pinned prefetch pipeline
            // Prime layer 0 prefetch
            CUDA_CHECK(cudaMemcpyAsync(d_compute, h_pinned_0, layer_size_bytes, cudaMemcpyHostToDevice, prefetch_stream));
            CUDA_CHECK(cudaStreamSynchronize(prefetch_stream));

            for (int l = 0; l < num_layers; ++l) {
                // If there's a next layer, prefetch it into d_prefetch concurrently
                if (l + 1 < num_layers) {
                    void * h_src = ((l + 1) % 2 == 0) ? h_pinned_0 : h_pinned_1;
                    CUDA_CHECK(cudaMemcpyAsync(d_prefetch, h_src, layer_size_bytes, cudaMemcpyHostToDevice, prefetch_stream));
                }

                // Simulate layer compute work on compute_stream (GPU kernel execution)
                // We use a small memory copy or dummy kernel to simulate compute duration (~0.5ms)
                CUDA_CHECK(cudaMemsetAsync(d_compute, 0xAA, 1024 * 1024, compute_stream));
                std::this_thread::sleep_for(std::chrono::microseconds(500));

                CUDA_CHECK(cudaStreamSynchronize(compute_stream));
                if (l + 1 < num_layers) {
                    CUDA_CHECK(cudaStreamSynchronize(prefetch_stream));
                    std::swap(d_compute, d_prefetch);
                }
            }
        } else {
            // Arm A: Synchronous on-demand staging per layer (no overlapped prefetch)
            for (int l = 0; l < num_layers; ++l) {
                // Synchronous transfer from unpinned buffer
                CUDA_CHECK(cudaMemcpy(d_compute, h_unpinned, layer_size_bytes, cudaMemcpyHostToDevice));

                // Layer compute
                CUDA_CHECK(cudaMemsetAsync(d_compute, 0xAA, 1024 * 1024, compute_stream));
                std::this_thread::sleep_for(std::chrono::microseconds(500));
                CUDA_CHECK(cudaStreamSynchronize(compute_stream));
            }
        }

        auto t_end = std::chrono::high_resolution_clock::now();
        total_pipeline_ms += std::chrono::duration<double, std::milli>(t_end - t_start).count();
    }

    double avg_pipeline_ms = total_pipeline_ms / iters;
    double avg_layer_ms = avg_pipeline_ms / num_layers;

    printf("{\"scratchpad\": %s, \"layers\": %d, \"scratchpad_mb\": %.1f, \"avg_pipeline_ms\": %.4f, \"avg_layer_ms\": %.4f}\n",
           use_scratchpad ? "true" : "false", num_layers, layer_size_bytes / (1024.0 * 1024.0),
           avg_pipeline_ms, avg_layer_ms);

    CUDA_CHECK(cudaStreamDestroy(compute_stream));
    CUDA_CHECK(cudaStreamDestroy(prefetch_stream));
    CUDA_CHECK(cudaFree(d_compute));
    CUDA_CHECK(cudaFree(d_prefetch));
    CUDA_CHECK(cudaFreeHost(h_pinned_0));
    CUDA_CHECK(cudaFreeHost(h_pinned_1));
    std::free(h_unpinned);

    return 0;
}
