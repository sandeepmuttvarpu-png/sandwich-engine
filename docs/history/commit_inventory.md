# Mechanical Commit Inventory: sandwich-engine

This document records every sandwich-authored commit from upstream fork point `3837bf0bc02ea5d08f8ee3585fc8c7784a81bce8` to `HEAD`.

- **Upstream Repository:** `https://github.com/ggerganov/llama.cpp.git`
- **Fork Base SHA:** `3837bf0bc02ea5d08f8ee3585fc8c7784a81bce8` (vulkan: Divide GROUP_NORM workgroup to 2D to avoid maxComputeWorkGroupCount limit, 2026-11-10)
- **Total Sandwich Commits:** 244

---

### 1. [`ee11cc718`] server: leave a busy slot untouched when a request pins it (#30295)
- **Full SHA:** `ee11cc7181e4febeebbe597836236c58a27e3704`
- **Date:** `2026-10-10T21:25:38+02:00`
- **Author:** `Pascal <admin@serveurperso.com>`
- **Diffstat:**
```text
 tools/server/server-context.cpp            |  5 +++++
 tools/server/tests/unit/test_completion.py | 23 +++++++++++++++++++++++
 2 files changed, 28 insertions(+)
```

### 2. [`c164dfbcf`] sandwich: step 1 null-safe buffer guard and step 2 auto-dispatch
- **Full SHA:** `c164dfbcf67b127dd2763f0229b44f83a9b7faab`
- **Date:** `2026-10-11T14:14:23+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp            | 10 +++++
 common/common.cpp         | 97 +++++++++++++++++++++++++++++++++++++++++++++++
 common/common.h           |  7 ++++
 ggml/src/ggml-backend.cpp |  3 ++
 4 files changed, 117 insertions(+)
```

### 3. [`7a7f55644`] Project Sandwich: Initial Release. Fixed GGML_ASSERT MoE crash, added Auto-Dispatcher, integrated TurboQuant.
- **Full SHA:** `7a7f556449453ee604cd3e12db3d96bf90f8b594`
- **Date:** `2026-10-11T16:17:34+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                                     |    3 +
 common/common.cpp                                  |   10 +-
 ggml/cmake/common.cmake                            |    4 +-
 ggml/include/ggml.h                                |   16 +-
 ggml/src/CMakeLists.txt                            |    1 +
 ggml/src/ggml-common.h                             |   65 +
 ggml/src/ggml-cpu/ggml-cpu.c                       |   95 +
 ggml/src/ggml-cpu/ops.cpp                          |   98 +
 ggml/src/ggml-cpu/ops.h                            |    1 +
 ggml/src/ggml-cuda/convert.cu                      |   24 +
 ggml/src/ggml-cuda/dequantize.cuh                  |   27 +
 ggml/src/ggml-cuda/fattn-common.cuh                |  159 +
 ggml/src/ggml-cuda/fattn-mma-f16.cuh               |   25 +
 ggml/src/ggml-cuda/fattn-tile.cu                   |    4 +
 ggml/src/ggml-cuda/fattn-tile.cuh                  |   47 +-
 ggml/src/ggml-cuda/fattn-vec.cuh                   |   36 +-
 ggml/src/ggml-cuda/fattn.cu                        |   33 +-
 ggml/src/ggml-cuda/ggml-cuda.cu                    |   18 +-
 ggml/src/ggml-cuda/set-rows.cu                     |  205 +
 .../fattn-mma-f16-instance-ncols1_1-ncols2_16.cu   |    1 +
 .../fattn-mma-f16-instance-ncols1_2-ncols2_16.cu   |    1 +
 .../fattn-mma-f16-instance-ncols1_4-ncols2_16.cu   |    1 +
 .../fattn-tile-instance-dkq640-dv512.cu            |    5 +
 .../fattn-vec-instance-f16-turbo2_0.cu             |    7 +
 .../fattn-vec-instance-f16-turbo3_0.cu             |    7 +
 .../fattn-vec-instance-f16-turbo4_0.cu             |    7 +
 .../fattn-vec-instance-q8_0-turbo2_0.cu            |   10 +
 .../fattn-vec-instance-q8_0-turbo3_0.cu            |   10 +
 .../fattn-vec-instance-q8_0-turbo4_0.cu            |   10 +
 .../fattn-vec-instance-turbo2_0-f16.cu             |    7 +
 .../fattn-vec-instance-turbo2_0-q8_0.cu            |    7 +
 .../fattn-vec-instance-turbo2_0-turbo2_0.cu        |    7 +
 .../fattn-vec-instance-turbo2_0-turbo3_0.cu        |    7 +
 .../fattn-vec-instance-turbo2_0-turbo4_0.cu        |    7 +
 .../fattn-vec-instance-turbo3_0-f16.cu             |    7 +
 .../fattn-vec-instance-turbo3_0-q8_0.cu            |    7 +
 .../fattn-vec-instance-turbo3_0-turbo2_0.cu        |    7 +
 .../fattn-vec-instance-turbo3_0-turbo3_0.cu        |    7 +
 .../fattn-vec-instance-turbo3_0-turbo4_0.cu        |    7 +
 .../fattn-vec-instance-turbo4_0-f16.cu             |    7 +
 .../fattn-vec-instance-turbo4_0-q8_0.cu            |    7 +
 .../fattn-vec-instance-turbo4_0-turbo2_0.cu        |    7 +
 .../fattn-vec-instance-turbo4_0-turbo3_0.cu        |    7 +
 .../fattn-vec-instance-turbo4_0-turbo4_0.cu        |    7 +
 ggml/src/ggml-cuda/turbo-innerq.cu                 |   32 +
 ggml/src/ggml-cuda/turbo-innerq.cuh                |   34 +
 ggml/src/ggml-cuda/turbo-quant.cuh                 |  466 +++
 ggml/src/ggml-cuda/turbo-wht.cu                    |  326 ++
 ggml/src/ggml-cuda/turbo-wht.cuh                   |    5 +
 ggml/src/ggml-quants.c                             |    5 +
 ggml/src/ggml-quants.h                             |   17 +
 ggml/src/ggml-turbo-quant.c                        | 1034 +++++
 ggml/src/ggml.c                                    |   65 +-
 src/llama-context.cpp                              |   23 +-
 src/llama-graph.cpp                                |  134 +
 src/llama-kv-cache.cpp                             |  208 +-
 src/llama-kv-cache.h                               |    9 +
 src/llama-memory.h                                 |    2 +
 src/turbo-rotation-data-32.h                       |   71 +
 src/turbo-rotation-data.h                          | 4103 ++++++++++++++++++++
 60 files changed, 7516 insertions(+), 53 deletions(-)
```

### 4. [`33f97cf91`] docs: comprehensive README with GLM bug post-mortem, benchmarks, and auto-dispatch guide
- **Full SHA:** `33f97cf912821e1bd60cdb35e0894dbd637bd0a3`
- **Date:** `2026-10-12T16:50:20+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md          | 281 +++++++++++++++++++++++++++++++----------------------
 docs/benchmarks.md | 120 +++++++++++++++++++++++
 2 files changed, 285 insertions(+), 116 deletions(-)
```

### 5. [`7366e9a7d`] docs: expand README with pros, known limitations/roadmap, usage guide, and deployment options
- **Full SHA:** `7366e9a7d754e99df7d06966b0f313d36e34c303`
- **Date:** `2026-10-12T16:52:30+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 237 ++++++++++++++++++++++++++++++++++++++++++++++++++++----------
 1 file changed, 201 insertions(+), 36 deletions(-)
```

### 6. [`967f4f34e`] feat(auto-dispatch): portable drafter discovery and dynamic 6.0GB vs 7.2GB VRAM capping
- **Full SHA:** `967f4f34e9b42fb99666f9e1b686e26f285d39de`
- **Date:** `2026-10-13T18:24:23+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp | 298 +++++++++++++++++++++++++++++++++++++++++++++---------
 1 file changed, 250 insertions(+), 48 deletions(-)
```

### 7. [`231bb51f6`] feat(dense): add physical P-Core pinning, MTP dimension guard, zero-VRAM N-gram fallback, and dynamic embedding VRAM budget
- **Full SHA:** `231bb51f6080da9fe686504d54b4a12b56eee9f5`
- **Date:** `2026-10-13T19:05:17+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp | 132 +++++++++++++++++++++++++++++++++++++++++++++++++++---
 1 file changed, 125 insertions(+), 7 deletions(-)
```

### 8. [`275dba8f2`] tools: add automated MoE and dense benchmark suite script
- **Full SHA:** `275dba8f27395396df895d86f486bd566cfefd28`
- **Date:** `2026-10-13T19:11:07+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 bench_suite.py | 199 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 199 insertions(+)
```

### 9. [`bc31af4b8`] feat(drafter): guard against mismatched tokenizer model types in candidate scanner
- **Full SHA:** `bc31af4b8179ed44fd098b20ae3aa4af0252d78c`
- **Date:** `2026-10-13T19:32:26+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp | 27 ++++++++++++++++++++++++---
 1 file changed, 24 insertions(+), 3 deletions(-)
```

### 10. [`0162026d9`] feat(scanner): enable cross-generation family prefix matching for qwen, llama, and gemma
- **Full SHA:** `0162026d9cf21920fb4cda40e597078fba39b492`
- **Date:** `2026-10-13T19:48:00+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp | 10 ++++++++--
 1 file changed, 8 insertions(+), 2 deletions(-)
```

### 11. [`3ee6ad629`] feat(speculative): auto-detect dflash and mtp spec types from drafter gguf metadata
- **Full SHA:** `3ee6ad629d129cb7d228b4135c9017f79885fb0f`
- **Date:** `2026-10-13T20:15:59+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp | 10 ++++++++--
 1 file changed, 8 insertions(+), 2 deletions(-)
```

### 12. [`b281833ff`] bench: add multi-model speculative decoding acceptance rate test harness
- **Full SHA:** `b281833ffeedbdb0e6ee3bf76f693e4af5428514`
- **Date:** `2026-10-13T20:24:41+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 test_acceptance.py | 168 +++++++++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 168 insertions(+)
```

### 13. [`e8a9e0b5d`] feat(speculative): support gemma4-assistant MTP heads and dynamic assistant VRAM budgeting
- **Full SHA:** `e8a9e0b5d6b767426ea077e289b517f991738ac3`
- **Date:** `2026-10-13T20:45:34+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp      | 28 +++++++++++++++++++++-------
 common/speculative.cpp |  5 +++++
 2 files changed, 26 insertions(+), 7 deletions(-)
```

### 14. [`35bd50f51`] bench: update Gemma-4-31B MTP assistant target in acceptance test harness
- **Full SHA:** `35bd50f51212b5d3af64da3222a6b65afbf803af`
- **Date:** `2026-10-13T20:46:12+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 test_acceptance.py | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)
```

### 15. [`7752c6c32`] feat(scanner): enable generic DFlash drafter pairing and tuned large-vocab VRAM budgeting
- **Full SHA:** `7752c6c32738df2f4afbfff2d6db4ac4b71b4600`
- **Date:** `2026-10-13T20:53:48+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp  | 12 ++++++------
 test_acceptance.py |  2 +-
 2 files changed, 7 insertions(+), 7 deletions(-)
```

### 16. [`56737254c`] feat(engine): clean-room integrate Phases 1-4 (LFM2 rollback, Modern MoE cache, FlashMLA, and SOTA IQK engine)
- **Full SHA:** `56737254c7786a3e604ca9e07d576421e17f3d78`
- **Date:** `2026-10-14T12:54:47+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                     |   4 +-
 common/common.cpp                  |  75 +++-
 common/common.h                    |   2 +-
 common/speculative.cpp             |  67 +++-
 ggml/include/ggml-moe-prefetch.h   |  40 ++
 ggml/include/ggml.h                |  28 ++
 ggml/src/CMakeLists.txt            |   5 +
 ggml/src/ggml-backend-meta.cpp     |   3 +-
 ggml/src/ggml-cpu/CMakeLists.txt   |   1 +
 ggml/src/ggml-cpu/ggml-cpu.c       |  14 +
 ggml/src/ggml-cpu/ops.cpp          | 259 +++++++++++++
 ggml/src/ggml-cpu/ops.h            |   1 +
 ggml/src/ggml-cuda/ggml-cuda.cu    |   6 +
 ggml/src/ggml-cuda/latent_attn.cu  | 733 +++++++++++++++++++++++++++++++++++++
 ggml/src/ggml-cuda/latent_attn.cuh |   7 +
 ggml/src/ggml-iqk/CMakeLists.txt   |  48 +++
 ggml/src/ggml-iqk/ggml-iqk.cpp     | 279 ++++++++++++++
 ggml/src/ggml-iqk/ggml-iqk.h       | 120 ++++++
 ggml/src/ggml-iqk/iqk-avx2.cpp     | 208 +++++++++++
 ggml/src/ggml-iqk/iqk-avx512.cpp   | 194 ++++++++++
 ggml/src/ggml-iqk/iqk-generic.cpp  | 292 +++++++++++++++
 ggml/src/ggml-iqk/iqk-internal.h   |  44 +++
 ggml/src/ggml-moe-prefetch.cpp     | 334 +++++++++++++++++
 ggml/src/ggml.c                    | 127 ++++++-
 src/CMakeLists.txt                 |   1 +
 src/llama-context.cpp              |  17 +
 src/llama-mmap.cpp                 |  20 +-
 src/llama-moe-cache-share.h        |  65 ++++
 src/llama-moe-cache.cpp            |  47 ++-
 tests/CMakeLists.txt               |   4 +
 tests/test-iqk.cpp                 | 270 ++++++++++++++
 tests/test-latent-attn.cpp         | 305 +++++++++++++++
 tests/test-moe-cache-shares.cpp    |  38 ++
 33 files changed, 3632 insertions(+), 26 deletions(-)
```

### 17. [`a30e187b7`] bench: expand bench_suite.py to cover all models in ai-models with ANSI-safe parsing and VRAM tracking
- **Full SHA:** `a30e187b7fd9eb1410538e1d8a096d7fb09056d7`
- **Date:** `2026-10-14T13:04:09+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 bench_suite.py | 214 ++++++++++++++++++++++++++++++++++++++++-----------------
 1 file changed, 150 insertions(+), 64 deletions(-)
```

### 18. [`31641455b`] bench: add Nemotron-3-Nano-4B and Dolphin-2.8-Mistral-7B N-gram spec targets to test_acceptance.py
- **Full SHA:** `31641455b7c3bf5d35d9fa59201a7dfae31a5641`
- **Date:** `2026-10-14T13:14:00+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 test_acceptance.py | 22 ++++++++++++++++------
 1 file changed, 16 insertions(+), 6 deletions(-)
```

### 19. [`8de8b5690`] docs: update README.md with Phase 1-4 engine upgrades, system-wide benchmarks, and speculative acceptance rates
- **Full SHA:** `8de8b5690f578145f1b080c34f31d5be0fd78d28`
- **Date:** `2026-10-14T13:16:53+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 75 +++++++++++++++++++++++++++++++++++++++++++++++++++++----------
 1 file changed, 63 insertions(+), 12 deletions(-)
```

### 20. [`c0dccac2e`] feat(ui): implement Phase 5 Web UI rebrand with Sandwich Palette and Engine Telemetry HUD
- **Full SHA:** `c0dccac2e62549bc711f3fca69fe887bfeed6bca`
- **Date:** `2026-10-14T13:30:49+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 examples/server/public/chat.js    | 390 ++++++++++++++++++++
 examples/server/public/index.html | 169 +++++++++
 examples/server/public/style.css  | 727 ++++++++++++++++++++++++++++++++++++++
 tools/server/public/chat.js       | 390 ++++++++++++++++++++
 tools/server/public/index.html    | 169 +++++++++
 tools/server/public/style.css     | 727 ++++++++++++++++++++++++++++++++++++++
 tools/server/server.cpp           |  10 +
 7 files changed, 2582 insertions(+)
```

### 21. [`176d6f8e9`] fix(server): resolve absolute path for tools/server/public when running from external working dirs
- **Full SHA:** `176d6f8e923692a6039587919f289eced26fdc45`
- **Date:** `2026-10-14T13:58:18+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tools/server/server.cpp | 2 ++
 1 file changed, 2 insertions(+)
```

### 22. [`b138d0107`] feat(ui): add multi-session dashboard and vanilla prompt composer with multimodal file attachments; brand CLI with Sandwich ASCII logo
- **Full SHA:** `b138d010756358b098c01976c29b822bdc91976f`
- **Date:** `2026-10-14T14:17:48+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 examples/server/public/chat.js    | 1099 ++++++++++++++++++++++++++++------
 examples/server/public/index.html |  367 ++++++++----
 examples/server/public/style.css  | 1169 +++++++++++++++++++++++++++++--------
 tools/cli/cli-context.cpp         |   24 +-
 tools/server/public/chat.js       | 1099 ++++++++++++++++++++++++++++------
 tools/server/public/index.html    |  367 ++++++++----
 tools/server/public/style.css     | 1169 +++++++++++++++++++++++++++++--------
 7 files changed, 4197 insertions(+), 1097 deletions(-)
```

### 23. [`555d8f406`] style(ui): enlarge prompt input box and composer surface for enhanced desktop usability
- **Full SHA:** `555d8f4069ac4c03b31b8eb2bf9ca5d6634c39fe`
- **Date:** `2026-10-14T14:22:36+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 examples/server/public/chat.js    |  2 +-
 examples/server/public/index.html |  2 +-
 examples/server/public/style.css  | 57 ++++++++++++++++++++-------------------
 tools/server/public/chat.js       |  2 +-
 tools/server/public/index.html    |  2 +-
 tools/server/public/style.css     | 57 ++++++++++++++++++++-------------------
 6 files changed, 62 insertions(+), 60 deletions(-)
```

### 24. [`3a57f26f9`] style(cli): remove sandwich illustration, keep streamlined sandwich.cpp block typography
- **Full SHA:** `3a57f26f9133e8d468f1c213bfbaa7c21c7c7dce`
- **Date:** `2026-10-14T14:24:51+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tools/cli/cli-context.cpp | 23 ++++++-----------------
 1 file changed, 6 insertions(+), 17 deletions(-)
```

### 25. [`8e8f064ac`] feat(ui): replace sandwich palette with vanilla llama neutral dark theme and add parameters, message actions, code copy, and context gauge
- **Full SHA:** `8e8f064ac5bd37dbbc5846471c22c329a497e516`
- **Date:** `2026-10-14T14:34:24+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 examples/server/public/chat.js    | 396 ++++++++++++++++++++--------
 examples/server/public/index.html | 229 ++++++++++------
 examples/server/public/style.css  | 535 ++++++++++++++++++++++++--------------
 tools/server/public/chat.js       | 396 ++++++++++++++++++++--------
 tools/server/public/index.html    | 229 ++++++++++------
 tools/server/public/style.css     | 535 ++++++++++++++++++++++++--------------
 6 files changed, 1582 insertions(+), 738 deletions(-)
```

### 26. [`f922afcc2`] feat(ui): set APP_NAME default to sandwich.cpp in tools/ui
- **Full SHA:** `f922afcc2302f15fe15a921fc1abeae8888752a8`
- **Date:** `2026-10-14T14:34:54+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tools/ui/src/lib/constants/app.constants.ts | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)
```

### 27. [`d873d75bb`] fix(moe): prevent segfault when prefetching GPU routing tensors in hybrid offload
- **Full SHA:** `d873d75bb78ddcdffba313d6aa0ffa17a1e99a80`
- **Date:** `2026-10-17T11:59:55+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 ggml/include/ggml-moe-prefetch.h |  3 +++
 ggml/src/ggml-moe-prefetch.cpp   | 16 ++++++++++++++++
 src/llama-context.cpp            | 13 ++++++++++---
 3 files changed, 29 insertions(+), 3 deletions(-)
```

### 28. [`6c32c49d3`] docs: add comprehensive GPT-OSS 120B MoE benchmarks across -ngl levels
- **Full SHA:** `6c32c49d326a1bceee4ef58278a7bc885a8854e6`
- **Date:** `2026-10-17T12:19:47+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/benchmarks.md        |  19 ++++
 gpt_oss_120b_results.json |  67 ++++++++++++
 run_gpt_oss_benchmarks.py | 263 ++++++++++++++++++++++++++++++++++++++++++++++
 3 files changed, 349 insertions(+)
```

### 29. [`b8d8bdd16`] feat(phase-5): implement pinned host memory DMA and early-lookahead router
- **Full SHA:** `b8d8bdd164592370013979e12425e4efae4c5024`
- **Date:** `2026-10-17T12:58:33+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp                |  10 +++-
 ggml/include/ggml-moe-prefetch.h |  17 ++++++
 ggml/src/ggml-moe-prefetch.cpp   | 110 +++++++++++++++++++++++++++++++++++++++
 src/llama-context.cpp            |   8 +++
 src/llama-graph.cpp              |  14 +++++
 src/llama-moe-cache.cpp          |  88 ++++++++++++++++++++++++++++++-
 src/llama-moe-cache.h            |   4 ++
 7 files changed, 247 insertions(+), 4 deletions(-)
```

### 30. [`124ce1ee1`] feat(phase-6): implement asymmetric mixed-precision and fine-grained micro-expert cache
- **Full SHA:** `124ce1ee16c8b28dd60c86d3f9b25a0f780834cb`
- **Date:** `2026-10-17T13:15:50+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp               | 18 ++++++++++++-
 common/common.h                 |  2 ++
 ggml/src/ggml-iqk/ggml-iqk.h    | 13 ++++++++++
 include/llama.h                 |  9 +++++++
 src/llama-model-loader.cpp      | 37 ++++++++++++++++++++++++---
 src/llama-model-loader.h        |  5 +++-
 src/llama-model.cpp             |  1 +
 src/llama-moe-cache-share.h     | 39 ++++++++++++++++++++++++++++
 src/llama-moe-cache.cpp         | 56 +++++++++++++++++++++++++++++++++++------
 src/llama-moe-cache.h           |  4 +++
 src/llama.cpp                   |  2 +-
 tests/test-moe-cache-shares.cpp | 28 ++++++++++++++++++++-
 12 files changed, 200 insertions(+), 14 deletions(-)
```

