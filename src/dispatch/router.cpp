#include "router.hpp"
#include "hardware_profiler.hpp"
#include "model_scanner.hpp"
#include "autotune.hpp"
#include "common.h"
#include "../../ggml/src/ggml-cuda/sandwich-kernel-cache.cuh"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <filesystem>

// Forward declaration of engine execution entry point
int llama_cli(int argc, char ** argv);

namespace sandwich {
namespace dispatch {

bool Router::is_manual_cli_invocation(int argc, char ** argv) {
    if (argc <= 1) return false;

    // Check if any argument is a standard inference flag
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-m" || arg == "--model" ||
            arg == "-ngl" || arg == "--n-gpu-layers" ||
            arg == "-fitt" || arg == "--fit-target" ||
            arg == "-t" || arg == "--threads" ||
            arg == "-c" || arg == "--ctx-size" ||
            arg == "-p" || arg == "--prompt" ||
            arg == "-i" || arg == "--interactive" ||
            arg == "run" || arg == "cli" || arg == "serve" || arg == "bench") {
            return true;
        }
    }
    return false;
}

void Router::print_help(const char * progname) {
    std::cout << "\033[1;38;2;229;169;59m🥪 Sandwich.cpp ── Next-Generation Independent Inference Engine\033[0m\n\n";
    std::cout << "\033[1mUSAGE:\033[0m\n";
    std::cout << "  " << progname << "                  Launch Interactive Strata TUI Wizard\n";
    std::cout << "  " << progname << " --tui            Explicitly launch Strata TUI Wizard\n";
    std::cout << "  " << progname << " [flags...]       Run direct CLI inference (bypasses TUI)\n\n";

    std::cout << "\033[1mAUTO-DISPATCH & PROFILING COMMANDS:\033[0m\n";
    std::cout << "  --tune, --profile           Run hardware profiling and display JSON diagnostics\n";
    std::cout << "  --scan-models               Force rescan of local GGUF models into cache\n";
    std::cout << "  --autotune                  Benchmark FlashAttention kernels and cache >2σ winners\n";
    std::cout << "  --autotune-disable          Disable kernel autotuning cache lookup & storage\n";
    std::cout << "  --autotune-invalidate       Clear kernel autotuning cache and re-tune\n";
    std::cout << "  -h, --help                  Show this help overview\n\n";

    std::cout << "\033[1mCORE INFERENCE FLAGS (Bypass Mode):\033[0m\n";
    std::cout << "  -m, --model <path>          Path to GGUF model file\n";
    std::cout << "  -md, --model-draft <path>   Path to speculative decoding drafter GGUF\n";
    std::cout << "  -fitt <mb>                  Target VRAM budget in MB (Enforces 7,100 MB Guard)\n";
    std::cout << "  -t, --threads <n>           Number of compute threads (Optimal: physical P-cores)\n";
    std::cout << "  -c, --ctx-size <n>          Context window size (default: 4096)\n";
    std::cout << "  -fa, --flash-attn           Enable Flash Attention 2\n";
    std::cout << "  -sm row                     MoE row-split cache tiering across VRAM/RAM/NVMe\n";
    std::cout << "  -ctk, -ctv <type>           KV cache quantization (e.g. turbo3, turbo4, turbo2, f16)\n";
    std::cout << "  --chunked-prefill           Enable micro-chunked prompt prefill (prevents MoE memory stalls)\n";
    std::cout << "  --dynamic-watermark         Live memory watermarking (HWM 85% / LWM 70% Wayland protection)\n";
    std::cout << "  --asym-prefetch             Asymmetric parallel prefetch (pinned DMA + async lookahead)\n";
    std::cout << "  --ram-cache-mb <mb>         Dedicated Host RAM L2 Cache budget (2MB THP Pinned)\n";
}

int Router::run_standalone_profiling(bool force) {
    std::cout << "\033[1;38;2;78;201;176mExecuting Sandwich Hardware Profiler...\033[0m\n";
    HardwareProfile p = HardwareProfiler::run_profiling(force);
    std::cout << p.to_json() << std::endl;
    std::cout << "\n\033[38;2;34;197;94m✔ Hardware profile stored at " << HardwareProfiler::get_profile_path() << "\033[0m\n";
    return 0;
}

int Router::run_standalone_scan(const std::vector<std::string> & paths) {
    std::cout << "\033[1;38;2;78;201;176mScanning filesystem for GGUF models and speculative drafters...\033[0m\n";
    ModelCatalog cat = ModelScanner::run_full_scan(paths);

    std::cout << "\n\033[1mDiscovered Base Models (" << cat.models.size() << "):\033[0m\n";
    for (const auto & m : cat.models) {
        std::cout << "  • " << std::left << std::setw(42) << m.display_name
                  << " [" << std::setw(8) << m.quantization << "] "
                  << std::fixed << std::setprecision(2) << std::setw(6) << m.size_gb << " GB  "
                  << (m.is_moe ? "(MoE)" : "(Dense)") << "\n";
    }

    std::cout << "\n\033[1mDiscovered Speculative Drafters (" << cat.drafters.size() << "):\033[0m\n";
    for (const auto & d : cat.drafters) {
        std::cout << "  • " << std::left << std::setw(42) << d.filename
                  << " [" << std::setw(8) << d.quantization << "] "
                  << std::fixed << std::setprecision(2) << std::setw(6) << d.size_gb << " GB\n";
    }

    std::cout << "\n\033[38;2;34;197;94m✔ Model catalog written to " << ModelScanner::get_cache_path() << "\033[0m\n";
    return 0;
}

