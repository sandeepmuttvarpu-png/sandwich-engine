#pragma once

#include <cstddef>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

// Plugin discovery and dispatch entry points for IQ3_S dot products
bool sandwich_plugin_iq3_s_available(void);
void sandwich_plugin_vec_dot_iq3_s_q8_K(int n, float * s, const void * vx, const void * vy);

#ifdef __cplusplus
}

namespace sandwich {
namespace plugin {

bool load_iq3_s_plugin(const std::string & cubin_path = "");
bool is_iq3_s_plugin_loaded();
void set_iq3_s_plugin_enabled(bool enabled);

} // namespace plugin
} // namespace sandwich
#endif
