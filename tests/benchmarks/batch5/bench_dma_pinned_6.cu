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

// Microbenchmark for Technique #6: Pinned Host Memory & Zero-Copy PCIe DMA (cudaHostRegister)
// Tests host-to-device PCIe transfer bandwidth of cudaHostRegister pinned memory vs standard pageable allocations.
int main(int argc, char ** argv) {
    bool use_pinned = false;
    size_t buffer_size_mb = 128; // 128 MiB buffer
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

    // Allocate page-aligned host buffer (4096 byte boundary)
    void * host_ptr = nullptr;
    int ret = posix_memalign(&host_ptr, 4096, total_bytes);
    if (ret != 0 || !host_ptr) {
        fprintf(stderr, "Failed to allocate %zu bytes aligned host memory\n", total_bytes);
        return 1;
    }

    // Touch memory to ensure physical pages are mapped
    std::memset(host_ptr, 0x5A, total_bytes);

    if (use_pinned) {
        // Technique #6: Pin existing host virtual memory via cudaHostRegister
        CUDA_CHECK(cudaHostRegister(host_ptr, total_bytes, cudaHostRegisterDefault));
    }

    void * device_ptr = nullptr;
    CUDA_CHECK(cudaMalloc(&device_ptr, total_bytes));

    cudaStream_t stream;
    CUDA_CHECK(cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking));

    // Warm-up run to establish CUDA context and stream pipeline
    CUDA_CHECK(cudaMemcpyAsync(device_ptr, host_ptr, total_bytes, cudaMemcpyHostToDevice, stream));
    CUDA_CHECK(cudaStreamSynchronize(stream));

    // Timed transfer loop
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
        CUDA_CHECK(cudaHostUnregister(host_ptr));
    }
    std::free(host_ptr);

    return 0;
}
