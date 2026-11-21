// Unit tests for the sandwich-engine persistent kernel cache.
//
// The cache is deliberately dependency-free and header-only, so it can be tested
// on CPU without a CUDA device present.

#include "../ggml/src/ggml-cuda/sandwich-kernel-cache.cuh"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#if !defined(_WIN32)
#  include <unistd.h>
#endif

#define CACHE_CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::fprintf(stderr, "TEST ASSERTION FAILED: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while (0)

using namespace sandwich::autotune;

static void set_env(const char * k, const char * v) {
#if !defined(_WIN32)
    if (v == nullptr) {
        unsetenv(k);
    } else {
        setenv(k, v, 1);
    }
#else
    (void) k;
    (void) v;
#endif
}

static std::string temp_cache_path(const char * tag) {
#if !defined(_WIN32)
    char buf[512];
    std::snprintf(buf, sizeof(buf), "/tmp/sandwich-kernel-cache-test-%ld-%s.txt",
                  static_cast<long>(getpid()), tag);
    return std::string(buf);
#else
    return std::string("sandwich-kernel-cache-test-") + tag + ".txt";
#endif
}

// ---------------------------------------------------------------------------

static void test_bucket_pow2() {
    std::printf("[test_bucket_pow2]\n");
    CACHE_CHECK(bucket_pow2(0) == 1);
    CACHE_CHECK(bucket_pow2(1) == 1);
    CACHE_CHECK(bucket_pow2(2) == 2);
    CACHE_CHECK(bucket_pow2(3) == 4);
    CACHE_CHECK(bucket_pow2(4096) == 4096);
    CACHE_CHECK(bucket_pow2(4097) == 8192);
    // Two nearby long contexts must land in the same bucket, or the cache would
    // grow one entry per sequence length.
    CACHE_CHECK(bucket_pow2(5000) == bucket_pow2(6000));
}

static void test_fingerprint_stability_and_sensitivity() {
    std::printf("[test_fingerprint_stability]\n");

    const uint64_t a = fingerprint_fattn(12, 0, 128, 1, 4, 1, 1, 4096, true, false);
    const uint64_t b = fingerprint_fattn(12, 0, 128, 1, 4, 1, 1, 4096, true, false);
    CACHE_CHECK(a == b);

    // Every input that can change the winning kernel must change the fingerprint.
    CACHE_CHECK(a != fingerprint_fattn(12, 0, 64,  1, 4, 1, 1, 4096, true, false)); // head_dim
    CACHE_CHECK(a != fingerprint_fattn(12, 0, 128, 7, 4, 1, 1, 4096, true, false)); // n_queries (Q=7 verify)
    CACHE_CHECK(a != fingerprint_fattn(12, 0, 128, 1, 8, 1, 1, 4096, true, false)); // gqa
    CACHE_CHECK(a != fingerprint_fattn(12, 0, 128, 1, 4, 8, 1, 4096, true, false)); // K type
    CACHE_CHECK(a != fingerprint_fattn(12, 0, 128, 1, 4, 1, 8, 4096, true, false)); // V type
    CACHE_CHECK(a != fingerprint_fattn(12, 0, 128, 1, 4, 1, 1, 4096, false, false)); // mask
    CACHE_CHECK(a != fingerprint_fattn(12, 0, 128, 1, 4, 1, 1, 4096, true, true));   // bias
    CACHE_CHECK(a != fingerprint_fattn(8, 9, 128, 1, 4, 1, 1, 4096, true, false));   // device

    // Bucketed context length: same bucket collides by design.
    CACHE_CHECK(fingerprint_fattn(12, 0, 128, 1, 4, 1, 1, 4096, true, false) ==
                fingerprint_fattn(12, 0, 128, 1, 4, 1, 1, 4000, true, false));
    // Different bucket must not collide.
    CACHE_CHECK(fingerprint_fattn(12, 0, 128, 1, 4, 1, 1, 4096, true, false) !=
                fingerprint_fattn(12, 0, 128, 1, 4, 1, 1, 8192, true, false));
}