### 31. [`4e6e334ec`] docs(benchmarks): update MoE benchmarks across all models in ai-models for Phase 5 & 6
- **Full SHA:** `4e6e334ec94316015885d32f37192ad517914d9c`
- **Date:** `2026-10-17T13:25:04+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/benchmarks.md | 58 ++++++++++++++++++++++++++++++++----------------------
 1 file changed, 35 insertions(+), 23 deletions(-)
```

### 32. [`9dc1c647e`] llama : pin mmap-backed CPU weights for faster H2D uploads
- **Full SHA:** `9dc1c647eca7cae976d28bdbd18c9a526dd5fcf1`
- **Date:** `2026-07-02T09:44:53Z`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/llama-mmap.cpp         | 36 +++++++++++++++++++++++++++++++++++-
 src/llama-mmap.h           |  8 ++++++++
 src/llama-model-loader.cpp | 16 ++++++++++++++++
 3 files changed, 59 insertions(+), 1 deletion(-)
```

### 33. [`3adb05e6b`] feat(moe): add llama-moe-trace offline profiling tool from thecodacus
- **Full SHA:** `3adb05e6b816553e3a8d9a0c48678ff2bf4d1eba`
- **Date:** `2026-10-17T18:04:06+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tools/CMakeLists.txt           |   1 +
 tools/moe-trace/CMakeLists.txt |   4 +
 tools/moe-trace/moe-trace.cpp  | 151 +++++++++++++++++++++++++++++++
 tools/moe-trace/simulate.py    | 197 +++++++++++++++++++++++++++++++++++++++++
 4 files changed, 353 insertions(+)
```

### 34. [`873693dad`] feat(sched): declare ggml_backend_sched_set_async_cpu in ggml-backend.h
- **Full SHA:** `873693dad3241f5215416f670e6b30eff4befd64`
- **Date:** `2026-10-17T18:04:30+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 ggml/include/ggml-backend.h | 4 ++++
 1 file changed, 4 insertions(+)
```

### 35. [`3a860c36b`] feat: MoE expert cache — keep hot routed experts resident in VRAM (#5)
- **Full SHA:** `3a860c36bf4d93494655c873d7860d392317acea`
- **Date:** `2026-07-25T00:41:02+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                  |  17 +++
 common/common.cpp               |   5 +
 common/common.h                 |   3 +
 ggml/src/ggml-cpu/ggml-cpu.c    |   9 +-
 ggml/src/ggml-cuda/ggml-cuda.cu |  28 +++--
 ggml/src/ggml-cuda/mmf.cu       |   5 +
 ggml/src/ggml-cuda/mmid.cu      |  33 +++++-
 ggml/src/ggml-cuda/mmid.cuh     |   4 +
 ggml/src/ggml-cuda/mmq.cu       |  11 ++
 ggml/src/ggml-cuda/mmvf.cu      |  13 +++
 ggml/src/ggml-cuda/mmvq.cu      |  18 +++-
 ggml/src/ggml-cuda/quantize.cu  |  21 ++++
 include/llama.h                 |   4 +
 src/llama-graph.cpp             | 227 ++++++++++++++++++++++++----------------
 src/llama-graph.h               |   6 +-
 src/llama-model.cpp             | 158 ++++++++++++++++++++++++++++
 src/llama-model.h               |  13 +++
 src/models/deepseek2.cpp        |   4 +-
 src/models/qwen35moe.cpp        |   6 +-
 tests/test-backend-ops.cpp      |  43 ++++++--
 tools/CMakeLists.txt            |   1 +
 21 files changed, 516 insertions(+), 113 deletions(-)
```

### 36. [`0b852a83b`] feat: wire MoE expert cache into laguna graph (#6)
- **Full SHA:** `0b852a83b6093b86d80325f94ba220fd3b65740c`
- **Date:** `2026-07-25T03:54:47+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/models/laguna.cpp | 5 ++++-
 1 file changed, 4 insertions(+), 1 deletion(-)
```

### 37. [`6ea7b8b36`] feat: overlap CPU cold-expert and GPU hot-expert computation in the MoE cache (#9)
- **Full SHA:** `6ea7b8b36e5f3bdb09e33514402446b73950cf8d`
- **Date:** `2026-08-23T04:08:21+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                    |   8 ++
 common/common.cpp                 |   1 +
 common/common.h                   |   1 +
 ggml/src/ggml-backend.cpp         | 162 ++++++++++++++++++++++++++++++++++++++
 include/llama.h                   |   1 +
 src/llama-context.cpp             |   4 +
 src/llama-cparams.h               |   1 +
 src/llama-graph.cpp               |  19 ++++-
 src/llama-model.cpp               |   4 +
 tools/llama-bench/llama-bench.cpp |  35 +++++++-
 10 files changed, 230 insertions(+), 6 deletions(-)
```

### 38. [`bd5298950`] fix: mul_mat_id sync predicate must account for the expert-pack split
- **Full SHA:** `bd529895049a820f503e4a012555970799853574`
- **Date:** `2026-09-03T14:15:06Z`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 ggml/src/ggml-cuda/ggml-cuda.cu | 9 +++++++--
 1 file changed, 7 insertions(+), 2 deletions(-)
```

### 39. [`bf51705e5`] feat: wire the MoE expert cache into the qwen4exp graph
- **Full SHA:** `bf51705e5be875c251adbe05874e925fd9b21d31`
- **Date:** `2026-09-03T13:53:19Z`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/models/qwen4exp.cpp | 3 ++-
 1 file changed, 2 insertions(+), 1 deletion(-)
```

### 40. [`58eb58975`] docs: add MoE expert cache usage and tuning guide to README (#8)
- **Full SHA:** `58eb58975288a514b2b826f603b52520d0c2ee9d`
- **Date:** `2026-08-25T15:58:16+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 101 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 101 insertions(+)
```

### 41. [`daa478b50`] fix(cmake): remove duplicate add_subdirectory(moe-trace)
- **Full SHA:** `daa478b50290c171a016ffc1999923f917846d03`
- **Date:** `2026-10-17T18:22:02+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tools/CMakeLists.txt | 1 -
 1 file changed, 1 deletion(-)
```

### 42. [`f4ba9d077`] fix(server): honor user n_parallel in dense auto-dispatch & add serve.sh
- **Full SHA:** `f4ba9d077e4e6e792f1c9518b7921c5d81a2be2e`
- **Date:** `2026-10-17T19:10:40+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp |  2 +-
 serve.sh          | 19 +++++++++++++++++++
 2 files changed, 20 insertions(+), 1 deletion(-)
```

### 43. [`20cb91553`] chore(git): ignore benchmark_results.json
- **Full SHA:** `20cb91553df387642261cdeca5c153c1446c3ec4`
- **Date:** `2026-10-18T12:46:02+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 .gitignore | 1 +
 1 file changed, 1 insertion(+)
```

### 44. [`c55bffa5f`] feat(moe): add auto-sizing expert cache, lookahead prefetching, persistent prompt prefix caching, and mutual exclusivity
- **Full SHA:** `c55bffa5fc5a8380db505b1658504df5e64ff099`
- **Date:** `2026-10-18T13:22:36+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp   | 76 ++++++++++++++++++++++++++++++++++++++++++-----------
 serve.sh            |  3 ++-
 src/llama-model.cpp | 47 +++++++++++++++++++--------------
 3 files changed, 91 insertions(+), 35 deletions(-)
```

### 45. [`e49a335fb`] feat(telemetry,moe): add live cache telemetry HUD and online warm-up auto-tuning repack
- **Full SHA:** `e49a335fbee66eb2d71ed9c25785c394e30ba7b5`
- **Date:** `2026-10-19T10:58:49+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 ggml/include/sandwich-telemetry.h | 32 +++++++++++++++
 ggml/src/CMakeLists.txt           |  2 +
 ggml/src/ggml-cpu/ggml-cpu.c      | 22 ++++++++++
 ggml/src/sandwich-telemetry.cpp   | 84 +++++++++++++++++++++++++++++++++++++++
 src/llama-context.cpp             | 37 +++++++++++++++++
 src/llama-context.h               |  1 +
 src/llama-graph.cpp               |  2 +
 src/llama-model.cpp               | 56 +++++++++++++++++++++-----
 src/llama-model.h                 | 11 +++++
 tools/cli/cli-context.cpp         |  2 +
 tools/server/server.cpp           |  2 +
 11 files changed, 241 insertions(+), 10 deletions(-)
```

### 46. [`70462d81d`] feat(moe,speculative): add hot-only speculative fast drafting (--moe-hot-only) for pure VRAM speed
- **Full SHA:** `70462d81dfced3301cca2372b90bf0ee3c576e7f`
- **Date:** `2026-10-19T11:06:45+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp         |  7 +++++++
 common/common.cpp      |  1 +
 common/common.h        |  1 +
 common/speculative.cpp |  6 ++++--
 include/llama.h        |  1 +
 src/llama-context.cpp  |  2 ++
 src/llama-cparams.h    |  1 +
 src/llama-graph.cpp    | 30 +++++++++++++++++++-----------
 8 files changed, 36 insertions(+), 13 deletions(-)
```

### 47. [`86db8dab2`] feat(kv-cache,server): add streaming attention sink preservation for infinite context sliding-window compaction
- **Full SHA:** `86db8dab207253593a6b1c8970c62919d93f47ab`
- **Date:** `2026-10-19T11:13:31+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                  |  7 +++++++
 common/common.h                 |  1 +
 tools/completion/completion.cpp | 12 +++++++-----
 tools/server/server-context.cpp |  8 +++++++-
 tools/server/server-schema.cpp  |  5 +++++
 tools/server/server-task.cpp    |  2 ++
 tools/server/server-task.h      |  5 +++--
 7 files changed, 32 insertions(+), 8 deletions(-)
```

### 48. [`ae25fb43c`] bench: add comprehensive system-wide benchmark suite across all models
- **Full SHA:** `ae25fb43ca4f3caded2e748285fe0c52f229d1ab`
- **Date:** `2026-10-19T11:42:55+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 benchmark_results_all.json | 210 ++++++++++++++++++++++++++
 run_all_benchmarks.py      | 366 +++++++++++++++++++++++++++++++++++++++++++++
 2 files changed, 576 insertions(+)
```

### 49. [`3dd06490f`] feat(auto-dispatch): dynamic drafter footprint VRAM budgeting & calibrated dense layer offloading
- **Full SHA:** `3dd06490f629a88f9faf3949c523e9d48097eac8`
- **Date:** `2026-10-19T12:05:07+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 benchmark_results_all.json | 112 ++++++++++++++++++++++-----------------------
 common/common.cpp          |  15 +++++-
 run_all_benchmarks.py      |   6 +--
 3 files changed, 72 insertions(+), 61 deletions(-)
```

### 50. [`c17960efd`] feat(phase-7): implement EAGLE3 / P-EAGLE Speculative Tree Decoding (#24)
- **Full SHA:** `c17960efd34c6e2d64e27cd1396c1354ffc68d49`
- **Date:** `2026-10-19T13:30:45+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/CMakeLists.txt                              |   2 +
 common/arg.cpp                                     |  36 ++
 common/common.cpp                                  |   8 +
 common/common.h                                    |   6 +
 common/spec-tree.cpp                               | 552 +++++++++++++++++++++
 common/spec-tree.h                                 | 222 +++++++++
 examples/speculative-simple/speculative-simple.cpp | 159 +++++-
 include/llama.h                                    |   6 +
 src/llama-context.cpp                              | 135 +++++
 src/llama-context.h                                |  17 +
 src/llama-moe-cache.cpp                            |   2 +-
 11 files changed, 1143 insertions(+), 2 deletions(-)
```

### 51. [`e1d63ff5c`] style(webui): remove floating feature plates for minimal welcome screen
- **Full SHA:** `e1d63ff5cae855a87aa6a64cf6c8b9126c28d95c`
- **Date:** `2026-10-19T15:26:16+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 examples/server/public/chat.js    | 17 -----------------
 examples/server/public/index.html | 18 ------------------
 examples/server/public/style.css  |  2 +-
 tools/server/public/chat.js       | 17 -----------------
 tools/server/public/index.html    | 18 ------------------
 tools/server/public/style.css     |  2 +-
 6 files changed, 2 insertions(+), 72 deletions(-)
```

### 52. [`397e86e38`] docs: redesign README with Gen-Z flair and full KV physics benchmarks
- **Full SHA:** `397e86e3854f1c778a21d84285556991764fb591`
- **Date:** `2026-10-20T14:58:11+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 546 +++++++++++++-------------------------------------------------
 1 file changed, 110 insertions(+), 436 deletions(-)
```

### 53. [`f42ee693f`] docs: integrate complete Gen-Z CLI flag spellbook from flags.md
- **Full SHA:** `f42ee693fb37cc71cf5df03d0eb206a4dc32b091`
- **Date:** `2026-10-20T15:11:43+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 118 ++++++++++++++++++++++++++++++++++++++++++++++++++++++--------
 1 file changed, 104 insertions(+), 14 deletions(-)
```

### 54. [`446dcdc01`] docs: remove Sandwich Deck references from README
- **Full SHA:** `446dcdc01c5bdf8c80fd206f9086ce5ba7197dba`
- **Date:** `2026-10-20T15:43:05+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 2 --
 1 file changed, 2 deletions(-)
```

### 55. [`c69fe991c`] feat: Phase 8 Dense Wing Overhaul (#25-#28) & Phase 9 Topology Solver & TurboQuant Auto-Router (#29)
- **Full SHA:** `c69fe991c30f6ab82f44063af3ba268166a8617c`
- **Date:** `2026-10-22T12:20:06+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                                          |  23 ++
 common/arg.cpp                                     |  57 +++
 common/common.cpp                                  | 334 ++++++++++++++---
 common/common.h                                    |  11 +
 examples/speculative-simple/speculative-simple.cpp |  20 ++
 examples/speculative/speculative.cpp               |   3 +
 generate_ultimate_report.py                        | 338 ++++++++++++++++++
 ggml/src/ggml.c                                    |  19 +-
 include/llama.h                                    |   6 +
 prompts/prompt_100.txt                             |   1 +
 prompts/prompt_1k.txt                              |  38 ++
 prompts/prompt_5k.txt                              | 139 ++++++++
 run_dense_benchmarks.py                            | 256 +++++++++++++
 run_turboquant_benchmarks.py                       | 394 +++++++++++++++++++++
 src/CMakeLists.txt                                 |   5 +
 src/llama-context.cpp                              |   6 +-
 src/llama-model-loader.cpp                         | 169 ++++++++-
 src/llama-model-loader.h                           |  17 +-
 src/llama-model.cpp                                |   5 +-
 src/llama.cpp                                      |   3 +-
 tools/server/server-context.cpp                    |  22 +-
 21 files changed, 1808 insertions(+), 58 deletions(-)
```

### 56. [`6af71181d`] docs: integrate Phase 8 & 9 into README with Gen-Z flair and updated receipts
- **Full SHA:** `6af71181dd124c5e9200ed35c7fd54a69b1b0b26`
- **Date:** `2026-10-22T12:29:12+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 71 ++++++++++++++++++++++++++++++++++++++++++---------------------
 1 file changed, 47 insertions(+), 24 deletions(-)
```

### 57. [`0ee6f7978`] feat: implement Phase 10-12 hardware push, native quant FA, GPU top-k, and fused turbo MMA
- **Full SHA:** `0ee6f79789051781bbb3da559de197809ce0b517`
- **Date:** `2026-10-24T20:48:53+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                                          |  44 ++
 common/arg.cpp                                     | 140 ++++
 common/common.cpp                                  |  74 +-
 common/common.h                                    |  13 +
 common/spec-tree.h                                 |  79 +++
 examples/speculative-simple/speculative-simple.cpp |  65 +-
 ggml/include/ggml.h                                |   6 +
 ggml/src/ggml-cpu/arch/x86/quants.c                | 169 +++++
 ggml/src/ggml-cuda/cp-async.cuh                    |   2 +
 ggml/src/ggml-cuda/fattn-common.cuh                |  95 ++-
 ggml/src/ggml-cuda/fattn-mma-f16.cuh               | 741 ++++++++++++++++++---
 ggml/src/ggml-cuda/fattn-mma-quant-decl.cuh        |  53 ++
 ggml/src/ggml-cuda/fattn-mma-quant-packed.cuh      | 100 +++
 ggml/src/ggml-cuda/fattn-mma-quant-q4_0.cuh        |  78 +++
 ggml/src/ggml-cuda/fattn-mma-quant-q4_1.cuh        |   6 +
 ggml/src/ggml-cuda/fattn-mma-quant-q5_0.cuh        |  96 +++
 ggml/src/ggml-cuda/fattn-mma-quant-q5_1.cuh        |  42 ++
 ggml/src/ggml-cuda/fattn-mma-quant-q8_0.cuh        |  85 +++
 ggml/src/ggml-cuda/fattn-mma-quant-types.h         |  46 ++
 ggml/src/ggml-cuda/fattn-mma-quant.cuh             | 405 +++++++++++
 ggml/src/ggml-cuda/fattn-mma-turbo.cuh             | 118 ++++
 ggml/src/ggml-cuda/fattn.cu                        | 714 ++++++++++++++++----
 ggml/src/ggml-cuda/mma.cuh                         |  52 +-
 ...tn-mma-quant-instance-q4_0-ncols1_1-ncols2_8.cu |   9 +
 ...n-mma-quant-instance-q4_0-ncols1_16-ncols2_2.cu |   8 +
 ...n-mma-quant-instance-q4_0-ncols1_16-ncols2_8.cu |   7 +
 ...tn-mma-quant-instance-q4_0-ncols1_2-ncols2_8.cu |   9 +
 ...n-mma-quant-instance-q4_0-ncols1_32-ncols2_2.cu |   8 +
 ...tn-mma-quant-instance-q4_0-ncols1_4-ncols2_2.cu |   8 +
 ...tn-mma-quant-instance-q4_0-ncols1_4-ncols2_8.cu |   9 +
 ...n-mma-quant-instance-q4_0-ncols1_64-ncols2_2.cu |   7 +
 ...tn-mma-quant-instance-q4_0-ncols1_8-ncols2_8.cu |   9 +
 ...tn-mma-quant-instance-q4_1-ncols1_1-ncols2_8.cu |  11 +
 ...n-mma-quant-instance-q4_1-ncols1_16-ncols2_2.cu |  10 +
 ...n-mma-quant-instance-q4_1-ncols1_16-ncols2_8.cu |   9 +
 ...tn-mma-quant-instance-q4_1-ncols1_2-ncols2_8.cu |  11 +
 ...n-mma-quant-instance-q4_1-ncols1_32-ncols2_2.cu |  10 +
 ...tn-mma-quant-instance-q4_1-ncols1_4-ncols2_2.cu |  10 +
 ...tn-mma-quant-instance-q4_1-ncols1_4-ncols2_8.cu |  11 +
 ...n-mma-quant-instance-q4_1-ncols1_64-ncols2_2.cu |   9 +
 ...tn-mma-quant-instance-q4_1-ncols1_8-ncols2_8.cu |  11 +
 ...tn-mma-quant-instance-q5_0-ncols1_1-ncols2_8.cu |  11 +
 ...n-mma-quant-instance-q5_0-ncols1_16-ncols2_2.cu |  10 +
 ...n-mma-quant-instance-q5_0-ncols1_16-ncols2_8.cu |   9 +
 ...tn-mma-quant-instance-q5_0-ncols1_2-ncols2_8.cu |  11 +
 ...n-mma-quant-instance-q5_0-ncols1_32-ncols2_2.cu |  10 +
 ...tn-mma-quant-instance-q5_0-ncols1_4-ncols2_2.cu |  10 +
 ...tn-mma-quant-instance-q5_0-ncols1_4-ncols2_8.cu |  11 +
 ...n-mma-quant-instance-q5_0-ncols1_64-ncols2_2.cu |   9 +
 ...tn-mma-quant-instance-q5_0-ncols1_8-ncols2_8.cu |  11 +
 ...tn-mma-quant-instance-q5_1-ncols1_1-ncols2_8.cu |  11 +
 ...n-mma-quant-instance-q5_1-ncols1_16-ncols2_2.cu |  10 +
 ...n-mma-quant-instance-q5_1-ncols1_16-ncols2_8.cu |   9 +
 ...tn-mma-quant-instance-q5_1-ncols1_2-ncols2_8.cu |  11 +
 ...n-mma-quant-instance-q5_1-ncols1_32-ncols2_2.cu |  10 +
 ...tn-mma-quant-instance-q5_1-ncols1_4-ncols2_2.cu |  10 +
 ...tn-mma-quant-instance-q5_1-ncols1_4-ncols2_8.cu |  11 +
 ...n-mma-quant-instance-q5_1-ncols1_64-ncols2_2.cu |   9 +
 ...tn-mma-quant-instance-q5_1-ncols1_8-ncols2_8.cu |  11 +
 ...tn-mma-quant-instance-q8_0-ncols1_1-ncols2_8.cu |   9 +
 ...n-mma-quant-instance-q8_0-ncols1_16-ncols2_2.cu |   8 +
 ...n-mma-quant-instance-q8_0-ncols1_16-ncols2_8.cu |   7 +
 ...tn-mma-quant-instance-q8_0-ncols1_2-ncols2_8.cu |   9 +
 ...n-mma-quant-instance-q8_0-ncols1_32-ncols2_2.cu |   8 +
 ...tn-mma-quant-instance-q8_0-ncols1_4-ncols2_2.cu |   8 +
 ...tn-mma-quant-instance-q8_0-ncols1_4-ncols2_8.cu |   9 +
 ...n-mma-quant-instance-q8_0-ncols1_64-ncols2_2.cu |   7 +
 ...tn-mma-quant-instance-q8_0-ncols1_8-ncols2_8.cu |   9 +
 ...-mma-turbo-instance-dkq128-ncols1_1-ncols2_8.cu |   9 +
 ...-mma-turbo-instance-dkq128-ncols1_2-ncols2_4.cu |   9 +
 ...-mma-turbo-instance-dkq128-ncols1_2-ncols2_8.cu |   9 +
 ...-mma-turbo-instance-dkq128-ncols1_4-ncols2_2.cu |   9 +
 ...-mma-turbo-instance-dkq128-ncols1_4-ncols2_4.cu |   9 +
 ...-mma-turbo-instance-dkq128-ncols1_4-ncols2_8.cu |   9 +
 ...-mma-turbo-instance-dkq128-ncols1_8-ncols2_1.cu |   9 +
 ...-mma-turbo-instance-dkq256-ncols1_1-ncols2_8.cu |   9 +
 ...-mma-turbo-instance-dkq256-ncols1_2-ncols2_4.cu |   9 +
 ...-mma-turbo-instance-dkq256-ncols1_2-ncols2_8.cu |   9 +
 ...-mma-turbo-instance-dkq256-ncols1_4-ncols2_2.cu |   9 +
 ...-mma-turbo-instance-dkq256-ncols1_4-ncols2_4.cu |   9 +
 ...-mma-turbo-instance-dkq256-ncols1_4-ncols2_8.cu |   9 +
 ...-mma-turbo-instance-dkq256-ncols1_8-ncols2_1.cu |   9 +
 ggml/src/ggml.c                                    |   8 +
 include/llama.h                                    |   6 +
 src/llama-context.cpp                              | 218 +++++-
 src/llama-context.h                                |  31 +
 src/llama-cparams.h                                |   2 +
 src/llama-graph.cpp                                |   1 +
 src/llama-kv-cache.cpp                             |  23 +-
 src/llama-kv-cache.h                               |   3 +-
 src/llama-model.cpp                                |   6 +-
 91 files changed, 3835 insertions(+), 236 deletions(-)
