#include "sandwich-telemetry.h"

#include <cstdio>
#include <cinttypes>
#include <atomic>
#include <mutex>
#include <unordered_map>

static std::atomic<uint64_t> g_hot_hits{0};
static std::atomic<uint64_t> g_cold_misses{0};
static std::atomic<uint64_t> g_prefill_hot_hits{0};
static std::atomic<uint64_t> g_prefill_cold_misses{0};
static std::atomic<uint64_t> g_decode_hot_hits{0};
static std::atomic<uint64_t> g_decode_cold_misses{0};
static std::atomic<uint64_t> g_cum_hot_hits{0};
static std::atomic<uint64_t> g_cum_cold_misses{0};
static std::atomic<uint64_t> g_warmup_tokens{0};

// Phase awareness
static std::atomic<int> g_current_phase{SANDWICH_PHASE_DECODE};

// GPU-side expert counters (layers 0..12)
static std::atomic<uint64_t> g_gpu_calls_prefill{0};
static std::atomic<uint64_t> g_gpu_calls_decode{0};

// NRC sequential batching histogram
static std::atomic<uint64_t> g_nrc_hist_prefill[9] = {};
static std::atomic<uint64_t> g_nrc_hist_decode[9]  = {};

static std::mutex g_freq_mutex;
static std::unordered_map<uint64_t, int64_t> g_expert_freqs;

void sandwich_telemetry_set_phase(sandwich_telemetry_phase_t phase) {
    g_current_phase.store((int)phase, std::memory_order_relaxed);
}

sandwich_telemetry_phase_t sandwich_telemetry_get_phase(void) {
    return (sandwich_telemetry_phase_t)g_current_phase.load(std::memory_order_relaxed);
}

void sandwich_telemetry_record_gpu_calls(int32_t il, uint64_t n_calls) {
    (void)il;
    if (g_current_phase.load(std::memory_order_relaxed) == (int)SANDWICH_PHASE_PREFILL) {
        g_gpu_calls_prefill.fetch_add(n_calls, std::memory_order_relaxed);
    } else {
        g_gpu_calls_decode.fetch_add(n_calls, std::memory_order_relaxed);
    }
}

void sandwich_telemetry_record_hits(uint64_t hits) {
    if (g_current_phase.load(std::memory_order_relaxed) == (int)SANDWICH_PHASE_PREFILL) {
        g_prefill_hot_hits.fetch_add(hits, std::memory_order_relaxed);
    } else {
        g_hot_hits.fetch_add(hits, std::memory_order_relaxed);
        g_decode_hot_hits.fetch_add(hits, std::memory_order_relaxed);
    }
    g_cum_hot_hits.fetch_add(hits, std::memory_order_relaxed);
}

void sandwich_telemetry_record_misses(uint64_t misses) {
    if (g_current_phase.load(std::memory_order_relaxed) == (int)SANDWICH_PHASE_PREFILL) {
        g_prefill_cold_misses.fetch_add(misses, std::memory_order_relaxed);
    } else {
        g_cold_misses.fetch_add(misses, std::memory_order_relaxed);
        g_decode_cold_misses.fetch_add(misses, std::memory_order_relaxed);
    }
    g_cum_cold_misses.fetch_add(misses, std::memory_order_relaxed);
}

void sandwich_telemetry_record_prefill_evals(uint64_t hits, uint64_t misses) {
    g_prefill_hot_hits.fetch_add(hits, std::memory_order_relaxed);
    g_prefill_cold_misses.fetch_add(misses, std::memory_order_relaxed);
    g_cum_hot_hits.fetch_add(hits, std::memory_order_relaxed);
    g_cum_cold_misses.fetch_add(misses, std::memory_order_relaxed);
}

void sandwich_telemetry_record_expert(int32_t il, int32_t expert_id) {
    if (expert_id < 0 || il < 0) return;
    const uint64_t key = (static_cast<uint64_t>(il) << 32) | static_cast<uint32_t>(expert_id);
    std::lock_guard<std::mutex> lock(g_freq_mutex);
    g_expert_freqs[key]++;
}

