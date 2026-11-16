#include "sandwich-telemetry.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <cstdint>

#define NUM_EVALS 10000000ULL // 10 Million iterations

// Mode 0: Arm A (Baseline without telemetry)
// Mode 1: Arm B (Test with sandwich_telemetry_record_hits / misses)
double run_telemetry_arm(int mode) {
    if (mode == 1) {
        sandwich_telemetry_reset_all();
    }
    
    volatile uint64_t dummy = 0;
    auto t0 = std::chrono::high_resolution_clock::now();

    if (mode == 0) {
        for (uint64_t i = 0; i < NUM_EVALS; ++i) {
            dummy += (i & 1);
        }
    } else {
        for (uint64_t i = 0; i < NUM_EVALS; ++i) {
            dummy += (i & 1);
            if ((i & 1) == 0) {
                sandwich_telemetry_record_hits(1);
            } else {
                sandwich_telemetry_record_misses(1);
            }
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (mode == 1) {
        auto stats = sandwich_telemetry_get_stats();
        if (stats.cum_total_evals != NUM_EVALS) {
            fprintf(stderr, "Telemetry count mismatch! Expected %llu, got %llu\n", 
                    (unsigned long long)NUM_EVALS, (unsigned long long)stats.cum_total_evals);
            return -1.0;
        }
    }

    return ms;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <0=Arm A | 1=Arm B>\n", argv[0]);
        return 1;
    }
    int mode = std::atoi(argv[1]);
    double elapsed_ms = run_telemetry_arm(mode);
    printf("%.4f\n", elapsed_ms);
    return 0;
}