static void test_store_lookup_persist() {
    std::printf("[test_store_lookup]\n");

    const std::string path = temp_cache_path("persist");
    std::remove(path.c_str());
    set_env("SANDWICH_KERNEL_CACHE", path.c_str());
    set_env("SANDWICH_AUTOTUNE_DISABLE", nullptr);
    set_env("SANDWICH_AUTOTUNE_INVALIDATE", nullptr);

    Cache & c = Cache::instance();
    c.clear();

    const uint64_t fp = fingerprint_fattn(12, 0, 128, 7, 4, 1, 1, 4096, true, false);

    entry miss;
    CACHE_CHECK(!c.lookup(family::fattn, fp, miss));

    entry e;
    e.fingerprint = fp;
    e.variant     = 400; // BEST_FATTN_KERNEL_MMA_F16
    e.ncols1      = 4;
    e.ncols2      = 8;
    e.us_winner   = 120.5;
    e.us_baseline = 180.25;
    c.store(family::fattn, e);

    entry hit;
    CACHE_CHECK(c.lookup(family::fattn, fp, hit));
    CACHE_CHECK(hit.variant == 400);
    CACHE_CHECK(hit.ncols1 == 4);
    CACHE_CHECK(hit.ncols2 == 8);
    CACHE_CHECK(hit.us_winner > 120.4 && hit.us_winner < 120.6);
    CACHE_CHECK(hit.us_baseline > 180.2 && hit.us_baseline < 180.3);

    // Persistence: drop memory, reload from disk.
    c.reset_in_memory();
    entry reloaded;
    CACHE_CHECK(c.lookup(family::fattn, fp, reloaded));
    CACHE_CHECK(reloaded.variant == 400);

    // A different family must not collide with the same fingerprint.
    entry other;
    CACHE_CHECK(!c.lookup(static_cast<family>(99), fp, other));

    // Atomic write must not leave a temp file behind.
    const std::string tmp = path + ".tmp";
    FILE * leftover = std::fopen(tmp.c_str(), "rb");
    CACHE_CHECK(leftover == nullptr);
    if (leftover != nullptr) {
        std::fclose(leftover);
    }

    std::remove(path.c_str());
    c.reset_in_memory();
}

static void test_disable_switch() {
    std::printf("[test_disable_switch]\n");

    const std::string path = temp_cache_path("disable");
    std::remove(path.c_str());
    set_env("SANDWICH_KERNEL_CACHE", path.c_str());
    set_env("SANDWICH_AUTOTUNE_DISABLE", nullptr);

    Cache & c = Cache::instance();
    c.clear();

    const uint64_t fp = fingerprint_fattn(12, 0, 64, 1, 8, 1, 1, 2048, true, false);
    entry e;
    e.fingerprint = fp;
    e.variant     = 100; // BEST_FATTN_KERNEL_VEC
    c.store(family::fattn, e);

    entry hit;
    CACHE_CHECK(c.lookup(family::fattn, fp, hit));

    set_env("SANDWICH_AUTOTUNE_DISABLE", "1");
    CACHE_CHECK(!c.enabled());
    CACHE_CHECK(!c.lookup(family::fattn, fp, hit));
    set_env("SANDWICH_AUTOTUNE_DISABLE", nullptr);

    std::remove(path.c_str());
    c.reset_in_memory();
}

static void test_malformed_and_schema_guard() {
    std::printf("[test_malformed_and_schema_guard]\n");

    const std::string path = temp_cache_path("malformed");
    set_env("SANDWICH_KERNEL_CACHE", path.c_str());
    set_env("SANDWICH_AUTOTUNE_DISABLE", nullptr);
    set_env("SANDWICH_AUTOTUNE_INVALIDATE", nullptr);

    Cache & c = Cache::instance();
    c.clear();

    // Hand-write a file with one valid record, one garbage line, and a wrong schema
    // block. The valid record must survive; garbage must be skipped, not fatal.
    FILE * f = std::fopen(path.c_str(), "wb");
    CACHE_CHECK(f != nullptr);
    std::fprintf(f, "# sandwich kernel cache\n");
    std::fprintf(f, "# schema %d\n", CACHE_SCHEMA);
    std::fprintf(f, "1 123456789 400 4 8 10.0 20.0\n");
    std::fprintf(f, "this is not a record\n");
    std::fprintf(f, "1 notanumber 400 4 8 10.0 20.0\n");
    std::fclose(f);

    c.reset_in_memory();
    entry hit;
    CACHE_CHECK(c.lookup(family::fattn, 123456789ull, hit));
    CACHE_CHECK(hit.variant == 400);
    CACHE_CHECK(c.size() == 1);

    // A file written under a different schema must be ignored wholesale.
    f = std::fopen(path.c_str(), "wb");
    CACHE_CHECK(f != nullptr);
    std::fprintf(f, "# schema %d\n", CACHE_SCHEMA + 1);
    std::fprintf(f, "1 999 400 4 8 10.0 20.0\n");
    std::fclose(f);

    c.reset_in_memory();
    entry stale;
    CACHE_CHECK(!c.lookup(family::fattn, 999ull, stale));
    CACHE_CHECK(c.size() == 0);

    std::remove(path.c_str());
    c.reset_in_memory();
    set_env("SANDWICH_KERNEL_CACHE", nullptr);
}

int main() {
    std::printf("=== sandwich kernel cache tests ===\n");
    test_bucket_pow2();
    test_fingerprint_stability_and_sensitivity();
    test_store_lookup_persist();
    test_disable_switch();
    test_malformed_and_schema_guard();
    std::printf("=== all kernel cache tests passed ===\n");
    return 0;
}
