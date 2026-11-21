// Unit tests for the sandwich-engine CUDA plugin loader.
//
// The loader must behave correctly on a host with no NVIDIA driver as well as on
// one with a GPU, so the tests branch on driver_available() and assert the
// documented failure mode instead of assuming a device exists.

#include "plugin_loader.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#define PLUGIN_CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::fprintf(stderr, "TEST ASSERTION FAILED: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while (0)

using namespace sandwich::plugin;

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

int main() {
    std::printf("=== sandwich plugin loader tests ===\n");
    test_error_paths_without_driver();
    test_driver_state();
    test_ptx_round_trip();
    test_registry();
    std::printf("=== all plugin loader tests passed ===\n");
    return 0;
}
