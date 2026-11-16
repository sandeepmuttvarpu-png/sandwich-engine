#include "hardware_profiler.hpp"
#include <cstdio>
#include <cstdlib>
#include <chrono>

using namespace sandwich::dispatch;

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <0=Arm A (force reprofile) | 1=Arm B (cached lock)>\n", argv[0]);
        return 1;
    }
    int mode = std::atoi(argv[1]);

    // Ensure cache exists first
    HardwareProfiler::run_profiling(false);

    auto t0 = std::chrono::high_resolution_clock::now();
    HardwareProfile p;
    if (mode == 0) {
        // Arm A: Force re-probe
        p = HardwareProfiler::run_profiling(true);
    } else {
        // Arm B: Fast path via lockfile
        p = HardwareProfiler::load_or_profile();
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (p.cpu.optimal_threads <= 0) {
        fprintf(stderr, "Failed to load profile!\n");
        return 1;
    }

    printf("%.4f\n", ms);
    return 0;
}
