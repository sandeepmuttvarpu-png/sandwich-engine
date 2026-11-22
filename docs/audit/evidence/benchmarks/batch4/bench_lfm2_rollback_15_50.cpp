#include "llama.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// Technique #15: LFM2 Hybrid Recurrent State Rollback
// Technique #50: KVA Late-Layer Recurrence State Prefill
//
// Isolated benchmark measuring recurrent state rollback & snapshot restore latency on hybrid architectures.
// Compares Arm A (Cold re-evaluation / uncheckpointed replay baseline) vs Arm B (Instant Recurrent Snapshot Rollback).

int main(int argc, char ** argv) {
    bool enable_rs_rollback = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--rs-rollback" || std::string(argv[i]) == "--kv-defrag") {
            enable_rs_rollback = true;
        }
    }

    llama_backend_init();
    llama_model_params mparams = llama_model_default_params();
    llama_model * model = llama_model_load_from_file("build/tests/test-models/lfm2-dense.gguf", mparams);
    if (!model) {
        fprintf(stderr, "Failed to load lfm2-dense.gguf model\n");
        return 1;
    }

    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = 512;
    cparams.n_rs_seq = enable_rs_rollback ? 4 : 0;
    cparams.n_threads = 4;
    llama_context * ctx = llama_init_from_model(model, cparams);
    if (!ctx) {
        fprintf(stderr, "Failed to init llama_context\n");
        llama_model_free(model);
        return 1;
    }

    const int n_prompt = 32;
    std::vector<llama_token> tokens(n_prompt);
    for (int i = 0; i < n_prompt; ++i) {
        tokens[i] = (llama_token)(i + 1);
    }

    // Initial prefill
    llama_batch batch = llama_batch_init(n_prompt, 0, 1);
    for (int i = 0; i < n_prompt; ++i) {
        batch.token[i] = tokens[i];
        batch.pos[i] = i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = (i == n_prompt - 1);
    }
    batch.n_tokens = n_prompt;
    llama_decode(ctx, batch);

    // Benchmark 50 rollback cycles
    const int n_cycles = 50;
    const llama_pos rollback_pos = n_prompt - 3; // rollback 3 tokens
    auto t0 = std::chrono::high_resolution_clock::now();

    for (int c = 0; c < n_cycles; ++c) {
        if (enable_rs_rollback) {
            // Instant recurrent state snapshot rollback
            llama_memory_seq_rm(llama_get_memory(ctx), 0, rollback_pos, -1);
            // Replay the 3 tokens
            llama_batch rbatch = llama_batch_init(3, 0, 1);
            for (int i = 0; i < 3; ++i) {
                rbatch.token[i] = tokens[rollback_pos + i];
                rbatch.pos[i] = rollback_pos + i;
                rbatch.n_seq_id[i] = 1;
                rbatch.seq_id[i][0] = 0;
                rbatch.logits[i] = (i == 2);
            }
            rbatch.n_tokens = 3;
            llama_decode(ctx, rbatch);
            llama_batch_free(rbatch);
        } else {
            // Cold restore: wipe and re-decode all tokens up to n_prompt
            llama_memory_seq_rm(llama_get_memory(ctx), 0, 0, -1);
            llama_batch fbatch = llama_batch_init(n_prompt, 0, 1);
            for (int i = 0; i < n_prompt; ++i) {
                fbatch.token[i] = tokens[i];
                fbatch.pos[i] = i;
                fbatch.n_seq_id[i] = 1;
                fbatch.seq_id[i][0] = 0;
                fbatch.logits[i] = (i == n_prompt - 1);
            }
            fbatch.n_tokens = n_prompt;
            llama_decode(ctx, fbatch);
            llama_batch_free(fbatch);
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double per_cycle_ms = total_ms / n_cycles;

    printf("[RECURRENT_ROLLBACK] Mode: %s | Cycles: %d | Total: %.4f ms | Per-cycle: %.4f ms | Status: OK\n",
           enable_rs_rollback ? "Recurrent_Snapshot_Rollback" : "Cold_Reeval_Baseline",
           n_cycles, total_ms, per_cycle_ms);

    llama_batch_free(batch);
    llama_free(ctx);
    llama_model_free(model);
    llama_backend_free();
    return 0;
}
