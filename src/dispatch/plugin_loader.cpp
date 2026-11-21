// sandwich-engine: CUDA Driver API plugin loader (implementation)
//
// libcuda is resolved at runtime so this translation unit has no link-time
// dependency on the CUDA driver. Only the handful of driver entry points needed
// to load, resolve, and launch a kernel are declared here.

#include "plugin_loader.hpp"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

namespace sandwich {
namespace plugin {
namespace {

// ---------------------------------------------------------------------------
// Minimal CUDA Driver API surface
//
// These are declared locally rather than pulled from <cuda.h> so that the host
// build does not need the CUDA toolkit on its include path.
// ---------------------------------------------------------------------------

using CUresult = int;
using CUdevice = int;
struct CUmod_st;
struct CUfunc_st;
struct CUctx_st;
struct CUstream_st;
using CUmodule = CUmod_st *;
using CUfunction = CUfunc_st *;
using CUcontext = CUctx_st *;
using CUstream = CUstream_st *;

using PFN_cuInit                 = CUresult (*)(unsigned int);
using PFN_cuDriverGetVersion     = CUresult (*)(int *);
using PFN_cuDeviceGetCount       = CUresult (*)(int *);
using PFN_cuDeviceGet            = CUresult (*)(CUdevice *, int);
using PFN_cuCtxGetCurrent        = CUresult (*)(CUcontext *);
using PFN_cuCtxSetCurrent        = CUresult (*)(CUcontext);
using PFN_cuDevicePrimaryCtxRetain = CUresult (*)(CUcontext *, CUdevice);
using PFN_cuModuleLoad           = CUresult (*)(CUmodule *, const char *);
using PFN_cuModuleLoadData       = CUresult (*)(CUmodule *, const void *);
using PFN_cuModuleUnload         = CUresult (*)(CUmodule);
using PFN_cuModuleGetFunction    = CUresult (*)(CUfunction *, CUmodule, const char *);
using PFN_cuLaunchKernel         = CUresult (*)(CUfunction, unsigned, unsigned, unsigned,
                                                unsigned, unsigned, unsigned, unsigned,
                                                CUstream, void **, void **);
using PFN_cuCtxSynchronize       = CUresult (*)();
using PFN_cuGetErrorName         = CUresult (*)(CUresult, const char **);
using PFN_cuGetErrorString       = CUresult (*)(CUresult, const char **);

// ---------------------------------------------------------------------------
// Dynamic library shim
// ---------------------------------------------------------------------------

#if defined(_WIN32)
using lib_handle = HMODULE;
lib_handle open_lib(const char * name) { return ::LoadLibraryA(name); }
void * lib_symbol(lib_handle h, const char * name) {
    return reinterpret_cast<void *>(::GetProcAddress(h, name));
}
void close_lib(lib_handle h) { if (h != nullptr) { ::FreeLibrary(h); } }
constexpr const char * kDriverNames[] = {"nvcuda.dll"};
#else
using lib_handle = void *;
lib_handle open_lib(const char * name) { return ::dlopen(name, RTLD_NOW | RTLD_LOCAL); }
void * lib_symbol(lib_handle h, const char * name) { return ::dlsym(h, name); }
void close_lib(lib_handle h) { if (h != nullptr) { ::dlclose(h); } }
constexpr const char * kDriverNames[] = {"libcuda.so.1", "libcuda.so"};
#endif

// Converting a data pointer to a function pointer is not permitted by the C++
// standard, and -Wpedantic flags the direct cast. Round-tripping through memcpy
// is the portable equivalent and is what the dynamic-loader pattern expects.
template <typename Fn>
Fn symbol_as(void * p) {
    Fn fn = nullptr;
    static_assert(sizeof(fn) == sizeof(p), "function pointer and data pointer sizes differ");
    std::memcpy(&fn, &p, sizeof(fn));
    return fn;
}

// ---------------------------------------------------------------------------
// Driver singleton
// ---------------------------------------------------------------------------

struct Driver {
    lib_handle handle    = nullptr;
    bool       attempted = false;
    bool       ready     = false;
    std::string load_error;

