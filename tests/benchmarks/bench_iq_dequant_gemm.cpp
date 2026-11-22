// Benchmark for quantized vector dot products: hot vs streaming cyc/32.
// Demonstrates ALU-bound vs Memory Bandwidth-bound behavior across quant types:
// - F16, Q4_0, IQ3_S, IQ2_S
//
// Exact build line:
//   cmake --build build --target bench_iq_dequant_gemm
// Or standalone:
//   g++ -O3 -mavx2 -mfma tests/benchmarks/bench_iq_dequant_gemm.cpp -Iggml/include -Iggml/src -Iggml/src/ggml-cpu -Lbuild/bin -lggml -lggml-cpu -Wl,-rpath,build/bin -o build/bin/bench_iq_dequant_gemm

#include "ggml.h"
#include "ggml-cpu.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>

#if defined(__x86_64__) || defined(_M_X64)
#  include <x86intrin.h>
#  include <pthread.h>
#else
static inline uint64_t __rdtsc() {
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}
#endif

struct BenchConfig {
    int nrc = 1;
    int iters = 2000;
    int trials = 7;
    int k = 4096;
    int streaming_rows = 2048; // ~32MB working set for 4096 elements
    bool compare_nrc = false;
};

static void pin_to_pcore() {
#if defined(__x86_64__) || defined(_M_X64)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(2, &cpuset); // P-core 2
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#endif
}

static void print_usage(const char * prog) {
    std::printf("Usage: %s [options]\n", prog);
    std::printf("  --nrc <N>         Number of activation columns/rows (default: 1)\n");
    std::printf("  --iters <N>       Iterations per trial (default: 2000)\n");
    std::printf("  --trials <N>      Number of measurement trials (default: 7)\n");
    std::printf("  --k <N>           Vector element length K (default: 4096)\n");
    std::printf("  --rows <N>        Streaming working set rows (default: 2048)\n");
    std::printf("  --compare-nrc     Run matched-work nrc amortization ablation (nrc in {2,4,8})\n");
    std::printf("  --help            Show this help\n");
}

static BenchConfig parse_args(int argc, char ** argv) {
    BenchConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--nrc" && i + 1 < argc) {
            cfg.nrc = std::atoi(argv[++i]);
        } else if (arg == "--iters" && i + 1 < argc) {
            cfg.iters = std::atoi(argv[++i]);
        } else if (arg == "--trials" && i + 1 < argc) {
            cfg.trials = std::atoi(argv[++i]);
        } else if (arg == "--k" && i + 1 < argc) {
            cfg.k = std::atoi(argv[++i]);
        } else if (arg == "--rows" && i + 1 < argc) {
            cfg.streaming_rows = std::atoi(argv[++i]);
        } else if (arg == "--compare-nrc") {
            cfg.compare_nrc = true;
        } else if (arg == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        }
    }
    return cfg;
}

struct StatResult {
    double mean;
    double stddev;
};

static StatResult calc_stats(const std::vector<double> & samples) {
    if (samples.empty()) return {0.0, 0.0};
    double sum = 0.0;
    for (double v : samples) sum += v;
    double m = sum / samples.size();
    double var = 0.0;
    for (double v : samples) var += (v - m) * (v - m);
    double s = samples.size() > 1 ? std::sqrt(var / (samples.size() - 1)) : 0.0;
    return {m, s};
}

