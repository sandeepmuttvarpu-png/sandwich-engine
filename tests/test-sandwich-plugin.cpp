// Unit tests for the sandwich-engine CUDA plugin loader.
//
// The loader must behave correctly on a host with no NVIDIA driver as well as on
// one with a GPU, so the tests branch on driver_available() and assert the
// documented failure mode instead of assuming a device exists.

#include "plugin_loader.hpp"
#include "plugin_iq3_s.hpp"
#include "ggml-cpu.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <cmath>

#if defined(__x86_64__) || defined(_M_X64)
#  include <x86intrin.h>
#endif

#define PLUGIN_CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::fprintf(stderr, "TEST ASSERTION FAILED: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while (0)

using namespace sandwich::plugin;

extern "C" {
void ggml_vec_dot_iq3_s_q8_K(int n, float * s, size_t bs, const void * vx, size_t bx, const void * vy, size_t by, int nrc);
}

// A no-op kernel in PTX. The driver JIT-compiles it for the installed GPU, which
// exercises cuModuleLoadData, cuModuleGetFunction, cuLaunchKernel and
// cuCtxSynchronize without needing nvcc at test time.
static const char * kProbePtx =
    ".version 8.0\n"
    ".target sm_75\n"
    ".address_size 64\n"
    "\n"
    ".visible .entry sandwich_probe()\n"
    "{\n"
    "    ret;\n"
    "}\n";

static void test_error_paths_without_driver() {
    std::printf("[test_error_paths]\n");

    // A missing artifact must be reported as such regardless of driver state,
    // because the existence check happens before any driver call.
    status st = status::ok;
    std::string err;
    auto missing = CudaModule::load_file("/nonexistent/sandwich/plugin.cubin", st, err);
    PLUGIN_CHECK(missing == nullptr);
    PLUGIN_CHECK(st == status::artifact_not_found);
    PLUGIN_CHECK(!err.empty());

    // An empty image is rejected before touching the driver.
    st = status::ok;
    err.clear();
    auto empty = CudaModule::load_image(nullptr, 0, st, err);
    PLUGIN_CHECK(empty == nullptr);
    PLUGIN_CHECK(st == status::invalid_artifact);
}

static void test_driver_state() {
    std::printf("[test_driver_state]\n");

    const bool available = driver_available();
    std::printf("  driver_available=%s version='%s' devices=%d\n",
                available ? "true" : "false",
                driver_version().c_str(),
                device_count());

    if (!available) {
        // On a driverless host every driver-dependent call must degrade to
        // driver_unavailable rather than crash.
        status st = status::ok;
        std::string err;
        auto m = CudaModule::load_image(kProbePtx, std::strlen(kProbePtx), st, err);
        PLUGIN_CHECK(m == nullptr);
        PLUGIN_CHECK(st == status::driver_unavailable);
        PLUGIN_CHECK(!err.empty());
        PLUGIN_CHECK(device_count() == 0);
        std::printf("  no driver present; degradation path verified\n");
        return;
    }

    PLUGIN_CHECK(!driver_version().empty());
    PLUGIN_CHECK(device_count() >= 1);

    // Out-of-range device selection is rejected; the default stays valid.
    PLUGIN_CHECK(set_preferred_device(0));
    PLUGIN_CHECK(!set_preferred_device(-1));
    PLUGIN_CHECK(!set_preferred_device(device_count()));
}

