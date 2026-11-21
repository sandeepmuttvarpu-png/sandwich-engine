// sandwich-engine: persistent kernel variant cache
//
// Header-only and dependency-free so it can be included from ggml-cuda translation
// units without pulling a JSON library or an ABI surface into ggml.
//
// Design notes:
//   * The cache only ever *overrides* a heuristic. A miss is not an error; the
//     caller keeps its compiled-in default. This makes the cache safe to ship
//     enabled and safe to delete at any time.
//   * Entries are keyed by a fingerprint that mixes device, shape, and cache
//     dtypes, bucketed so the entry count stays bounded across a long run.
//   * The on-disk format is one record per line. It is parsed defensively:
//     malformed lines are skipped rather than aborting inference.

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#if defined(_WIN32)
#  include <direct.h>
#  include <io.h>
#  define SANDWICH_MKDIR(p) _mkdir(p)
#else
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>
#  define SANDWICH_MKDIR(p) mkdir((p), 0755)
#endif

namespace sandwich {
namespace autotune {

// Bump when the meaning of a cached field changes. Old files are then ignored
// wholesale instead of being misread.
constexpr int CACHE_SCHEMA = 1;

// A kernel family is a group of interchangeable implementations of one op.
// Values are stable on disk; never renumber an existing family.
enum class family : int {
    fattn = 1, // FlashAttention decode/prefill kernel selection
};

struct entry {
    uint64_t fingerprint = 0;
    int      variant     = 0; // family-specific variant id
    int      ncols1      = 0; // tile shape, 0 when the variant does not use one
    int      ncols2      = 0;
    double   us_winner   = 0.0; // measured latency of the chosen variant
    double   us_baseline = 0.0; // measured latency of the heuristic default
};

// ---------------------------------------------------------------------------
// environment / paths
// ---------------------------------------------------------------------------

inline const char * env_or_null(const char * name) {
#if defined(_MSC_VER)
    // getenv is flagged as unsafe on MSVC; the value is only ever read.
    static char * buf = nullptr;
    static size_t cap = 0;
    if (buf != nullptr) {
        free(buf);
        buf = nullptr;
    }
    if (_dupenv_s(&buf, &cap, name) != 0 || buf == nullptr) {
        return nullptr;
    }
    return buf;
#else
    return std::getenv(name);
#endif
}

inline bool env_flag(const char * name) {
    const char * v = env_or_null(name);
    if (v == nullptr || v[0] == '\0') {
        return false;
    }
    if (std::strcmp(v, "0") == 0 || std::strcmp(v, "false") == 0 || std::strcmp(v, "off") == 0) {
        return false;
    }
    return true;
}

inline std::string cache_path() {
    const char * override_path = env_or_null("SANDWICH_KERNEL_CACHE");
    if (override_path != nullptr && override_path[0] != '\0') {
        return std::string(override_path);
    }
    const char * xdg = env_or_null("XDG_CACHE_HOME");
    if (xdg != nullptr && xdg[0] != '\0') {
        return std::string(xdg) + "/sandwich/kernel_cache.txt";
    }
    const char * home = env_or_null("HOME");
    if (home != nullptr && home[0] != '\0') {
        return std::string(home) + "/.cache/sandwich/kernel_cache.txt";
    }
    return std::string("sandwich-kernel-cache.txt");
}

// Recursively create the parent directory of `path`. Best effort: a failure here
// only costs persistence, never correctness.
inline void ensure_parent_dir(const std::string & path) {
    const size_t slash = path.find_last_of('/');
    if (slash == std::string::npos || slash == 0) {
        return;
    }
    const std::string dir = path.substr(0, slash);
    std::string acc;
    acc.reserve(dir.size());
    for (size_t i = 0; i < dir.size(); ++i) {
        acc.push_back(dir[i]);
        if (dir[i] == '/' && acc.size() > 1) {
            SANDWICH_MKDIR(acc.c_str());
        }
    }
    SANDWICH_MKDIR(acc.c_str());
}

// ---------------------------------------------------------------------------
// fingerprinting
// ---------------------------------------------------------------------------

// FNV-1a, 64-bit. Chosen for a tiny, allocation-free, well-mixed accumulator.
inline uint64_t fnv1a_mix(uint64_t h, uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        h ^= (v >> (8 * i)) & 0xFFull;
        h *= 1099511628211ull;
    }
    return h;
}

inline uint64_t fingerprint_begin() {
    return 1469598103934665603ull;
}

// Bucket a dimension so long contexts reuse cache entries instead of creating a
// new one per sequence length. Round up to the next power of two.
inline int bucket_pow2(int v) {
    if (v <= 1) {
        return 1;
    }
    int b = 1;
    while (b < v) {
        b <<= 1;
    }
    return b;
}

// Fingerprint for the FlashAttention family. Every input that can change which
// kernel wins must be represented here, otherwise the cache would serve a stale
// winner for a different problem.
inline uint64_t fingerprint_fattn(
        int device_cc_major, int device_cc_minor,
        int head_dim, int n_queries, int gqa_ratio,
        int k_type, int v_type,
        int kv_len, bool has_mask, bool has_bias) {
    uint64_t h = fingerprint_begin();
    h = fnv1a_mix(h, static_cast<uint64_t>(static_cast<int>(family::fattn)));
    h = fnv1a_mix(h, static_cast<uint64_t>(device_cc_major * 100 + device_cc_minor));
    h = fnv1a_mix(h, static_cast<uint64_t>(head_dim));
    h = fnv1a_mix(h, static_cast<uint64_t>(n_queries));
    h = fnv1a_mix(h, static_cast<uint64_t>(gqa_ratio));
    h = fnv1a_mix(h, static_cast<uint64_t>(k_type));
    h = fnv1a_mix(h, static_cast<uint64_t>(v_type));
    h = fnv1a_mix(h, static_cast<uint64_t>(bucket_pow2(kv_len)));
    h = fnv1a_mix(h, static_cast<uint64_t>(has_mask ? 1 : 0));
    h = fnv1a_mix(h, static_cast<uint64_t>(has_bias ? 1 : 0));
    return h;
}

// ---------------------------------------------------------------------------
// cache store
// ---------------------------------------------------------------------------

class Cache {
public:
    static Cache & instance() {
        static Cache c;
        return c;
    }

