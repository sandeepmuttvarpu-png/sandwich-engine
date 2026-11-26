#include "gguf.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

struct DispatchFixture {
    std::string name;
    uint64_t    model_bytes;
    int         total_shards;
    uint32_t    layers;
    bool        is_moe;
    uint32_t    vram_mb;
    uint32_t    host_mb;
    std::string spec_type;
    double      shard1_ratio; // e.g. 0.62 for 2-shard 62%/38% split
};

struct ParsedPlan {
    std::string raw_argv;
    int         ngl             = -999;
    int         threads         = -1;
    int         ctx             = -1;
    std::string ctk;
    std::string ctv;
    bool        fa              = false;
    std::string draft;
    int         ngld            = -999;
    int         moe_warm_ram_mb = -1;
    bool        io_uring        = false;
    bool        moe_2q          = false;

    bool operator==(const ParsedPlan & o) const {
        return ngl == o.ngl &&
               threads == o.threads &&
               ctx == o.ctx &&
               ctk == o.ctk &&
               ctv == o.ctv &&
               fa == o.fa &&
               draft == o.draft &&
               ngld == o.ngld &&
               moe_warm_ram_mb == o.moe_warm_ram_mb &&
               io_uring == o.io_uring &&
               moe_2q == o.moe_2q;
    }
};

static std::vector<std::string> split_shell_tokens(const std::string & s) {
    std::vector<std::string> out;
    std::string cur;
    bool in_quotes = false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (!in_quotes && (c == ' ' || c == '\t')) {
            if (!cur.empty()) {
                out.push_back(cur);
                cur.clear();
            }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) {
        out.push_back(cur);
    }
    return out;
}

static ParsedPlan parse_plan_from_stderr(const std::string & err_text) {
    ParsedPlan p;
    const std::string prefix = "SANDWICH_DISPATCH_PRINT argv:";
    size_t pos = err_text.rfind(prefix);
    if (pos == std::string::npos) {
        return p;
    }
    size_t line_start = pos + prefix.size();
    size_t line_end   = err_text.find('\n', line_start);
    p.raw_argv = err_text.substr(line_start, line_end == std::string::npos ? std::string::npos : line_end - line_start);

    const auto toks = split_shell_tokens(p.raw_argv);
    for (size_t i = 0; i < toks.size(); ++i) {
        const std::string & t = toks[i];
        if (t == "-ngl" && i + 1 < toks.size()) {
            p.ngl = std::atoi(toks[++i].c_str());
        } else if (t == "-t" && i + 1 < toks.size()) {
            p.threads = std::atoi(toks[++i].c_str());
        } else if (t == "-c" && i + 1 < toks.size()) {
            p.ctx = std::atoi(toks[++i].c_str());
        } else if (t == "-ctk" && i + 1 < toks.size()) {
            p.ctk = toks[++i];
        } else if (t == "-ctv" && i + 1 < toks.size()) {
            p.ctv = toks[++i];
        } else if (t == "-fa") {
            p.fa = true;
        } else if (t == "-md" && i + 1 < toks.size()) {
            p.draft = toks[++i];
        } else if (t == "-ngld" && i + 1 < toks.size()) {
            p.ngld = std::atoi(toks[++i].c_str());
        } else if (t == "--moe-warm-ram-mb" && i + 1 < toks.size()) {
            p.moe_warm_ram_mb = std::atoi(toks[++i].c_str());
        } else if (t == "--io-uring") {
            p.io_uring = true;
        } else if (t == "--moe-2q") {
            p.moe_2q = true;
        }
    }
    return p;
}

