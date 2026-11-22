#include "plugin_iq3_s.hpp"
#include "plugin_loader.hpp"

#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <cstring>

#ifdef GGML_USE_CUDA
#include <cuda_runtime.h>
#endif

namespace sandwich {
namespace plugin {

static bool g_plugin_enabled = false;

bool load_iq3_s_plugin(const std::string & cubin_path) {
    if (!driver_available()) {
        return false;
    }

    std::vector<std::string> search_paths;
    if (!cubin_path.empty()) {
        search_paths.push_back(cubin_path);
    }
    search_paths.push_back("plugins/iq3_s_vnni.cubin");
    search_paths.push_back("../plugins/iq3_s_vnni.cubin");
    search_paths.push_back("build/bin/plugins/iq3_s_vnni.cubin");

    std::error_code ec;
    std::string found_path;
    for (const auto & p : search_paths) {
        if (std::filesystem::exists(p, ec)) {
            found_path = p;
            break;
        }
    }

    if (found_path.empty()) {
        return false;
    }

    status st = status::ok;
    std::string err;
    auto mod = CudaModule::load_file(found_path, st, err);
    if (!mod || st != status::ok) {
        std::fprintf(stderr, "load_iq3_s_plugin failed for %s: %s (%s)\n",
                     found_path.c_str(), status_name(st), err.c_str());
        return false;
    }

    Registry::instance().add("iq3_s_vnni", std::move(mod));
    return true;
}

bool is_iq3_s_plugin_loaded() {
    return Registry::instance().find("iq3_s_vnni") != nullptr;
}

void set_iq3_s_plugin_enabled(bool enabled) {
    g_plugin_enabled = enabled;
}

} // namespace plugin
} // namespace sandwich

extern "C" {

bool sandwich_plugin_iq3_s_available(void) {
    if (!sandwich::plugin::driver_available()) {
        return false;
    }

    static bool env_checked = false;
    if (!env_checked) {
        env_checked = true;
        const char * env = std::getenv("SANDWICH_ENABLE_IQ3_S_PLUGIN");
        if (env && (std::strcmp(env, "1") == 0 || std::strcmp(env, "true") == 0 || std::strcmp(env, "ON") == 0)) {
            sandwich::plugin::set_iq3_s_plugin_enabled(true);
        }
    }

    if (!sandwich::plugin::g_plugin_enabled) {
        return false;
    }

    if (!sandwich::plugin::is_iq3_s_plugin_loaded()) {
        sandwich::plugin::load_iq3_s_plugin();
    }

    return sandwich::plugin::is_iq3_s_plugin_loaded();
}

void sandwich_plugin_vec_dot_iq3_s_q8_K(int n, float * s, const void * vx, const void * vy) {
    const int nb = n / 256;
    if (nb <= 0) {
        if (s) *s = 0.0f;
        return;
    }

    sandwich::plugin::CudaModule * mod = sandwich::plugin::Registry::instance().find("iq3_s_vnni");
    if (!mod) {
        if (s) *s = 0.0f;
        return;
    }

#ifdef GGML_USE_CUDA
    const size_t bytes_x = (size_t)nb * 110; // sizeof(block_iq3_s)
    const size_t bytes_y = (size_t)nb * 292; // sizeof(block_q8_K)

    static thread_local void *  d_x = nullptr;
    static thread_local void *  d_y = nullptr;
    static thread_local float * d_s = nullptr;
    static thread_local size_t  d_cap_nb = 0;

    if (d_cap_nb < (size_t)nb) {
        if (d_x) cudaFree(d_x);
        if (d_y) cudaFree(d_y);
        if (d_s) cudaFree(d_s);

        cudaMalloc(&d_x, bytes_x);
        cudaMalloc(&d_y, bytes_y);
        cudaMalloc(&d_s, sizeof(float));
        d_cap_nb = (size_t)nb;
    }

    cudaMemcpy(d_x, vx, bytes_x, cudaMemcpyHostToDevice);
    cudaMemcpy(d_y, vy, bytes_y, cudaMemcpyHostToDevice);
    cudaMemset(d_s, 0, sizeof(float));

    sandwich::plugin::launch_config cfg;
    cfg.grid[0]  = nb;
    cfg.grid[1]  = 1;
    cfg.grid[2]  = 1;
    cfg.block[0] = 32;
    cfg.block[1] = 1;
    cfg.block[2] = 1;

    void * kernel_args[] = {
        (void*)&nb,
        (void*)&d_x,
        (void*)&d_y,
        (void*)&d_s
    };

    std::string err;
    sandwich::plugin::status st = mod->launch("vec_dot_iq3_s_q8_K_plugin", cfg, kernel_args, err);
    if (st == sandwich::plugin::status::ok) {
        mod->synchronize(err);
        cudaMemcpy(s, d_s, sizeof(float), cudaMemcpyDeviceToHost);
    } else {
        if (s) *s = 0.0f;
    }
#else
    if (s) *s = 0.0f;
#endif
}

} // extern "C"