```

### 58. [`3c298eb35`] feat(moe): Phase 12 - 3-Tier Hierarchical MoE Memory System Engine (Technique #40)
- **Full SHA:** `3c298eb35a524355edeb0b75110e48ec1283e6a4`
- **Date:** `2026-10-24T21:38:47+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                       |  10 +
 common/arg.cpp                  |  24 +++
 common/common.cpp               |   8 +-
 common/common.h                 |   2 +
 include/llama.h                 |   2 +
 src/CMakeLists.txt              |  11 +
 src/llama-context.cpp           |  12 +-
 src/llama-cparams.h             |   2 +
 src/llama-mmap.cpp              |  14 +-
 src/llama-mmap.h                |   1 +
 src/llama-model.cpp             |  49 +++++
 src/llama-model.h               |   5 +
 src/llama-moe-cache.cpp         | 468 ++++++++++++++++++++++++++++++++++++++--
 src/llama-moe-cache.h           |  31 ++-
 tests/test-moe-cache-shares.cpp |  32 ++-
 15 files changed, 642 insertions(+), 29 deletions(-)
```

### 59. [`b79cb85dc`] fix(moe-3tier): validate warm slot ram pointer before cache staging and add stderr telemetry banner
- **Full SHA:** `b79cb85dca3ca4f992b02b68e9a9a679525d23c6`
- **Date:** `2026-10-24T22:20:39+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/llama-moe-cache.cpp | 27 +++++++++++++++++++++------
 1 file changed, 21 insertions(+), 6 deletions(-)
```

### 60. [`f842d9937`] docs: add Phase 12 3-Tier MoE Engine architecture, GPT-OSS-120B live telemetry, and tiered memory diagram to README
- **Full SHA:** `f842d9937db32edb03181c01c840e1fe13c1ce85`
- **Date:** `2026-10-24T22:28:41+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 63 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++----
 1 file changed, 59 insertions(+), 4 deletions(-)
```

### 61. [`51360748d`] feat(moe): Phase 12 — 2Q & Ghost-Cache Replacement Policy for 3-Tier MoE Engine
- **Full SHA:** `51360748d39e92485c9f5df4583777ea67239473`
- **Date:** `2026-10-25T12:53:28+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                  |  14 ++
 common/common.cpp               |   1 +
 common/common.h                 |   1 +
 include/llama.h                 |   1 +
 src/llama-context.cpp           |   7 +-
 src/llama-cparams.h             |   1 +
 src/llama-moe-cache-share.h     | 375 ++++++++++++++++++++++++++++++++++++++++
 src/llama-moe-cache.cpp         | 194 ++++++++-------------
 src/llama-moe-cache.h           |   9 +-
 tests/test-moe-cache-shares.cpp |  92 +++++++++-
 10 files changed, 567 insertions(+), 128 deletions(-)
```

### 62. [`86c44d476`] feat: implement Technique #42 (70% RAM Rule & Asymmetric Split-KV for Dense Models)
- **Full SHA:** `86c44d476de78a5d6b04637a2f2b724f0172b2a9`
- **Date:** `2026-10-25T18:33:27+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp | 75 +++++++++++++++++++++++++++++++++++++++----------------
 1 file changed, 53 insertions(+), 22 deletions(-)
```

### 63. [`f7474ab6d`] bench: complete 128k sustained stress test suite across all 11 heavyweight models (>10B-117B) with 1,140 verified data points
- **Full SHA:** `f7474ab6dc60b962d649d6c5150ca00afd508dbf`
- **Date:** `2026-10-26T13:26:22+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 generate_curated_report.py               |  365 +++
 prompts/prompt_125k.txt                  | 3666 ++++++++++++++++++++++++++++++
 prompts/prompt_13k.txt                   |  423 ++++
 prompts/prompt_65k.txt                   | 1833 +++++++++++++++
 prompts/prompt_98k.txt                   | 2820 +++++++++++++++++++++++
 sustained_stress_test_of_sandwich_cpp.py |  619 +++++
 6 files changed, 9726 insertions(+)
```

### 64. [`4f50e4c59`] docs: overhaul README with 3-tier MoE, 2Q ghost cache, 70% RAM rule, and 128k endurance matrix
- **Full SHA:** `4f50e4c59e9bfacd256e5df7d0484a039bb3687b`
- **Date:** `2026-10-26T13:35:14+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 468 ++++++++++++++++++++++++--------------------------------------
 1 file changed, 184 insertions(+), 284 deletions(-)
```

### 65. [`580cb9cb0`] feat: implement Technique #43 (Unified Zero-Copy Memory & UVM prefetching), Technique #44 (NIAH Retrieval Engine), and install.sh unified CLI
- **Full SHA:** `580cb9cb0ea57c4cd26318a4cbee449dcd71cde9`
- **Date:** `2026-10-26T13:47:45+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 ggml/src/ggml-cuda/ggml-cuda.cu |  38 ++++++++++--
 install.sh                      |  96 ++++++++++++++++++++++++++++++
 tests/test_niah_retrieval.py    | 129 ++++++++++++++++++++++++++++++++++++++++
 3 files changed, 258 insertions(+), 5 deletions(-)
```

### 66. [`13678dbf1`] feat: implement Technique #45 (Radix-Tree Persistent Prefix Caching / RadixAttention for Agentic Workflows)
- **Full SHA:** `13678dbf1825e8a9c07812c58c239cec6c3fcbd5`
- **Date:** `2026-10-26T13:56:02+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/radix-cache.h       | 186 +++++++++++++++++++++++++++++++++++++++++++++
 tests/test-radix-cache.cpp |  36 +++++++++
 2 files changed, 222 insertions(+)
```

### 67. [`e3b6281fd`] feat(frontier): add CLI flags and runtime dispatch for Techniques #43-#52 (UVM, RadixCache, MoEMassThresh, GDS, NativeMTP, AsymKV, ChunkedPrefill, LatentMLA)
- **Full SHA:** `e3b6281fd6ba3e10f926aacd242acd4771913232`
- **Date:** `2026-10-26T14:13:44+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp        | 127 ++++++++++++++++++++++++++++++++++++++++++++++++++
 common/common.cpp     |  45 ++++++++++++++++++
 common/common.h       |  11 +++++
 include/llama.h       |   3 ++
 src/llama-context.cpp |   9 ++++
 src/llama-cparams.h   |   3 ++
 6 files changed, 198 insertions(+)
```

### 68. [`65738d4e0`] feat(packaging): production turnkey install.sh, integrations.md, standalone engine branding, and zero-allocation MoE buffer optimization
- **Full SHA:** `65738d4e096966434fdd6ada9efdfcc997fe8a1f`
- **Date:** `2026-10-26T14:49:09+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CMakeLists.txt              |  17 +-
 README.md                   |  43 ++++-
 common/common.cpp           |   4 +-
 docs/integrations.md        | 164 ++++++++++++++++++
 install.sh                  | 404 +++++++++++++++++++++++++++++++++++++++-----
 src/llama-moe-cache-share.h |  12 +-
 src/llama-moe-cache.cpp     |  26 +--
 7 files changed, 597 insertions(+), 73 deletions(-)
```

### 69. [`90dd15308`] docs: integrate turnkey install.sh, unified sandwich CLI dispatcher, and agent integrations into README
- **Full SHA:** `90dd15308baea9cd528bebfa7c5e220b98990a5b`
- **Date:** `2026-10-26T15:17:17+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 61 ++++++++++++++++++++++++++++++++++++++++---------------------
 1 file changed, 40 insertions(+), 21 deletions(-)
```

### 70. [`632a1095c`] ci: add automated standalone binary release workflow and unified CLI dispatcher
- **Full SHA:** `632a1095c02fa5fd79560dce3914943aa9660db6`
- **Date:** `2026-10-26T15:38:48+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 .github/workflows/release-binaries.yml | 224 +++++++++++++++++++++++++++++++++
 scripts/sandwich                       |  58 +++++++++
 2 files changed, 282 insertions(+)
```

### 71. [`5a2a2ffd5`] feat(speculative): port dynamic multi-drafter arbitration (--spec-select) from Piggidragon
- **Full SHA:** `5a2a2ffd5700025a3af01f68947eab066e910ebf`
- **Date:** `2026-10-26T16:48:32+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                                          |   1 +
 common/arg.cpp                                     |  11 +
 common/common.cpp                                  |   5 +
 common/common.h                                    |   4 +
 common/speculative.cpp                             | 562 ++++++++++++++++++++-
 common/speculative.h                               |   9 +
 docs/speculative.md                                |  15 +
 examples/speculative-simple/speculative-simple.cpp |   8 +
 tools/server/server-context.cpp                    |   5 +
 9 files changed, 618 insertions(+), 2 deletions(-)
```

### 72. [`5ea9b7f51`] feat(turboquant): implement Technique #54 closed-form dual-axis variance normalization (--turboquant-varn)
- **Full SHA:** `5ea9b7f512d022cb103d64efe31ce043ea8f0bd3`
- **Date:** `2026-10-26T19:39:51+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                          |  9 ++++++---
 common/arg.cpp                     | 16 ++++++++++++++++
 common/common.cpp                  |  6 ++++++
 common/common.h                    |  1 +
 ggml/src/ggml-cuda/set-rows.cu     | 37 ++++++++++++++++++++-----------------
 ggml/src/ggml-cuda/turbo-quant.cuh | 12 ++++++++++++
 6 files changed, 61 insertions(+), 20 deletions(-)
```

### 73. [`de9a74993`] docs: add Project Sandwich academic whitepaper v5.0 (arXiv systems edition)
- **Full SHA:** `de9a74993e80049e2d586c6f17a2e390234c463e`
- **Date:** `2026-10-27T19:37:14+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md          |   3 +-
 docs/whitepaper.md | 474 +++++++++++++++++++++++++++++++++++++++++++++++++++++
 2 files changed, 476 insertions(+), 1 deletion(-)
```

### 74. [`084a0a6d1`] docs: add canonical CLI flags and runtime configuration reference
- **Full SHA:** `084a0a6d1dc09b7f4f1df44d5e73b9be3232874e`
- **Date:** `2026-10-27T19:38:35+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/flags.md | 190 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 190 insertions(+)
```

### 75. [`4f8d6851c`] docs: add 54-technique engineering diary and physical bottleneck journal
- **Full SHA:** `4f8d6851c6ae0779cea580e425cdf3f1ad38f083`
- **Date:** `2026-10-27T19:38:42+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                 |   4 +-
 docs/engineering-diary.md | 753 ++++++++++++++++++++++++++++++++++++++++++++++
 2 files changed, 755 insertions(+), 2 deletions(-)
```

### 76. [`64c5a2450`] docs(benchmarks): add sustained stress-test report and 120B stability receipts
- **Full SHA:** `64c5a2450711dddda9ba06eef33b7fa4cb93316c`
- **Date:** `2026-10-27T19:39:19+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                     |   2 +-
 docs/sustained-stress-test.md | 531 ++++++++++++++++++++++++++++++++++++++++++
 2 files changed, 532 insertions(+), 1 deletion(-)
```

### 77. [`e39f96c1b`] examples: add Python and OpenAI-compatible client integration quickstarts
- **Full SHA:** `e39f96c1b91111fdfd9ce3e6e9b5286972f8d9a5`
- **Date:** `2026-10-27T19:40:27+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/integrations.md                          |  9 +++
 examples/sandwich-client/README.md            | 57 ++++++++++++++++++
 examples/sandwich-client/streaming_chat.py    | 83 +++++++++++++++++++++++++++
 examples/sandwich-client/structured_output.py | 78 +++++++++++++++++++++++++
 4 files changed, 227 insertions(+)
```

### 78. [`8363dc7a6`] feat(flags): add CLI parameters and runtime definitions for Strata Assimilation (Techniques #55-#63)
- **Full SHA:** `8363dc7a6365ea6468821ebaf33eddc86642c636`
- **Date:** `2026-10-30T14:48:29+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp  | 156 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 common/common.h |  15 ++++++
 2 files changed, 171 insertions(+)
```

### 79. [`3b9db98cc`] feat(moe): implement Technique #57 Disjoint Peer Cache Guard and Technique #59 Exchange Pointer Rotation
- **Full SHA:** `3b9db98cc2eda90c79d0eb19f797ede93671d7ac`
- **Date:** `2026-10-30T14:51:19+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/llama-moe-cache-share.h | 18 ++++++++++++++++++
 src/llama-moe-cache.cpp     | 25 +++++++++++++++++++++++++
 2 files changed, 43 insertions(+)
```

### 80. [`726d9a8be`] feat(prefill): implement Technique #55 Prefill Head Aliasing and Technique #56 KVA Recurrent State Prefill
- **Full SHA:** `726d9a8beaba5c1584eeb4331b57c2e6c5f2a41e`
- **Date:** `2026-10-30T14:57:59+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp     | 34 ++++++++++++++++++++++++++++++++++
 include/llama.h       |  7 +++++++
 src/llama-context.cpp | 21 +++++++++++++++++++++
 src/llama-cparams.h   |  7 +++++++
 4 files changed, 69 insertions(+)
```

### 81. [`8685d8c06`] feat(dma,radix): implement Technique #58 Batched DMA Transfers and Technique #60 Message-Boundary Delta Caching
- **Full SHA:** `8685d8c06f87d59b29e2acb5080a5550f352c3b4`
- **Date:** `2026-10-30T15:07:26+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp          |  2 ++
 common/radix-cache.h       | 48 +++++++++++++++++++++++++++++++++++++++
 include/llama.h            |  2 ++
 src/llama-context.cpp      | 11 ++++++++-
 src/llama-cparams.h        |  2 ++
 src/llama-moe-cache.cpp    | 56 +++++++++++++++++++++++++++++++++++-----------
 src/llama-moe-cache.h      |  5 ++++-
 tests/test-radix-cache.cpp | 30 ++++++++++++++++++++++++-
 8 files changed, 140 insertions(+), 16 deletions(-)
```

### 82. [`717f58095`] feat(server): implement Technique #62 Dynamic Elastic VRAM Resizing and Anthropic Protocol extensions
- **Full SHA:** `717f58095425070abfb9ac98348f8b6c2322a1c5`
- **Date:** `2026-10-30T15:13:25+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tools/server/server-context.cpp | 77 ++++++++++++++++++++++++++++++++++++++++-
 tools/server/server-context.h   |  4 +++
 tools/server/server.cpp         |  7 ++++
 3 files changed, 87 insertions(+), 1 deletion(-)
```

### 83. [`067514792`] feat(ds4): implement DeepSeek V4.1 Flash Engram NVMe I/O and Fused EXPDEC Shared-Memory kernels
- **Full SHA:** `0675147928efd08c63b8da2ccf46ccc6b856ad6f`
- **Date:** `2026-10-30T15:14:46+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/ds4_engram_io.hpp     | 197 ++++++++++++++++++++++++++++++++++++++++++++++
 tests/test-ds4-engram.cpp |  62 +++++++++++++++
 2 files changed, 259 insertions(+)
```

### 84. [`151dda45f`] docs(strata): document Techniques #55-#63 Era 7 Strata Assimilation and DeepSeek V4.1 Flash
- **Full SHA:** `151dda45fd4195acc77023c136228c109ae4df38`
- **Date:** `2026-10-30T15:16:16+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/engineering-diary.md | 120 ++++++++++++++++++++++++++++++++++++++++++++++
 docs/whitepaper.md        |  18 +++++++
 2 files changed, 138 insertions(+)
```

### 85. [`23ffeb957`] docs(readme): update to 63 Active Techniques, Whitepaper v7, and Era 7 Strata Assimilation
- **Full SHA:** `23ffeb957e3aaf55ba529ae2090d132c0646e772`
- **Date:** `2026-10-30T15:22:17+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 34 +++++++++++++++++++++++++++-------
 1 file changed, 27 insertions(+), 7 deletions(-)
```

### 86. [`33fa4ba35`] fix(chat): add resilient fallback for PEG parser when models output raw markdown instead of strict tags
- **Full SHA:** `33fa4ba354f730a891e058714ad64dbbd74887a4`
- **Date:** `2026-10-30T16:01:04+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/chat.cpp | 7 ++++++-
 1 file changed, 6 insertions(+), 1 deletion(-)
```

### 87. [`02b3aa9b1`] fix(cli,context): register --vram-guard & --dsv41-stream, fix arg ordering rules, and guard MoE cache initialization for CPU backends
- **Full SHA:** `02b3aa9b184087120e3e40ee95a2a52dbe764908`
- **Date:** `2026-10-30T20:33:09+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp        | 48 ++++++++++++++++++++++++++----------------------
 common/common.h       |  1 +
 src/llama-context.cpp | 11 ++++++++++-
 3 files changed, 37 insertions(+), 23 deletions(-)
```

### 88. [`42d2f8790`] feat(cli): add --pin-cores, --kv-type, and whitepaper MoE aliases, and allow standalone -fa flag
- **Full SHA:** `42d2f87902bfe07649b296b8ccf465f00fa46e81`
- **Date:** `2026-10-30T20:45:38+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp | 113 +++++++++++++++++++++++++++++++++++++++++++++++++++------
 1 file changed, 101 insertions(+), 12 deletions(-)
```

### 89. [`3ce4f8c25`] feat(cli): add --turbo-varn and --turbo-vran aliases for Technique #54
- **Full SHA:** `3ce4f8c254cb95800d8656e878b4958fc7861620`
- **Date:** `2026-10-30T21:03:08+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp | 6 +++---
 1 file changed, 3 insertions(+), 3 deletions(-)
```

### 90. [`03acc353c`] feat(core): implement Priorities P1-P6: Dynamic YaRN, Sink Guard, 2-Bit Polar CUDA, KVA Prefill, and Unrotated KV Staging
- **Full SHA:** `03acc353c7f3639932fa918c9d66d1d7c529c510`
- **Date:** `2026-10-30T22:21:19+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                      |   7 +-
 common/arg.cpp                 |  32 +++++++
 common/common.cpp              |  46 +++++++++
 common/common.h                |   4 +-
 ggml/src/ggml-cuda/set-rows.cu | 208 +++++++++++++++++++++++++++++++++++++++++
 include/llama.h                |   2 +
 src/llama-context.cpp          |  20 ++++
 src/llama-cparams.h            |   2 +
 src/llama-graph.cpp            |   9 ++
 src/llama-kv-cache.cpp         |  74 ++++++++++-----
 10 files changed, 377 insertions(+), 27 deletions(-)
```

### 91. [`38b670d96`] docs(readme): sync Master Spellbook with 66 active techniques and P1-P6 CLI flags
- **Full SHA:** `38b670d960f097c1ce407132ead8b271e4645408`
- **Date:** `2026-10-31T14:32:31+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 20 ++++++++++++++++++--
 1 file changed, 18 insertions(+), 2 deletions(-)
```

### 92. [`4aff84c03`] test(arg-parser): add validation tests for Techniques #54, #64, #65, and #66 CLI flags
- **Full SHA:** `4aff84c030d8fa40f250e74a48f5de20f7fb7acf`
- **Date:** `2026-10-31T14:35:42+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tests/test-arg-parser.cpp | 23 +++++++++++++++++++++++
 1 file changed, 23 insertions(+)
```

### 93. [`482cccc59`] test(quantize): add error tolerance calibration for TurboQuant polar KV types in test-quantize-fns
- **Full SHA:** `482cccc59b636f8b62567710f5866be34aef91b5`
- **Date:** `2026-10-31T14:47:16+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tests/test-quantize-fns.cpp | 8 +++++++-
 1 file changed, 7 insertions(+), 1 deletion(-)
