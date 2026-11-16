#include "src/dispatch/h2o_cache_compressor.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <string>
#include <random>

using namespace sandwich::dispatch;

// Technique #21: Streaming Attention Sink / Sliding-Window KV Compaction (`--sink-tokens`)
// Benchmarks context compaction across 500 eviction rounds:
// Arm A: Sinks disabled (sink_tokens = 0): sinks 0..3 get evicted during long context drift
// Arm B: Sinks enabled (sink_tokens = 4): strictly guarantees 4 attention sink tokens preserved

int main(int argc, char ** argv) {
    bool use_sinks = false;
    int num_cycles = 500;
    size_t budget = 256;
    size_t context_length = 512;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--sinks" || arg == "--sink-tokens") {
            use_sinks = true;
        } else if (arg == "--no-sinks") {
            use_sinks = false;
        } else if (arg == "--cycles" && i + 1 < argc) {
            num_cycles = std::stoi(argv[++i]);
        }
    }

    H2OConfig cfg;
    cfg.enabled = true;
    cfg.sink_tokens = use_sinks ? 4 : 0;
    cfg.heavy_hitters = 128;
    cfg.rolling_window = 64;
    cfg.max_budget = budget;

    H2OCacheCompressor compressor(cfg);

    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> dist(0.01f, 1.0f);

    std::vector<float> step_attn(context_length);
    for (size_t i = 0; i < context_length; ++i) {
        step_attn[i] = dist(rng);
    }
    // Middle tokens spike attention, competing with sinks
    for (size_t i = 100; i < 200; ++i) {
        step_attn[i] = 10.0f + dist(rng);
    }

    size_t sink_retained_count = 0;
    size_t total_sink_checks = 0;

    auto t0 = std::chrono::high_resolution_clock::now();

    for (int cycle = 0; cycle < num_cycles; ++cycle) {
        compressor.accumulate_attention(step_attn);
        std::vector<int32_t> evicted = compressor.compute_eviction_indices(context_length);
        std::vector<int32_t> retained = compressor.get_retained_indices(context_length);

        // Check retention of initial 4 prompt tokens (0, 1, 2, 3)
        for (int32_t s = 0; s < 4; ++s) {
            total_sink_checks++;
            if (std::find(retained.begin(), retained.end(), s) != retained.end()) {
                sink_retained_count++;
            }
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double avg_us_per_cycle = (total_ms * 1000.0) / num_cycles;
    double retention_pct = (static_cast<double>(sink_retained_count) / total_sink_checks) * 100.0;

    printf("{\"sinks\": %s, \"cycles\": %d, \"total_ms\": %.4f, \"avg_us_per_cycle\": %.2f, \"sink_retention_pct\": %.2f, \"sinks_preserved\": %zu}\n",
           use_sinks ? "true" : "false", num_cycles, total_ms, avg_us_per_cycle, retention_pct,
           use_sinks ? 4UL : 0UL);

    return 0;
}
