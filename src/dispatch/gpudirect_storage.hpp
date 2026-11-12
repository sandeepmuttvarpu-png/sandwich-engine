#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>

#if __has_include(<cuda_runtime.h>)
    #include <cuda_runtime.h>
    #define SANDWICH_CUDA_AVAILABLE 1
#endif

namespace sandwich {
namespace dispatch {

constexpr size_t GDS_RING_SLOT_COUNT  = 32;      // Power of two
constexpr size_t GDS_SLOT_CAPACITY    = 4194304; // 4 MiB max slice
constexpr size_t GDS_SECTOR_ALIGNMENT = 4096;    // 4KB NVMe direct alignment

enum class GDSBackendMode {
    DISABLED,               // GDS explicitly turned off
    DIRECT_CUFILE,          // Native NVIDIA cuFile kernel-bypass DMA directly to VRAM
    COMPAT_IO_URING_DIRECT, // Linux io_uring asynchronous O_DIRECT DMA to pinned staging
    COMPAT_POSIX_DIRECT     // POSIX aligned O_DIRECT pread DMA fallback
};

struct GDSConfig {
    bool   enabled             = true;
    size_t block_alignment     = GDS_SECTOR_ALIGNMENT; // NVMe hardware sector alignment (4KB)
    size_t max_batch_size      = 32;   // Max simultaneous DMA transfers
    bool   force_compatibility = false;// Force compat fallback mode
};

struct GDSStatus {
    bool           enabled            = false;
    GDSBackendMode active_backend     = GDSBackendMode::DISABLED;
    bool           has_cufile_driver  = false;
    bool           has_io_uring       = false;
    uint64_t       total_transfers    = 0;
    uint64_t       total_bytes        = 0;
    double         peak_bandwidth_gbs = 0.0;
    std::string    backend_name;
    std::string    diagnostics;
};

struct GDSTransferRequest {
    int         fd          = -1;
    uint64_t    file_offset = 0;
    size_t      length      = 0;
    void *      dest_ptr    = nullptr;
    bool        is_gpu_vram = false;
};

struct GDSTransferResult {
    bool        success           = false;
    size_t      bytes_transferred = 0;
    double      elapsed_ms        = 0.0;
    double      bandwidth_gbs     = 0.0;
    std::string error_message;
};

class GDSManager {
public:
    explicit GDSManager(const GDSConfig & config = GDSConfig{});
    ~GDSManager();

    GDSManager(const GDSManager &) = delete;
    GDSManager & operator=(const GDSManager &) = delete;

    void probe_capabilities();
    bool init();
    void shutdown();
    bool is_active() const;

    GDSStatus get_status() const;
    GDSTransferResult transfer(const GDSTransferRequest & req);
    std::vector<GDSTransferResult> transfer_batch(const std::vector<GDSTransferRequest> & reqs);

    void reset_metrics();
    std::string get_diagnostics() const;

    static void align_bounds(
        uint64_t offset,
        size_t length,
        size_t alignment,
        uint64_t & aligned_offset,
        size_t & aligned_length,
        size_t & offset_padding
    );

private:
    struct PinnedSlot {
        uint8_t * buffer = nullptr;
        size_t capacity = 0;
        bool is_pinned = false;
        std::atomic<bool> in_use{false};
    };

    GDSConfig                   config_;
    mutable std::mutex          mutex_;
    GDSStatus                   status_;
    std::atomic<bool>           initialized_{false};

    size_t                      slot_count_{GDS_RING_SLOT_COUNT};
    size_t                      slot_mask_{GDS_RING_SLOT_COUNT - 1};
    std::atomic<uint64_t>       head_{0};
    std::unique_ptr<PinnedSlot[]> ring_;

#if defined(SANDWICH_CUDA_AVAILABLE)
    cudaStream_t                dma_stream_ = nullptr;
#endif

    uint32_t acquire_slot();
    void release_slot(uint32_t idx);
    GDSTransferResult execute_posix_direct(const GDSTransferRequest & req);
};

} // namespace dispatch
} // namespace sandwich