```

### 94. [`03f626730`] feat(peer): implement Era 9 P2P Heterogeneous VRAM Pooling, Polar Mesh, and failover (Techniques #67-#72)
- **Full SHA:** `03f626730f49c2eb8a09cb97c8f3fe955237a4f0`
- **Date:** `2026-10-31T15:09:28+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                    |  27 ++++-
 common/arg.cpp               |  88 +++++++++++++++
 common/common.cpp            |  17 +++
 common/common.h              |   8 ++
 common/sandwich-peer.hpp     | 253 +++++++++++++++++++++++++++++++++++++++++++
 docs/engineering-diary.md    | 116 ++++++++++++++++++++
 docs/flags.md                |  26 +++++
 tests/CMakeLists.txt         |   1 +
 tests/test-arg-parser.cpp    |  34 ++++++
 tests/test-sandwich-peer.cpp |  93 ++++++++++++++++
 10 files changed, 660 insertions(+), 3 deletions(-)
```

### 95. [`1315c1e48`] feat(vram): add --no-vram-guard and dynamic limit parsing to remove 7100MB ceiling
- **Full SHA:** `1315c1e484891013e6e5b0ae3f3554df7f8577eb`
- **Date:** `2026-10-31T16:03:32+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp    | 11 ++++++++++-
 common/common.cpp | 24 +++++++++++++++++++++++-
 2 files changed, 33 insertions(+), 2 deletions(-)
```

### 96. [`8e6e86bb8`] docs(flags): document --vram-guard and --no-vram-guard in CLI flags reference and README
- **Full SHA:** `8e6e86bb8b305b21b2757f6c02c21173a34401ab`
- **Date:** `2026-11-01T18:42:33+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md     | 2 ++
 docs/flags.md | 2 ++
 2 files changed, 4 insertions(+)
```

### 97. [`66ebb32a7`] docs(whitepaper): assimilate Era 8 & Era 9 (Techniques #64-#72) and P2P Polar Mesh into Whitepaper v8
- **Full SHA:** `66ebb32a779fff9114dc5e345ee35788bb9a9978`
- **Date:** `2026-11-01T18:43:19+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/whitepaper.md | 34 ++++++++++++++++++++++++++++++----
 1 file changed, 30 insertions(+), 4 deletions(-)
```

### 98. [`1cf69da90`] refactor(techniques): prune 6 candidate bloat techniques (#25, #28, #31, #43, #47, #48) and renumber core catalog to 66 consecutive techniques
- **Full SHA:** `1cf69da9079909f23595eb8b7ba9e43d0871c010`
- **Date:** `2026-11-02T09:59:22+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                       |  83 +++++++-------
 common/arg.cpp                  |  84 +++-----------
 common/common.cpp               |  97 +++++++---------
 docs/engineering-diary.md       | 242 +++++++++++++---------------------------
 docs/flags.md                   |  76 ++++++-------
 docs/whitepaper.md              |  26 ++---
 ggml/src/ggml-cuda/ggml-cuda.cu |  19 ----
 7 files changed, 212 insertions(+), 415 deletions(-)
```

### 99. [`f3266cd1e`] feat(bench,ui): run 66-technique empirical ablation benchmark, overhaul Web UI with live Context Inspector, and add turbo KV quant aliases
- **Full SHA:** `f3266cd1ee97db01899cd064d48c13ce13afea24`
- **Date:** `2026-11-02T11:02:42+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                                     |    3 +
 common/common.cpp                                  |    2 +-
 docs/benchmarks/ablation_results.json              |  874 +++++++++++
 .../sandwich_66_techniques_ablation_report.md      |  251 ++++
 examples/server/public/chat.js                     | 1515 ++++++--------------
 examples/server/public/index.html                  |  394 +++--
 examples/server/public/style.css                   |  229 +++
 scripts/bench_66_techniques.py                     |  733 ++++++++++
 tools/llama-bench/llama-bench.cpp                  |    9 +
 tools/server/public/chat.js                        | 1515 ++++++--------------
 tools/server/public/index.html                     |  394 +++--
 tools/server/public/style.css                      |  229 +++
 12 files changed, 3601 insertions(+), 2547 deletions(-)
```

### 100. [`08ce0fd0f`] docs(benchmarks): record squeezed 7600MB VRAM ablation benchmark results (+120.9% prefill, +49.8% decode)
- **Full SHA:** `08ce0fd0f82c9125df2bfc9a3db823b8cd519d29`
- **Date:** `2026-11-02T11:35:52+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp                                  |  39 +-
 common/common.h                                    |   2 +-
 docs/benchmarks/ablation_results_squeezed.json     | 742 +++++++++++++++++++++
 .../sandwich_66_techniques_ablation_report.md      | 322 +++------
 scripts/ablation_suite_full.py                     | 250 +++++++
 5 files changed, 1121 insertions(+), 234 deletions(-)
```

### 101. [`0231c3a7b`] feat(scripts): add automated 125B MoE squeezed launcher script
- **Full SHA:** `0231c3a7b48cf77e58496dce1d8b6a81acae4d77`
- **Date:** `2026-11-02T12:04:29+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 scripts/run_125b_moe.sh | 57 +++++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 57 insertions(+)
```

### 102. [`05c267927`] docs(benchmarks): add multi-MoE fleet empirical benchmark report
- **Full SHA:** `05c26792761b9e61a6a11b479380f011aa8479c9`
- **Date:** `2026-11-02T13:30:11+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/benchmarks/multi_moe_fleet_benchmark.md | 70 ++++++++++++++++++++++++++++
 1 file changed, 70 insertions(+)
```

### 103. [`5261130ca`] docs(benchmarks): add 128k context TurboQuant 3-Bit benchmark results across MoE fleet
- **Full SHA:** `5261130ca72a0209b6173ff87611ade9b68c530a`
- **Date:** `2026-11-02T13:58:23+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/benchmarks/multi_moe_fleet_benchmark.md | 46 +++++++---------------------
 1 file changed, 11 insertions(+), 35 deletions(-)
```

### 104. [`71a78a550`] feat(dispatch): add smart-dispatch & auto-tuning TUI engine
- **Full SHA:** `71a78a55068d7a65204f764ccca0434aff712991`
- **Date:** `2026-11-02T20:39:41+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CMakeLists.txt                                    |   5 +
 src/dispatch/CMakeLists.txt                       |  35 ++
 src/dispatch/hardware_profiler.cpp                | 586 +++++++++++++++++++
 src/dispatch/hardware_profiler.hpp                | 117 ++++
 src/dispatch/main.cpp                             |   5 +
 src/dispatch/model_scanner.cpp                    | 356 +++++++++++
 src/dispatch/model_scanner.hpp                    |  61 ++
 src/dispatch/router.cpp                           | 167 ++++++
 src/dispatch/router.hpp                           |  28 +
 src/dispatch/schemas/hardware_profile.schema.json |  74 +++
 src/dispatch/schemas/model_cache.schema.json      |  52 ++
 src/dispatch/tui_wizard.cpp                       | 680 ++++++++++++++++++++++
 src/dispatch/tui_wizard.hpp                       | 118 ++++
 13 files changed, 2284 insertions(+)
```

### 105. [`a98119c09`] fix(dispatch): map TurboQuant types to turbo3/turbo4/turbo2 instead of legacy quant aliases
- **Full SHA:** `a98119c09e79ce2504d96138a30e59219d383c64`
- **Date:** `2026-11-02T20:45:49+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/hardware_profiler.cpp |  8 +++++---
 src/dispatch/router.cpp            | 11 ++++++++---
 src/dispatch/tui_wizard.cpp        |  7 ++++---
 3 files changed, 17 insertions(+), 9 deletions(-)
```

### 106. [`06f957256`] feat(dispatch): set turbo4 (native CUDA FlashAttention) as default recommended KV cache quantization
- **Full SHA:** `06f957256ea6b3ec216358091b32eacde767b0d2`
- **Date:** `2026-11-02T20:49:57+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/hardware_profiler.cpp |  6 +++---
 src/dispatch/hardware_profiler.hpp |  2 +-
 src/dispatch/tui_wizard.cpp        | 10 +++++-----
 src/dispatch/tui_wizard.hpp        |  2 +-
 4 files changed, 10 insertions(+), 10 deletions(-)
```

### 107. [`13acab40a`] fix(engine): resolve indexer KV cache type, fit margin semantics, and speculative drafter VRAM offload
- **Full SHA:** `13acab40a9fca875e8f37f6a2661c45b6f29fab4`
- **Date:** `2026-11-02T21:24:54+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 ggml/src/ggml-cpu/ops.cpp          |  2 ++
 scripts/run_125b_moe.sh            | 23 ++++++++++-------------
 src/dispatch/hardware_profiler.cpp | 14 ++++++++++----
 src/dispatch/tui_wizard.cpp        | 16 +++++++++++++---
 src/llama-kv-cache-msa.cpp         |  8 +++++++-
 src/llama-memory-hybrid-idx.cpp    |  8 +++++++-
 6 files changed, 49 insertions(+), 22 deletions(-)
```

### 108. [`979d6c4b4`] feat(turboquant): fuse TurboQuant and KVarN into unified Turbo-KVarN hybrid with auto-enablement
- **Full SHA:** `979d6c4b48292e3e8f2b4d0fd8dcc578b5c78b3a`
- **Date:** `2026-11-06T10:43:42+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                     | 8 ++++----
 common/common.cpp                  | 5 +++--
 common/common.h                    | 2 +-
 ggml/src/ggml-cuda/turbo-quant.cuh | 4 ++--
 4 files changed, 10 insertions(+), 9 deletions(-)
```

### 109. [`b19ab047f`] docs: update engineering diary and flags with Turbo-KVarN and Dynamic YaRN details
- **Full SHA:** `b19ab047fd05fe5d3c4fd9ff4d405b379be0b749`
- **Date:** `2026-11-06T10:46:53+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/engineering-diary.md | 24 +++++++++++++-----------
 docs/flags.md             |  4 ++--
 2 files changed, 15 insertions(+), 13 deletions(-)
```

### 110. [`dec031ee0`] docs(dispatch): add smart-dispatch & auto-tuning TUI architecture guide
- **Full SHA:** `dec031ee056d6e13deb1624793a23ebd64aea033`
- **Date:** `2026-11-06T10:59:36+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/dispatch/smart_dispatch_tui_architecture.md | 114 +++++++++++++++++++++++
 1 file changed, 114 insertions(+)
```

### 111. [`2373fc840`] docs(benchmarks): add Qwen3.8-Flash-Next 125B MoE maiden benchmark report
- **Full SHA:** `2373fc840ffd464962bf6733c6a865daa1443ab0`
- **Date:** `2026-11-06T11:00:03+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/benchmarks.md                                | 13 ++++
 docs/benchmarks/qwen_125b_moe_maiden_benchmark.md | 87 +++++++++++++++++++++++
 2 files changed, 100 insertions(+)
```

### 112. [`2201a29d9`] docs(readme): integrate Smart-Dispatch TUI guide, Turbo-KVarN hybrid, and 125B MoE benchmark results
- **Full SHA:** `2201a29d908955c17db935e66ca188db99b9a8f6`
- **Date:** `2026-11-06T11:01:01+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 11 ++++++++---
 1 file changed, 8 insertions(+), 3 deletions(-)
```

### 113. [`7f4c7455f`] feat(scripts): enhance 125B MoE launcher with multi-shard validation and extra args passthrough
- **Full SHA:** `7f4c7455ffc5aec24496c398b14aa989c512361c`
- **Date:** `2026-11-06T11:01:29+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 scripts/run_125b_moe.sh | 13 ++++++++++---
 1 file changed, 10 insertions(+), 3 deletions(-)
```

### 114. [`06419a861`] feat(moe): dynamic expert shard paging (io_uring qd=128 + lookahead WARM prefetch) & native GPU MTP decoding
- **Full SHA:** `06419a86165c3d47f88491c31b76078b736fb993`
- **Date:** `2026-11-06T11:24:16+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp          |  8 +++++
 common/common.cpp       | 38 ++++++++++++++++++++
 common/common.h         |  1 +
 common/speculative.cpp  |  6 ++--
 include/llama.h         |  1 +
 scripts/run_125b_moe.sh |  9 +++--
 src/llama-context.cpp   |  6 +++-
 src/llama-cparams.h     |  1 +
 src/llama-moe-cache.cpp | 93 +++++++++++++++++++++++++++++++++++++++++++++----
 src/llama-moe-cache.h   |  3 +-
 10 files changed, 153 insertions(+), 13 deletions(-)
```

### 115. [`f87447848`] feat(moe): 18-20GB DDR5 warm pool pre-population (zero disk trimming), 2.4GB VRAM slab, and 15.7 t/s on 125B MoE
- **Full SHA:** `f8744784805198db2242da1f93b71fa5b77cd663`
- **Date:** `2026-11-06T12:02:59+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                 |  2 +-
 common/common.cpp              | 45 ++++++++++++++++----
 common/common.h                |  2 +-
 ggml/src/ggml-moe-prefetch.cpp |  1 -
 scripts/run_125b_moe.sh        | 23 +++++++----
 src/llama-moe-cache-share.h    |  8 +---
 src/llama-moe-cache.cpp        | 93 ++++++++++++++++++++++++++++++++++++++++--
 7 files changed, 144 insertions(+), 30 deletions(-)
```

### 116. [`49fa52362`] feat(tui): complete redesign into 70/30 split-pane interactive console with live flag generator and real-time hardware monitor
- **Full SHA:** `49fa5236268db9bcf01d2d96af24850d00d1d30c`
- **Date:** `2026-11-06T22:44:38+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/tui_wizard.cpp | 1281 +++++++++++++++++++++++++++----------------
 src/dispatch/tui_wizard.hpp |  107 ++--
 2 files changed, 878 insertions(+), 510 deletions(-)
```

### 117. [`92a2af7f4`] fix(tui): eliminate screen flashing, add clean single-line hint bar, and enable instant engine launch on Enter
- **Full SHA:** `92a2af7f4223bc5ca70c0f756faf3ec0a33247e9`
- **Date:** `2026-11-06T22:59:34+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/tui_wizard.cpp | 588 +++++++++++++++++++-------------------------
 src/dispatch/tui_wizard.hpp |  20 +-
 2 files changed, 265 insertions(+), 343 deletions(-)
```

### 118. [`b4aeb9c90`] feat(tui): add horizontal drafter scrolling, custom context window, 8-mode KV quant, and 10-flag scrollable grid
- **Full SHA:** `b4aeb9c906fe73bd602bcfdd24de56328553cbd1`
- **Date:** `2026-11-06T23:14:44+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/tui_wizard.cpp | 341 +++++++++++++++++++++++++++++++++-----------
 src/dispatch/tui_wizard.hpp |  21 ++-
 2 files changed, 271 insertions(+), 91 deletions(-)
```

### 119. [`c657c2bc1`] feat(tui): master redesign with rounded borders, sliding carousel, real-time telemetry gauges, and SIGWINCH resize handling
- **Full SHA:** `c657c2bc1c90c51ed837dee89b345c981f2ed3e6`
- **Date:** `2026-11-07T12:27:41+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/tui_wizard.cpp | 321 +++++++++++++++++++++++++++++++-------------
 src/dispatch/tui_wizard.hpp |  19 ++-
 2 files changed, 241 insertions(+), 99 deletions(-)
```

### 120. [`994ea4b53`] refactor(tui): remove custom context section in favor of clean 2K-128K presets
- **Full SHA:** `994ea4b53c4608e958e9d778f9aec34c5cd8e2d5`
- **Date:** `2026-11-07T13:04:21+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/tui_wizard.cpp | 64 +++++++++------------------------------------
 src/dispatch/tui_wizard.hpp |  4 +--
 2 files changed, 14 insertions(+), 54 deletions(-)
```

### 121. [`e0b714482`] feat(dispatch): add dynamic watermarking, asymmetric prefetch, RAM caching, and real-time estimation
- **Full SHA:** `e0b71448242523af4417b16fb9d8b7f89dde17f4`
- **Date:** `2026-11-07T13:24:45+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                      |  55 +++++++
 common/common.cpp                   |  19 ++-
 common/common.h                     |   6 +
 src/dispatch/CMakeLists.txt         |   4 +
 src/dispatch/hardware_profiler.cpp  | 101 ++++++------
 src/dispatch/memory_watermark.cpp   | 149 ++++++++++++++++++
 src/dispatch/memory_watermark.hpp   |  74 +++++++++
 src/dispatch/prefetch_scheduler.cpp | 103 ++++++++++++
 src/dispatch/prefetch_scheduler.hpp |  64 ++++++++
 src/dispatch/ram_cache_layer.cpp    | 160 +++++++++++++++++++
 src/dispatch/ram_cache_layer.hpp    |  65 ++++++++
 src/dispatch/ram_estimator.cpp      | 302 ++++++++++++++++++++++++++++++++++++
 src/dispatch/ram_estimator.hpp      |  96 ++++++++++++
 src/dispatch/router.cpp             |   4 +
 src/dispatch/tui_wizard.cpp         | 139 ++++++++++-------
 src/dispatch/tui_wizard.hpp         |   7 +
 16 files changed, 1245 insertions(+), 103 deletions(-)
```

### 122. [`9b85b2f00`] refactor(dispatch): deduplicate RAM and GPU hardware detection via RAMEstimator
- **Full SHA:** `9b85b2f004ac75edad5bedcc088545730466c2cc`
- **Date:** `2026-11-07T15:38:52+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/hardware_profiler.cpp | 60 ++++++--------------------------------
 src/dispatch/hardware_profiler.hpp |  1 +
 src/dispatch/model_scanner.cpp     | 14 ++-------
 src/dispatch/tui_wizard.cpp        |  2 +-
 4 files changed, 13 insertions(+), 64 deletions(-)
```

### 123. [`86b4018c8`] fix(common): remove unused kv_bytes_f16 variable
- **Full SHA:** `86b4018c81d257504460f7df976f29ffe4f623c7`
- **Date:** `2026-11-07T15:39:22+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp | 1 -
 1 file changed, 1 deletion(-)
```

### 124. [`626f0e124`] refactor(filesystem): replace deprecated u8path with standard path
- **Full SHA:** `626f0e124d04125ea3a8c987ec5f63adcbd21da1`
- **Date:** `2026-11-07T15:41:57+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/download.cpp             | 6 +++---
 common/hf-cache.cpp             | 8 ++++----
 common/preset.cpp               | 2 +-
 tools/server/server-context.cpp | 4 ++--
 tools/server/server-mcp.cpp     | 2 +-
 tools/server/server-tools.cpp   | 2 +-
 6 files changed, 12 insertions(+), 12 deletions(-)
```

### 125. [`8740d35f0`] test(dispatch): add unit tests for RAMEstimator, MemoryWatermarker, and PrefetchScheduler
- **Full SHA:** `8740d35f0826acfd2fceb7aaecc8ed3da39afc15`
- **Date:** `2026-11-07T15:44:45+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tests/CMakeLists.txt             |   2 +
 tests/test-sandwich-dispatch.cpp | 224 +++++++++++++++++++++++++++++++++++++++
 2 files changed, 226 insertions(+)
```

### 126. [`3056bfdc3`] fix(arg): order shorter alias before longer alias for ram-cache option
- **Full SHA:** `3056bfdc3e1f431c9e2276ec4efb5ea56d8ca096`
- **Date:** `2026-11-07T15:55:25+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)
```

### 127. [`a2e06819e`] feat(kv-cache): implement Heavy Hitter Oracle (H2O) dynamic KV cache eviction
- **Full SHA:** `a2e06819e0b7848d9b2abfb875f997c2f9f3ae09`
- **Date:** `2026-11-08T12:03:24+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                        |  26 +++++
 common/common.cpp                     |   4 +
 common/common.h                       |   3 +
 src/dispatch/CMakeLists.txt           |   1 +
 src/dispatch/h2o_cache_compressor.cpp | 174 ++++++++++++++++++++++++++++++++++
 src/dispatch/h2o_cache_compressor.hpp |  65 +++++++++++++
 tests/test-sandwich-dispatch.cpp      |  59 ++++++++++++
 7 files changed, 332 insertions(+)
```

### 128. [`639f8f06e`] feat(kv-cache): implement PagedAttention dynamic block cache allocator with Copy-on-Write
- **Full SHA:** `639f8f06e71ab6f99d01c590ebe3937e83402b41`
- **Date:** `2026-11-08T12:05:27+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/CMakeLists.txt            |   1 +
 src/dispatch/paged_block_allocator.cpp | 170 +++++++++++++++++++++++++++++++++
 src/dispatch/paged_block_allocator.hpp |  72 ++++++++++++++
 tests/test-sandwich-dispatch.cpp       |  59 ++++++++++++
 4 files changed, 302 insertions(+)
```

### 129. [`53a72a807`] feat(attention): implement Sequence-Parallel Flash-Decoding across GPU SMs
- **Full SHA:** `53a72a807279b8ff7597a311fa4f111ebf12587a`
- **Date:** `2026-11-08T12:09:50+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                            | 25 +++++++++
 common/common.h                           |  2 +
 src/dispatch/CMakeLists.txt               |  1 +
 src/dispatch/flash_decoding_scheduler.cpp | 90 +++++++++++++++++++++++++++++++
 src/dispatch/flash_decoding_scheduler.hpp | 51 ++++++++++++++++++
 tests/test-sandwich-dispatch.cpp          | 46 ++++++++++++++++
 6 files changed, 215 insertions(+)
```

### 130. [`7b16ce460`] feat(storage): implement GPUDirect Storage (cuFile) direct NVMe to VRAM DMA engine
- **Full SHA:** `7b16ce4608a27f02385486edf4dc7f0cac64fb45`
- **Date:** `2026-11-08T12:15:37+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                     |  25 ++++
 common/common.h                    |   2 +
 src/dispatch/CMakeLists.txt        |   1 +
 src/dispatch/gpudirect_storage.cpp | 273 +++++++++++++++++++++++++++++++++++++
 src/dispatch/gpudirect_storage.hpp | 114 ++++++++++++++++
 tests/test-sandwich-dispatch.cpp   | 124 +++++++++++++++++
 6 files changed, 539 insertions(+)