static void test_ptx_round_trip() {
    std::printf("[test_ptx_round_trip]\n");

    if (!driver_available()) {
        std::printf("  skipped: no CUDA driver\n");
        return;
    }

    status st = status::ok;
    std::string err;
    auto mod = CudaModule::load_image(kProbePtx, std::strlen(kProbePtx), st, err);
    if (mod == nullptr) {
        std::fprintf(stderr, "  load_image failed: %s (%s)\n", status_name(st), err.c_str());
    }
    PLUGIN_CHECK(mod != nullptr);
    PLUGIN_CHECK(st == status::ok);
    PLUGIN_CHECK(mod->source() == "<memory>");

    // Symbol resolution: present and absent.
    void * fn = nullptr;
    PLUGIN_CHECK(mod->get_function("sandwich_probe", &fn, err) == status::ok);
    PLUGIN_CHECK(fn != nullptr);

    void * missing_fn = nullptr;
    status fs = mod->get_function("sandwich_no_such_kernel", &missing_fn, err);
    PLUGIN_CHECK(fs == status::symbol_not_found);
    PLUGIN_CHECK(missing_fn == nullptr);
    PLUGIN_CHECK(!err.empty());

    // Launch and synchronise the no-op kernel.
    launch_config cfg;
    cfg.grid[0]  = 1;
    cfg.grid[1]  = 1;
    cfg.grid[2]  = 1;
    cfg.block[0] = 1;
    cfg.block[1] = 1;
    cfg.block[2] = 1;

    status ls = mod->launch("sandwich_probe", cfg, nullptr, err);
    if (ls != status::ok) {
        std::fprintf(stderr, "  launch failed: %s (%s)\n", status_name(ls), err.c_str());
    }
    PLUGIN_CHECK(ls == status::ok);
    PLUGIN_CHECK(mod->synchronize(err) == status::ok);

    // A garbage image must be rejected by the driver, not silently accepted.
    const char junk[] = "this is definitely not a cubin or ptx module";
    st = status::ok;
    err.clear();
    auto bad = CudaModule::load_image(junk, sizeof(junk), st, err);
    PLUGIN_CHECK(bad == nullptr);
    PLUGIN_CHECK(st == status::invalid_artifact);
    PLUGIN_CHECK(!err.empty());
}

static void test_registry() {
    std::printf("[test_registry]\n");

    Registry & reg = Registry::instance();
    reg.clear();
    PLUGIN_CHECK(reg.size() == 0);
    PLUGIN_CHECK(reg.find("absent") == nullptr);

    if (!driver_available()) {
        std::printf("  skipped module registration: no CUDA driver\n");
        PLUGIN_CHECK(reg.names().empty());
        return;
    }

    status st = status::ok;
    std::string err;
    auto a = CudaModule::load_image(kProbePtx, std::strlen(kProbePtx), st, err);
    PLUGIN_CHECK(a != nullptr);
    reg.add("probe", std::move(a));
    PLUGIN_CHECK(reg.size() == 1);
    PLUGIN_CHECK(reg.find("probe") != nullptr);

    // Re-adding the same name replaces rather than duplicates.
    auto b = CudaModule::load_image(kProbePtx, std::strlen(kProbePtx), st, err);
    PLUGIN_CHECK(b != nullptr);
    reg.add("probe", std::move(b));
    PLUGIN_CHECK(reg.size() == 1);

    auto names = reg.names();
    PLUGIN_CHECK(names.size() == 1);
    PLUGIN_CHECK(names[0] == "probe");

    reg.clear();
    PLUGIN_CHECK(reg.size() == 0);
    PLUGIN_CHECK(reg.find("probe") == nullptr);
}