int Router::run_standalone_autotune(int argc, char ** argv) {
    sandwich::autotune::TuneOptions opts;
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "-m" || a == "--model") && i + 1 < argc) {
            opts.model_path = argv[++i];
        } else if ((a == "-c" || a == "--ctx-size") && i + 1 < argc) {
            opts.ctx_size = std::atoi(argv[++i]);
        } else if ((a == "-b" || a == "--batch-size") && i + 1 < argc) {
            opts.batch_size = std::atoi(argv[++i]);
        } else if (a == "--autotune-invalidate") {
            opts.force = true;
        }
    }
    if (opts.model_path.empty()) {
        const std::string canonical_qwen = "/home/sandeep/ai-models/moe/Qwen3.6-35B-A3B-MTP-GGUF/Qwen3.6-35B-A3B-UD-IQ3_XXS.gguf";
        std::error_code ec;
        if (std::filesystem::exists(canonical_qwen, ec)) {
            opts.model_path = canonical_qwen;
        } else {
            ModelCatalog cat = ModelScanner::load_or_scan();
            if (!cat.models.empty()) {
                opts.model_path = cat.models.front().file_path;
            }
        }
    }
    sandwich::autotune::run_autotune(opts);
    return 0;
}

int Router::auto_calibrate_and_dispatch(int argc, char ** argv) {
    bool has_model         = false;
    bool has_ngl           = false;
    bool has_sandwich_auto = false;

    int start_i = 1;
    if (argc >= 2 && std::strcmp(argv[1], "run") == 0) {
        start_i = 2;
    }

    for (int i = start_i; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "-m" || a == "--model") && i + 1 < argc) {
            has_model = true;
        } else if (a == "-ngl" || a == "--n-gpu-layers") {
            has_ngl = true;
        } else if (a == "--sandwich-auto") {
            has_sandwich_auto = true;
        }
    }

    // If no model specified or user explicitly supplied manual -ngl, pass through directly to llama_cli
    if (!has_model || has_ngl) {
        if (argc >= 2 && std::strcmp(argv[1], "run") == 0) {
            return llama_cli(argc - 1, argv + 1);
        }
        return llama_cli(argc, argv);
    }

    // Delegate auto-calibration to the single unified planner in llama_cli (`sandwich_auto_dispatch`).
    std::vector<std::string> args;
    args.reserve((size_t) argc + 2);
    args.push_back(argv[0]);
    for (int i = start_i; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    if (!has_sandwich_auto) {
        args.push_back("--sandwich-auto");
    }

    std::vector<char *> c_args;
    c_args.reserve(args.size());
    for (const auto & s : args) {
        c_args.push_back(const_cast<char *>(s.c_str()));
    }

    return llama_cli(static_cast<int>(c_args.size()), c_args.data());
}

int Router::route(int argc, char ** argv) {
    sandwich_set_autotune_fn(&sandwich::autotune::ensure_autotuned);

    // Process autotuning control flags globally across any subcommand
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--autotune-disable") == 0) {
            setenv("SANDWICH_AUTOTUNE_DISABLE", "1", 1);
            sandwich::autotune::Cache::notify_env_change();
        } else if (std::strcmp(argv[i], "--autotune-invalidate") == 0) {
            sandwich::autotune::Cache::instance().clear();
            setenv("SANDWICH_AUTOTUNE_INVALIDATE", "1", 1);
            sandwich::autotune::Cache::notify_env_change();
        }
    }

    // Case 1: Standalone Profiler flag
    if (argc >= 2 && (std::strcmp(argv[1], "--tune") == 0 || std::strcmp(argv[1], "--profile") == 0)) {
        return run_standalone_profiling(true);
    }

    // Case 2: Standalone Autotuner flag
    if (argc >= 2 && std::strcmp(argv[1], "--autotune") == 0) {
        return run_standalone_autotune(argc, argv);
    }

    // Case 3: Standalone Model Scanner flag
    if (argc >= 2 && std::strcmp(argv[1], "--scan-models") == 0) {
        std::vector<std::string> custom_paths;
        for (int i = 2; i < argc; ++i) {
            custom_paths.push_back(argv[i]);
        }
        return run_standalone_scan(custom_paths);
    }

    // Case 3: Help
    if (argc >= 2 && (std::strcmp(argv[1], "-h") == 0 || std::strcmp(argv[1], "--help") == 0)) {
        print_help(argv[0]);
        return 0;
    }

    // Case 4: Manual CLI flags or auto-dispatch
    if (is_manual_cli_invocation(argc, argv)) {
        return auto_calibrate_and_dispatch(argc, argv);
    }

    // Case 5: Interactive Strata TUI (default for argc == 1 or --tui)
    if (argc == 1 || (argc == 2 && (std::strcmp(argv[1], "--tui") == 0 || std::strcmp(argv[1], "-ui") == 0))) {
        if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
            print_help(argv[0]);
            return 0;
        }

        std::filesystem::path prog = argv[0];
        std::filesystem::path tui_path = prog.parent_path() / "sandwich-tui";
        std::error_code ec;

        if (std::filesystem::exists(tui_path, ec)) {
            char * const tui_argv[] = { const_cast<char*>("sandwich-tui"), nullptr };
            execv(tui_path.c_str(), tui_argv);
        } else if (std::filesystem::exists("./build/bin/sandwich-tui", ec)) {
            char * const tui_argv[] = { const_cast<char*>("sandwich-tui"), nullptr };
            execv("./build/bin/sandwich-tui", tui_argv);
        } else {
            char * const tui_argv[] = { const_cast<char*>("sandwich-tui"), nullptr };
            execvp("sandwich-tui", tui_argv);
        }

        print_help(argv[0]);
        return 0;
    }

    // Fallback: print help
    print_help(argv[0]);
    return 1;
}

} // namespace dispatch
} // namespace sandwich