static void run_nrc_comparison_for_type(ggml_type type, const char * name, const BenchConfig & cfg) {
    const auto * qfns_cpu = ggml_get_type_traits_cpu(type);
    if (!qfns_cpu || !qfns_cpu->vec_dot) return;
    const ggml_type vdot_type = qfns_cpu->vec_dot_type;
    const auto * vdot_fns_cpu = ggml_get_type_traits_cpu(vdot_type);
    if (!vdot_fns_cpu) return;

    const int k = cfg.k;
    const size_t row_size_x = ggml_row_size(type, k);
    const size_t row_size_y = ggml_row_size(vdot_type, k);

    std::vector<float> f_data_x(k);
    for (int i = 0; i < k; ++i) f_data_x[i] = 0.1f + 2.0f * std::cos((float)i);

    std::vector<uint8_t> x_bytes(row_size_x);
    auto from_float_x = qfns_cpu->from_float;
    if (!from_float_x) {
        const auto * qfns = ggml_get_type_traits(type);
        if (qfns) from_float_x = qfns->from_float_ref;
    }
    if (from_float_x) {
        from_float_x(f_data_x.data(), x_bytes.data(), k);
    } else {
        for (size_t i = 0; i < row_size_x; ++i) x_bytes[i] = (uint8_t)((i * 37 + 13) & 0xff);
        for (int b = 0; b < k / 256; ++b) {
            uint16_t * d_ptr = (uint16_t*)(x_bytes.data() + b * (row_size_x / (k / 256)));
            *d_ptr = 0x3c00;
        }
    }

    const int nrc_list[] = {2, 4, 8};

    for (int nrc : nrc_list) {
        // Cooldown between nrc sweeps
        std::this_thread::sleep_for(std::chrono::seconds(4));

        std::vector<uint8_t> y_bytes(row_size_y * nrc);
        for (int c = 0; c < nrc; ++c) {
            std::vector<float> f_col(k);
            for (int i = 0; i < k; ++i) f_col[i] = 0.05f + 1.8f * std::sin((float)(i + c * 23));
            vdot_fns_cpu->from_float(f_col.data(), y_bytes.data() + c * row_size_y, k);
        }

        const double elements_per_32_seq = (double)(k / 32);
        const double total_elements_per_32 = elements_per_32_seq * nrc;

        std::vector<float> result_ref(nrc, 0.0f);
        const size_t bs = 16;
        std::vector<float> s_nrc(bs * nrc, 0.0f);

        // 1. Warmup Baseline (nrc=1 across N calls)
        for (int w = 0; w < 50; ++w) {
            for (int c = 0; c < nrc; ++c) {
                qfns_cpu->vec_dot(k, &result_ref[c], 0, x_bytes.data(), 0, y_bytes.data() + c * row_size_y, 0, 1);
            }
        }

        // Measure Baseline (nrc=1 across N calls)
        std::vector<double> base_trials;
        for (int t = 0; t < cfg.trials; ++t) {
            uint64_t t0 = __rdtsc();
            for (int it = 0; it < cfg.iters; ++it) {
                for (int c = 0; c < nrc; ++c) {
                    qfns_cpu->vec_dot(k, &result_ref[c], 0, x_bytes.data(), 0, y_bytes.data() + c * row_size_y, 0, 1);
                }
            }
            uint64_t t1 = __rdtsc();
            base_trials.push_back((double)(t1 - t0) / (cfg.iters * total_elements_per_32));
        }

        // 2. Warmup Amortized (nrc=N in 1 call)
        for (int w = 0; w < 50; ++w) {
            qfns_cpu->vec_dot(k, s_nrc.data(), bs, x_bytes.data(), 0, y_bytes.data(), row_size_y, nrc);
        }

        // Measure Amortized (nrc=N in 1 call)
        std::vector<double> amort_trials;
        for (int t = 0; t < cfg.trials; ++t) {
            uint64_t t0 = __rdtsc();
            for (int it = 0; it < cfg.iters; ++it) {
                qfns_cpu->vec_dot(k, s_nrc.data(), bs, x_bytes.data(), 0, y_bytes.data(), row_size_y, nrc);
            }
            uint64_t t1 = __rdtsc();
            amort_trials.push_back((double)(t1 - t0) / (cfg.iters * total_elements_per_32));
        }

        StatResult stat_base = calc_stats(base_trials);
        StatResult stat_amort = calc_stats(amort_trials);

        double delta = stat_base.mean - stat_amort.mean;
        double speedup = stat_base.mean / stat_amort.mean;
        double sigma_comb = std::sqrt(stat_base.stddev * stat_base.stddev + stat_amort.stddev * stat_amort.stddev);
        double two_sigma = 2.0 * sigma_comb;
        double z = delta / (sigma_comb + 1e-9);
        const char * verdict = (delta > two_sigma) ? "PASS" : "FAIL (NO GAIN)";

        std::printf("%-6s | %3d | %5.2f ± %4.2f | %5.2f ± %4.2f | %6.2fx (%+5.1f%%) | Δ=%+5.2f, 2σ=%4.2f, z=%4.2f | %s\n",
                    name, nrc, stat_base.mean, stat_base.stddev,
                    stat_amort.mean, stat_amort.stddev,
                    speedup, (speedup - 1.0) * 100.0,
                    delta, two_sigma, z, verdict);
    }
}

