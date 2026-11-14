#pragma once

#include <string>
#include <vector>

struct llama_ui_asset {
    std::string           name;
    const unsigned char * data = nullptr;
    std::size_t           size = 0;
    std::string           etag;
    std::string           type;
};

inline const llama_ui_asset * llama_ui_find_asset(const std::string &) { return nullptr; }
inline bool llama_ui_use_gzip() { return false; }
inline const std::vector<llama_ui_asset> & llama_ui_get_assets() {
    static const std::vector<llama_ui_asset> empty;
    return empty;
}