void sandwich_telemetry_record_nrc(int nrc) {
    if (nrc < 1 || nrc > 8) return;
    if (g_current_phase.load(std::memory_order_relaxed) == (int)SANDWICH_PHASE_PREFILL) {
        g_nrc_hist_prefill[nrc].fetch_add(1, std::memory_order_relaxed);
    } else {
        g_nrc_hist_decode[nrc].fetch_add(1, std::memory_order_relaxed);
    }
}

int64_t sandwich_telemetry_get_expert_freq(int32_t il, int32_t expert_id) {
    const uint64_t key = (static_cast<uint64_t>(il) << 32) | static_cast<uint32_t>(expert_id);
    std::lock_guard<std::mutex> lock(g_freq_mutex);
    auto it = g_expert_freqs.find(key);
    return (it != g_expert_freqs.end()) ? it->second : 0;
}

uint64_t sandwich_telemetry_get_total_warmup_tokens(void) {
    return g_warmup_tokens.load(std::memory_order_relaxed);
}

void sandwich_telemetry_increment_warmup_tokens(void) {
    g_warmup_tokens.fetch_add(1, std::memory_order_relaxed);
}

sandwich_telemetry_stats_t sandwich_telemetry_get_stats(void) {
    sandwich_telemetry_stats_t stats;
    stats.hot_hits    = g_hot_hits.load(std::memory_order_relaxed);
    stats.cold_misses = g_cold_misses.load(std::memory_order_relaxed);
    stats.total_evals = stats.hot_hits + stats.cold_misses;
    stats.hit_rate_pct = stats.total_evals > 0
        ? (100.0 * (double) stats.hot_hits / (double) stats.total_evals)
        : 0.0;

    stats.prefill_hot_hits    = g_prefill_hot_hits.load(std::memory_order_relaxed);
    stats.prefill_cold_misses = g_prefill_cold_misses.load(std::memory_order_relaxed);
    stats.prefill_total_evals = stats.prefill_hot_hits + stats.prefill_cold_misses;
    stats.prefill_hit_rate_pct = stats.prefill_total_evals > 0
        ? (100.0 * (double) stats.prefill_hot_hits / (double) stats.prefill_total_evals)
        : 0.0;

    stats.decode_hot_hits    = g_decode_hot_hits.load(std::memory_order_relaxed);
    stats.decode_cold_misses = g_decode_cold_misses.load(std::memory_order_relaxed);
    stats.decode_total_evals = stats.decode_hot_hits + stats.decode_cold_misses;
    stats.decode_hit_rate_pct = stats.decode_total_evals > 0
        ? (100.0 * (double) stats.decode_hot_hits / (double) stats.decode_total_evals)
        : 0.0;

    stats.cum_hot_hits    = g_cum_hot_hits.load(std::memory_order_relaxed);
    stats.cum_cold_misses = g_cum_cold_misses.load(std::memory_order_relaxed);
    stats.cum_total_evals = stats.cum_hot_hits + stats.cum_cold_misses;
    stats.cum_hit_rate_pct = stats.cum_total_evals > 0
        ? (100.0 * (double) stats.cum_hot_hits / (double) stats.cum_total_evals)
        : 0.0;

    stats.gpu_calls_prefill = g_gpu_calls_prefill.load(std::memory_order_relaxed);
    stats.gpu_calls_decode  = g_gpu_calls_decode.load(std::memory_order_relaxed);
    stats.gpu_calls_total   = stats.gpu_calls_prefill + stats.gpu_calls_decode;
    stats.whole_model_total_evals = stats.cum_total_evals + stats.gpu_calls_total;
    stats.whole_model_coverage_pct = stats.whole_model_total_evals > 0 ? 100.0 : 0.0;
    return stats;
}

static uint64_t g_last_printed_evals = 0;

