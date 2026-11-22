#include <cuda_runtime.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
            exit(1); \
        } \
    } while (0)

int main(int argc, char ** argv) {
    bool use_pinned = false;
    size_t buffer_size_mb = 64; // 64 MiB matching llama-moe-cache ring size
    int iters = 10;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--pinned") {
            use_pinned = true;
        } else if (arg == "--unpinned") {
            use_pinned = false;
        } else if (arg == "--size-mb" && i + 1 < argc) {
            buffer_size_mb = std::stoul(argv[++i]);
        } else if (arg == "--iters" && i + 1 < argc) {
            iters = std::stoi(argv[++i]);
        }
    }

    size_t total_bytes = buffer_size_mb * 1024ULL * 1024ULL;

    void * host_ptr = nullptr;
    if (use_pinned) {
        // Technique #49: Zero-Copy Pinned Staging Pool (cudaHostAlloc)
        CUDA_CHECK(cudaHostAlloc(&host_ptr, total_bytes, cudaHostAllocPortable));
    } else {
        // Baseline: Standard malloc (pageable, unpinned)
        host_ptr = std::malloc(total_bytes);
        if (!host_ptr) {
            fprintf(stderr, "Failed to allocate %zu bytes host memory\n", total_bytes);
            return 1;
        }
    }

    // Touch memory to ensure physical backing
    std::memset(host_ptr, 0xAB, total_bytes);

    void * device_ptr = nullptr;
    CUDA_CHECK(cudaMalloc(&device_ptr, total_bytes));

    cudaStream_t stream;
    CUDA_CHECK(cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking));

    // Warm-up run
    CUDA_CHECK(cudaMemcpyAsync(device_ptr, host_ptr, total_bytes, cudaMemcpyHostToDevice, stream));
    CUDA_CHECK(cudaStreamSynchronize(stream));

    // Timed measurement loop
    auto t_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iters; ++i) {
        CUDA_CHECK(cudaMemcpyAsync(device_ptr, host_ptr, total_bytes, cudaMemcpyHostToDevice, stream));
    }
    CUDA_CHECK(cudaStreamSynchronize(stream));
    auto t_end = std::chrono::high_resolution_clock::now();

    double total_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    double avg_latency_ms = total_ms / iters;
    double gb_transferred = (static_cast<double>(total_bytes) / (1024.0 * 1024.0 * 1024.0));
    double total_gb = gb_transferred * iters;
    double bandwidth_gb_s = total_gb / (total_ms / 1000.0);

    printf("{\"pinned\": %s, \"buffer_size_mb\": %zu, \"iters\": %d, \"total_ms\": %.4f, \"avg_latency_ms\": %.4f, \"bandwidth_gb_s\": %.4f}\n",
           use_pinned ? "true" : "false", buffer_size_mb, iters, total_ms, avg_latency_ms, bandwidth_gb_s);

    CUDA_CHECK(cudaStreamDestroy(stream));
    CUDA_CHECK(cudaFree(device_ptr));
    if (use_pinned) {
        CUDA_CHECK(cudaFreeHost(host_ptr));
    } else {
        std::free(host_ptr);
    }

    return 0;
}