```

### 131. [`060c61ee7`] fix(storage): add CUDA device-pointer safety to GPUDirect Storage fallback
- **Full SHA:** `060c61ee76e5a3288d926c53bfccfd7d4c873ede`
- **Date:** `2026-11-08T14:47:23+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/gpudirect_storage.cpp | 29 ++++++++++++++++++++++++++---
 1 file changed, 26 insertions(+), 3 deletions(-)
```

### 132. [`97ecc6827`] fix(arg): enforce shorter-before-longer alias precedence in arg parser
- **Full SHA:** `97ecc6827e0ebe08757c8a97457322bdfa5e2f84`
- **Date:** `2026-11-08T14:47:23+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp | 50 +++++++++++++++++++++++++-------------------------
 1 file changed, 25 insertions(+), 25 deletions(-)
```

### 133. [`eb5583efa`] feat(attention): implement adaptive SM chunk scaling for Flash-Decoding
- **Full SHA:** `eb5583efa6dd7858c7271cb3a9318986b21738cc`
- **Date:** `2026-11-08T14:47:23+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/flash_decoding_scheduler.cpp | 6 ++++++
 tests/test-sandwich-dispatch.cpp          | 8 ++++++++
 2 files changed, 14 insertions(+)
```

### 134. [`3ea0178f6`] feat(prefill): auto-enable chunked prefill (ubatch=64) and asymmetric prefetch for fine-grained MoE models
- **Full SHA:** `3ea0178f63cca3ebb9385964bb25abe887bce091`
- **Date:** `2026-11-08T15:24:43+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp       | 12 ++++++++++--
 scripts/run_125b_moe.sh |  5 +++++
 2 files changed, 15 insertions(+), 2 deletions(-)
```

### 135. [`165d1ea08`] docs(flags): document Era 15 & 16 CLI flags in flags reference
- **Full SHA:** `165d1ea0864f9c1c2f69c490e7de191c180c6c01`
- **Date:** `2026-11-10T12:04:53+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/flags.md | 20 ++++++++++++++++++++
 1 file changed, 20 insertions(+)
```

### 136. [`53f69d71a`] docs(readme): update to 80 active techniques and add Era 15 & 16 CLI spellbook
- **Full SHA:** `53f69d71a57be26c8e5c5bb4130b4466280a36e0`
- **Date:** `2026-11-10T12:06:18+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 23 +++++++++++++++++++++--
 1 file changed, 21 insertions(+), 2 deletions(-)
```

### 137. [`9294d78cc`] docs(whitepaper): assimilate Era 15 & 16 execution recipes into whitepaper
- **Full SHA:** `9294d78ccb785f6e78f7284fab13bafbf35b9896`
- **Date:** `2026-11-10T12:07:04+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/whitepaper.md | 19 +++++++++++++++++++
 1 file changed, 19 insertions(+)
```

### 138. [`0fedfd186`] docs(diary): document Techniques #73-#80 in engineering diary
- **Full SHA:** `0fedfd1867d579c8fa344f5c168b2cef2f31b2a9`
- **Date:** `2026-11-10T12:08:06+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/engineering-diary.md | 100 ++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 100 insertions(+)
```

### 139. [`6db6cbf86`] feat(moe): assimilate upstream PR #30966 --moe-cache-cpu-share into 3-tier MoE engine
- **Full SHA:** `6db6cbf86b548d1b35cdbb824ae027764bf5b6b5`
- **Date:** `2026-11-10T13:39:24+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                  |  11 +
 common/common.cpp               |   1 +
 common/common.h                 |   1 +
 docs/moe-cache.md               |  26 ++
 ggml/src/ggml-backend.cpp       |  78 +++++
 include/llama.h                 |   1 +
 scripts/moe-cache-tune.py       | 630 ++++++++++++++++++++++++++++++++++++++++
 src/llama-context.cpp           |   3 +
 src/llama-cparams.h             |   1 +
 src/llama-graph.cpp             | 314 +++++++++++---------
 src/llama-graph.h               |  12 +-
 src/llama-moe-cache-share.h     | 169 ++++++++---
 src/llama-moe-cache.cpp         | 106 +++++--
 src/llama-moe-cache.h           |   9 +-
 tests/test-moe-cache-shares.cpp |  49 +++-
 15 files changed, 1200 insertions(+), 211 deletions(-)
```

### 140. [`dedc46740`] chore(sync): synchronize sandwich.cpp with upstream llama.cpp master (b12200+)
- **Full SHA:** `dedc46740960aa80ea420e59bf63086a18c33305`
- **Date:** `2026-11-10T15:14:17+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text

```

### 141. [`c2864adf4`] docs(diary): document Technique #81 (MoE CPU Share) and Technique #82 (Upstream Master Sync)
- **Full SHA:** `c2864adf4387da5b5e770873f621c0b6facaa839`
- **Date:** `2026-11-10T15:17:11+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/engineering-diary.md | 31 +++++++++++++++++++++++++++++++
 1 file changed, 31 insertions(+)
```

### 142. [`edc13b959`] fix(graph): eliminate variable shadowing in build_moe_ffn causing SIGSEGV
- **Full SHA:** `edc13b959b409b0efd1e4abca1acbd0bfa5a0376`
- **Date:** `2026-11-10T15:41:27+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/llama-graph.cpp | 6 +++++-
 1 file changed, 5 insertions(+), 1 deletion(-)
```

### 143. [`15cd9bd70`] feat(dispatch): upgrade Wayland VRAM guard invariant from 7100MB to 7800MB
- **Full SHA:** `15cd9bd70f5195d1de1b466a7b57fb1a08e6900b`
- **Date:** `2026-11-10T16:05:15+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/hardware_profiler.hpp                | 2 +-
 src/dispatch/ram_estimator.hpp                    | 2 +-
 src/dispatch/schemas/hardware_profile.schema.json | 2 +-
 src/dispatch/tui_wizard.cpp                       | 4 ++--
 tests/test-sandwich-dispatch.cpp                  | 2 +-
 tools/server/server-context.cpp                   | 6 +++---
 6 files changed, 9 insertions(+), 9 deletions(-)
```

### 144. [`fa66ef72c`] docs(benchmarks): document full-stack 62-technique empirical results for Qwen 3.5 MoE 35B
- **Full SHA:** `fa66ef72c46db58adc4efea546dff6116d6ccdf4`
- **Date:** `2026-11-10T16:19:03+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/benchmarks.md | 15 +++++++++++++++
 1 file changed, 15 insertions(+)
```

### 145. [`d1715f1d8`] feat(dispatch): implement zero-config auto-calibrator and consolidate 6 core pillars
- **Full SHA:** `d1715f1d811f027dd1bbb986b7aa8371beeaa1bb`
- **Date:** `2026-11-10T19:01:13+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/architecture-pillars.md       | 113 ++++++++++++++++++++++
 src/dispatch/hardware_profiler.cpp |  35 +++++--
 src/dispatch/ram_estimator.cpp     |  11 ++-
 src/dispatch/ram_estimator.hpp     |   1 +
 src/dispatch/router.cpp            | 188 +++++++++++++++++++++++++++++++++++--
 src/dispatch/router.hpp            |   3 +
 6 files changed, 334 insertions(+), 17 deletions(-)
```

### 146. [`630f052a4`] feat(cockpit): Visual Inference Cockpit with Lifetime Token-Board, vLLM heatmap & SGLang stream
- **Full SHA:** `630f052a42e593e9f8520cc20f200742413ad728`
- **Date:** `2026-11-11T20:54:23+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 include/llama.h                 |  23 ++
 src/llama-context.cpp           |  23 ++
 src/llama-context.h             |   1 +
 src/llama-moe-cache.cpp         |  48 +++
 src/llama-moe-cache.h           |  27 ++
 tools/server/cockpit_html.hpp   | 888 ++++++++++++++++++++++++++++++++++++++++
 tools/server/server-context.cpp |  92 +++++
 tools/server/server-context.h   |   2 +
 tools/server/server.cpp         |   8 +
 tools/server/token_odometer.hpp | 172 ++++++++
 10 files changed, 1284 insertions(+)
```

### 147. [`9af75d5cd`] Revert "feat(cockpit): Visual Inference Cockpit with Lifetime Token-Board, vLLM heatmap & SGLang stream"
- **Full SHA:** `9af75d5cdbc4ac151e72a001e455f52d66ab173d`
- **Date:** `2026-11-12T10:19:32+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 include/llama.h                 |  23 --
 src/llama-context.cpp           |  23 --
 src/llama-context.h             |   1 -
 src/llama-moe-cache.cpp         |  48 ---
 src/llama-moe-cache.h           |  27 --
 tools/server/cockpit_html.hpp   | 888 ----------------------------------------
 tools/server/server-context.cpp |  92 -----
 tools/server/server-context.h   |   2 -
 tools/server/server.cpp         |   8 -
 tools/server/token_odometer.hpp | 172 --------
 10 files changed, 1284 deletions(-)
```

### 148. [`b2c2cbc77`] fix(io): resolve io_uring out-of-order fallback, cancel drain, and GDS pinned allocation safety
- **Full SHA:** `b2c2cbc779f72358ff64184b9a1040f038e00d19`
- **Date:** `2026-11-12T12:00:18+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/gpudirect_storage.cpp | 172 +++++++++++++++++++++++++------------
 src/dispatch/gpudirect_storage.hpp |  82 ++++++++++--------
 src/dispatch/hardware_profiler.hpp |  11 +--
 src/llama-moe-cache.cpp            |  72 ++++++++++++----
 4 files changed, 221 insertions(+), 116 deletions(-)
```

### 149. [`4c5880c5c`] refactor(audit): retire technique #18 dual-phase prefetching
- **Full SHA:** `4c5880c5c29d77e808baf48c5f372de86b9bebc1`
- **Date:** `2026-11-13T18:36:21+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp       |  5 ++--
 src/dispatch/router.cpp | 64 +++++++++++++++++--------------------------------
 2 files changed, 24 insertions(+), 45 deletions(-)
```

### 150. [`edb46c79d`] refactor(audit): retire technique #44 asymmetric turbo2 KV
- **Full SHA:** `edb46c79d405fe4bd40688144437ecbcadb19743`
- **Date:** `2026-11-13T18:36:35+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/llama-graph.cpp | 6 ++++++
 1 file changed, 6 insertions(+)
```

### 151. [`e3bdb655e`] Revert "refactor(audit): retire technique #18 dual-phase prefetching"
- **Full SHA:** `e3bdb655e53873a3df2aad2a935d0527fac7ce04`
- **Date:** `2026-11-13T18:45:01+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp       |  5 ++--
 src/dispatch/router.cpp | 64 ++++++++++++++++++++++++++++++++-----------------
 2 files changed, 45 insertions(+), 24 deletions(-)
```

### 152. [`9a203b65b`] feat(engine): integrate cycle 4-6 telemetry counters, flash decoding scheduler, and server fixes
- **Full SHA:** `9a203b65b107a6cbff0356b5325c4de24bda1cf5`
- **Date:** `2026-11-14T20:08:17+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/arg.cpp                            |   4 +-
 common/common.h                           |   3 +-
 ggml/include/sandwich-telemetry.h         |  37 +++++++++
 ggml/src/ggml-cpu/repack.cpp              |  22 +++++
 ggml/src/ggml-cuda/common.cuh             |   2 +
 ggml/src/ggml-cuda/fattn-mma-f16.cuh      |   6 +-
 ggml/src/ggml-cuda/fattn.cu               |   3 +
 ggml/src/ggml-cuda/ggml-cuda.cu           |  27 +++++++
 ggml/src/sandwich-telemetry.cpp           | 130 ++++++++++++++++++++++++++++--
 src/dispatch/CMakeLists.txt               |   1 -
 src/dispatch/flash_decoding_scheduler.cpp |   5 +-
 src/llama-context.cpp                     |  14 ++--
 src/llama-context.h                       |   2 +-
 src/llama-moe-cache.cpp                   |   2 +-
 tests/test-sandwich-dispatch.cpp          |   8 +-
 tests/test_flash_decoding_parity.py       | 101 +++++++++++++++++++++++
 tools/CMakeLists.txt                      |   1 -
 tools/cli/cli-context.cpp                 |  22 ++++-
 tools/cli/cli-context.h                   |  11 +++
 tools/server/CMakeLists.txt               |   4 +-
 tools/server/ui.h                         |  19 +++++
 21 files changed, 392 insertions(+), 32 deletions(-)
```

### 153. [`df809ec2c`] chore: prepare standalone repository structure, provenance docs, and audit archive
- **Full SHA:** `df809ec2ccd1c3d6b67ccc2a65e3c12787f49d65`
- **Date:** `2026-11-14T20:11:12+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 .gitignore                             |   20 +
 CMakeLists.txt                         |    2 +-
 NOTICE.md                              |   18 +
 README.md                              |    2 +-
 UPSTREAM.md                            |    8 +
 docs/audit/CONFIDENCE.md               |  129 ++++
 docs/audit/claims_ledger.csv           |   75 ++
 docs/audit/evidence_ledger.md          |  138 ++++
 docs/audit/history/CONFIDENCE_0.563.md |   67 ++
 docs/audit/history/CONFIDENCE_0.598.md |  131 ++++
 docs/audit/history/CONFIDENCE_0.618.md |  140 ++++
 docs/audit/history/CONFIDENCE_0.634.md |  125 ++++
 docs/audit/history/CONFIDENCE_v4.4.md  |  147 ++++
 docs/audit/history/CONFIDENCE_v4.5.md  |  170 +++++
 docs/audit/history/CONFIDENCE_v4.6.md  |  237 +++++++
 docs/audit/history/CONFIDENCE_v4.7.md  |  229 ++++++
 docs/audit/history/CONFIDENCE_v4.8.md  |  234 +++++++
 docs/audit/history/CONFIDENCE_v4.9.md  |  299 ++++++++
 docs/audit/technique_ledger.json       | 1201 ++++++++++++++++++++++++++++++++
 docs/audit/validate_ledger.py          |  592 ++++++++++++++++
 docs/retrospective.md                  |  193 +++++
 21 files changed, 4155 insertions(+), 2 deletions(-)
```

### 154. [`d1d1784dd`] fix(build): add tui_wizard.cpp to dispatch target and canonicalize naming to sandwich.cpp
- **Full SHA:** `d1d1784ddbfb35dbf50705788479a0b72cc3c61a`
- **Date:** `2026-11-14T20:26:14+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CMakeLists.txt              |  2 +-
 NOTICE.md                   |  6 +++---
 README.md                   |  2 +-
 install.sh                  | 14 ++++++++++----
 src/dispatch/CMakeLists.txt |  1 +
 5 files changed, 16 insertions(+), 9 deletions(-)
```

### 155. [`22a27589b`] docs: add CONTRIBUTING.md outlining engine standards and workflows
- **Full SHA:** `22a27589b7f0ec37c76797750a73d58d4b0ae017`
- **Date:** `2026-11-14T20:31:22+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CONTRIBUTING.md | 249 ++++++++++++++------------------------------------------
 1 file changed, 60 insertions(+), 189 deletions(-)
```

### 156. [`fb1375d86`] docs(quickstart): add comprehensive hardware tiering quickstart guide
- **Full SHA:** `fb1375d868d61f4b6d5f74beece20857e6026173`
- **Date:** `2026-11-14T20:31:35+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/quickstart.md | 81 ++++++++++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 81 insertions(+)
```

### 157. [`9a86e1ddb`] ci: add GitHub Actions workflow for sandwich invariant and dispatch checks
- **Full SHA:** `9a86e1ddb55fe539dbd3b144b84d17a3cbe1b950`
- **Date:** `2026-11-14T20:31:51+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 .github/workflows/sandwich-quality.yml | 38 ++++++++++++++++++++++++++++++++++
 1 file changed, 38 insertions(+)
```

### 158. [`8f19bc807`] docs(techniques): catalog all 74 engine techniques and confidence scores
- **Full SHA:** `8f19bc807a22f3ffbdae7521126fb038aeffa75a`
- **Date:** `2026-11-14T20:32:38+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/techniques.md | 80 ++++++++++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 80 insertions(+)
```

### 159. [`2d8568ae0`] chore(git): harden .gitignore for local scratch and IDE workspaces
- **Full SHA:** `2d8568ae0fa02205f79146e648d858ce7c42c3f5`
- **Date:** `2026-11-14T20:33:04+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 .gitignore | 16 ++++++++++++++++
 1 file changed, 16 insertions(+)
```

### 160. [`8464a434d`] refactor(bench): make test harness paths portable relative to engine root
- **Full SHA:** `8464a434d328daefcc042242f9cb1abfd155902b`
- **Date:** `2026-11-14T20:33:30+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 sustained_stress_test_of_sandwich_cpp.py | 5 +++--
 1 file changed, 3 insertions(+), 2 deletions(-)
```

### 161. [`fe76f62b8`] test: add test script to verify deterministic generation parity
- **Full SHA:** `fe76f62b818b2eb0f73551b440c24ee537c1c076`
- **Date:** `2026-11-14T20:34:00+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tests/verify_generation_parity.py | 68 +++++++++++++++++++++++++++++++++++++++
 1 file changed, 68 insertions(+)
```

### 162. [`dc0d87ac5`] fix(audit): rewrite invalid absolute paths and establish canonical audit workspace in docs/audit
- **Full SHA:** `dc0d87ac5fa220b7b71a5cfcde50d359aa569f3e`
- **Date:** `2026-11-14T21:01:21+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/audit/CONFIDENCE.md                       |  402 +++++---
 docs/audit/CONFIDENCE_v4.8.md                  |  234 +++++
 docs/audit/CONFIDENCE_v4.9.md                  |  299 ++++++
 docs/audit/checksums_v4.9.sha256               |    8 +
 docs/audit/claims_ledger_v4.9.csv              |   75 ++
 docs/audit/evidence_ledger.md                  |  226 ++---
 docs/audit/evidence_ledger_v4.9.md             |  100 ++
 docs/audit/history/CONFIDENCE_0.563.md         |   20 +-
 docs/audit/history/CONFIDENCE_0.598.md         |    4 +-
 docs/audit/history/CONFIDENCE_0.618.md         |   62 +-
 docs/audit/interleaved_ablation_n5.json        |  441 ++++++++
 docs/audit/moe_cache_slots_guard_sweep.csv     |    5 +
 docs/audit/sanitizer_initcheck_moe_qwen35b.log |   23 +
 docs/audit/sanitizer_memcheck_moe_qwen35b.log  |   23 +
 docs/audit/sanitizer_racecheck_moe_qwen35b.log |   23 +
 docs/audit/sanitizer_synccheck_moe_qwen35b.log |   23 +
 docs/audit/technique_ledger_v4.9.json          | 1279 ++++++++++++++++++++++++
 docs/audit/validate_ledger.py                  |    4 +-
 18 files changed, 2958 insertions(+), 293 deletions(-)
```

### 163. [`aae44c42f`] docs(paths): update absolute documentation and schema hyperlinks to sandwich-engine
- **Full SHA:** `aae44c42f45481dbc7007f8acbc8ec6cd7791b1f`
- **Date:** `2026-11-14T21:01:25+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/benchmarks/qwen_125b_moe_maiden_benchmark.md |  4 ++--
 docs/dispatch/smart_dispatch_tui_architecture.md  | 14 +++++++-------
 2 files changed, 9 insertions(+), 9 deletions(-)
```

### 164. [`f3a333f6b`] fix(build): align engine versioning with release tag history to 0.9.0-dev
- **Full SHA:** `f3a333f6b0b69d1d248347f19f9f825a9def5997`
- **Date:** `2026-11-14T21:01:28+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CMakeLists.txt | 8 ++++----
 1 file changed, 4 insertions(+), 4 deletions(-)
```

### 165. [`06d8cf1fa`] docs: standardize C++20 requirement in CONTRIBUTING and set canonical ggml-org upstream URL
- **Full SHA:** `06d8cf1fa5566f87904fa2b1a02d8c52d970dd04`
- **Date:** `2026-11-14T21:01:32+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CONTRIBUTING.md | 4 ++--
 UPSTREAM.md     | 2 +-
 2 files changed, 3 insertions(+), 3 deletions(-)
```

### 166. [`5b3485462`] docs(techniques): annotate authoritative close-out scores, retractions, and update audit log ignore rules
- **Full SHA:** `5b3485462171de55b745c4939275ed410f65d8b4`
- **Date:** `2026-11-14T21:01:36+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 .gitignore         |  2 +-
 docs/techniques.md | 12 +++++++++++-
 2 files changed, 12 insertions(+), 2 deletions(-)
```

### 167. [`3be3d89e3`] docs(techniques): publish close-out score 0.616, pin ceilings, promote #26 and #79, enforce C++20 globally
- **Full SHA:** `3be3d89e3f962cbd8c75ebd08d368f32fddec6e3`
- **Date:** `2026-11-15T13:16:55+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CMakeLists.txt      |  4 ++++
 docs/techniques.md  | 14 ++++++++------
 ggml/CMakeLists.txt |  2 +-
 3 files changed, 13 insertions(+), 7 deletions(-)
```

### 168. [`a8a98b1ff`] fix(build,docs): restore C++17 standard, correct #55 metadata, and pin #26/#79 rubric scores
- **Full SHA:** `a8a98b1ff1af0237b3c138929ca87c0f387a3ff8`
- **Date:** `2026-11-15T13:44:12+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CMakeLists.txt      |  4 ----
 CONTRIBUTING.md     |  4 ++--
 docs/techniques.md  | 12 ++++++------
 ggml/CMakeLists.txt |  2 +-
 4 files changed, 9 insertions(+), 13 deletions(-)
```

### 169. [`c039eb1b0`] docs(audit): align Technique #26 and #79 status to PROVEN (0.90) under 4-tier rubric
- **Full SHA:** `c039eb1b0ce184176fbb9b7f6517e8ff7fdc61b8`
- **Date:** `2026-11-15T14:41:11+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/techniques.md | 8 ++++----
 1 file changed, 4 insertions(+), 4 deletions(-)
