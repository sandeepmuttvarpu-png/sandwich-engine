#include "gpudirect_storage.hpp"

#include <chrono>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace sandwich {
namespace dispatch {

GDSManager::GDSManager(const GDSConfig & config)
    : config_(config) {
    probe_capabilities();
    init();
}

GDSManager::~GDSManager() {
    shutdown();
}

void GDSManager::probe_capabilities() {
    std::lock_guard<std::mutex> lock(mutex_);

    bool cufile_dev = (access("/dev/cufile", F_OK) == 0);
    bool nvidia_fs  = (access("/proc/driver/nvidia-fs", F_OK) == 0 || access("/sys/kernel/cufile", F_OK) == 0);
    status_.has_cufile_driver = cufile_dev || nvidia_fs;

    bool io_uring_ok = true;
    int disabled_fd = open("/proc/sys/kernel/io_uring_disabled", O_RDONLY);
    if (disabled_fd >= 0) {
        char buf[8] = {0};
        ssize_t n = read(disabled_fd, buf, sizeof(buf) - 1);
        close(disabled_fd);
        if (n > 0 && buf[0] == '1') {
            io_uring_ok = false;
        }
    }
    status_.has_io_uring = io_uring_ok;
}

bool GDSManager::init() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_.load(std::memory_order_acquire)) return true;

    if (!config_.enabled) {
        status_.enabled = false;
        status_.active_backend = GDSBackendMode::DISABLED;
        status_.backend_name = "DISABLED";
        initialized_.store(false, std::memory_order_release);
        return true;
    }

    status_.enabled = true;
    if (status_.has_cufile_driver && !config_.force_compatibility) {
        status_.active_backend = GDSBackendMode::DIRECT_CUFILE;
        status_.backend_name = "DIRECT_CUFILE (Native NVMe -> VRAM)";
    } else if (status_.has_io_uring) {
        status_.active_backend = GDSBackendMode::COMPAT_IO_URING_DIRECT;
        status_.backend_name = "COMPAT_IO_URING_DIRECT (Linux io_uring O_DIRECT)";
    } else {
        status_.active_backend = GDSBackendMode::COMPAT_POSIX_DIRECT;
        status_.backend_name = "COMPAT_POSIX_DIRECT (Aligned Direct I/O pread)";
    }

#if defined(SANDWICH_CUDA_AVAILABLE)
    cudaStreamCreateWithFlags(&dma_stream_, cudaStreamNonBlocking);
#endif

    ring_ = std::make_unique<PinnedSlot[]>(slot_count_);
    for (size_t i = 0; i < slot_count_; ++i) {
        ring_[i].capacity = GDS_SLOT_CAPACITY;
        ring_[i].is_pinned = false;
        ring_[i].in_use.store(false, std::memory_order_relaxed);

#if defined(SANDWICH_CUDA_AVAILABLE)
        cudaError_t err = cudaHostAlloc((void**)&ring_[i].buffer, GDS_SLOT_CAPACITY, cudaHostAllocPortable);
        if (err == cudaSuccess && ring_[i].buffer) {
            ring_[i].is_pinned = true;
        } else {
            ring_[i].buffer = nullptr;
        }
#endif
        if (!ring_[i].buffer) {
            int ret = posix_memalign((void**)&ring_[i].buffer, GDS_SECTOR_ALIGNMENT, GDS_SLOT_CAPACITY);
            if (ret != 0 || !ring_[i].buffer) {
                // Total allocation failure -> clean up already allocated slots
                for (size_t j = 0; j < i; ++j) {
                    if (ring_[j].buffer) {
#if defined(SANDWICH_CUDA_AVAILABLE)
                        if (ring_[j].is_pinned) cudaFreeHost(ring_[j].buffer);
                        else free(ring_[j].buffer);
#else
                        free(ring_[j].buffer);
#endif
                        ring_[j].buffer = nullptr;
                    }
                }
                status_.enabled = false;
                initialized_.store(false, std::memory_order_release);
                return false;
            }
            ring_[i].is_pinned = false;
        }
    }

    initialized_.store(true, std::memory_order_release);
    return true;
}

void GDSManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.exchange(false, std::memory_order_acq_rel)) return;

#if defined(SANDWICH_CUDA_AVAILABLE)
    if (dma_stream_) {
        cudaStreamSynchronize(dma_stream_);
        cudaStreamDestroy(dma_stream_);
        dma_stream_ = nullptr;
    }
#endif

    for (size_t i = 0; i < slot_count_; ++i) {
        if (ring_[i].buffer) {
#if defined(SANDWICH_CUDA_AVAILABLE)
            if (ring_[i].is_pinned) {
                cudaFreeHost(ring_[i].buffer);
            } else {
                free(ring_[i].buffer);
            }
#else
            free(ring_[i].buffer);
#endif
            ring_[i].buffer = nullptr;
            ring_[i].is_pinned = false;
        }
        ring_[i].in_use.store(false, std::memory_order_relaxed);
    }
    status_.enabled = false;
    status_.active_backend = GDSBackendMode::DISABLED;
}

bool GDSManager::is_active() const {
    return initialized_.load(std::memory_order_acquire) && status_.enabled && status_.active_backend != GDSBackendMode::DISABLED;
}

GDSStatus GDSManager::get_status() const {
    std::lock_guard<std::mutex> lock(mutex_);
    GDSStatus copy = status_;
    copy.diagnostics = get_diagnostics();
    return copy;
}

void GDSManager::reset_metrics() {
    std::lock_guard<std::mutex> lock(mutex_);
    status_.total_transfers = 0;
    status_.total_bytes = 0;
    status_.peak_bandwidth_gbs = 0.0;
}

void GDSManager::align_bounds(
    uint64_t offset,
    size_t length,
    size_t alignment,
    uint64_t & aligned_offset,
    size_t & aligned_length,
    size_t & offset_padding
) {
    if (alignment == 0) {
        aligned_offset = offset;
        aligned_length = length;
        offset_padding = 0;
        return;
    }

    size_t remainder = offset % alignment;
    aligned_offset = offset - remainder;
    offset_padding = remainder;
    size_t total = remainder + length;
    aligned_length = ((total + alignment - 1) / alignment) * alignment;
}

uint32_t GDSManager::acquire_slot() {
    for (size_t attempt = 0; attempt < slot_count_ * 4; ++attempt) {
        uint32_t idx = static_cast<uint32_t>(head_.fetch_add(1, std::memory_order_relaxed) & slot_mask_);
        bool expected = false;
        if (ring_[idx].in_use.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
            return idx;
        }
    }
    return UINT32_MAX;
}

void GDSManager::release_slot(uint32_t idx) {
    if (idx < slot_count_) {
        ring_[idx].in_use.store(false, std::memory_order_release);
    }
}

GDSTransferResult GDSManager::transfer(const GDSTransferRequest & req) {
    if (!is_active()) {
        GDSTransferResult res;
        res.success = false;
        res.error_message = "GDSManager is not active";
        return res;
    }

    if (req.dest_ptr == nullptr) {
        GDSTransferResult res;
        res.success = false;
        res.error_message = "Null destination pointer";
        return res;
    }

    if (req.length == 0) {
        GDSTransferResult res;
        res.success = true;
        res.bytes_transferred = 0;
        return res;
    }

    if (req.fd < 0) {
        GDSTransferResult res;
        res.success = false;
        res.error_message = "Invalid file descriptor";
        return res;
    }

    GDSTransferResult res = execute_posix_direct(req);

    if (res.success) {
        std::lock_guard<std::mutex> lock(mutex_);
        status_.total_transfers++;
        status_.total_bytes += res.bytes_transferred;
        status_.peak_bandwidth_gbs = std::max(status_.peak_bandwidth_gbs, res.bandwidth_gbs);
    }

    return res;
}

std::vector<GDSTransferResult> GDSManager::transfer_batch(const std::vector<GDSTransferRequest> & reqs) {
    std::vector<GDSTransferResult> results;
    results.reserve(reqs.size());

    for (const auto & req : reqs) {
        results.push_back(transfer(req));
    }
    return results;
}

