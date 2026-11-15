#pragma once

#include "ggml.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // Active post-repack decode window
    uint64_t hot_hits;
    uint64_t cold_misses;
    uint64_t total_evals;
    double   hit_rate_pct;

    // Window 1: Prefill phase
    uint64_t prefill_hot_hits;
    uint64_t prefill_cold_misses;
    uint64_t prefill_total_evals;
    double   prefill_hit_rate_pct;

    // Window 2: Decode phase
    uint64_t decode_hot_hits;
    uint64_t decode_cold_misses;
    uint64_t decode_total_evals;
    double   decode_hit_rate_pct;

    // Total cumulative (prefill + decode)
    uint64_t cum_hot_hits;
    uint64_t cum_cold_misses;
    uint64_t cum_total_evals;
    double   cum_hit_rate_pct;

    // GPU-resident layers (layers 0..12) evaluated directly on CUDA
    uint64_t gpu_calls_prefill;
    uint64_t gpu_calls_decode;
    uint64_t gpu_calls_total;
    uint64_t whole_model_total_evals;
    double   whole_model_coverage_pct;
} sandwich_telemetry_stats_t;

typedef enum {
    SANDWICH_PHASE_DECODE  = 0,
    SANDWICH_PHASE_PREFILL = 1,
} sandwich_telemetry_phase_t;

GGML_API void sandwich_telemetry_set_phase(sandwich_telemetry_phase_t phase);
GGML_API sandwich_telemetry_phase_t sandwich_telemetry_get_phase(void);
GGML_API void sandwich_telemetry_record_gpu_calls(int32_t il, uint64_t n_calls);

GGML_API void sandwich_telemetry_record_hits(uint64_t hits);
GGML_API void sandwich_telemetry_record_misses(uint64_t misses);
GGML_API void sandwich_telemetry_record_prefill_evals(uint64_t hits, uint64_t misses);
GGML_API void sandwich_telemetry_record_expert(int32_t il, int32_t expert_id);

GGML_API sandwich_telemetry_stats_t sandwich_telemetry_get_stats(void);
GGML_API void sandwich_telemetry_reset(void);
GGML_API void sandwich_telemetry_reset_all(void);
GGML_API void sandwich_telemetry_reset_post_repack(void);
GGML_API void sandwich_telemetry_print(void);

// Online warm-up trace inspection
GGML_API int64_t sandwich_telemetry_get_expert_freq(int32_t il, int32_t expert_id);
GGML_API uint64_t sandwich_telemetry_get_total_warmup_tokens(void);
GGML_API void sandwich_telemetry_increment_warmup_tokens(void);

#ifdef __cplusplus
}
#endif