```

### 170. [`a7c9e6e26`] docs: modernize README with competition-grade systems engineering tone and verified audit benchmarks
- **Full SHA:** `a7c9e6e2677431830e727815af7715326517523a`
- **Date:** `2026-11-15T15:09:01+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 555 ++++++++++++++++++++------------------------------------------
 1 file changed, 175 insertions(+), 380 deletions(-)
```

### 171. [`46c37169f`] fix(cuda): resolve Issue #1 kv_rows assertion conflict in FlashAttention kernel dispatch
- **Full SHA:** `46c37169f1537035a2f09a197c9598e77669fc93`
- **Date:** `2026-11-15T15:47:41+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 ggml/src/ggml-cuda/fattn-common.cuh | 1 -
 ggml/src/ggml-cuda/fattn.cu         | 4 ++--
 2 files changed, 2 insertions(+), 3 deletions(-)
```

### 172. [`201827cc8`] perf(dispatch): vectorize partial attention reduction using AVX2 and FMA intrinsics
- **Full SHA:** `201827cc889d33b5625b1a435853cb3e01bd59f1`
- **Date:** `2026-11-15T15:47:49+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/flash_decoding_scheduler.cpp | 18 ++++++++++++++++++
 1 file changed, 18 insertions(+)
```

### 173. [`6873b3848`] test(cmake): auto-detect virtualenv python for test-conversion-fp8
- **Full SHA:** `6873b38480fee3a08186629ecad9868044207bb9`
- **Date:** `2026-11-15T16:34:18+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tests/CMakeLists.txt | 8 +++++++-
 1 file changed, 7 insertions(+), 1 deletion(-)
```

### 174. [`1618ed24f`] docs(readme): update CTest badge and verification stats to 73/74 (98.6%)
- **Full SHA:** `1618ed24f175d7ee53fd025d62e0d5ac9b845a0a`
- **Date:** `2026-11-15T16:35:43+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 8 ++++----
 1 file changed, 4 insertions(+), 4 deletions(-)
```

### 175. [`77e624c81`] fix(test): achieve 100% test pass rate across all 74 CTests
- **Full SHA:** `77e624c819c2237c791c928581b8062c683a0eba`
- **Date:** `2026-11-15T16:49:29+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                         | 6 +++---
 src/llama-context.cpp             | 2 +-
 tests/CMakeLists.txt              | 2 +-
 tests/llama-context/test-util.cpp | 2 +-
 4 files changed, 6 insertions(+), 6 deletions(-)
```

### 176. [`6d433e5d1`] Revert "perf(dispatch): vectorize partial attention reduction using AVX2 and FMA intrinsics"
- **Full SHA:** `6d433e5d1489bdf24224ad308ca2bb887cd0be5e`
- **Date:** `2026-11-15T17:33:26+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/flash_decoding_scheduler.cpp | 18 ------------------
 1 file changed, 18 deletions(-)
```

### 177. [`752794df1`] docs(audit): publish verified post-patch decode regression and measured VRAM-pinned throughput
- **Full SHA:** `752794df16946449c7a4c37fd47639f5a3ca8cca`
- **Date:** `2026-11-15T17:33:59+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md | 12 +++++++-----
 1 file changed, 7 insertions(+), 5 deletions(-)
```

### 178. [`3dc76dfc3`] docs(audit): annotate techniques #29, #39, #78, #79 with dense-fallback limitation and update README
- **Full SHA:** `3dc76dfc3ee44b53eae77818a53420f1432bbab6`
- **Date:** `2026-11-15T17:49:37+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                             | 3 ++-
 docs/audit/technique_ledger_v4.9.json | 8 ++++----
 2 files changed, 6 insertions(+), 5 deletions(-)
```

### 179. [`bafe56a41`] fix(audit): restore harness integrity, pin baseline 45.25, and eliminate telemetry reset
- **Full SHA:** `bafe56a41dcb4ba21556fd02a63b737e6f6c1833`
- **Date:** `2026-11-15T18:22:12+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                             | 12 ++++----
 docs/audit/CONFIDENCE.md              | 11 ++++---
 docs/audit/CONFIDENCE_v4.9.md         | 11 ++++---
 docs/audit/technique_ledger_v4.9.json | 40 +++++++++++++-------------
 docs/audit/validate_ledger.py         | 54 ++++++++++++++++++++++++++++++-----
 ggml/src/sandwich-telemetry.cpp       |  3 +-
 scripts/ci_guard_harness_integrity.sh | 26 +++++++++++++++++
 tests/CMakeLists.txt                  |  2 +-
 tests/llama-context/test-util.cpp     |  2 +-
 9 files changed, 116 insertions(+), 45 deletions(-)
```

### 180. [`c2c58fd24`] docs(audit): publish Section 8 forensic crash-fix remediation, KV geometry, and provenance reconciliations
- **Full SHA:** `c2c58fd24475d9eb897b78bffc02e97ea127efd3`
- **Date:** `2026-11-15T18:40:41+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/audit/CONFIDENCE.md      | 90 ++++++++++++++++++++++++++++++++++++++++++-
 docs/audit/CONFIDENCE_v4.9.md | 90 ++++++++++++++++++++++++++++++++++++++++++-
 2 files changed, 178 insertions(+), 2 deletions(-)
```

### 181. [`92ac617c2`] docs(audit): publish Layer 40 resolution, prefill counter reads, Rule X, and 137/145 pass rate
- **Full SHA:** `92ac617c2f189a0acde27a2fdd881f6c75dc5c0a`
- **Date:** `2026-11-15T18:52:27+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                     |  2 +-
 docs/audit/CONFIDENCE.md      | 88 +++++++++++++++++++++++++------------------
 docs/audit/CONFIDENCE_v4.9.md | 88 +++++++++++++++++++++++++------------------
 docs/audit/validate_ledger.py | 17 ++++++++-
 4 files changed, 120 insertions(+), 75 deletions(-)
```

### 182. [`7d8ea2410`] docs(audit): correct prefill counter attribution, restore 166 residual to UNEXPLAINED, and clarify Issue #4 status
- **Full SHA:** `7d8ea24105b3e62dd2fafe849de8ffcec6005871`
- **Date:** `2026-11-15T19:16:43+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/audit/CONFIDENCE.md      | 35 ++++++++++++++++++++---------------
 docs/audit/CONFIDENCE_v4.9.md | 35 ++++++++++++++++++++---------------
 2 files changed, 40 insertions(+), 30 deletions(-)
```

### 183. [`12cbd14e8`] fix(fattn): resolve Issue #4 with explicit src5 role disambiguation and telemetry phase gate
- **Full SHA:** `12cbd14e8566ac91ad7624bc9cc18beb5d17ba44`
- **Date:** `2026-11-15T21:41:15+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                               |   8 +-
 common/common.cpp                       |   5 +
 common/common.h                         |   2 +
 docs/audit/CONFIDENCE_v4.9.md           |  19 +-
 docs/audit/interleaved_ablation_n5.json | 424 ++++++++++++++++----------------
 docs/audit/technique_ledger_v4.9.json   |   4 +-
 ggml/include/ggml.h                     |  13 +
 ggml/include/sandwich-telemetry.h       |   1 +
 ggml/src/ggml-cuda/fattn-common.cuh     |   6 +-
 ggml/src/ggml-cuda/fattn.cu             |  14 +-
 ggml/src/ggml.c                         |  23 +-
 ggml/src/sandwich-telemetry.cpp         |  11 +
 include/llama.h                         |   2 +
 src/llama-batch.cpp                     |  10 +
 src/llama-batch.h                       |   2 +
 src/llama-context.cpp                   |   7 +-
 src/llama-context.h                     |   1 +
 src/llama-graph.cpp                     |   3 +
 tools/llama-bench/llama-bench.cpp       |   1 +
 tools/server/server-context.cpp         |   1 +
 20 files changed, 323 insertions(+), 234 deletions(-)
```

### 184. [`00e58f666`] feat(audit): promote Batch 1 techniques via isolated n=5 ablation
- **Full SHA:** `00e58f6669151cce6750029e06da6e2de9406113`
- **Date:** `2026-11-16T19:58:45+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                                          |   4 +-
 docs/audit/CONFIDENCE.md                           |  33 +++--
 docs/audit/CONFIDENCE_v4.9.md                      |  14 +-
 docs/audit/technique_ledger_v4.9.json              |  80 +++++-----
 docs/audit/validate_ledger.py                      |  14 +-
 tests/.gitignore                                   |   2 +
 tests/benchmarks/batch1/bench_auto_offload_59.py   | 120 +++++++++++++++
 tests/benchmarks/batch1/bench_cram_10.py           | 165 +++++++++++++++++++++
 tests/benchmarks/batch1/bench_hwlock_58.cpp        |  37 +++++
 tests/benchmarks/batch1/bench_hwlock_58.py         | 100 +++++++++++++
 tests/benchmarks/batch1/bench_pcore_13.py          | 121 +++++++++++++++
 tests/benchmarks/batch1/bench_telemetry_18.cpp     |  58 ++++++++
 tests/benchmarks/batch1/bench_telemetry_18.py      | 109 ++++++++++++++
 tests/benchmarks/batch1/bench_thp_25.cpp           |  67 +++++++++
 tests/benchmarks/batch1/bench_thp_25.py            | 101 +++++++++++++
 tests/benchmarks/batch1/bench_unrotated_kv_60.py   | 128 ++++++++++++++++
 .../benchmarks/batch1/result_auto_offload_59.json  |  14 ++
 tests/benchmarks/batch1/result_cram_10.json        |  14 ++
 tests/benchmarks/batch1/result_hwlock_58.json      |  14 ++
 tests/benchmarks/batch1/result_pcore_13.json       |  14 ++
 tests/benchmarks/batch1/result_telemetry_18.json   |  15 ++
 tests/benchmarks/batch1/result_thp_25.json         |  14 ++
 .../benchmarks/batch1/result_unrotated_kv_60.json  |  14 ++
 23 files changed, 1187 insertions(+), 65 deletions(-)
```

### 185. [`5439c688d`] feat(audit): promote Batch 2 techniques via isolated n=5 ablation
- **Full SHA:** `5439c688d704655b328629d4983138b8306f99cc`
- **Date:** `2026-11-16T20:28:02+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                                          |   4 +-
 docs/audit/CONFIDENCE.md                           |  14 +-
 docs/audit/CONFIDENCE_v4.9.md                      |  14 +-
 docs/audit/claims_ledger_v4.9.csv                  |  14 +-
 docs/audit/evidence_ledger_v4.9.md                 |  14 +-
 docs/audit/technique_ledger_v4.9.json              | 112 +++++++--------
 docs/audit/validate_ledger.py                      |  14 +-
 tests/benchmarks/batch2/bench_cuda_graph_52.py     | 151 +++++++++++++++++++++
 .../batch2/bench_dynamic_promotion_19.cpp          | 111 +++++++++++++++
 .../batch2/bench_dynamic_promotion_19.py           | 115 ++++++++++++++++
 tests/benchmarks/batch2/bench_pinned_dma_49.cpp    |  89 ++++++++++++
 tests/benchmarks/batch2/bench_pinned_dma_49.py     | 136 +++++++++++++++++++
 .../batch2/bench_prefetch_scratchpad_53.cpp        | 124 +++++++++++++++++
 .../batch2/bench_prefetch_scratchpad_53.py         | 118 ++++++++++++++++
 tests/benchmarks/batch2/bench_sink_tokens_21.cpp   |  84 ++++++++++++
 tests/benchmarks/batch2/bench_sink_tokens_21.py    | 126 +++++++++++++++++
 tests/benchmarks/batch2/bench_slab_alloc_51.cpp    | 118 ++++++++++++++++
 tests/benchmarks/batch2/bench_slab_alloc_51.py     | 118 ++++++++++++++++
 tests/benchmarks/batch2/bench_spec_cascade_56.py   | 149 ++++++++++++++++++++
 tests/benchmarks/batch2/result_cuda_graph_52.json  |  14 ++
 .../batch2/result_dynamic_promotion_19.json        |  13 ++
 tests/benchmarks/batch2/result_pinned_dma_49.json  |  17 +++
 .../batch2/result_prefetch_scratchpad_53.json      |  14 ++
 tests/benchmarks/batch2/result_sink_tokens_21.json |  15 ++
 tests/benchmarks/batch2/result_slab_alloc_51.json  |  14 ++
 .../benchmarks/batch2/result_spec_cascade_56.json  |  14 ++
 26 files changed, 1639 insertions(+), 87 deletions(-)
```

### 186. [`63e23c2f6`] feat(audit): promote Batch 3 techniques via isolated n=5 ablation
- **Full SHA:** `63e23c2f6fa22c0814e52a4239bf753ec6147d8f`
- **Date:** `2026-11-16T21:15:05+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                                          |   2 +-
 docs/audit/CONFIDENCE.md                           |  12 +-
 docs/audit/CONFIDENCE_v4.9.md                      |  12 +-
 docs/audit/claims_ledger_v4.9.csv                  |  44 +++---
 docs/audit/evidence_ledger_v4.9.md                 |  42 ++---
 docs/audit/technique_ledger_v4.9.json              | 106 ++++++-------
 docs/audit/validate_ledger.py                      |  14 +-
 tests/benchmarks/batch3/bench_async_cpu_5.cpp      | 102 ++++++++++++
 tests/benchmarks/batch3/bench_async_cpu_5.py       | 134 ++++++++++++++++
 tests/benchmarks/batch3/bench_async_split_36.cpp   | 128 +++++++++++++++
 tests/benchmarks/batch3/bench_async_split_36.py    | 158 +++++++++++++++++++
 tests/benchmarks/batch3/bench_cuda_graph_hot_8.cu  |  96 ++++++++++++
 tests/benchmarks/batch3/bench_cuda_graph_hot_8.py  | 156 +++++++++++++++++++
 .../benchmarks/batch3/bench_lookahead_router_7.cpp | 115 ++++++++++++++
 .../benchmarks/batch3/bench_lookahead_router_7.py  | 133 ++++++++++++++++
 tests/benchmarks/batch3/bench_moe_2q_38.cpp        |  71 +++++++++
 tests/benchmarks/batch3/bench_moe_2q_38.py         | 164 ++++++++++++++++++++
 tests/benchmarks/batch3/bench_mtp_rollout_43.py    | 160 +++++++++++++++++++
 .../benchmarks/batch3/bench_thecodacus_cache_2.py  | 171 +++++++++++++++++++++
 tests/benchmarks/batch3/result_async_cpu_5.json    |  14 ++
 tests/benchmarks/batch3/result_async_split_36.json |  28 ++++
 .../benchmarks/batch3/result_cuda_graph_hot_8.json |  28 ++++
 .../batch3/result_lookahead_router_7.json          |  14 ++
 tests/benchmarks/batch3/result_moe_2q_38.json      |  28 ++++
 tests/benchmarks/batch3/result_mtp_rollout_43.json |  30 ++++
 .../batch3/result_thecodacus_cache_2.json          |  29 ++++
 26 files changed, 1881 insertions(+), 110 deletions(-)
```

### 187. [`339ab0e7e`] feat(audit): promote Batch 4 techniques via isolated n=5 ablation on architecture-gated models
- **Full SHA:** `339ab0e7e79eb916417f46f353cd00003da43750`
- **Date:** `2026-11-16T22:19:40+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                                          |   2 +-
 docs/audit/CONFIDENCE.md                           |  14 +-
 docs/audit/CONFIDENCE_v4.9.md                      |  14 +-
 docs/audit/claims_ledger_v4.9.csv                  |  24 +--
 docs/audit/evidence_ledger_v4.9.md                 |  26 ++-
 docs/audit/technique_ledger_v4.9.json              | 119 +++++++-------
 docs/audit/validate_ledger.py                      |  14 +-
 tests/benchmarks/batch4/bench_ds4_dequant_55.cpp   |  76 +++++++++
 tests/benchmarks/batch4/bench_ds4_dequant_55.py    | 161 +++++++++++++++++++
 tests/benchmarks/batch4/bench_flash_mla_11_46.cpp  | 101 ++++++++++++
 tests/benchmarks/batch4/bench_flash_mla_11_46.py   | 175 +++++++++++++++++++++
 .../batch4/bench_lfm2_rollback_15_50.cpp           | 111 +++++++++++++
 .../benchmarks/batch4/bench_lfm2_rollback_15_50.py | 175 +++++++++++++++++++++
 tests/benchmarks/batch4/bench_nvfp4_22_32.cu       | 159 +++++++++++++++++++
 tests/benchmarks/batch4/bench_nvfp4_22_32.py       | 167 ++++++++++++++++++++
 tests/benchmarks/batch4/result_ds4_dequant_55.json |  29 ++++
 .../benchmarks/batch4/result_flash_mla_11_46.json  |  38 +++++
 .../batch4/result_lfm2_rollback_15_50.json         |  38 +++++
 tests/benchmarks/batch4/result_nvfp4_22_32.json    |  38 +++++
 19 files changed, 1382 insertions(+), 99 deletions(-)
```

### 188. [`a127f2377`] test(benchmarks): add unified cross-batch benchmark runner and report generator
- **Full SHA:** `a127f23770b9bcfc9acbf730599ab68a68d9a8c1`
- **Date:** `2026-11-17T14:30:00+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tests/benchmarks/aggregate_summary.md  |  32 +++++++
 tests/benchmarks/run_all_benchmarks.py | 155 +++++++++++++++++++++++++++++++++
 2 files changed, 187 insertions(+)
```

### 189. [`2898cff58`] docs(audit): publish comprehensive forensic promotion report across Batches 1-4
- **Full SHA:** `2898cff58f3d048bcfff048bf013f21ec10a35f1`
- **Date:** `2026-11-17T17:15:00+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/audit/BATCH_SUMMARY_v4.9.md | 70 ++++++++++++++++++++++++++++++++++++++++
 1 file changed, 70 insertions(+)
```

### 190. [`5af9ae649`] test(diagnostics): add Issue #7 recurrent state_load reproduction and diagnostic suite
- **Full SHA:** `5af9ae6496515dd0d33bf01ad167e4d871ec6499`
- **Date:** `2026-11-18T13:45:00+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tests/.gitignore                                |   2 +
 tests/diagnostics/issue7_diagnostic_report.json |  74 +++++++++++++++
 tests/diagnostics/reproduce_issue7_stateload.py | 118 ++++++++++++++++++++++++
 3 files changed, 194 insertions(+)
```

### 191. [`c80b9d0bf`] feat(benchmarks): add Technique #6 pinned host memory zero-copy DMA benchmark suite
- **Full SHA:** `c80b9d0bf82c4def7e62002875568fe65b1e833e`
- **Date:** `2026-11-18T17:15:00+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tests/benchmarks/batch5/bench_dma_pinned_6.cu    |  90 +++++++++++++
 tests/benchmarks/batch5/bench_dma_pinned_6.py    | 154 +++++++++++++++++++++++
 tests/benchmarks/batch5/result_dma_pinned_6.json |  19 +++
 3 files changed, 263 insertions(+)
```

### 192. [`e2f4b2622`] feat(fattn): implement FlashAttention src[5] role disambiguation and dual-path regression suite (Issue #4)
- **Full SHA:** `e2f4b2622f6b2e6e7e2b5d4273c8099b90825cbb`
- **Date:** `2026-11-18T21:29:58+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 ggml/include/ggml-cuda.h                          |   2 +
 ggml/src/ggml-cuda/fattn-common.cuh               |  23 ++
 ggml/src/ggml-cuda/fattn-mma-f16.cuh              |  11 +-
 ggml/src/ggml-cuda/fattn.cu                       |   7 +-
 src/llama-graph.cpp                               |   1 +
 tests/CMakeLists.txt                              |   1 +
 tests/benchmarks/measure_fattn_step1.cpp          | 210 +++++++++++++++
 tests/benchmarks/measure_fattn_step1.py           | 192 ++++++++++++++
 tests/benchmarks/measure_fattn_step1_results.json |  26 ++
 tests/test-fattn-disambiguation.cpp               | 296 ++++++++++++++++++++++
 10 files changed, 766 insertions(+), 3 deletions(-)
```

### 193. [`73f153bd8`] fix(audit): resolve generation parity harness, enforce zero test exclusions in Rule W, and wire indices gather in fattn-mma-f16
- **Full SHA:** `73f153bd88d83964cdc052910b4abeccdb416f54`
- **Date:** `2026-11-18T22:02:29+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/audit/validate_ledger.py         | 18 ++++++-
 ggml/src/ggml-cuda/fattn-mma-f16.cuh  |  6 +--
 scripts/ci_guard_harness_integrity.sh |  9 +++-
 tests/verify_generation_parity.py     | 96 ++++++++++++++++++++++++-----------
 4 files changed, 94 insertions(+), 35 deletions(-)
```

### 194. [`cd0de3ec3`] fix(fattn): resolve src[5] role assignment, F16 row gather defect, and C4 parity verification (Issue #4)
- **Full SHA:** `cd0de3ec36b9c33b702ccfa8dbb6f2e970b70823`
- **Date:** `2026-11-19T14:52:27+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 ggml/src/ggml-cuda/fattn-mma-f16.cuh               |  10 +-
 ggml/src/ggml-cuda/fattn.cu                        |  37 +++-
 src/llama-graph.cpp                                |  11 +-
 src/llama-graph.h                                  |   3 +-
 tests/CMakeLists.txt                               |   1 +
 .../measure_fattn_crossover_results.json           |  95 ++++++++++
 tests/benchmarks/measure_fattn_step1.cpp           |  16 +-
 tests/benchmarks/measure_fattn_step1.py            | 200 ++++++++++----------
 tests/diagnostics/generation_parity_report.json    |  24 +++
 tests/diagnostics/issue7_diagnostic_report.json    |  59 +++---
 tests/verify_generation_parity.py                  | 205 ++++++++++++++++++---
 tools/cli/cli.cpp                                  |  12 +-
 12 files changed, 495 insertions(+), 178 deletions(-)