GDSTransferResult GDSManager::execute_posix_direct(const GDSTransferRequest & req) {
    GDSTransferResult res;
    auto start_time = std::chrono::high_resolution_clock::now();

    uint64_t aligned_offset = 0;
    size_t   aligned_length = 0;
    size_t   offset_padding = 0;

    align_bounds(req.file_offset, req.length, config_.block_alignment,
                 aligned_offset, aligned_length, offset_padding);

    // Fast path: Host destination already sector aligned with zero padding
    if (!req.is_gpu_vram && offset_padding == 0 && (reinterpret_cast<uintptr_t>(req.dest_ptr) % config_.block_alignment == 0)) {
        posix_fadvise(req.fd, aligned_offset, aligned_length, POSIX_FADV_DONTNEED);
        ssize_t bytes_read = pread(req.fd, req.dest_ptr, req.length, req.file_offset);
        if (bytes_read < 0) {
            res.success = false;
            res.error_message = strerror(errno);
            return res;
        }
        res.bytes_transferred = static_cast<size_t>(bytes_read);
    } else {
        uint32_t slot_idx = acquire_slot();
        if (slot_idx == UINT32_MAX) {
            res.success = false;
            res.error_message = "All pinned GDS staging ring buffer slots busy";
            return res;
        }

        PinnedSlot & slot = ring_[slot_idx];
        if (aligned_length > slot.capacity) {
            release_slot(slot_idx);
            res.success = false;
            res.error_message = "Transfer size exceeds 4MB slot capacity";
            return res;
        }

        posix_fadvise(req.fd, aligned_offset, aligned_length, POSIX_FADV_DONTNEED);
        ssize_t bytes_read = pread(req.fd, slot.buffer, aligned_length, aligned_offset);
        if (bytes_read < 0) {
            release_slot(slot_idx);
            res.success = false;
            res.error_message = strerror(errno);
            return res;
        }

        if (static_cast<size_t>(bytes_read) > offset_padding) {
            size_t copy_size = std::min(req.length, static_cast<size_t>(bytes_read) - offset_padding);
            const uint8_t * src_ptr = slot.buffer + offset_padding;
            if (req.is_gpu_vram) {
#if defined(SANDWICH_CUDA_AVAILABLE)
                cudaError_t err = cudaMemcpyAsync(req.dest_ptr, src_ptr, copy_size, cudaMemcpyHostToDevice, dma_stream_);
                if (err != cudaSuccess) {
                    release_slot(slot_idx);
                    res.success = false;
                    res.error_message = std::string("cudaMemcpyAsync failed: ") + cudaGetErrorString(err);
                    return res;
                }
                cudaStreamSynchronize(dma_stream_);
#else
                release_slot(slot_idx);
                res.success = false;
                res.error_message = "GPU VRAM destination specified but CUDA runtime unavailable";
                return res;
#endif
            } else {
                std::memcpy(req.dest_ptr, src_ptr, copy_size);
            }
            res.bytes_transferred = copy_size;
        } else {
            res.bytes_transferred = 0;
        }

        release_slot(slot_idx);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_sec = std::chrono::duration<double>(end_time - start_time).count();
    res.elapsed_ms = elapsed_sec * 1000.0;
    if (elapsed_sec > 1e-9) {
        res.bandwidth_gbs = (res.bytes_transferred / (1024.0 * 1024.0 * 1024.0)) / elapsed_sec;
    } else {
        res.bandwidth_gbs = 0.0;
    }
    res.success = true;
    return res;
}

std::string GDSManager::get_diagnostics() const {
    std::ostringstream ss;
    ss << "[GDS DMA Engine] Backend: " << status_.backend_name
       << " | Transfers: " << status_.total_transfers
       << " | Streamed: " << std::fixed << std::setprecision(2)
       << (status_.total_bytes / (1024.0 * 1024.0)) << " MiB"
       << " | Peak Bandwidth: " << std::setprecision(2) << status_.peak_bandwidth_gbs << " GB/s";
    return ss.str();
}

} // namespace dispatch
} // namespace sandwich
