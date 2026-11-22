// Technique #38: 2Q & Ghost-Cache Replacement Policy Microbenchmark
// Implements an interleaved multi-turn trace test comparing standard single-queue LRU
// vs 2Q (A1in probation + Am frequent + A1out ghost) under scan pollution.
#include "src/llama-moe-cache-share.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace llama_moe_cache_detail;

int main(int argc, char** argv) {
    bool use_2q = true;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--no-moe-2q") {
            use_2q = false;
        }
    }

    const int n_layer = 16;
    const int n_expert = 64;
    const int n_slots = 16;
    const int n_turns = 100;

    moe_cache_lru lru;
    lru.init(n_layer, n_expert, n_slots, use_2q);

    std::vector<std::vector<int32_t>> slot_maps(n_layer, std::vector<int32_t>(n_expert, -1));
    for (int il = 0; il < n_layer; ++il) {
        lru.slot_map[il] = slot_maps[il].data();
    }

    std::vector<moe_cache_lru::fill> fills, cpu;
    std::vector<moe_cache_lru::eviction> evictions;
    size_t total_hits = 0;
    size_t total_queries = 0;

    auto t0 = std::chrono::high_resolution_clock::now();
    for (int turn = 0; turn < n_turns; ++turn) {
        // Prompt scan phase: 30 tokens accessing unique scanning experts (experts 16..63)
        for (int s = 0; s < 30; ++s) {
            int il = s % n_layer;
            int e1 = (16 + (turn * 30 + s) * 2) % (n_expert - 16) + 16;
            int e2 = (16 + (turn * 30 + s) * 2 + 1) % (n_expert - 16) + 16;
            std::vector<int32_t> ids = { e1, e2 };
            size_t hits = 0;
            lru.plan(il, ids.data(), ids.size(), 1, 0, 0, [](size_t) -> size_t { return 0; }, fills, cpu, evictions, hits);
            total_hits += hits;
            total_queries += ids.size();
        }
        // Generation phase: 40 tokens re-using hot working set (experts 0..7)
        for (int g = 0; g < 40; ++g) {
            int il = g % n_layer;
            int e1 = (g * 2) % 8;
            int e2 = (g * 2 + 1) % 8;
            std::vector<int32_t> ids = { e1, e2 };
            size_t hits = 0;
            lru.plan(il, ids.data(), ids.size(), 1, 0, 0, [](size_t) -> size_t { return 0; }, fills, cpu, evictions, hits);
            total_hits += hits;
            total_queries += ids.size();
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double hit_rate = (double)total_hits / total_queries * 100.0;

    printf("[MICROBENCHMARK] Policy: %s | HitRate: %.4f%% (%zu/%zu) | Total: %.4f ms | Status: OK\n",
           use_2q ? "2Q-Ghost" : "Naive-LRU", hit_rate, total_hits, total_queries, total_ms);
    return 0;
}