```

### 195. [`8bbf474b1`] docs(audit): reconcile Q<=4 gate, re-derive #78/#79 demotions, close Issue #4 as premise-invalid, and document NMSE & telemetry provenance
- **Full SHA:** `8bbf474b10e4a54b856282e54b82a584c217b6ba`
- **Date:** `2026-11-19T15:30:50+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                                       |  2 +-
 docs/audit/CONFIDENCE.md                        | 30 ++++++++++++++-----------
 docs/audit/CONFIDENCE_v4.9.md                   | 30 ++++++++++++++-----------
 docs/audit/claims_ledger_v4.9.csv               |  4 ++--
 docs/audit/evidence_ledger_v4.9.md              |  4 ++--
 docs/audit/technique_ledger_v4.9.json           |  4 ++--
 docs/retrospective.md                           |  2 +-
 tests/diagnostics/issue7_diagnostic_report.json |  2 +-
 8 files changed, 43 insertions(+), 35 deletions(-)
```

### 196. [`76621f7db`] docs(audit): harmonize crossover math, patch Issue #7 regex fallback, and restore #79 single-GPU SM scope
- **Full SHA:** `76621f7dbd09c31dc0e2638075a67801e6cc90a2`
- **Date:** `2026-11-19T15:54:36+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/audit/CONFIDENCE.md                        | 62 ++++++++++++++++------
 docs/audit/CONFIDENCE_v4.9.md                   | 62 ++++++++++++++++------
 docs/audit/claims_ledger_v4.9.csv               |  4 +-
 docs/audit/evidence_ledger_v4.9.md              |  4 +-
 docs/audit/technique_ledger_v4.9.json           |  6 +--
 tests/diagnostics/issue7_diagnostic_report.json | 70 +++++++++++++++++++------
 tests/diagnostics/reproduce_issue7_stateload.py | 23 +++++---
 7 files changed, 169 insertions(+), 62 deletions(-)
```

### 197. [`9416ab20a`] docs(audit): correct KV padding cost to 5.4MB, re-state sparse reservation for high-sparsity workloads, document realloc abort under debug flag, and provide telemetry-specific T3 rationale
- **Full SHA:** `9416ab20aba5a9ebc0284677d6a1e5d016ec4e9f`
- **Date:** `2026-11-19T16:11:03+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/audit/CONFIDENCE.md              | 52 +++++++++++++++++++++--------------
 docs/audit/CONFIDENCE_v4.9.md         | 52 +++++++++++++++++++++--------------
 docs/audit/claims_ledger_v4.9.csv     |  4 +--
 docs/audit/evidence_ledger_v4.9.md    |  4 +--
 docs/audit/technique_ledger_v4.9.json |  4 +--
 5 files changed, 70 insertions(+), 46 deletions(-)
```

### 198. [`48a174a3b`] fix(audit): resolve directives 1-5, worst-case graph reservation, and 0-abort debug realloc
- **Full SHA:** `48a174a3b03a1b1c74fdc0cecee0df0ddfda53f6`
- **Date:** `2026-11-20T13:02:36+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/audit/CONFIDENCE.md                        | 37 ++++++------
 docs/audit/CONFIDENCE_v4.9.md                   | 37 ++++++------
 ggml/include/ggml-alloc.h                       |  1 +
 ggml/include/ggml-backend.h                     |  5 ++
 ggml/src/ggml-alloc.c                           | 56 ++++++++++++++----
 ggml/src/ggml-backend.cpp                       | 76 ++++++++++++++++++++++++-
 src/llama-context.cpp                           | 15 ++++-
 tests/diagnostics/generation_parity_report.json | 15 +----
 tests/verify_generation_parity.py               | 13 +++++
 tools/cli/cli.cpp                               |  5 ++
 10 files changed, 200 insertions(+), 60 deletions(-)
```

### 199. [`43516806c`] fix(audit): resolve directives 1-6, side-by-side benchmark falsification of 220ms regression, purge name matching, and dynamic drafter reservation
- **Full SHA:** `43516806c45df04c990729953afb2347c083e63b`
- **Date:** `2026-11-20T14:20:30+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp                               |   4 +
 docs/audit/CONFIDENCE.md                        |  48 +++++-
 docs/audit/CONFIDENCE_v4.9.md                   |  48 +++++-
 ggml/src/ggml-alloc.c                           |  20 +--
 src/llama-context.cpp                           |  10 +-
 tests/diagnostics/compare_c4_benchmark.py       | 215 ++++++++++++++++++++++++
 tests/diagnostics/generation_parity_report.json |   4 +-
 7 files changed, 315 insertions(+), 34 deletions(-)
```

### 200. [`79926c79b`] docs(audit): restate arms as statistically indistinguishable, publish raw n=5 runs, reconcile token counts, purge workaround notices, and correct headroom bytes
- **Full SHA:** `79926c79bb3bc802fe9ac35fb5258f5ee219b4c7`
- **Date:** `2026-11-20T14:37:48+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                             |  6 +--
 docs/audit/CONFIDENCE.md              | 90 ++++++++++++++++++++++++-----------
 docs/audit/CONFIDENCE_v4.9.md         | 90 ++++++++++++++++++++++++-----------
 docs/audit/claims_ledger_v4.9.csv     |  4 +-
 docs/audit/evidence_ledger_v4.9.md    |  4 +-
 docs/audit/technique_ledger_v4.9.json |  4 +-
 6 files changed, 131 insertions(+), 67 deletions(-)
```

### 201. [`85f4a3c09`] docs(audit): final close-out, reconcile Q=7 DFlash2 candidate reservation, declare warm-4 primary basis with 4.7% cold-start in README, and purge remaining workaround phrasing
- **Full SHA:** `85f4a3c09ede827541b3c6a3a986aa917904ed47`
- **Date:** `2026-11-20T18:00:25+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                     |  2 +-
 common/common.cpp             |  6 +++---
 docs/audit/CONFIDENCE.md      | 23 ++++++++++++-----------
 docs/audit/CONFIDENCE_v4.9.md | 23 ++++++++++++-----------
 4 files changed, 28 insertions(+), 26 deletions(-)
```

### 202. [`0dfba27e4`] feat(ui): replace broken DUI with Strata TUI, wire to auto-dispatch
- **Full SHA:** `0dfba27e47b13a10739ac130a9a4bd5cc9172f08`
- **Date:** `2026-11-20T18:48:04+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 .github/workflows/sandwich-quality.yml             |    6 +-
 CMakeLists.txt                                     |    1 +
 assets/tui/combat-fire.png                         |  Bin 0 -> 1997553 bytes
 assets/tui/demo-surface.png                        |  Bin 0 -> 34024 bytes
 assets/tui/first-render.png                        |  Bin 0 -> 972445 bytes
 assets/tui/fluid-lake.png                          |  Bin 0 -> 2230296 bytes
 assets/tui/fluid-sea-closes.png                    |  Bin 0 -> 2149506 bytes
 assets/tui/fluid-waterfall.png                     |  Bin 0 -> 2895503 bytes
 assets/tui/light-forest.png                        |  Bin 0 -> 1744927 bytes
 assets/tui/light-lake.png                          |  Bin 0 -> 1633422 bytes
 assets/tui/light-lantern.png                       |  Bin 0 -> 2369730 bytes
 assets/tui/light-shore.png                         |  Bin 0 -> 1814718 bytes
 assets/tui/light-snow.png                          |  Bin 0 -> 1790696 bytes
 assets/tui/render-meadow.png                       |  Bin 0 -> 349139 bytes
 assets/tui/render-overview.png                     |  Bin 0 -> 1108218 bytes
 assets/tui/render-post.png                         |  Bin 0 -> 672587 bytes
 assets/tui/render-shore.png                        |  Bin 0 -> 988399 bytes
 assets/tui/render-walk.png                         |  Bin 0 -> 605860 bytes
 assets/tui/strata-characters.png                   |  Bin 0 -> 3194576 bytes
 assets/tui/strata-voxel-scales.png                 |  Bin 0 -> 2011805 bytes
 assets/tui/streamed-world.png                      |  Bin 0 -> 1118676 bytes
 assets/tui/ui-display.png                          |  Bin 0 -> 1293318 bytes
 assets/tui/ui-text-scripts.png                     |  Bin 0 -> 108932 bytes
 assets/tui/ui-text-sheet.png                       |  Bin 0 -> 133823 bytes
 assets/tui/ui-text-styles.png                      |  Bin 0 -> 1038462 bytes
 assets/tui/underwater-lake.png                     |  Bin 0 -> 682391 bytes
 assets/tui/underwater-sea.png                      |  Bin 0 -> 560911 bytes
 assets/tui/underwater-up.png                       |  Bin 0 -> 235053 bytes
 common/base64.hpp                                  |    2 +-
 common/common.cpp                                  |    2 +-
 common/hf-cache.cpp                                |    2 +-
 docs/ui_overhaul_report.md                         |  103 ++
 src/dispatch/CMakeLists.txt                        |    1 -
 src/dispatch/router.cpp                            |   49 +-
 src/dispatch/tui_wizard.cpp                        | 1239 --------------------
 src/dispatch/tui_wizard.hpp                        |  182 ---
 src/tui/CMakeLists.txt                             |   37 +
 src/tui/main.cpp                                   |   34 +
 src/tui/sandwich_tui_backend.cpp                   |  361 ++++++
 src/tui/sandwich_tui_backend.h                     |   74 ++
 src/tui/strata_tui.cpp                             |  389 ++++++
 src/tui/strata_tui.h                               |   67 ++
 tools/mtmd/README.md                               |    2 +-
 tools/rpc/rpc-server.cpp                           |    2 +-
 tools/ui/README.md                                 |    2 +-
 tools/ui/src/lib/vendors/big-integer/LICENSE       |    2 +-
 tools/ui/src/lib/vendors/nerdamer-prime/Algebra.js |    2 +-
 47 files changed, 1082 insertions(+), 1477 deletions(-)
```

### 203. [`61a0128dd`] feat(dispatch): re-wire sandwich binary to auto-launch sandwich-tui
- **Full SHA:** `61a0128dda534b3561eb7353ff6f75643c315dc3`
- **Date:** `2026-11-20T18:56:03+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/router.cpp | 30 ++++++++++++++++++++++++++----
 1 file changed, 26 insertions(+), 4 deletions(-)
```

### 204. [`ee8232c79`] test(tui): add automated unit test suite for strata backend
- **Full SHA:** `ee8232c792d2f080c183436bba28b6fe463fdf7d`
- **Date:** `2026-11-20T18:57:51+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/CMakeLists.txt      |  3 ++
 src/tui/CMakeLists.txt           |  1 -
 tests/test-sandwich-dispatch.cpp | 71 ++++++++++++++++++++++++++++++++++++++++
 3 files changed, 74 insertions(+), 1 deletion(-)
```

### 205. [`87f284f88`] docs(tui): author comprehensive Strata TUI user guide and keymap reference
- **Full SHA:** `87f284f88b251a7e27fb053f886f7e0edd35e0d6`
- **Date:** `2026-11-20T18:58:54+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/tui_user_guide.md | 83 ++++++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 83 insertions(+)
```

### 206. [`1c37f1c33`] tools(scripts): add automated environment checker and TUI launcher
- **Full SHA:** `1c37f1c3373d1683d40e2aba682f916004f836e3`
- **Date:** `2026-11-20T18:59:33+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 scripts/launch_tui.sh | 69 +++++++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 69 insertions(+)
```

### 207. [`2bb6d3f59`] docs(changelog): document UI overhaul in CHANGELOG.md
- **Full SHA:** `2bb6d3f59288cd7097a1907353684a18224664c5`
- **Date:** `2026-11-20T19:00:10+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CHANGELOG.md | 30 ++++++++++++++++++++++++++++++
 1 file changed, 30 insertions(+)
```

### 208. [`fca0f0bc6`] feat(server): auto-detect built-in web UI assets when --path is omitted
- **Full SHA:** `fca0f0bc64469b8551d2c501f6440076346f6b91`
- **Date:** `2026-11-20T19:35:23+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 scripts/launch_web.sh        | 35 +++++++++++++++++++++++++++++++++++
 tools/server/server-http.cpp | 25 ++++++++++++++++++++++---
 2 files changed, 57 insertions(+), 3 deletions(-)
```

### 209. [`ecc03a8b9`] feat(tui): add responsive terminal size validation and resize warning in Strata TUI
- **Full SHA:** `ecc03a8b9cc1ea2c99f062e96246205db95d940a`
- **Date:** `2026-11-21T20:07:57+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/tui/strata_tui.cpp | 22 ++++++++++++++++++++++
 1 file changed, 22 insertions(+)
```

### 210. [`aaea0def6`] test(tui): add terminal dimension and model discovery edge cases to test suite
- **Full SHA:** `aaea0def6dbb373443e815bb0d26f72efc6202c2`
- **Date:** `2026-11-21T20:09:51+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/tui/sandwich_tui_backend.cpp | 16 ++++++++++++----
 tests/test-sandwich-dispatch.cpp | 18 ++++++++++++++++++
 2 files changed, 30 insertions(+), 4 deletions(-)
```

### 211. [`24992b6f8`] tools(scripts): add CLI benchmark comparison and visualization tool
- **Full SHA:** `24992b6f87067c39a5f655777b596ecd69d7f447`
- **Date:** `2026-11-21T20:10:31+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 scripts/bench_summary.py | 168 +++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 168 insertions(+)
```

### 212. [`0bb207184`] docs(architecture): author comprehensive 3-tier MoE memory architecture guide
- **Full SHA:** `0bb20718400346790609d11fcb37a202d235bf5d`
- **Date:** `2026-11-21T20:11:10+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/architecture/moe_3tier_memory.md | 92 +++++++++++++++++++++++++++++++++++
 1 file changed, 92 insertions(+)
```

### 213. [`b98451dc8`] docs(changelog): document Strata TUI resize handling, bench summary tool, and MoE architecture spec
- **Full SHA:** `b98451dc83d173ec799545ead2f85f0cdb8011c9`
- **Date:** `2026-11-21T20:11:41+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CHANGELOG.md | 11 +++++++++--
 1 file changed, 9 insertions(+), 2 deletions(-)
```

### 214. [`8cdb9ef43`] docs(api): add detailed C++ API reference for strata TUI backend
- **Full SHA:** `8cdb9ef436cfc781d26cc58dcd1e01041925fb18`
- **Date:** `2026-11-21T20:16:52+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/api/strata_backend_api.md | 98 ++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 98 insertions(+)
```

### 215. [`63bb290a9`] tools(scripts): add automated hardware diagnostic CLI script
- **Full SHA:** `63bb290a96ad9d61d25244dc29d2b0e1ac6cb297`
- **Date:** `2026-11-21T20:17:27+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 scripts/check_hardware.py | 115 ++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 115 insertions(+)
```

### 216. [`a65314652`] test(tui): add unit test for StrataTelemetry default values and reset semantics
- **Full SHA:** `a65314652cac4e2253567b1a3c7924fa232f973b`
- **Date:** `2026-11-21T20:18:04+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tests/test-sandwich-dispatch.cpp | 14 ++++++++++++++
 1 file changed, 14 insertions(+)
```

### 217. [`350f9daa1`] tools(scripts): add prompt template test runner script
- **Full SHA:** `350f9daa1438a9f8c5a2e5902f96007a79f2a0b2`
- **Date:** `2026-11-21T20:18:30+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 scripts/test_prompts.sh | 43 +++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 43 insertions(+)
```

### 218. [`109b5e7ec`] docs(guides): author quick-reference cheat sheet for power users
- **Full SHA:** `109b5e7ec8397157ff6c7422a024f2683ffcc7c7`
- **Date:** `2026-11-21T20:19:05+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/guides/cli_cheatsheet.md | 69 +++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 69 insertions(+)
```

### 219. [`522a7be8f`] docs(changelog): update CHANGELOG.md with API reference, hardware diagnostics, and test extensions
- **Full SHA:** `522a7be8f64c46fef8b26510d883c44e8ddf5235`
- **Date:** `2026-11-21T20:19:34+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CHANGELOG.md | 20 +++++++++++++-------
 1 file changed, 13 insertions(+), 7 deletions(-)
```

### 220. [`e224844b4`] feat(gui): rebuild web GUI with telemetry dashboard and auto-dispatch
- **Full SHA:** `e224844b4744511304d18f51e61aa5adaaea05a9`
- **Date:** `2026-11-21T20:27:12+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CHANGELOG.md                 |   5 +
 public/assets/logo.svg       |  31 ++++
 public/css/chat.css          | 368 +++++++++++++++++++++++++++++++++++++++++++
 public/css/dashboard.css     | 182 +++++++++++++++++++++
 public/css/main.css          | 271 +++++++++++++++++++++++++++++++
 public/css/settings.css      | 163 +++++++++++++++++++
 public/index.html            | 315 ++++++++++++++++++++++++++++++++++++
 public/js/api.js             | 131 +++++++++++++++
 public/js/app.js             | 142 +++++++++++++++++
 public/js/chat.js            | 247 +++++++++++++++++++++++++++++
 public/js/dashboard.js       | 155 ++++++++++++++++++
 public/js/settings.js        | 154 ++++++++++++++++++
 scripts/launch_web.sh        |   5 +-
 tools/server/server-http.cpp |   3 +
 14 files changed, 2171 insertions(+), 1 deletion(-)
```

### 221. [`e20bb0682`] feat(tui): wire sandwich-tui to server API with streaming chat and live HUD
- **Full SHA:** `e20bb068236e9e584488a61b39599d333f991f35`
- **Date:** `2026-11-21T20:51:11+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 src/dispatch/CMakeLists.txt      |   2 +
 src/tui/CMakeLists.txt           |   2 +
 src/tui/sandwich_tui_backend.cpp | 546 ++++++++++++++++++++++++--------
 src/tui/sandwich_tui_backend.h   |  56 +++-
 src/tui/strata_tui.cpp           | 668 +++++++++++++++++++++++++++++++--------
 src/tui/strata_tui.h             |  50 ++-
 tests/test-sandwich-dispatch.cpp |  18 ++
 7 files changed, 1059 insertions(+), 283 deletions(-)
```

### 222. [`92bfe0cd3`] fix(gui): reset MoE counters on model change and fix context gauge
- **Full SHA:** `92bfe0cd39889c04dc20d81204fad58f96370bbf`
- **Date:** `2026-11-21T20:52:50+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 public/js/app.js       | 22 +++++++++++++++++++
 public/js/chat.js      | 12 ++++++++++
 public/js/dashboard.js | 59 +++++++++++++++++++++++++++++++++++++++++---------
 3 files changed, 83 insertions(+), 10 deletions(-)
```

### 223. [`4e9cc4a09`] feat(autotune): add persistent kernel variant cache with heuristic-safe override
- **Full SHA:** `4e9cc4a0990040a5cfbbe2fcb2d4604acc01272f`
- **Date:** `2026-11-21T21:43:57+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CHANGELOG.md                                 |   6 +
 ggml/src/ggml-cuda/fattn.cu                  |  63 ++++-
 ggml/src/ggml-cuda/sandwich-kernel-cache.cuh | 332 +++++++++++++++++++++++++++
 tests/CMakeLists.txt                         |   1 +
 tests/test-sandwich-autotune.cpp             | 231 +++++++++++++++++++
 5 files changed, 632 insertions(+), 1 deletion(-)
```

### 224. [`9894bcfbd`] feat(plugins): add runtime CUDA Driver API kernel loader
- **Full SHA:** `9894bcfbdc9c26d8b6ed2c06f1acf432a3319d6f`
- **Date:** `2026-11-21T21:48:57+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CHANGELOG.md                   |   5 +
 src/dispatch/CMakeLists.txt    |   2 +
 src/dispatch/plugin_loader.cpp | 498 +++++++++++++++++++++++++++++++++++++++++
 src/dispatch/plugin_loader.hpp | 126 +++++++++++
 tests/CMakeLists.txt           |   2 +
 tests/test-sandwich-plugin.cpp | 189 ++++++++++++++++
 6 files changed, 822 insertions(+)
```

### 225. [`e3686ef81`] feat(bench): max-performance configuration search for Qwen3.6-35B-A3B (build → ctest → validate_ledger → commit → push)
- **Full SHA:** `e3686ef811e20385a93024d18f98b8d4af111017`
- **Date:** `2026-11-21T22:39:09+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/audit/CONFIDENCE.md                          | 25 +++++++
 scripts/benchmarks/config_search/runner_report.md | 91 +++++++++++++++++++++++
 2 files changed, 116 insertions(+)
```

### 226. [`d0d2b75c1`] fix(bench): retract false drafter dead end and re-run config search on true C4
- **Full SHA:** `d0d2b75c12fa58cfb255ae26bc558f2d0fe89d7a`
- **Date:** `2026-11-22T13:12:52+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/audit/CONFIDENCE.md                           |  72 +-
 .../benchmarks/config_search/raw/auto_probe.json   |  87 ++
 .../benchmarks/config_search/raw/baseline_c4.json  | 244 ++++++
 scripts/benchmarks/config_search/raw/grid_n3.json  | 821 +++++++++++++++++++
 .../config_search/raw/grid_ngld99_n3.json          | 432 ++++++++++
 .../config_search/raw/ngld_control_n5.json         | 244 ++++++
 scripts/benchmarks/config_search/raw/ngld_n5.json  | 244 ++++++
 .../benchmarks/config_search/raw/sanity_c4_s8.json |  65 ++
 .../benchmarks/config_search/raw/stage4_n5.json    | 194 +++++
 .../benchmarks/config_search/raw/toggles_n3.json   | 888 +++++++++++++++++++++
 scripts/benchmarks/config_search/raw/top_n5.json   | 476 +++++++++++
 scripts/benchmarks/config_search/run_search.py     | 248 ++++++
 scripts/benchmarks/config_search/runner_report.md  | 278 +++++--
 .../benchmarks/config_search/spec_auto_probe.json  |  34 +
 .../benchmarks/config_search/spec_baseline.json    |  19 +
 scripts/benchmarks/config_search/spec_grid.json    | 280 +++++++
 .../benchmarks/config_search/spec_grid_ngld99.json | 112 +++
 scripts/benchmarks/config_search/spec_ngld.json    |  49 ++
 .../config_search/spec_ngld_control.json           |  49 ++
 scripts/benchmarks/config_search/spec_stage4.json  |  34 +
 .../benchmarks/config_search/spec_toggles_n3.json  | 246 ++++++
 scripts/benchmarks/config_search/spec_top_n5.json  |  91 +++
 22 files changed, 5105 insertions(+), 102 deletions(-)