void sandwich_telemetry_reset_post_repack(void) {
    // Preserves cumulative counters across warm-up repacks (avoids wiping active decode stats)
}

void sandwich_telemetry_reset(void) {
    g_hot_hits.store(0, std::memory_order_relaxed);
    g_cold_misses.store(0, std::memory_order_relaxed);
    g_decode_hot_hits.store(0, std::memory_order_relaxed);
    g_decode_cold_misses.store(0, std::memory_order_relaxed);
    g_gpu_calls_decode.store(0, std::memory_order_relaxed);
    for (int i = 0; i <= 8; ++i) {
        g_nrc_hist_decode[i].store(0, std::memory_order_relaxed);
    }
    // Prefill counters (g_prefill_hot_hits, g_prefill_cold_misses, g_gpu_calls_prefill)
    // and cumulative totals (g_cum_hot_hits, g_cum_cold_misses) are preserved across
    // internal context resets so whole-session telemetry accurately captures prompt prefill.
}

void sandwich_telemetry_reset_all(void) {
    g_hot_hits.store(0, std::memory_order_relaxed);
    g_cold_misses.store(0, std::memory_order_relaxed);
    g_prefill_hot_hits.store(0, std::memory_order_relaxed);
    g_prefill_cold_misses.store(0, std::memory_order_relaxed);
    g_decode_hot_hits.store(0, std::memory_order_relaxed);
    g_decode_cold_misses.store(0, std::memory_order_relaxed);
    g_cum_hot_hits.store(0, std::memory_order_relaxed);
    g_cum_cold_misses.store(0, std::memory_order_relaxed);
    g_gpu_calls_prefill.store(0, std::memory_order_relaxed);
    g_gpu_calls_decode.store(0, std::memory_order_relaxed);
    g_warmup_tokens.store(0, std::memory_order_relaxed);
    for (int i = 0; i <= 8; ++i) {
        g_nrc_hist_prefill[i].store(0, std::memory_order_relaxed);
        g_nrc_hist_decode[i].store(0, std::memory_order_relaxed);
    }
    g_last_printed_evals = 0;
    std::lock_guard<std::mutex> lock(g_freq_mutex);
    g_expert_freqs.clear();
}

