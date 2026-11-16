#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <numeric>
#include <random>
#include <string>

// Technique #51: Contiguous Single-Allocation Expert Slabs
// Compares:
// Arm A: Fragmented multiple individual allocations (e.g., 64 experts * 3 projections = 192 heap allocations)
// Arm B: Contiguous single slab allocation partitioned via offset arithmetic (repack_moe_expert_cache pattern)

int main(int argc, char ** argv) {
    bool use_slab = false;
    size_t num_experts = 64;
    size_t projections_per_expert = 3; // gate, up, down
    size_t proj_bytes = 2 * 1024 * 1024; // 2 MiB per projection tensor
    int iters = 20;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--slab") {
            use_slab = true;
        } else if (arg == "--fragmented") {
            use_slab = false;
        } else if (arg == "--experts" && i + 1 < argc) {
            num_experts = std::stoul(argv[++i]);
        } else if (arg == "--iters" && i + 1 < argc) {
            iters = std::stoi(argv[++i]);
        }
    }

    size_t total_allocations = num_experts * projections_per_expert;
    size_t total_bytes = total_allocations * proj_bytes;

    // Fixed PRNG seed for deterministic routing pattern
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist_expert(0, num_experts - 1);
    std::uniform_int_distribution<size_t> dist_proj(0, projections_per_expert - 1);

    std::vector<std::pair<size_t, size_t>> access_sequence;
    for (int i = 0; i < 10000; ++i) {
        access_sequence.push_back({dist_expert(rng), dist_proj(rng)});
    }

    double total_alloc_ms = 0.0;
    double total_access_ms = 0.0;

    for (int iter = 0; iter < iters; ++iter) {
        if (use_slab) {
            // Arm B: Single Contiguous Slab Allocation
            auto t0 = std::chrono::high_resolution_clock::now();
            uint8_t * slab = (uint8_t *) std::malloc(total_bytes);
            if (!slab) {
                fprintf(stderr, "Failed to allocate slab\n");
                return 1;
            }
            // Zero/touch memory
            std::memset(slab, 0x5A, total_bytes);
            auto t1 = std::chrono::high_resolution_clock::now();
            total_alloc_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();

            // Simulate contiguous offset-based expert access & pack
            auto t2 = std::chrono::high_resolution_clock::now();
            volatile uint64_t checksum = 0;
            for (const auto & req : access_sequence) {
                size_t offset = (req.first * projections_per_expert + req.second) * proj_bytes;
                uint8_t * ptr = slab + offset;
                checksum += ptr[0] + ptr[proj_bytes / 2] + ptr[proj_bytes - 1];
            }
            auto t3 = std::chrono::high_resolution_clock::now();
            total_access_ms += std::chrono::duration<double, std::milli>(t3 - t2).count();

            std::free(slab);
        } else {
            // Arm A: Fragmented Individual Expert Allocations
            auto t0 = std::chrono::high_resolution_clock::now();
            std::vector<uint8_t *> individual_ptrs(total_allocations, nullptr);
            for (size_t i = 0; i < total_allocations; ++i) {
                individual_ptrs[i] = (uint8_t *) std::malloc(proj_bytes);
                if (!individual_ptrs[i]) {
                    fprintf(stderr, "Failed individual alloc\n");
                    return 1;
                }
                std::memset(individual_ptrs[i], 0x5A, proj_bytes);
            }
            auto t1 = std::chrono::high_resolution_clock::now();
            total_alloc_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();

            // Simulate fragmented heap pointer indirection access & pack
            auto t2 = std::chrono::high_resolution_clock::now();
            volatile uint64_t checksum = 0;
            for (const auto & req : access_sequence) {
                size_t idx = req.first * projections_per_expert + req.second;
                uint8_t * ptr = individual_ptrs[idx];
                checksum += ptr[0] + ptr[proj_bytes / 2] + ptr[proj_bytes - 1];
            }
            auto t3 = std::chrono::high_resolution_clock::now();
            total_access_ms += std::chrono::duration<double, std::milli>(t3 - t2).count();

            for (size_t i = 0; i < total_allocations; ++i) {
                std::free(individual_ptrs[i]);
            }
        }
    }

    double avg_alloc_ms = total_alloc_ms / iters;
    double avg_access_ms = total_access_ms / iters;
    double total_time_ms = avg_alloc_ms + avg_access_ms;

    printf("{\"slab\": %s, \"experts\": %zu, \"total_mb\": %.1f, \"avg_alloc_ms\": %.4f, \"avg_access_ms\": %.4f, \"total_time_ms\": %.4f}\n",
           use_slab ? "true" : "false", num_experts, total_bytes / (1024.0 * 1024.0),
           avg_alloc_ms, avg_access_ms, total_time_ms);

    return 0;
}