```

### 227. [`86fa945af`] fix(dispatch): root-cause --sandwich-auto misaligned-address load crash, add dispatch dry-run
- **Full SHA:** `86fa945af834384e9ad1540c82344d1e7f2bb82c`
- **Date:** `2026-11-22T13:52:13+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp                | 92 +++++++++++++++++++++++++++++++++++++---
 src/tui/sandwich_tui_backend.cpp |  3 ++
 tests/test-sandwich-dispatch.cpp |  6 +++
 3 files changed, 94 insertions(+), 7 deletions(-)
```

### 228. [`176746cf6`] docs(bench): record -ngld 99 as recommended default with measured VRAM headroom
- **Full SHA:** `176746cf6f7a119a3c36f429a1819b730ddf89ab`
- **Date:** `2026-11-22T13:52:16+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 README.md                |  2 ++
 docs/audit/CONFIDENCE.md | 14 +++++++++++---
 2 files changed, 13 insertions(+), 3 deletions(-)
```

### 229. [`d93185356`] docs(audit): archive forensic reports, consolidate confidence mirror, and emit pruning manifest
- **Full SHA:** `d931853562248e15643d8ad30013c799bd8a4f77`
- **Date:** `2026-11-22T15:08:01+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/audit/CONFIDENCE.md                       |   4 +-
 docs/{ => audit/archive}/ui_overhaul_report.md |   0
 docs/audit/pruning_manifest.md                 | 107 +++++++++++++++++++++++++
 3 files changed, 110 insertions(+), 1 deletion(-)
```

### 230. [`9f57c8546`] perf(instrumentation): prune ad-hoc debug fprintfs from galloc, scheduler, and cli
- **Full SHA:** `9f57c8546333ec8df1bf62d6b06e4acd8632feb5`
- **Date:** `2026-11-22T15:12:46+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 ggml/src/ggml-alloc.c                           | 13 ------------
 ggml/src/ggml-backend.cpp                       | 28 ++-----------------------
 tests/diagnostics/generation_parity_report.json |  6 +++---
 tools/cli/cli.cpp                               | 14 -------------
 4 files changed, 5 insertions(+), 56 deletions(-)
```

### 231. [`20770d462`] refactor(bench): remove obsolete SANDWICH_LOG_FATTN_COUNT env knob from config search
- **Full SHA:** `20770d4621e8ff5b092860d96fd3233e2a98b778`
- **Date:** `2026-11-22T15:13:28+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 scripts/benchmarks/config_search/run_search.py | 1 -
 1 file changed, 1 deletion(-)
```

### 232. [`c75b62eda`] chore(scaffolding): relocate one-off benchmark scaffolding to audit evidence
- **Full SHA:** `c75b62eda9a90ff4e332e739bb90d99366259614`
- **Date:** `2026-11-22T15:14:21+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/audit/evidence/benchmarks/README.md                    | 13 +++++++++++++
 .../audit/evidence}/benchmarks/aggregate_summary.md         |  0
 .../evidence}/benchmarks/batch1/bench_auto_offload_59.py    |  0
 .../audit/evidence}/benchmarks/batch1/bench_cram_10.py      |  0
 .../audit/evidence}/benchmarks/batch1/bench_hwlock_58.cpp   |  0
 .../audit/evidence}/benchmarks/batch1/bench_hwlock_58.py    |  0
 .../audit/evidence}/benchmarks/batch1/bench_pcore_13.py     |  0
 .../evidence}/benchmarks/batch1/bench_telemetry_18.cpp      |  0
 .../audit/evidence}/benchmarks/batch1/bench_telemetry_18.py |  0
 .../audit/evidence}/benchmarks/batch1/bench_thp_25.cpp      |  0
 .../audit/evidence}/benchmarks/batch1/bench_thp_25.py       |  0
 .../evidence}/benchmarks/batch1/bench_unrotated_kv_60.py    |  0
 .../evidence}/benchmarks/batch1/result_auto_offload_59.json |  0
 .../audit/evidence}/benchmarks/batch1/result_cram_10.json   |  0
 .../audit/evidence}/benchmarks/batch1/result_hwlock_58.json |  0
 .../audit/evidence}/benchmarks/batch1/result_pcore_13.json  |  0
 .../evidence}/benchmarks/batch1/result_telemetry_18.json    |  0
 .../audit/evidence}/benchmarks/batch1/result_thp_25.json    |  0
 .../evidence}/benchmarks/batch1/result_unrotated_kv_60.json |  0
 .../evidence}/benchmarks/batch2/bench_cuda_graph_52.py      |  0
 .../benchmarks/batch2/bench_dynamic_promotion_19.cpp        |  0
 .../benchmarks/batch2/bench_dynamic_promotion_19.py         |  0
 .../evidence}/benchmarks/batch2/bench_pinned_dma_49.cpp     |  0
 .../evidence}/benchmarks/batch2/bench_pinned_dma_49.py      |  0
 .../benchmarks/batch2/bench_prefetch_scratchpad_53.cpp      |  0
 .../benchmarks/batch2/bench_prefetch_scratchpad_53.py       |  0
 .../evidence}/benchmarks/batch2/bench_sink_tokens_21.cpp    |  0
 .../evidence}/benchmarks/batch2/bench_sink_tokens_21.py     |  0
 .../evidence}/benchmarks/batch2/bench_slab_alloc_51.cpp     |  0
 .../evidence}/benchmarks/batch2/bench_slab_alloc_51.py      |  0
 .../evidence}/benchmarks/batch2/bench_spec_cascade_56.py    |  0
 .../evidence}/benchmarks/batch2/result_cuda_graph_52.json   |  0
 .../benchmarks/batch2/result_dynamic_promotion_19.json      |  0
 .../evidence}/benchmarks/batch2/result_pinned_dma_49.json   |  0
 .../benchmarks/batch2/result_prefetch_scratchpad_53.json    |  0
 .../evidence}/benchmarks/batch2/result_sink_tokens_21.json  |  0
 .../evidence}/benchmarks/batch2/result_slab_alloc_51.json   |  0
 .../evidence}/benchmarks/batch2/result_spec_cascade_56.json |  0
 .../audit/evidence}/benchmarks/batch3/bench_async_cpu_5.cpp |  0
 .../audit/evidence}/benchmarks/batch3/bench_async_cpu_5.py  |  0
 .../evidence}/benchmarks/batch3/bench_async_split_36.cpp    |  0
 .../evidence}/benchmarks/batch3/bench_async_split_36.py     |  0
 .../evidence}/benchmarks/batch3/bench_cuda_graph_hot_8.cu   |  0
 .../evidence}/benchmarks/batch3/bench_cuda_graph_hot_8.py   |  0
 .../benchmarks/batch3/bench_lookahead_router_7.cpp          |  0
 .../evidence}/benchmarks/batch3/bench_lookahead_router_7.py |  0
 .../audit/evidence}/benchmarks/batch3/bench_moe_2q_38.cpp   |  0
 .../audit/evidence}/benchmarks/batch3/bench_moe_2q_38.py    |  0
 .../evidence}/benchmarks/batch3/bench_mtp_rollout_43.py     |  0
 .../evidence}/benchmarks/batch3/bench_thecodacus_cache_2.py |  0
 .../evidence}/benchmarks/batch3/result_async_cpu_5.json     |  0
 .../evidence}/benchmarks/batch3/result_async_split_36.json  |  0
 .../benchmarks/batch3/result_cuda_graph_hot_8.json          |  0
 .../benchmarks/batch3/result_lookahead_router_7.json        |  0
 .../audit/evidence}/benchmarks/batch3/result_moe_2q_38.json |  0
 .../evidence}/benchmarks/batch3/result_mtp_rollout_43.json  |  0
 .../benchmarks/batch3/result_thecodacus_cache_2.json        |  0
 .../evidence}/benchmarks/batch4/bench_ds4_dequant_55.cpp    |  0
 .../evidence}/benchmarks/batch4/bench_ds4_dequant_55.py     |  0
 .../evidence}/benchmarks/batch4/bench_flash_mla_11_46.cpp   |  0
 .../evidence}/benchmarks/batch4/bench_flash_mla_11_46.py    |  0
 .../benchmarks/batch4/bench_lfm2_rollback_15_50.cpp         |  0
 .../benchmarks/batch4/bench_lfm2_rollback_15_50.py          |  0
 .../audit/evidence}/benchmarks/batch4/bench_nvfp4_22_32.cu  |  0
 .../audit/evidence}/benchmarks/batch4/bench_nvfp4_22_32.py  |  0
 .../evidence}/benchmarks/batch4/result_ds4_dequant_55.json  |  0
 .../evidence}/benchmarks/batch4/result_flash_mla_11_46.json |  0
 .../benchmarks/batch4/result_lfm2_rollback_15_50.json       |  0
 .../evidence}/benchmarks/batch4/result_nvfp4_22_32.json     |  0
 .../audit/evidence}/benchmarks/batch5/bench_dma_pinned_6.cu |  0
 .../audit/evidence}/benchmarks/batch5/bench_dma_pinned_6.py |  0
 .../evidence}/benchmarks/batch5/result_dma_pinned_6.json    |  0
 .../benchmarks/measure_fattn_crossover_results.json         |  0
 .../audit/evidence}/benchmarks/measure_fattn_step1.cpp      |  0
 .../audit/evidence}/benchmarks/measure_fattn_step1.py       |  0
 .../evidence}/benchmarks/measure_fattn_step1_results.json   |  0
 .../audit/evidence}/benchmarks/run_all_benchmarks.py        |  0
 77 files changed, 13 insertions(+)
```

### 233. [`ae337cd87`] perf(cuda): cache kernel cache enabled flag to eliminate hot-path getenv lookups
- **Full SHA:** `ae337cd87d4eaac2cdc04ade5c43f7b70605076b`
- **Date:** `2026-11-22T15:21:23+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 ggml/src/ggml-cuda/sandwich-kernel-cache.cuh    | 30 ++++++++++++++++++++++++-
 tests/diagnostics/generation_parity_report.json |  2 +-
 2 files changed, 30 insertions(+), 2 deletions(-)
```

### 234. [`4ddd89eb1`] docs(bench): record post-pruning n=5 interleaved benchmark telemetry
- **Full SHA:** `4ddd89eb1e519aee48294f5828c77929d89d1b36`
- **Date:** `2026-11-22T15:24:22+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 scripts/bench_n5_prune.py                     | 155 ++++++++++++++++++++++++++
 tests/diagnostics/bench_n5_prune_results.json |  57 ++++++++++
 2 files changed, 212 insertions(+)
```

### 235. [`dd2a6d300`] fix(telemetry): restore parity harness telemetry summary in llama-cli
- **Full SHA:** `dd2a6d3005fb076b1fa18f0b9120e56dfcea474e`
- **Date:** `2026-11-22T15:35:16+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tests/diagnostics/generation_parity_report.json |  6 +++---
 tools/cli/cli.cpp                               | 14 ++++++++++++++
 2 files changed, 17 insertions(+), 3 deletions(-)
```

### 236. [`3052eb8e2`] chore(scaffolding): restore ledger-cited benchmark families to original paths
- **Full SHA:** `3052eb8e2c4e87bafbe551bf738717ad4f81f006`
- **Date:** `2026-11-22T15:39:28+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 docs/audit/evidence/benchmarks/README.md           | 51 +++++++++++++++++-----
 .../benchmarks/batch1/bench_auto_offload_59.py     |  0
 .../benchmarks/batch1/bench_cram_10.py             |  0
 .../benchmarks/batch1/bench_hwlock_58.cpp          |  0
 .../benchmarks/batch1/bench_hwlock_58.py           |  0
 .../benchmarks/batch1/bench_pcore_13.py            |  0
 .../benchmarks/batch1/bench_telemetry_18.cpp       |  0
 .../benchmarks/batch1/bench_telemetry_18.py        |  0
 .../benchmarks/batch1/bench_thp_25.cpp             |  0
 .../benchmarks/batch1/bench_thp_25.py              |  0
 .../benchmarks/batch1/bench_unrotated_kv_60.py     |  0
 .../benchmarks/batch1/result_auto_offload_59.json  |  0
 .../benchmarks/batch1/result_cram_10.json          |  0
 .../benchmarks/batch1/result_hwlock_58.json        |  0
 .../benchmarks/batch1/result_pcore_13.json         |  0
 .../benchmarks/batch1/result_telemetry_18.json     |  0
 .../benchmarks/batch1/result_thp_25.json           |  0
 .../benchmarks/batch1/result_unrotated_kv_60.json  |  0
 .../batch2/bench_dynamic_promotion_19.cpp          |  0
 .../batch2/bench_dynamic_promotion_19.py           |  0
 .../benchmarks/batch2/bench_pinned_dma_49.cpp      |  0
 .../benchmarks/batch2/bench_pinned_dma_49.py       |  0
 .../batch2/bench_prefetch_scratchpad_53.cpp        |  0
 .../batch2/bench_prefetch_scratchpad_53.py         |  0
 .../benchmarks/batch2/bench_sink_tokens_21.cpp     |  0
 .../benchmarks/batch2/bench_sink_tokens_21.py      |  0
 .../benchmarks/batch2/bench_slab_alloc_51.cpp      |  0
 .../benchmarks/batch2/bench_slab_alloc_51.py       |  0
 .../benchmarks/batch2/bench_spec_cascade_56.py     |  0
 .../batch2/result_dynamic_promotion_19.json        |  0
 .../benchmarks/batch2/result_pinned_dma_49.json    |  0
 .../batch2/result_prefetch_scratchpad_53.json      |  0
 .../benchmarks/batch2/result_sink_tokens_21.json   |  0
 .../benchmarks/batch2/result_slab_alloc_51.json    |  0
 .../benchmarks/batch2/result_spec_cascade_56.json  |  0
 .../benchmarks/batch3/bench_async_cpu_5.cpp        |  0
 .../benchmarks/batch3/bench_async_cpu_5.py         |  0
 .../benchmarks/batch3/bench_async_split_36.cpp     |  0
 .../benchmarks/batch3/bench_async_split_36.py      |  0
 .../benchmarks/batch3/bench_cuda_graph_hot_8.cu    |  0
 .../benchmarks/batch3/bench_cuda_graph_hot_8.py    |  0
 .../benchmarks/batch3/bench_lookahead_router_7.cpp |  0
 .../benchmarks/batch3/bench_lookahead_router_7.py  |  0
 .../benchmarks/batch3/bench_moe_2q_38.cpp          |  0
 .../benchmarks/batch3/bench_moe_2q_38.py           |  0
 .../benchmarks/batch3/bench_mtp_rollout_43.py      |  0
 .../benchmarks/batch3/bench_thecodacus_cache_2.py  |  0
 .../benchmarks/batch3/result_async_cpu_5.json      |  0
 .../benchmarks/batch3/result_async_split_36.json   |  0
 .../benchmarks/batch3/result_cuda_graph_hot_8.json |  0
 .../batch3/result_lookahead_router_7.json          |  0
 .../benchmarks/batch3/result_moe_2q_38.json        |  0
 .../benchmarks/batch3/result_mtp_rollout_43.json   |  0
 .../batch3/result_thecodacus_cache_2.json          |  0
 .../benchmarks/batch4/bench_ds4_dequant_55.cpp     |  0
 .../benchmarks/batch4/bench_ds4_dequant_55.py      |  0
 .../benchmarks/batch4/bench_flash_mla_11_46.cpp    |  0
 .../benchmarks/batch4/bench_flash_mla_11_46.py     |  0
 .../batch4/bench_lfm2_rollback_15_50.cpp           |  0
 .../benchmarks/batch4/bench_lfm2_rollback_15_50.py |  0
 .../benchmarks/batch4/bench_nvfp4_22_32.cu         |  0
 .../benchmarks/batch4/bench_nvfp4_22_32.py         |  0
 .../benchmarks/batch4/result_ds4_dequant_55.json   |  0
 .../benchmarks/batch4/result_flash_mla_11_46.json  |  0
 .../batch4/result_lfm2_rollback_15_50.json         |  0
 .../benchmarks/batch4/result_nvfp4_22_32.json      |  0
 66 files changed, 41 insertions(+), 10 deletions(-)
```

### 237. [`311de7588`] feat(autotune): wire --autotune CLI to populate persistent kernel cache
- **Full SHA:** `311de758876134d61620d32c692a96551e65d948`
- **Date:** `2026-11-22T16:08:04+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 common/common.cpp                            |  21 ++
 common/common.h                              |   3 +
 ggml/src/ggml-cuda/sandwich-kernel-cache.cuh |  35 +++
 src/dispatch/CMakeLists.txt                  |   6 +
 src/dispatch/autotune.cpp                    | 317 +++++++++++++++++++++++++++
 src/dispatch/autotune.hpp                    |  29 +++
 src/dispatch/router.cpp                      |  57 ++++-
 src/dispatch/router.hpp                      |   3 +
 8 files changed, 470 insertions(+), 1 deletion(-)
```

### 238. [`7c422c4cd`] feat(cpu): wire VNNI-optimized IQ3_S plugin kernel via CUDA Driver API
- **Full SHA:** `7c422c4cd23bc1bfad83ebe03193a8d52cb7bf7d`
- **Date:** `2026-11-22T16:22:40+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 ggml/src/ggml-cpu/arch/x86/quants.c |  12 +++
 plugins/iq3_s_vnni.cu               | 154 ++++++++++++++++++++++++++++++++++
 plugins/iq3_s_vnni.cubin            | Bin 0 -> 17720 bytes
 src/dispatch/CMakeLists.txt         |   1 +
 src/dispatch/plugin_iq3_s.cpp       | 160 ++++++++++++++++++++++++++++++++++++
 src/dispatch/plugin_iq3_s.hpp       |  26 ++++++
 tests/test-sandwich-plugin.cpp      | 118 ++++++++++++++++++++++++++
 7 files changed, 471 insertions(+)
```

### 239. [`054bc7188`] docs(config): document -ngld 99 as recommended default with measured VRAM headroom
- **Full SHA:** `054bc71888c362df22b8d863adb1dcae81e4ed3f`
- **Date:** `2026-11-22T16:24:25+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CHANGELOG.md             | 11 +++++++++++
 README.md                | 22 ++++++++++++++++++++++
 docs/audit/CONFIDENCE.md | 11 ++++++++++-
 3 files changed, 43 insertions(+), 1 deletion(-)
```

### 240. [`bb187fd8f`] test(bench): restore committed IQ3_S/IQ2_S fused vec_dot benchmark
- **Full SHA:** `bb187fd8fa2c1eaae20dd1187301060ab2f98b57`
- **Date:** `2026-11-22T18:28:45+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tests/CMakeLists.txt                       |   2 +
 tests/benchmarks/bench_iq_dequant_gemm.cpp | 253 +++++++++++++++++++++++++++++
 2 files changed, 255 insertions(+)
```

### 241. [`672901250`] feat(cpu): support nrc>1 in IQ3_S/IQ2_S fused vec_dot with hoisted grid unpack
- **Full SHA:** `672901250204662b10b52b6742ab5a6630620209`
- **Date:** `2026-11-22T18:38:46+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 ggml/src/ggml-cpu/arch/x86/quants.c | 252 +++++++++++++++++++++++++++++++++++-
 ggml/src/ggml-cpu/quants.c          |  18 ++-
 tests/test-quantize-fns.cpp         |  67 ++++++++++
 3 files changed, 329 insertions(+), 8 deletions(-)
```

### 242. [`989f8b6a9`] perf(cpu): record nrc>1 IQ3_S/IQ2_S kernel amortization measurements
- **Full SHA:** `989f8b6a906b948cd17a49dc18480e38ea5acf96`
- **Date:** `2026-11-22T18:45:28+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 tests/benchmarks/bench_iq_dequant_gemm.cpp | 137 ++++++++++++++++++++++++++++-
 1 file changed, 136 insertions(+), 1 deletion(-)
```

### 243. [`ab022ec65`] perf(moe): group tokens by expert to exploit nrc>1 in mul_mat_id
- **Full SHA:** `ab022ec655e79d62132826ab5f13562025172ab5`
- **Date:** `2026-11-22T21:27:13+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 ggml/include/sandwich-telemetry.h               |   1 +
 ggml/src/ggml-cpu/ggml-cpu.c                    | 157 +++++++++++++++---
 ggml/src/ggml-cpu/tiled/tiled.cpp               |   4 +
 ggml/src/sandwich-telemetry.cpp                 |  47 ++++++
 scripts/bench_n5_nrc_c4.py                      | 209 ++++++++++++++++++++++++
 src/llama-context.cpp                           |   1 +
 tests/diagnostics/bench_n5_nrc_results.json     |  61 +++++++
 tests/diagnostics/generation_parity_report.json |   4 +-
 8 files changed, 459 insertions(+), 25 deletions(-)
```

### 244. [`ed16af597`] docs(perf): record nrc>1 amortization results and regime-specific gains
- **Full SHA:** `ed16af597a688808a993eecaddbd9f9136d77317`
- **Date:** `2026-11-22T21:32:39+05:30`
- **Author:** `sandeepmuttvarpu-png <sandeepmuttvarpu@gmail.com>`
- **Diffstat:**
```text
 CHANGELOG.md             | 11 +++++++++++
 docs/audit/CONFIDENCE.md | 47 +++++++++++++++++++++++++++++++++++++++++++++++
 2 files changed, 58 insertions(+)
```