static void benchmark_type(ggml_type type, const char * name, const BenchConfig & cfg) {
    const auto * qfns_cpu = ggml_get_type_traits_cpu(type);
    if (!qfns_cpu || !qfns_cpu->vec_dot) {
        std::printf("Type %s: no CPU vec_dot available\n", name);
        return;
    }

    const ggml_type vdot_type = qfns_cpu->vec_dot_type;
    const auto * vdot_fns_cpu = ggml_get_type_traits_cpu(vdot_type);
    if (!vdot_fns_cpu) {
        std::printf("Type %s: no CPU vec_dot_type traits\n", name);
        return;
    }

    const int k = cfg.k;
    const size_t row_size_x = ggml_row_size(type, k);
    const size_t row_size_y = ggml_row_size(vdot_type, k);

    // Prepare random data
    std::vector<float> f_data(k);
    for (int i = 0; i < k; ++i) {
        f_data[i] = 0.1f + 2.0f * std::cos((float)i);
    }

    // 1. Hot buffer: 1 row for X, nrc rows for Y
    std::vector<uint8_t> hot_x(row_size_x);
    auto from_float_x = qfns_cpu->from_float;
    if (!from_float_x) {
        const auto * qfns = ggml_get_type_traits(type);
        if (qfns) {
            from_float_x = qfns->from_float_ref;
        }
    }
    if (from_float_x) {
        from_float_x(f_data.data(), hot_x.data(), k);
    } else {
        for (size_t i = 0; i < row_size_x; ++i) hot_x[i] = (uint8_t)((i * 37 + 13) & 0xff);
        // Valid fp16 scales
        for (int b = 0; b < k / 256; ++b) {
            uint16_t * d_ptr = (uint16_t*)(hot_x.data() + b * (row_size_x / (k / 256)));
            *d_ptr = 0x3c00;
        }
    }

    std::vector<uint8_t> hot_y(row_size_y * cfg.nrc);
    for (int col = 0; col < cfg.nrc; ++col) {
        std::vector<float> f_col(k);
        for (int i = 0; i < k; ++i) {
            f_col[i] = 0.1f + 1.8f * std::sin((float)(i + col * 17));
        }
        if (vdot_fns_cpu->from_float) {
            vdot_fns_cpu->from_float(f_col.data(), hot_y.data() + col * row_size_y, k);
        } else {
            for (size_t i = 0; i < row_size_y; ++i) {
                hot_y[col * row_size_y + i] = (uint8_t)((i * 59 + 7 + col) & 0xff);
            }
        }
    }

    // 2. Streaming buffers: streaming_rows for X
    const int num_stream_rows = cfg.streaming_rows;
    std::vector<uint8_t> stream_x(row_size_x * num_stream_rows);
    for (int r = 0; r < num_stream_rows; ++r) {
        std::vector<float> f_r(k);
        for (int i = 0; i < k; ++i) {
            f_r[i] = 0.05f + 1.5f * std::cos((float)(i + r * 7));
        }
        if (from_float_x) {
            from_float_x(f_r.data(), stream_x.data() + r * row_size_x, k);
        } else {
            for (size_t i = 0; i < row_size_x; ++i) {
                stream_x[r * row_size_x + i] = (uint8_t)((i * 37 + 13 + r) & 0xff);
            }
            for (int b = 0; b < k / 256; ++b) {
                uint16_t * d_ptr = (uint16_t*)(stream_x.data() + r * row_size_x + b * (row_size_x / (k / 256)));
                *d_ptr = 0x3c00;
            }
        }
    }

    std::vector<float> result(16 * cfg.nrc, 0.0f);
    const double elements_per_32 = (double)(k / 32) * cfg.nrc;

    // --- HOT BENCHMARK ---
    // Warmup
    for (int w = 0; w < 50; ++w) {
        if (cfg.nrc == 1) {
            qfns_cpu->vec_dot(k, result.data(), 0, hot_x.data(), 0, hot_y.data(), 0, 1);
        } else {
            qfns_cpu->vec_dot(k, result.data(), 16, hot_x.data(), 0, hot_y.data(), row_size_y, cfg.nrc);
        }
    }

    std::vector<double> hot_trials;
    for (int t = 0; t < cfg.trials; ++t) {
        uint64_t t0 = __rdtsc();
        for (int it = 0; it < cfg.iters; ++it) {
            if (cfg.nrc == 1) {
                qfns_cpu->vec_dot(k, result.data(), 0, hot_x.data(), 0, hot_y.data(), 0, 1);
            } else {
                qfns_cpu->vec_dot(k, result.data(), 16, hot_x.data(), 0, hot_y.data(), row_size_y, cfg.nrc);
            }
        }
        uint64_t t1 = __rdtsc();
        hot_trials.push_back((double)(t1 - t0) / (cfg.iters * elements_per_32));
    }

    // --- STREAMING BENCHMARK ---
    // Warmup
    for (int w = 0; w < 50; ++w) {
        const uint8_t * x_ptr = stream_x.data() + (w % num_stream_rows) * row_size_x;
        if (cfg.nrc == 1) {
            qfns_cpu->vec_dot(k, result.data(), 0, x_ptr, 0, hot_y.data(), 0, 1);
        } else {
            qfns_cpu->vec_dot(k, result.data(), 16, x_ptr, 0, hot_y.data(), row_size_y, cfg.nrc);
        }
    }

    std::vector<double> stream_trials;
    for (int t = 0; t < cfg.trials; ++t) {
        uint64_t t0 = __rdtsc();
        for (int it = 0; it < cfg.iters; ++it) {
            const uint8_t * x_ptr = stream_x.data() + (it % num_stream_rows) * row_size_x;
            if (cfg.nrc == 1) {
                qfns_cpu->vec_dot(k, result.data(), 0, x_ptr, 0, hot_y.data(), 0, 1);
            } else {
                qfns_cpu->vec_dot(k, result.data(), 16, x_ptr, 0, hot_y.data(), row_size_y, cfg.nrc);
            }
        }
        uint64_t t1 = __rdtsc();
        stream_trials.push_back((double)(t1 - t0) / (cfg.iters * elements_per_32));
    }

    StatResult hot_stat = calc_stats(hot_trials);
    StatResult stream_stat = calc_stats(stream_trials);

    double gap_pct = 100.0 * (stream_stat.mean - hot_stat.mean) / hot_stat.mean;
    const char * bottleneck = (gap_pct < 10.0) ? "ALU-bound" : "Bandwidth-bound";

    std::printf("%-10s | %6.2f ± %4.2f | %6.2f ± %4.2f | %+6.1f%% | %s\n",
                name, hot_stat.mean, hot_stat.stddev,
                stream_stat.mean, stream_stat.stddev,
                gap_pct, bottleneck);
}