void sandwich_telemetry_print(void) {
    const auto stats = sandwich_telemetry_get_stats();
    if (stats.whole_model_total_evals == 0 || stats.whole_model_total_evals == g_last_printed_evals) {
        return;
    }
    g_last_printed_evals = stats.whole_model_total_evals;

    fprintf(stderr, "\n");
    fprintf(stderr, "=======================================================\n");
    fprintf(stderr, "🥪 [Project Sandwich] MoE Cache Runtime Telemetry\n");
    fprintf(stderr, "   Whole-Model Total Evaluations  : %10" PRIu64 " (100.00%% coverage)\n", stats.whole_model_total_evals);
    fprintf(stderr, "   GPU-Resident Evaluations (CUDA): %10" PRIu64 "\n", stats.gpu_calls_total);
    fprintf(stderr, "   Host-Resident Evaluations      : %10" PRIu64 "\n", stats.cum_total_evals);
    fprintf(stderr, "   Host-Resident Hot Hits         : %10" PRIu64 " (%.2f%%)\n", stats.cum_hot_hits, stats.cum_hit_rate_pct);
    fprintf(stderr, "   Host-Resident Cold Misses      : %10" PRIu64 " (%.2f%%)\n", stats.cum_cold_misses, 100.0 - stats.cum_hit_rate_pct);

    if (stats.prefill_total_evals > 0 || stats.gpu_calls_prefill > 0) {
        fprintf(stderr, "   --- Window 1: Prompt Prefill Phase ---\n");
        fprintf(stderr, "   Prefill GPU CUDA Evals         : %10" PRIu64 "\n", stats.gpu_calls_prefill);
        fprintf(stderr, "   Prefill Host Invocations       : %10" PRIu64 "\n", stats.prefill_total_evals);
        fprintf(stderr, "   Prefill Host Hot Hits          : %10" PRIu64 " (%.2f%%)\n", stats.prefill_hot_hits, stats.prefill_hit_rate_pct);
        fprintf(stderr, "   Prefill Host Cold Misses       : %10" PRIu64 " (%.2f%%)\n", stats.prefill_cold_misses, 100.0 - stats.prefill_hit_rate_pct);
    }

    if (stats.decode_total_evals > 0 || stats.gpu_calls_decode > 0) {
        fprintf(stderr, "   --- Window 2: Token Decode Phase ---\n");
        fprintf(stderr, "   Decode GPU CUDA Evals          : %10" PRIu64 "\n", stats.gpu_calls_decode);
        fprintf(stderr, "   Decode Host Invocations        : %10" PRIu64 "\n", stats.decode_total_evals);
        fprintf(stderr, "   Decode Host Hot Hits           : %10" PRIu64 " (%.2f%%)\n", stats.decode_hot_hits, stats.decode_hit_rate_pct);
        fprintf(stderr, "   Decode Host Cold Misses        : %10" PRIu64 " (%.2f%%)\n", stats.decode_cold_misses, 100.0 - stats.decode_hit_rate_pct);
    }

    if (stats.total_evals > 0 && stats.total_evals != stats.decode_total_evals) {
        fprintf(stderr, "   --- Post-Repack Active Decode Phase ---\n");
        fprintf(stderr, "   Post-Repack Invocations        : %10" PRIu64 "\n", stats.total_evals);
        fprintf(stderr, "   Post-Repack Hot Hits           : %10" PRIu64 " (%.2f%%)\n", stats.hot_hits, stats.hit_rate_pct);
        fprintf(stderr, "   Post-Repack Cold Misses        : %10" PRIu64 " (%.2f%%)\n", stats.cold_misses, 100.0 - stats.hit_rate_pct);
    }

    uint64_t total_nrc_prefill = 0;
    uint64_t total_nrc_decode = 0;
    for (int i = 1; i <= 8; ++i) {
        total_nrc_prefill += g_nrc_hist_prefill[i].load(std::memory_order_relaxed);
        total_nrc_decode  += g_nrc_hist_decode[i].load(std::memory_order_relaxed);
    }
    if (total_nrc_prefill > 0 || total_nrc_decode > 0) {
        fprintf(stderr, "   --- NRC Sequential Batching Histogram ---\n");
        if (total_nrc_prefill > 0) {
            fprintf(stderr, "   Prefill NRC Invocations (total %10" PRIu64 "): [nrc=1: %" PRIu64 ", nrc=2: %" PRIu64 ", nrc=4: %" PRIu64 ", nrc=8: %" PRIu64 "]\n",
                    total_nrc_prefill,
                    g_nrc_hist_prefill[1].load(std::memory_order_relaxed),
                    g_nrc_hist_prefill[2].load(std::memory_order_relaxed),
                    g_nrc_hist_prefill[4].load(std::memory_order_relaxed),
                    g_nrc_hist_prefill[8].load(std::memory_order_relaxed));
        }
        if (total_nrc_decode > 0) {
            fprintf(stderr, "   Decode NRC Invocations  (total %10" PRIu64 "): [nrc=1: %" PRIu64 ", nrc=2: %" PRIu64 ", nrc=4: %" PRIu64 ", nrc=8: %" PRIu64 "]\n",
                    total_nrc_decode,
                    g_nrc_hist_decode[1].load(std::memory_order_relaxed),
                    g_nrc_hist_decode[2].load(std::memory_order_relaxed),
                    g_nrc_hist_decode[4].load(std::memory_order_relaxed),
                    g_nrc_hist_decode[8].load(std::memory_order_relaxed));
        }
    }

    fprintf(stderr, "   Hot-Pack Hit Rate (Host-Resident Layers 13..40): %10.2f%%\n", stats.cum_hit_rate_pct);
    fprintf(stderr, "=======================================================\n");
    fprintf(stderr, "\n");
}