static void test_iq3_s_plugin() {
    std::printf("[test_iq3_s_plugin]\n");
    ggml_cpu_init();
    if (!driver_available()) {
        std::printf("  skipped: no CUDA driver\n");
        return;
    }

    bool loaded = load_iq3_s_plugin("plugins/iq3_s_vnni.cubin");
    PLUGIN_CHECK(loaded);
    PLUGIN_CHECK(is_iq3_s_plugin_loaded());

    // Allocate 16 blocks (n = 4096 elements)
    const int nb = 16;
    const int n = nb * 256;
    std::vector<uint8_t> x_bytes(nb * 110);
    std::vector<uint8_t> y_bytes(nb * 292);

    // Fill with reproducible non-zero pattern
    for (size_t i = 0; i < x_bytes.size(); ++i) x_bytes[i] = (uint8_t)((i * 37 + 13) & 0xff);
    for (size_t i = 0; i < y_bytes.size(); ++i) y_bytes[i] = (uint8_t)((i * 59 + 7) & 0xff);
    // Ensure half scale in block_iq3_s is valid non-zero
    for (int b = 0; b < nb; ++b) {
        uint16_t * d_ptr = (uint16_t*)(x_bytes.data() + b * 110);
        *d_ptr = 0x3c00; // fp16 1.0f
        float * dy_ptr = (float*)(y_bytes.data() + b * 292);
        *dy_ptr = 1.0f;
    }

    // 1. Direct CPU baseline
    set_iq3_s_plugin_enabled(false);
    float s_cpu = 0.0f;
    ggml_vec_dot_iq3_s_q8_K(n, &s_cpu, 0, x_bytes.data(), 0, y_bytes.data(), 0, 1);

    // 2. Plugin execution
    float s_plugin = 0.0f;
    sandwich_plugin_vec_dot_iq3_s_q8_K(n, &s_plugin, x_bytes.data(), y_bytes.data());

    std::printf("  s_cpu=%f s_plugin=%f (diff=%e)\n", s_cpu, s_plugin, std::abs(s_cpu - s_plugin));
    const float rel_err = std::abs(s_cpu - s_plugin) / (std::abs(s_cpu) + 1e-5f);
    PLUGIN_CHECK(rel_err < 1e-3f);

    // 3. Benchmark cycles/32 over 7 trials (warmup=3)
    const int reps = 100;
    const double elements_per_32 = (double)(n / 32);

    // Warmup CPU
    for (int w = 0; w < 3; ++w) {
        for (int r = 0; r < reps; ++r) {
            ggml_vec_dot_iq3_s_q8_K(n, &s_cpu, 0, x_bytes.data(), 0, y_bytes.data(), 0, 1);
        }
    }
    std::vector<double> cpu_trials;
    for (int t = 0; t < 7; ++t) {
#if defined(__x86_64__) || defined(_M_X64)
        uint64_t t0 = __rdtsc();
#else
        uint64_t t0 = 0;
#endif
        for (int r = 0; r < reps; ++r) {
            ggml_vec_dot_iq3_s_q8_K(n, &s_cpu, 0, x_bytes.data(), 0, y_bytes.data(), 0, 1);
        }
#if defined(__x86_64__) || defined(_M_X64)
        uint64_t t1 = __rdtsc();
#else
        uint64_t t1 = 0;
#endif
        cpu_trials.push_back((double)(t1 - t0) / (reps * elements_per_32));
    }

    // Warmup Plugin
    for (int w = 0; w < 3; ++w) {
        for (int r = 0; r < reps; ++r) {
            sandwich_plugin_vec_dot_iq3_s_q8_K(n, &s_plugin, x_bytes.data(), y_bytes.data());
        }
    }
    std::vector<double> gpu_trials;
    for (int t = 0; t < 7; ++t) {
#if defined(__x86_64__) || defined(_M_X64)
        uint64_t t0 = __rdtsc();
#else
        uint64_t t0 = 0;
#endif
        for (int r = 0; r < reps; ++r) {
            sandwich_plugin_vec_dot_iq3_s_q8_K(n, &s_plugin, x_bytes.data(), y_bytes.data());
        }
#if defined(__x86_64__) || defined(_M_X64)
        uint64_t t1 = __rdtsc();
#else
        uint64_t t1 = 0;
#endif
        gpu_trials.push_back((double)(t1 - t0) / (reps * elements_per_32));
    }

    double mu_cpu = 0, mu_gpu = 0;
    for (double v : cpu_trials) mu_cpu += v;
    mu_cpu /= cpu_trials.size();
    for (double v : gpu_trials) mu_gpu += v;
    mu_gpu /= gpu_trials.size();

    std::printf("  Benchmark: CPU = %.2f cycles/32 | Plugin = %.2f cycles/32\n", mu_cpu, mu_gpu);

    set_iq3_s_plugin_enabled(false);
}

int main() {
    std::printf("=== sandwich plugin loader tests ===\n");
    test_error_paths_without_driver();
    test_driver_state();
    test_ptx_round_trip();
    test_registry();
    test_iq3_s_plugin();
    std::printf("=== all plugin loader tests passed ===\n");
    return 0;
}