    PFN_cuInit                   cuInit                   = nullptr;
    PFN_cuDriverGetVersion       cuDriverGetVersion       = nullptr;
    PFN_cuDeviceGetCount         cuDeviceGetCount         = nullptr;
    PFN_cuDeviceGet              cuDeviceGet              = nullptr;
    PFN_cuCtxGetCurrent          cuCtxGetCurrent          = nullptr;
    PFN_cuCtxSetCurrent          cuCtxSetCurrent          = nullptr;
    PFN_cuDevicePrimaryCtxRetain cuDevicePrimaryCtxRetain = nullptr;
    PFN_cuModuleLoad             cuModuleLoad             = nullptr;
    PFN_cuModuleLoadData         cuModuleLoadData         = nullptr;
    PFN_cuModuleUnload           cuModuleUnload           = nullptr;
    PFN_cuModuleGetFunction      cuModuleGetFunction      = nullptr;
    PFN_cuLaunchKernel           cuLaunchKernel           = nullptr;
    PFN_cuCtxSynchronize         cuCtxSynchronize         = nullptr;
    PFN_cuGetErrorName           cuGetErrorName           = nullptr;
    PFN_cuGetErrorString         cuGetErrorString         = nullptr;
};

Driver & driver() {
    static Driver d;
    static std::once_flag once;
    std::call_once(once, [] {
        Driver & drv = d;
        drv.attempted = true;
        for (const char * name : kDriverNames) {
            drv.handle = open_lib(name);
            if (drv.handle != nullptr) {
                break;
            }
        }
        if (drv.handle == nullptr) {
            drv.load_error = "could not open the CUDA driver library";
            return;
        }
        auto bind = [&drv](const char * sym, void ** out) {
            void * p = lib_symbol(drv.handle, sym);
            if (p == nullptr) {
                drv.load_error = std::string("missing driver symbol: ") + sym;
                return false;
            }
            *out = p;
            return true;
        };
        void * p_init = nullptr, * p_ver = nullptr, * p_cnt = nullptr, * p_get = nullptr;
        void * p_gcur = nullptr, * p_scur = nullptr, * p_retain = nullptr;
        void * p_mload = nullptr, * p_mdata = nullptr, * p_munload = nullptr;
        void * p_mgetfn = nullptr, * p_launch = nullptr, * p_sync = nullptr;
        void * p_ename = nullptr, * p_estr = nullptr;
        const bool bound =
            bind("cuInit", &p_init) && bind("cuDriverGetVersion", &p_ver) &&
            bind("cuDeviceGetCount", &p_cnt) && bind("cuDeviceGet", &p_get) &&
            bind("cuCtxGetCurrent", &p_gcur) && bind("cuCtxSetCurrent", &p_scur) &&
            bind("cuDevicePrimaryCtxRetain", &p_retain) &&
            bind("cuModuleLoad", &p_mload) && bind("cuModuleLoadData", &p_mdata) &&
            bind("cuModuleUnload", &p_munload) && bind("cuModuleGetFunction", &p_mgetfn) &&
            bind("cuLaunchKernel", &p_launch) && bind("cuCtxSynchronize", &p_sync) &&
            bind("cuGetErrorName", &p_ename) && bind("cuGetErrorString", &p_estr);
        if (!bound) {
            return;
        }
        drv.cuInit                   = symbol_as<PFN_cuInit>(p_init);
        drv.cuDriverGetVersion       = symbol_as<PFN_cuDriverGetVersion>(p_ver);
        drv.cuDeviceGetCount         = symbol_as<PFN_cuDeviceGetCount>(p_cnt);
        drv.cuDeviceGet              = symbol_as<PFN_cuDeviceGet>(p_get);
        drv.cuCtxGetCurrent          = symbol_as<PFN_cuCtxGetCurrent>(p_gcur);
        drv.cuCtxSetCurrent          = symbol_as<PFN_cuCtxSetCurrent>(p_scur);
        drv.cuDevicePrimaryCtxRetain = symbol_as<PFN_cuDevicePrimaryCtxRetain>(p_retain);
        drv.cuModuleLoad             = symbol_as<PFN_cuModuleLoad>(p_mload);
        drv.cuModuleLoadData         = symbol_as<PFN_cuModuleLoadData>(p_mdata);
        drv.cuModuleUnload           = symbol_as<PFN_cuModuleUnload>(p_munload);
        drv.cuModuleGetFunction      = symbol_as<PFN_cuModuleGetFunction>(p_mgetfn);
        drv.cuLaunchKernel           = symbol_as<PFN_cuLaunchKernel>(p_launch);
        drv.cuCtxSynchronize         = symbol_as<PFN_cuCtxSynchronize>(p_sync);
        drv.cuGetErrorName           = symbol_as<PFN_cuGetErrorName>(p_ename);
        drv.cuGetErrorString         = symbol_as<PFN_cuGetErrorString>(p_estr);

        if (drv.cuInit(0) != 0) {
            drv.load_error = "cuInit failed";
            return;
        }
        drv.ready = true;
    });
    return d;
}

std::atomic<int> g_preferred_device{0};

std::string cu_error(CUresult r) {
    Driver & d = driver();
    const char * name = nullptr;
    const char * desc = nullptr;
    if (d.cuGetErrorName != nullptr) {
        d.cuGetErrorName(r, &name);
    }
    if (d.cuGetErrorString != nullptr) {
        d.cuGetErrorString(r, &desc);
    }
    char buf[256];
    std::snprintf(buf, sizeof(buf), "CUDA error %d (%s): %s",
                  r,
                  name != nullptr ? name : "UNKNOWN",
                  desc != nullptr ? desc : "no description");
    return std::string(buf);
}

// Make a context current for this thread. Returns ok if one already is.
status ensure_context(std::string & error) {
    Driver & d = driver();
    if (!d.ready) {
        error = d.load_error.empty() ? "CUDA driver unavailable" : d.load_error;
        return status::driver_unavailable;
    }

    CUcontext current = nullptr;
    if (d.cuCtxGetCurrent(&current) == 0 && current != nullptr) {
        return status::ok;
    }

    int count = 0;
    if (d.cuDeviceGetCount(&count) != 0 || count <= 0) {
        error = "the CUDA driver reported no devices";
        return status::no_device;
    }

    int idx = g_preferred_device.load();
    if (idx < 0 || idx >= count) {
        idx = 0;
    }

    CUdevice dev = 0;
    if (d.cuDeviceGet(&dev, idx) != 0) {
        error = "cuDeviceGet failed for device " + std::to_string(idx);
        return status::no_device;
    }

    CUcontext ctx = nullptr;
    if (d.cuDevicePrimaryCtxRetain(&ctx, dev) != 0 || ctx == nullptr) {
        error = "cuDevicePrimaryCtxRetain failed for device " + std::to_string(idx);
        return status::no_context;
    }
    if (d.cuCtxSetCurrent(ctx) != 0) {
        error = "cuCtxSetCurrent failed for device " + std::to_string(idx);
        return status::no_context;
    }
    return status::ok;
}

bool file_exists(const std::string & path) {
    std::ifstream f(path, std::ios::binary);
    return f.good();
}

} // namespace

// ---------------------------------------------------------------------------
// public helpers
// ---------------------------------------------------------------------------

const char * status_name(status s) {
    switch (s) {
        case status::ok:                 return "ok";
        case status::driver_unavailable: return "driver_unavailable";
        case status::no_device:          return "no_device";
        case status::no_context:         return "no_context";
        case status::artifact_not_found: return "artifact_not_found";
        case status::invalid_artifact:   return "invalid_artifact";
        case status::symbol_not_found:   return "symbol_not_found";
        case status::launch_failed:      return "launch_failed";
        case status::sync_failed:        return "sync_failed";
    }
    return "unknown";
}

bool driver_available() {
    return driver().ready;
}

std::string driver_version() {
    Driver & d = driver();
    if (!d.ready) {
        return std::string();
    }
    int version = 0;
    if (d.cuDriverGetVersion(&version) != 0 || version <= 0) {
        return std::string();
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d.%d", version / 1000, (version % 1000) / 10);
    return std::string(buf);
}

int device_count() {
    Driver & d = driver();
    if (!d.ready) {
        return 0;
    }
    int count = 0;
    if (d.cuDeviceGetCount(&count) != 0 || count < 0) {
        return 0;
    }
    return count;
}

bool set_preferred_device(int index) {
    if (index < 0) {
        return false;
    }
    const int count = device_count();
    if (count > 0 && index >= count) {
        return false;
    }
    g_preferred_device.store(index);
    return true;
}

// ---------------------------------------------------------------------------
// CudaModule
// ---------------------------------------------------------------------------

struct CudaModule::impl {
    CUmodule module = nullptr;
};

CudaModule::CudaModule() : p_(new impl()) {}

CudaModule::~CudaModule() {
    if (p_ != nullptr && p_->module != nullptr) {
        Driver & d = driver();
        if (d.ready && d.cuModuleUnload != nullptr) {
            // Best effort: unloading requires a context, but a failure here has
            // no effect on correctness and the driver reclaims the module at
            // process exit regardless.
            d.cuModuleUnload(p_->module);
        }
        p_->module = nullptr;
    }
}

std::unique_ptr<CudaModule> CudaModule::load_file(const std::string & path, status & st, std::string & error) {
    if (!file_exists(path)) {
        st = status::artifact_not_found;
        error = "plugin artifact not found: " + path;
        return nullptr;
    }
    st = ensure_context(error);
    if (st != status::ok) {
        return nullptr;
    }
    std::unique_ptr<CudaModule> m(new CudaModule());
    Driver & d = driver();
    const CUresult r = d.cuModuleLoad(&m->p_->module, path.c_str());
    if (r != 0 || m->p_->module == nullptr) {
        st = status::invalid_artifact;
        error = "cuModuleLoad failed for " + path + ": " + cu_error(r);
        return nullptr;
    }
    m->source_ = path;
    st = status::ok;
    return m;
}

std::unique_ptr<CudaModule> CudaModule::load_image(const void * image, size_t size, status & st, std::string & error) {
    if (image == nullptr || size == 0) {
        st = status::invalid_artifact;
        error = "empty plugin image";
        return nullptr;
    }
    st = ensure_context(error);
    if (st != status::ok) {
        return nullptr;
    }
    std::unique_ptr<CudaModule> m(new CudaModule());
    Driver & d = driver();
    const CUresult r = d.cuModuleLoadData(&m->p_->module, image);
    if (r != 0 || m->p_->module == nullptr) {
        st = status::invalid_artifact;
        error = "cuModuleLoadData failed: " + cu_error(r);
        return nullptr;
    }
    m->source_ = "<memory>";
    st = status::ok;
    return m;
}

status CudaModule::get_function(const std::string & name, void ** out_fn, std::string & error) {
    if (out_fn == nullptr) {
        error = "null output pointer";
        return status::invalid_artifact;
    }
    *out_fn = nullptr;
    Driver & d = driver();
    if (!d.ready) {
        error = d.load_error.empty() ? "CUDA driver unavailable" : d.load_error;
        return status::driver_unavailable;
    }
    if (p_->module == nullptr) {
        error = "module is not loaded";
        return status::invalid_artifact;
    }
    CUfunction fn = nullptr;
    const CUresult r = d.cuModuleGetFunction(&fn, p_->module, name.c_str());
    if (r != 0 || fn == nullptr) {
        error = "kernel symbol '" + name + "' not found: " + cu_error(r);
        return status::symbol_not_found;
    }
    *out_fn = reinterpret_cast<void *>(fn);
    return status::ok;
}

status CudaModule::launch(const std::string & name, const launch_config & cfg,
                          void ** kernel_args, std::string & error) {
    void * fn = nullptr;
    const status fs = get_function(name, &fn, error);
    if (fs != status::ok) {
        return fs;
    }
    Driver & d = driver();
    CUfunction cu_fn = reinterpret_cast<CUfunction>(fn);
    CUstream stream = reinterpret_cast<CUstream>(cfg.stream);
    const CUresult r = d.cuLaunchKernel(cu_fn,
                                        cfg.grid[0], cfg.grid[1], cfg.grid[2],
                                        cfg.block[0], cfg.block[1], cfg.block[2],
                                        cfg.shared_bytes, stream, kernel_args, cfg.extra);
    if (r != 0) {
        error = "cuLaunchKernel('" + name + "') failed: " + cu_error(r);
        return status::launch_failed;
    }
    return status::ok;
}

status CudaModule::synchronize(std::string & error) {
    Driver & d = driver();
    if (!d.ready) {
        error = d.load_error.empty() ? "CUDA driver unavailable" : d.load_error;
        return status::driver_unavailable;
    }
    const CUresult r = d.cuCtxSynchronize();
    if (r != 0) {
        error = "cuCtxSynchronize failed: " + cu_error(r);
        return status::sync_failed;
    }
    return status::ok;
}

// ---------------------------------------------------------------------------
// Registry
// ---------------------------------------------------------------------------

Registry & Registry::instance() {
    static Registry r;
    return r;
}

void Registry::add(const std::string & name, std::unique_ptr<CudaModule> module) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto & kv : modules_) {
        if (kv.first == name) {
            kv.second = std::move(module);
            return;
        }
    }
    modules_.emplace_back(name, std::move(module));
}

CudaModule * Registry::find(const std::string & name) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto & kv : modules_) {
        if (kv.first == name) {
            return kv.second.get();
        }
    }
    return nullptr;
}

std::vector<std::string> Registry::names() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> out;
    out.reserve(modules_.size());
    for (const auto & kv : modules_) {
        out.push_back(kv.first);
    }
    return out;
}

size_t Registry::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return modules_.size();
}

void Registry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    modules_.clear();
}

} // namespace plugin
} // namespace sandwich