    // False when the user disabled autotuning; callers then never override.
    bool enabled() const {
        return !env_flag("SANDWICH_AUTOTUNE_DISABLE");
    }

    bool lookup(family fam, uint64_t fp, entry & out) {
        if (!enabled()) {
            return false;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        load_locked();
        const auto it = entries_.find(Key{fam, fp});
        if (it == entries_.end()) {
            return false;
        }
        out = it->second;
        return true;
    }

    // Insert or replace an entry and persist. The winner must be strictly faster
    // than the baseline, otherwise the caller should not have measured it.
    void store(family fam, const entry & e) {
        std::lock_guard<std::mutex> lock(mutex_);
        load_locked();
        entries_[Key{fam, e.fingerprint}] = e;
        save_locked();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.clear();
        loaded_ = true;
        save_locked();
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mutex_);
        load_locked();
        return entries_.size();
    }

    // Test seam: drop in-memory state so the next access re-reads from disk.
    void reset_in_memory() {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.clear();
        loaded_ = false;
    }

private:
    struct Key {
        family   fam;
        uint64_t fp;
        bool operator<(const Key & o) const {
            if (fam != o.fam) {
                return static_cast<int>(fam) < static_cast<int>(o.fam);
            }
            return fp < o.fp;
        }
    };

    Cache() = default;

    void load_locked() {
        if (loaded_) {
            return;
        }
        loaded_ = true;
        if (env_flag("SANDWICH_AUTOTUNE_INVALIDATE")) {
            return;
        }
        const std::string path = cache_path();
        FILE * f = std::fopen(path.c_str(), "rb");
        if (f == nullptr) {
            return;
        }
        char line[512];
        int schema = -1;
        while (std::fgets(line, sizeof(line), f) != nullptr) {
            if (line[0] == '#') {
                // "# schema N"
                int s = 0;
                if (std::sscanf(line, "# schema %d", &s) == 1) {
                    schema = s;
                }
                continue;
            }
            if (schema != CACHE_SCHEMA) {
                continue;
            }
            int fam = 0;
            unsigned long long fp = 0;
            entry e;
            int variant = 0, ncols1 = 0, ncols2 = 0;
            double usw = 0.0, usb = 0.0;
            if (std::sscanf(line, "%d %llu %d %d %d %lf %lf",
                            &fam, &fp, &variant, &ncols1, &ncols2, &usw, &usb) != 7) {
                continue;
            }
            e.fingerprint = static_cast<uint64_t>(fp);
            e.variant     = variant;
            e.ncols1      = ncols1;
            e.ncols2      = ncols2;
            e.us_winner   = usw;
            e.us_baseline = usb;
            entries_[Key{static_cast<family>(fam), e.fingerprint}] = e;
        }
        std::fclose(f);
    }

    void save_locked() {
        const std::string path = cache_path();
        ensure_parent_dir(path);
        const std::string tmp = path + ".tmp";
        FILE * f = std::fopen(tmp.c_str(), "wb");
        if (f == nullptr) {
            return;
        }
        std::fprintf(f, "# sandwich kernel cache\n");
        std::fprintf(f, "# schema %d\n", CACHE_SCHEMA);
        for (const auto & kv : entries_) {
            const entry & e = kv.second;
            std::fprintf(f, "%d %llu %d %d %d %.6f %.6f\n",
                         static_cast<int>(kv.first.fam),
                         static_cast<unsigned long long>(e.fingerprint),
                         e.variant, e.ncols1, e.ncols2,
                         e.us_winner, e.us_baseline);
        }
        std::fclose(f);
#if defined(_WIN32)
        std::remove(path.c_str());
#endif
        if (std::rename(tmp.c_str(), path.c_str()) != 0) {
            std::remove(tmp.c_str());
        }
    }

    std::mutex mutex_;
    bool loaded_ = false;
    std::map<Key, entry> entries_;
};

} // namespace autotune
} // namespace sandwich