int main(int argc, char ** argv) {
    pin_to_pcore();
    ggml_cpu_init();
    ggml_quantize_init(GGML_TYPE_IQ3_S);
    ggml_quantize_init(GGML_TYPE_IQ2_S);

    BenchConfig cfg = parse_args(argc, argv);

    if (cfg.compare_nrc) {
        std::printf("========================================================================================================================\n");
        std::printf("NRC Amortization Benchmark at Matched Total Work (iters = %d, trials = %d, K = %d)\n",
                    cfg.iters, cfg.trials, cfg.k);
        std::printf("Protocol: 7 trials, 2000 iters, P-core pinned, 4s cooldown. Unit: cyc/32 vals/seq.\n");
        std::printf("========================================================================================================================\n");
        std::printf("%-6s | %3s | %-13s | %-13s | %-16s | %-28s | %s\n",
                    "Type", "nrc", "nrc=1 (cyc/32)", "nrc=N (cyc/32)", "Speedup", "2-Sigma Significance", "Verdict");
        std::printf("-------+-----+---------------+---------------+------------------+------------------------------+--------------------\n");
        run_nrc_comparison_for_type(GGML_TYPE_IQ3_S, "IQ3_S", cfg);
        run_nrc_comparison_for_type(GGML_TYPE_IQ2_S, "IQ2_S", cfg);
        std::printf("========================================================================================================================\n");
        return 0;
    }

    std::printf("========================================================================================\n");
    std::printf("IQ Dequant & Fused Vector Dot Benchmark (nrc = %d, iters = %d, trials = %d, K = %d)\n",
                cfg.nrc, cfg.iters, cfg.trials, cfg.k);
    std::printf("========================================================================================\n");
    std::printf("%-10s | %-15s | %-15s | %-8s | %s\n",
                "Type", "Hot (cyc/32)", "Stream (cyc/32)", "Gap (%)", "Limiting Bottleneck");
    std::printf("-----------+-----------------+-----------------+----------+-----------------------------\n");

    benchmark_type(GGML_TYPE_F16,  "F16",  cfg);
    benchmark_type(GGML_TYPE_Q4_0, "Q4_0", cfg);
    benchmark_type(GGML_TYPE_IQ3_S,"IQ3_S",cfg);
    benchmark_type(GGML_TYPE_IQ2_S,"IQ2_S",cfg);

    std::printf("========================================================================================\n");
    return 0;
}
