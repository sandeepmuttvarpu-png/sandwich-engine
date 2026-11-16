#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <chrono>
#include <cstring>
#include <vector>
#include <sys/mman.h>
#include <unistd.h>

#define THP_ALIGNMENT (2 * 1024 * 1024)
#define BUFFER_SIZE   (512ULL * 1024ULL * 1024ULL) // 512 MB

// Mode 0: Arm A (Baseline: 4KB alignment, no THP)
// Mode 1: Arm B (Test: 2MB posix_memalign + MADV_HUGEPAGE)
double run_benchmark_arm(int mode, size_t size) {
    void* ptr = nullptr;
    auto t0 = std::chrono::high_resolution_clock::now();

    if (mode == 0) {
        // Arm A: posix_memalign 4KB, MADV_NOHUGEPAGE
        int ret = posix_memalign(&ptr, 4096, size);
        if (ret != 0 || !ptr) return -1.0;
#if defined(MADV_NOHUGEPAGE)
        madvise(ptr, size, MADV_NOHUGEPAGE);
#endif
    } else {
        // Arm B: posix_memalign 2MB, MADV_HUGEPAGE
        size_t aligned_size = (size + THP_ALIGNMENT - 1) & ~(THP_ALIGNMENT - 1);
        int ret = posix_memalign(&ptr, THP_ALIGNMENT, aligned_size);
        if (ret != 0 || !ptr) return -1.0;
#if defined(MADV_HUGEPAGE)
        madvise(ptr, aligned_size, MADV_HUGEPAGE);
#endif
    }

    // Touch all pages (write pattern to force page fault & kernel THP backing)
    volatile uint8_t* byte_ptr = static_cast<volatile uint8_t*>(ptr);
    for (size_t i = 0; i < size; i += 4096) {
        byte_ptr[i] = static_cast<uint8_t>(i & 0xFF);
    }

    // High throughput read/write passes simulating weight cache access
    for (size_t i = 0; i < size; i += 64) {
        byte_ptr[i] = static_cast<uint8_t>(byte_ptr[i] ^ 0xAA);
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    free(ptr);
    return ms;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <0=Arm A | 1=Arm B> [size_mb]\n", argv[0]);
        return 1;
    }
    int mode = std::atoi(argv[1]);
    size_t size = BUFFER_SIZE;
    if (argc >= 3) {
        size = std::stoull(argv[2]) * 1024ULL * 1024ULL;
    }
    double elapsed_ms = run_benchmark_arm(mode, size);
    printf("%.4f\n", elapsed_ms);
    return 0;
}