static bool run_capture_stderr(const std::vector<std::string> & argv, std::string & out_stderr) {
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        // Silence stdout
        FILE * devnull = std::fopen("/dev/null", "w");
        if (devnull) {
            dup2(fileno(devnull), STDOUT_FILENO);
            std::fclose(devnull);
        }
        setenv("SANDWICH_DISPATCH_PRINT", "1", 1);
        setenv("SANDWICH_AUTOTUNE_DISABLE", "1", 1);

        std::vector<char *> c_args;
        c_args.reserve(argv.size() + 1);
        for (const auto & a : argv) {
            c_args.push_back(const_cast<char *>(a.c_str()));
        }
        c_args.push_back(nullptr);
        execv(argv[0].c_str(), c_args.data());
        _exit(127);
    }

    close(pipefd[1]);
    out_stderr.clear();
    char buf[4096];
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
        out_stderr.append(buf, (size_t) n);
    }
    close(pipefd[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static fs::path create_fixture_gguf(const fs::path & dir, const DispatchFixture & f) {
    auto write_one = [&](const fs::path & path, uint64_t bytes, int split_no, int split_count) {
        struct gguf_context * g = gguf_init_empty();
        if (!g) {
            std::abort();
        }
        const char * arch = f.is_moe ? "qwen35moe" : "llama";
        gguf_set_val_str(g, "general.architecture", arch);
        gguf_set_val_u32(g, (std::string(arch) + ".block_count").c_str(), f.layers);
        gguf_set_val_u32(g, (std::string(arch) + ".attention.head_count").c_str(), 32);
        gguf_set_val_u32(g, (std::string(arch) + ".attention.head_count_kv").c_str(), 8);
        gguf_set_val_u32(g, (std::string(arch) + ".embedding_length").c_str(), 4096);
        if (f.is_moe) {
            gguf_set_val_u32(g, "general.expert_count", 128);
            gguf_set_val_u32(g, (std::string(arch) + ".expert_count").c_str(), 128);
            gguf_set_val_u32(g, (std::string(arch) + ".expert_used_count").c_str(), 8);
        }
        if (split_count > 1) {
            gguf_set_val_u16(g, "split.no", (uint16_t) split_no);
            gguf_set_val_u16(g, "split.count", (uint16_t) split_count);
        }
        gguf_set_val_str(g, "tokenizer.ggml.model", "llama");
        const char * toks[2] = { "<unk>", "<s>" };
        gguf_set_arr_str(g, "tokenizer.ggml.tokens", toks, 2);

        if (!gguf_write_to_file(g, path.c_str(), /*only_meta =*/ true)) {
            std::abort();
        }
        gguf_free(g);

        const uint64_t actual = (uint64_t) fs::file_size(path);
        if (bytes > actual) {
            fs::resize_file(path, bytes);
        }
    };

    if (f.total_shards <= 1) {
        fs::path p = dir / (f.name + ".gguf");
        write_one(p, f.model_bytes, 0, 1);
        return p;
    }

    fs::path primary;
    uint64_t remaining = f.model_bytes;
    for (int i = 0; i < f.total_shards; ++i) {
        char suffix[32];
        std::snprintf(suffix, sizeof(suffix), "-%05d-of-%05d.gguf", i + 1, f.total_shards);
        fs::path p = dir / (f.name + suffix);
        uint64_t sz = (i + 1 < f.total_shards)
            ? (uint64_t) (f.model_bytes * f.shard1_ratio)
            : remaining;
        if (sz > remaining) sz = remaining;
        remaining -= sz;
        write_one(p, sz, i, f.total_shards);
        if (i == 0) primary = p;
    }
    return primary;
}

static void print_plan(const char * label, const ParsedPlan & p) {
    std::printf("  %-18s ngl=%d t=%d c=%d ctk=%s ctv=%s fa=%d md=%s ngld=%d warm=%d uring=%d moe2q=%d\n",
                label, p.ngl, p.threads, p.ctx,
                p.ctk.c_str(), p.ctv.c_str(), (int) p.fa,
                p.draft.empty() ? "<none>" : p.draft.c_str(),
                p.ngld, p.moe_warm_ram_mb, (int) p.io_uring, (int) p.moe_2q);
}

int main() {
    std::error_code ec;
    const fs::path exe_dir = fs::canonical("/proc/self/exe", ec).parent_path();
    const fs::path sandwich_bin = exe_dir / "sandwich";
    const fs::path llama_cli_bin = exe_dir / "llama-cli";

    if (!fs::exists(sandwich_bin) || !fs::exists(llama_cli_bin)) {
        std::fprintf(stderr, "Missing binaries in %s\n", exe_dir.c_str());
        return 1;
    }

    const fs::path tmp_dir = fs::temp_directory_path() / ("sandwich_parity_" + std::to_string((int) getpid()));
    fs::remove_all(tmp_dir, ec);
    fs::create_directories(tmp_dir);

    // Fixture matrix: (name, model_bytes, total_shards, layers, is_moe, vram_mb, host_mb, spec_type, shard1_ratio)
    const std::vector<DispatchFixture> fixtures = {
        { "full-fit-dense-7b",   2000ULL * 1024 * 1024,  1, 32, false, 7767, 24000, "none", 1.00 },
        { "partial-dense-30b",  28000ULL * 1024 * 1024,  1, 60, false, 7767, 24000, "none", 1.00 },
        { "moe-2shard-62pct",   70910ULL * 1024 * 1024,  2, 48, true,  7767, 24000, "none", 0.62 },
        { "moe-1shard-20gb",    20000ULL * 1024 * 1024,  1, 40, true,  7767, 24000, "none", 1.00 },
        { "moe-full-fit-90pct",  6672ULL * 1024 * 1024,  1, 28, true,  7767, 24000, "none", 1.00 },
    };

    int failures = 0;
    for (const auto & f : fixtures) {
        const fs::path model_path = create_fixture_gguf(tmp_dir, f);

        std::string err_sandwich;
        std::string err_cli;

        std::vector<std::string> argv_sandwich = {
            sandwich_bin.string(),
            "-m", model_path.string(),
            "--spec-type", f.spec_type,
        };
        std::vector<std::string> argv_cli = {
            llama_cli_bin.string(),
            "-m", model_path.string(),
            "--sandwich-auto",
            "--spec-type", f.spec_type,
        };

        const bool ok_s = run_capture_stderr(argv_sandwich, err_sandwich);
        const bool ok_c = run_capture_stderr(argv_cli, err_cli);

        const ParsedPlan p_s = parse_plan_from_stderr(err_sandwich);
        const ParsedPlan p_c = parse_plan_from_stderr(err_cli);

        std::printf("[Fixture: %s | shards=%d | layers=%u | is_moe=%d | spec=%s]\n",
                    f.name.c_str(), f.total_shards, f.layers, (int) f.is_moe, f.spec_type.c_str());
        print_plan("sandwich:", p_s);
        print_plan("llama-cli:", p_c);

        if (!ok_s || !ok_c || p_s.raw_argv.empty() || p_c.raw_argv.empty() || !(p_s == p_c)) {
            std::printf("  -> FAIL (mismatch or missing SANDWICH_DISPATCH_PRINT)\n");
            ++failures;
        } else if (f.name == "moe-full-fit-90pct" &&
                   (p_c.ngl != 99 || p_c.io_uring || p_c.moe_2q || p_c.moe_warm_ram_mb > 0)) {
            std::printf("  -> FAIL (MoE full-fit fixture did not disable tiering)\n");
            ++failures;
        } else {
            std::printf("  -> PASS (identical plan)\n");
        }
    }

    fs::remove_all(tmp_dir, ec);
    std::printf("=== Parity Summary: %zu fixtures, %d failures ===\n", fixtures.size(), failures);
    return failures == 0 ? 0 : 1;
}
