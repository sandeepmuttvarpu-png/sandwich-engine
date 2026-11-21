// sandwich-engine: runtime kernel plugin loading via the CUDA Driver API
//
// Goals
//   * Load a specialized kernel (a .cubin, or PTX that the driver JITs) at
//     runtime, after the engine has already started.
//   * No C++ ABI coupling: plugins are addressed by symbol name and launched
//     through the driver API with a plain argument array, exactly like any other
//     externally built CUDA object.
//   * No hard dependency on the CUDA driver at link time. libcuda is opened with
//     dlopen() so a CPU-only host, or a host without an NVIDIA GPU, still builds
//     and runs; plugin use simply reports driver_unavailable.
//
// Threading and context rules
//   The CUDA driver associates a module with the context that was current when
//   it was loaded. A module must therefore be launched from the same thread and
//   with the same context current. This module does not migrate contexts; if no
//   context is current it retains the primary context of the selected device and
//   makes it current for the calling thread.

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace sandwich {
namespace plugin {

enum class status {
    ok = 0,
    driver_unavailable, // libcuda.so.1 could not be opened or initialised
    no_device,          // the driver reported zero devices
    no_context,         // no current context and the primary context could not be retained
    artifact_not_found, // the requested .cubin/.ptx path does not exist
    invalid_artifact,   // the driver rejected the module image
    symbol_not_found,   // the requested kernel name is absent from the module
    launch_failed,      // cuLaunchKernel returned an error
    sync_failed,        // cuCtxSynchronize returned an error
};

const char * status_name(status s);

// True when libcuda.so.1 was opened and cuInit succeeded.
bool driver_available();

// Driver version as "major.minor", or an empty string when unavailable.
std::string driver_version();

// Number of CUDA devices visible to the driver, or 0 when unavailable.
int device_count();

// Selects which device's primary context is retained when no context is
// current. Defaults to 0. Returns false if the device index is out of range.
bool set_preferred_device(int index);

struct launch_config {
    unsigned grid[3]        = {1, 1, 1};
    unsigned block[3]       = {1, 1, 1};
    unsigned shared_bytes   = 0;
    void *   stream         = nullptr; // CUstream, or nullptr for the default stream
    void **  extra          = nullptr; // reserved for future launch extensions
};

class CudaModule {
public:
    // Load a .cubin or .ptx file. PTX is JIT-compiled by the driver.
    static std::unique_ptr<CudaModule> load_file(const std::string & path, status & st, std::string & error);

    // Load a module from an in-memory image (cubin bytes, or PTX text). The
    // buffer only needs to be valid for the duration of this call.
    static std::unique_ptr<CudaModule> load_image(const void * image, size_t size, status & st, std::string & error);

    ~CudaModule();

    CudaModule(const CudaModule &)            = delete;
    CudaModule & operator=(const CudaModule &) = delete;

    // Resolve a kernel symbol. The returned handle stays valid until the module
    // is destroyed.
    status get_function(const std::string & name, void ** out_fn, std::string & error);

    // Resolve and launch in one step. `kernel_args` is the array of pointers to
    // the kernel parameters, in declaration order (the same array cuLaunchKernel
    // expects).
    status launch(const std::string & name, const launch_config & cfg,
                  void ** kernel_args, std::string & error);

    status synchronize(std::string & error);

    const std::string & source() const { return source_; }

private:
    CudaModule();
    struct impl;
    std::unique_ptr<impl> p_;
    std::string           source_;
};

// A name -> module registry so plugins can be resolved by logical name at
// runtime without the caller holding every handle.
class Registry {
public:
    static Registry & instance();

    // Replaces any existing entry with the same name.
    void add(const std::string & name, std::unique_ptr<CudaModule> module);

    // Returns a borrowed pointer, or nullptr. The module stays owned by the
    // registry.
    CudaModule * find(const std::string & name);

    std::vector<std::string> names() const;
    size_t size() const;
    void clear();

private:
    Registry() = default;
    mutable std::mutex                       mutex_;
    std::vector<std::pair<std::string, std::unique_ptr<CudaModule>>> modules_;
};

} // namespace plugin
} // namespace sandwich
