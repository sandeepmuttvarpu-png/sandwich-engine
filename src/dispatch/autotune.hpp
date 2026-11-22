#pragma once

#include <string>

namespace sandwich {
namespace autotune {

struct TuneOptions {
    std::string model_path;
    int ctx_size   = 4096;
    int batch_size = 1;
    bool force     = false;
};

struct TuneResult {
    int shapes_tuned   = 0;
    int winners_cached = 0;
    double elapsed_sec = 0.0;
};

// Benchmark FlashAttention kernels and cache statistically significant (>2σ) winners.
TuneResult run_autotune(const TuneOptions & opts);

// Ensure autotune cache is populated for the current model & GPU topology.
// If cache is empty and autotuning is enabled, runs quick calibration.
void ensure_autotuned(const std::string & model_path, int ctx_size);

} // namespace autotune
} // namespace sandwich
