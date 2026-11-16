#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <map>
#include <random>
#include <string>

// Technique #19: Frequency-Based Cold-to-Hot Expert Dynamic Promotion (`--moe-cache-profile`)
// Benchmarks MoE cache hit rate under non-uniform routing distribution:
// Arm A: Static initial placement (experts 0..S-1, no dynamic promotion)
// Arm B: Frequency-based dynamic promotion (online telemetry ranks hot experts and repacks cache)

int main(int argc, char ** argv) {
    bool use_dynamic = false;
    size_t num_experts = 64;
    size_t cache_slots = 16;
    size_t num_queries = 2000;
    int iters = 20;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dynamic" || arg == "--profile") {
            use_dynamic = true;
        } else if (arg == "--static") {
            use_dynamic = false;
        } else if (arg == "--experts" && i + 1 < argc) {
            num_experts = std::stoul(argv[++i]);
        } else if (arg == "--slots" && i + 1 < argc) {
            cache_slots = std::stoul(argv[++i]);
        } else if (arg == "--iters" && i + 1 < argc) {
            iters = std::stoi(argv[++i]);
        }
    }

    // Generate non-uniform (Zipfian-like / skewed) expert activation sequence
    // A subset of experts (e.g. 18, 22, 35, 41, 50, ...) are hot
    std::mt19937 rng(999);
    std::discrete_distribution<int> dist({
        1, 2, 1, 3, 2, 1, 2, 1,
        2, 1, 3, 2, 1, 2, 1, 2,
        15, 20, 25, 30, 40, 50, 60, 70, // Experts 16-23 are extremely hot!
        50, 40, 30, 25, 20, 15, 10, 5,
        2, 1, 2, 1, 3, 2, 1, 2,
        1, 2, 1, 3, 2, 1, 2, 1,
        1, 1, 2, 1, 2, 1, 1, 2,
        1, 1, 1, 2, 1, 1, 1, 2
    });

    std::vector<int> query_sequence;
    for (size_t i = 0; i < num_queries; ++i) {
        query_sequence.push_back(dist(rng));
    }

    double total_repack_ms = 0.0;
    double total_hit_rate = 0.0;

    for (int iter = 0; iter < iters; ++iter) {
        // Initial static cache: experts 0..cache_slots-1
        std::vector<int> cache(cache_slots);
        for (size_t i = 0; i < cache_slots; ++i) {
            cache[i] = static_cast<int>(i);
        }

        std::map<int, int64_t> freq_telemetry;
        size_t hits = 0;
        double repack_ms = 0.0;

        for (size_t q_idx = 0; q_idx < query_sequence.size(); ++q_idx) {
            int expert = query_sequence[q_idx];
            freq_telemetry[expert]++;

            // Dynamic promotion trigger at step 500 (matching Sandwich online auto-tuning)
            if (use_dynamic && q_idx == 500) {
                auto t0 = std::chrono::high_resolution_clock::now();

                // Sort experts by frequency
                std::vector<std::pair<int64_t, int>> ranked;
                for (const auto & [e, count] : freq_telemetry) {
                    ranked.push_back({-count, e});
                }
                std::sort(ranked.begin(), ranked.end());

                // Repack top-S hot experts into cache
                for (size_t s = 0; s < cache_slots && s < ranked.size(); ++s) {
                    cache[s] = ranked[s].second;
                }

                auto t1 = std::chrono::high_resolution_clock::now();
                repack_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
            }

            // Check if expert is resident in cache
            if (std::find(cache.begin(), cache.end(), expert) != cache.end()) {
                hits++;
            }
        }

        total_repack_ms += repack_ms;
        total_hit_rate += (static_cast<double>(hits) / query_sequence.size()) * 100.0;
    }

    double avg_hit_rate = total_hit_rate / iters;
    double avg_repack_ms = total_repack_ms / iters;

    printf("{\"dynamic\": %s, \"hit_rate_pct\": %.2f, \"repack_ms\": %.4f, \"queries\": %zu, \"slots\": %zu}\n",
           use_dynamic ? "true" : "false", avg_hit_rate, avg_repack_ms, num_queries, cache_slots);

    return 0;
}
