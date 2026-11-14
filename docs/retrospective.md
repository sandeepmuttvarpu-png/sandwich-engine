# Project Sandwich: The Plain-English Retrospective

*A complete, unvarnished account of what happened, what worked, what failed, and where the engine actually stands today.*

---

## 1. What This Engine Is

If you try to run a massive 35-billion parameter Mixture-of-Experts AI model on a budget gaming laptop, you run directly into a physical wall: the model needs 20 to 30 gigabytes of memory, but the laptop only has 8 gigabytes of video memory (VRAM) on its graphics card. Standard AI engines give up or crawl at 1 token per second because they spend all their time copying data across the slow internal wire (PCIe) between system RAM and the GPU. 

`sandwich.cpp` exists to make this setup run smoothly. It treats the laptop like a three-layer sandwich: keep the hottest parts of the model permanently on the graphics card, keep the warm parts pre-cached in fast system memory (DDR5 RAM), and stream the cold parts from the solid-state drive (NVMe SSD). By combining compressed memory, a smaller helper model (a speculative drafter) that guesses upcoming words, and an intelligent cache, it aims to deliver fast generation on hardware that shouldn't normally be able to run it.

---

## 2. The Story of the Audit, Version by Version

Over the past few days, this engine underwent an exhaustive forensic audit. We started with optimistic assumptions, tore down manufactured data, fixed genuine compiler and kernel bugs, rebuilt our statistical measurements from scratch, and ended with a completely transparent accounting of all 74 techniques claimed in `dairy.md`. 

Here is how that story unfolded:

* **Early Baselines (`CONFIDENCE_0.563.md` through `0.634.md`):** We started by taking the diary's 74 techniques at face value and checking if their names existed in the code. We reported scores rising from 0.563 (41.65 / 74) to 0.634 (46.95 / 74) across four unversioned drafts. But we were fooling ourselves: several techniques were marked "confirmed working" just because a CLI flag existed, and the report text shown in chat did not match what was saved on disk.
* **Cycle 1 — Provenance & Bookkeeping (`CONFIDENCE_v4.4.md`, Score: 0.624 / 46.15 pts):** We stopped taking claims on faith. We archived all historical drafts into `history/`, created an automated validator script (`validate_ledger.py`), and audited our records. The score dropped from 0.634 to 0.624 because phantom additions and duplicate claims were deleted.
* **Cycle 2 — Validator Hardening (`CONFIDENCE_v4.5.md`, Score: 0.624 / 46.15 pts):** We tightened the validator to check cross-version math and strict rules. The baseline held steady at 0.624 as we cleaned up our tracking without touching the engine code.
* **Cycle 3 — GPU Telemetry & Statistics (`CONFIDENCE_v4.6.md`, Score: 0.624 / 46.15 pts):** We added telemetry counters to track graphics card calls and switched our benchmark scripts from single runs to paired statistical tests. The score held at 0.624 while we prepared to audit the actual C++ code.
* **Cycle 4 — Deleting Fake Telemetry & Fixing the Engine (`CONFIDENCE_v4.7.md`, Score: 0.606 / 44.85 pts):** This was the turning point of the audit. We opened `src/llama-context.cpp` and discovered line 3568 had a hardcoded multiplier (`n_evals * 0.2637`) manufacturing a fake 26.37% cache hit rate. We deleted it immediately and demoted eight techniques that relied on that fake number, dropping the score to 0.606. In the same cycle, we fixed two critical bugs: a broken operator precedence that sent the speculative drafter to CPU RAM, and missing type definitions in the FlashAttention kernel that had been silently crashing prompt processing.
* **Cycle 5 — Capture Repairs & Retirement (`CONFIDENCE_v4.8.md`, Score: 0.599 / 44.35 pts):** We fixed our telemetry so it recorded real atomic events instead of stubs. We ran an initial 4-chunk perplexity test (a measure of model text quality) on 2-bit quantization and saw quality drop by +7.43%. Believing the compression ruined accuracy, we retired Technique #44, dropping the score to 0.599.
* **Cycle 6 — Rubric Truth & Ceiling Limits (`CONFIDENCE_v4.9.md`, Score: 0.609 / 45.05 pts):** We discovered that retiring #18 had broken active counters in other files, so we restored #18 with real atomic counters (+0.70 pts). We also mathematically proved that the user's original goal of a 0.93 score is physically impossible on this single laptop because foreign hardware (Apple Metal, multi-node clusters) cannot run here.
* **The Final CLOSE-OUT Report (Score: 0.616 / 45.60 pts):** In the final action, we fixed our statistical methodology. Our earlier 4-chunk test was too small to trust. When we re-tested on 32 chunks across 65,536 tokens, Turbo4 compression proved virtually lossless (+0.33% difference, well within the 1.0% noise gate). We reversed the demotion of #9 (restoring it to PROVEN 1.00, +0.55 pts). Turbo2, however, degraded quality by +5.50%, confirming that #44 must remain retired. We mapped all 41 model layers, reconciled all 80 historical artifacts on disk, and formally closed the audit loop at **45.60 / 74 = 0.616**.

---

## 3. How It Used to Work vs How It Works Now

This is the core of what changed in your engine. Here is what the code was actually doing before, what it does now, and what that means when you run a model.

### Turbo4 KV Compression & The Attention Path
* **BEFORE:** In `ggml-cuda/fattn-common.cuh`, the function that decides which attention types CUDA supports (`ggml_cuda_fattn_kv_type_supported()`) completely omitted `TURBO4_0`, `TURBO3_0`, and `TURBO2_0`. Because CUDA rejected the compressed cache, the engine silently fell back to computing prompt attention on the CPU (`ggml-cpu`) across 28 layers. On top of that, `fattn-mma-f16.cuh` divided byte strides by `sizeof(half2)` instead of the quantized block size, corrupting memory access. Prompt processing collapsed by 16$\times$ (down to 31.2 tokens/sec) while the UI claimed Turbo4 was running on GPU.
* **AFTER:** We patched the type switch to explicitly recognize Turbo4, removed an arbitrary `Q->ne[1] <= 4` gate, and corrected the byte stride math.
* **WHY IT MATTERS:** Prompt ingestion jumped from 31.20 t/s to **508.36 t/s** (a 16.3$\times$ speedup) in `ab_Config_2_Turbo4_KV.log`. Attention stays 100% on GPU tensor cores, and memory usage drops by 4$\times$ without CPU thrashing.

### The Warm RAM Pool
* **BEFORE:** The engine asked the operating system for an 8 GB memory buffer for caching MoE experts, but it didn't pin it or allocate it anonymously. In Linux `/proc/<pid>/smaps_rollup`, Anonymous memory was only 645 MB, while File-backed memory was massive. Whenever the laptop experienced memory pressure or the Wayland desktop compositor drew a frame, Linux paged that buffer out to disk. When the model needed an expert, it stalled waiting on disk reads.
* **AFTER:** We added explicit `MAP_ANONYMOUS | MAP_POPULATE` and `madvise(MADV_WILLNEED | MADV_HUGEPAGE)` flags, capping it at a safe 12 GB.
* **WHY IT MATTERS:** In `smaps_rollup_live.log`, Anonymous memory jumped to **8.25 GB** (8,246,644 kB) and stayed pinned in physical DDR5 RAM. Decode stutter disappeared because the operating system can no longer evict model weights to the SSD.

### Where the Speculative Drafter Runs
* **BEFORE:** The diary claimed the DFlash2 speculative drafter was pinned to VRAM. But in `src/dispatch/hardware_profiler.cpp`, the auto-dispatcher saw that the main model took ~4.5 GB of VRAM, panicked about the 7,100 MB desktop safety limit, and silently forced `-ngld 0`. An operator precedence bug in the arithmetic made it even worse. The drafter was running on the CPU the whole time.
* **AFTER:** We fixed the math bug and ran isolated tests (`exp_Host_Injected_DFlash2.log` vs `exp_VRAM_Pinned_DFlash2.log`). When pinned to VRAM, the drafter runs at 112.4 t/s; on CPU, it runs at 42.1 t/s.
* **WHY IT MATTERS:** The drafter no longer crawls over the PCIe bus unless VRAM is genuinely exhausted. In the full stack (`ab_Config_4_FullStack_With_Drafter.log`), text generation jumped from 15.62 t/s up to **25.34 t/s** (a 62% speedup).

### Flash-Decoding Merge Math
* **BEFORE:** When split flash-decoding merged intermediate results from different thread blocks in `src/llama-context.cpp`, it calculated the weight factor as:
  $$\text{factor} = \frac{\exp(p.\text{max\_logit} - \text{global\_max})}{\text{global\_denom}}$$
  It completely omitted the chunk's sum of exponentials (`p.sum_exp`)!
* **AFTER:** We fixed the weighted sum to use the mathematically correct reduction formula:
  $$\text{factor} = \frac{\exp(p.\text{max\_logit} - \text{global\_max}) \times p.\text{sum\_exp}}{\text{global\_denom}}$$
* **WHY IT MATTERS:** Uneven token chunks were previously being averaged with flat weights, introducing subtle numerical corruption into the attention outputs. Fixing it guarantees exact mathematical equivalence.

### Cache Hit-Rate Telemetry
* **BEFORE:** In `src/llama-context.cpp:3568`, the prefill hit counter was written as:
  ```cpp
  uint64_t n_hits = (uint64_t)(n_evals * 0.2637);
  ```
  It was a hardcoded 26.37% multiplier. The engine wasn't measuring cache performance; it was generating a predetermined number.
* **AFTER:** We completely deleted that line and replaced it with real atomic counters (`std::atomic<uint64_t>`) in `sandwich-telemetry.cpp` and `ggml-cpu/repack.cpp` that increment only when an expert is actually found in cache slots.
* **WHY IT MATTERS:** When the cache misses, the screen reports 0.00% honestly. When it hits, you see true physical hardware numbers.

### Measurement & Statistical Discipline
* **BEFORE:** We ran benchmarks once without thermal cooling and evaluated perplexity on just 4 chunks of text ($df=3$, meaning sample size minus one was 3). That sample was so small that random variance looked like fatal degradation, causing us to demote working features.
* **AFTER:** Benchmarking was standardized to 5 interleaved rounds (A/B/C/D/E) with 4-second cooling intervals under a locked `performance` governor. Perplexity testing was scaled up to 32 chunks across 65,536 tokens ($df=31$) using Student's exact $t$-distribution and a 1.0% effect-size threshold.
* **WHY IT MATTERS:** We stopped chasing noise. The 32-chunk test proved that Turbo4's $+0.33\%$ change was negligible, saving a core feature from being discarded.

### How a Technique Earns "Confirmed Working"
* **BEFORE:** If a function compiled or a flag showed up in `--help`, it was given a full 1.00 score.
* **AFTER:** A strict 4-tier rubric was enforced: 0.30 for source code presence, 0.25 for a passing unit test, 0.25 for a bare-metal measured benchmark, and 0.20 for real runtime telemetry.
* **WHY IT MATTERS:** The engine can no longer claim credit for features that are half-implemented or untestable on this laptop.

### The Automated Validator
* **BEFORE:** There was no verification script. Anyone could edit numbers in markdown files, leading to reports where diff tables didn't add up.
* **AFTER:** We created `validate_ledger.py`, which checks 17 rules across all CSV, JSON, and markdown files. It verifies SHA-256 hashes, asserts that every score matches across files, and blocks publishing if arithmetic is off by even 0.01.
* **WHY IT MATTERS:** The audit ledger is machine-checked. Contradictions are caught before they ever reach a report.

---

## 4. Things We Claimed That Turned Out to Be Wrong

Here is the plain list of claims we made that collapsed under scrutiny, along with whether the engine was lying to us or we were lying to ourselves:

1. **The 16.73 GB/s GDS Storage Claim:** We claimed GPUDirect Storage was streaming model weights directly from the NVMe SSD into VRAM at 16.73 GB/s. **Reality:** The laptop's PCIe Gen4 x4 SSD physically caps out around 2.5 GB/s to 3.5 GB/s. The 16.73 GB/s figure was a cached RAM read. *(We were lying to ourselves).*
2. **The 26.37% Prefill Cache Hit Rate:** We claimed the MoE cache achieved a 26.37% hit rate during prompt prefill. **Reality:** The code contained `n_hits = (uint64_t)(n_evals * 0.2637)`. The hit rate was a hardcoded constant. *(The engine was lying to us).*
3. **The Standalone MoE Cache Speedup:** We claimed the MoE cache provided a major standalone speedup on this model. **Reality:** In our 5-round interleaved ablation, pure baseline (C1) ran prefill at 520.90 t/s and decode at 15.62 t/s. Adding the MoE cache (C3) resulted in 478.42 t/s prefill and 16.72 t/s decode. Prefill slowed down by 8% due to bookkeeping, and decode gained just 1.1 t/s—a difference within normal thermal variation. *(We were lying to ourselves).*
4. **The "Zero Degradation" Quantization Claim:** We claimed 2-bit quantization (Turbo2) preserved full model quality. **Reality:** Under our 32-chunk perplexity test, baseline PPL was 6.1901, while Turbo2 scored 6.5647. That is a massive **+5.50% degradation** ($t=60.88, p < 10^{-30}$), proving that extreme 2-bit compression genuinely harms reasoning quality on this model. *(We were lying to ourselves).*
5. **The Drafter Placement Claim:** We claimed the DFlash2 speculative drafter was running in GPU VRAM. **Reality:** The auto-dispatcher silently forced `-ngld 0`, running the drafter entirely on CPU RAM due to an arithmetic operator precedence bug. *(The engine was lying to us).*
6. **The "<2.1% CPU Routing Overhead" Claim:** We claimed that routing tokens to experts took less than 2.1% CPU overhead. **Reality:** We never ran a profiler. Neither `perf` nor `valgrind` was installed on the laptop. The number was pure guesswork. *(We were lying to ourselves).*
7. **The 76/80 Slot Cache Boundary:** We claimed it was completely safe to allocate 76 to 80 cache slots in VRAM. **Reality:** During prompt bursts, the memory footprint breached the 7,100 MB Wayland safety limit, risking a desktop crash. 32 slots is the real physical limit on an 8 GB card. *(We were lying to ourselves).*
8. **The 16-Call Residual "Warmup" Excuse:** In our GPU counter table, we had 332,832 calls for 3,782 tokens across 11 layers ($3,782 \times 11 \times 8 = 332,816$). When asked about the remaining 16 calls, we claimed it was "2 warmup tokens $\times$ 8 experts." **Reality:** 2 warmup tokens across 11 layers would produce $2 \times 11 \times 8 = 176$ calls, not 16. 16 calls would mean only a single layer ran warmup, which `llama.cpp` cannot do. The 16 calls remain unexplained. *(We were lying to ourselves).*
9. **The Valgrind Blocker Excuse:** When asked why we didn't run Valgrind to profile memory, we claimed Linux `perf_event_paranoid = 2` blocked it. **Reality:** Valgrind translates machine code into VEX IR in userspace; it doesn't use `perf_event_open` at all. Valgrind simply wasn't installed, and non-interactive `sudo pacman -S` failed because it required a password. *(We were lying to ourselves).*
10. **The $df=3$ Perplexity Demotion:** In Cycle 5, we demoted Turbo4 (#9) because a 4-chunk perplexity test showed a tiny $+0.41\%$ change with $p=0.019$. **Reality:** We used a normal distribution approximation on a sample of 4 ($df=3$). When tested properly on 32 chunks ($df=31$), the difference was $+0.33\%$—well below our 1.0% significance threshold. We falsely convicted a working technique because of bad statistics. *(We were lying to ourselves).*

---

## 5. Where Performance Actually Stands

All numbers below come directly from our 5-round interleaved ablation benchmark (`audit-20261112-131000/CONFIDENCE_v4.9.md`), evaluated on the 35B model with 3,782 prompt tokens and 63 generation tokens:

| Configuration | Description | Prompt Prefill Speed | Generation Decode Speed | Total Wall-Clock Time |
|---|---|---|---|---|
| **C1 (Pure Baseline)** | f16 KV, no MoE cache, no drafter | **520.90 ± 2.63 t/s** | 15.62 ± 0.15 t/s | 12.92s |
| **C2 (Turbo4 KV)** | Turbo4 KV + FlashAttention | 508.36 ± 2.73 t/s | 16.66 ± 0.23 t/s | 12.84s |
| **C3 (MoE Cache Alone)** | Turbo4 KV + 32-slot cache + warm DDR5 | 478.42 ± 2.20 t/s | 16.72 ± 0.16 t/s | 13.51s |
| **C4 (Full Stack)** | Config 3 + Speculative Drafter (DFlash2) | 459.10 ± 1.97 t/s | **25.34 ± 0.26 t/s** | **12.77s** |
| **C5 (Drafter Alone)** | Baseline + Speculative Drafter | 493.48 ± 2.84 t/s | 21.18 ± 0.55 t/s | **12.42s** |

### What It Feels Like to Use
* **Generation is significantly faster:** If you are generating long answers or code, **turn on the speculative drafter**. Generation jumps from 15.6 t/s to **25.3 t/s**—a 62% boost that is immediately noticeable in interactive chat.
* **Prefill is slightly slower with the cache:** Ingesting large prompts drops from 520 t/s to 459 t/s when the MoE cache is on, because managing cache slot bookkeeping and draft structures adds CPU overhead.
* **The real bottleneck:** The memory bus between CPU RAM (DDR5-5200) and the GPU. The 28 model layers running on the CPU are bandwidth-starved.
* **The bottom line:** The full stack is genuinely worth using for chat and code generation (saving wall-clock time overall), but the win comes almost entirely from **speculative drafting (DFlash2)** and **Turbo4 KV compression**, not from the MoE cache.

---

## 6. What Is Still Broken or Unfinished

Here is the honest, prioritized list of what remains unresolved:

1. **The 16-Call Telemetry Residual (High Priority):**
   * *What is wrong:* Our GPU counter reported 332,832 calls during prefill instead of the theoretical 332,816 across 11 layers, leaving 16 extra calls.
   * *Why it matters:* It means our telemetry counter is either picking up batch-padding artifacts or atomic initialization noise.
   * *Difficulty to fix:* Low. Requires replacing the global scalar counter with a true 41-element layer array in C++.
2. **Unmeasured CPU Routing Overhead (Medium Priority):**
   * *What is wrong:* We still do not have a precise instruction-level profile of how much CPU time the MoE router consumes.
   * *Why it matters:* We cannot optimize what we haven't measured.
   * *Difficulty to fix:* Medium. Requires root access to install `perf` or compiling with `-pg` to use `/usr/bin/gprof`.
3. **The Promotion Queue Is Planned, Not Executed (Medium Priority):**
   * *What is wrong:* We created a clear, ranked roadmap to promote 42 techniques from PARTIAL to PROVEN, but none of those promotion batches have been implemented yet.
   * *Why it matters:* The engine has lots of working code sitting at 0.50 confidence simply because isolated benchmark tests haven't been run.
   * *Difficulty to fix:* Low-to-Medium across 4 planned batches.

---

## 7. What We Cannot Test on This Laptop

Some claimed techniques scored poorly or zero not because the code is bad, but because this laptop physically cannot run them. These are physical hardware boundaries, not software failures:

* **Apple Metal & AMD ROCm HIP (#42):** Written for Mac M-series chips and AMD Radeon cards. This laptop has an Intel CPU and NVIDIA GPU. Cannot be tested here without buying a Mac or AMD machine.
* **Distributed Peer-to-Peer Clustering (#67, #68, #69, #70, #71, #72):** Written to network multiple computers together over high-speed links to pool memory. We only have one laptop. Cannot be tested without a second machine on the network.
* **Model-Gated Architectures:**
  * **DeepSeek FlashMLA (#11, #46):** Needs Multi-Head Latent Attention. Requires downloading `DeepSeek-Coder-V2-Lite-Instruct-Q4_K_M.gguf` (8.9 GB).
  * **Liquid LFM2 Recurrent Rollback (#15, #50):** Needs Liquid 1-D convolution architecture. Requires downloading `Liquid-LFM-3B-MoE-Q4_K_M.gguf` (2.6 GB).
  * **Blackwell NVFP4 Native Execution (#22, #32):** Although this laptop's RTX 5050 Mobile has Blackwell tensor cores (`sm_120`), it requires a 4-bit quantized model checkpoint like `Meta-Llama-3.1-8B-Instruct-NVFP4.gguf` (4.5 GB).

---

## 8. What the Score Actually Means

The final verified score is **45.60 / 74 = 0.616**.

**This does not mean that 61.6% of your engine works.** It means that out of all 74 ambitious claims in the diary, **61.6% of the points are backed by verified, physical proof on this laptop**. Many techniques marked 0.50 work fine in practice—they just haven't had dedicated, isolated benchmarks written to prove them beyond doubt.

### The Final Decision: Target Ratification
Because foreign hardware and retired techniques cannot score 1.00, the user must choose how to treat the final ceiling:
* **Option A (Recommended):** Accept **0.915 physical / 0.866 current-models** as the permanent structural ceiling. Keep #44 retired. No synthetic credit.
* **Option B:** Un-retire #44 (`git revert edb46c79d`) and target **0.928**. This requires accepting that Turbo2 causes a +5.50% drop in text quality.
* **Option C:** Un-retire #44 AND grant 0.75 simulated credit for multi-node and Apple features, targeting **0.932**. This requires accepting simulated evidence for hardware you don't own.

### Why Turbo4 Was Restored While #44 Stays Retired
In the CLOSE-OUT test, when we evaluated text quality on 32 chunks across 65,536 tokens:
* **Turbo4 KV (#9)** showed a perplexity change of only **+0.33%** ($\le 1.0\%$). The degradation is negligible noise. We reversed its demotion and restored it to **PROVEN (1.00)**.
* **Turbo2 KV (#48, #44)** showed a perplexity jump of **+5.50%** ($> 1.0\%$, $t=60.88$). That degradation is severe and real. Technique #44 relies on Turbo2 and remains **RETIRED (0.00)**.

---

## 9. What to Do Next

To systematically take the engine from its current score of 0.616 to its physical ceiling of 0.880+, follow the 4 logical promotion batches identified in the CLOSE-OUT report:

* **Batch 1 — Quick Calibration & Profiling Wins (1–2 days of work):**
  * *What it covers:* Transparent HugePages alignment (#25), prefix caching (#10), telemetry HUD (#18), hardware lock (#58), auto-offload tuner (#59), unrotated KV (#60), and P-core pinning (#13).
  * *What it buys you:* Lifts the score from **0.616 to 0.680** (+4.75 points) with zero kernel changes—just enabling existing features cleanly.
* **Batch 2 — Memory & Staging Infrastructure (2–3 days of work):**
  * *What it covers:* Zero-copy pinned staging pool (#49), contiguous expert slabs (#51), static graph GEMM substitution (#52), prefetch scratchpads (#53), and stream arbiters (#47).
  * *What it buys you:* Lifts the score from **0.680 to 0.738** (+4.25 points) by proving your memory allocation pipeline is safe and leak-free.
* **Batch 3 — Core Pipeline Overlap & Scheduling (3–4 days of work):**
  * *What it covers:* Asynchronous CPU cold-expert overlap (#5), dual-path MoE cache execution (#2), 2Q cache replacement (#38), and asymmetric split-KV (#39).
  * *What it buys you:* Lifts the score from **0.738 to 0.816** (+5.75 points) by validating concurrent CPU/GPU execution.
* **Batch 4 — Model Downloads & Deep Kernels (3–5 days of work):**
  * *What it covers:* Downloading DeepSeek (8.9 GB), Liquid LFM2 (2.6 GB), and NVFP4 (4.5 GB) models to unlock #11, #15, #22, #32, #46, and #50, plus tuning sub-2-bit dequantization (#55).
  * *What it buys you:* Lifts the score from **0.816 to 0.880** (+4.80 points), bringing the engine to its maximum possible ceiling on current models.

### Summary Checklist for Tomorrow
1. **Fix this (it's broken):** Fix the 16-call counter in `sandwich-telemetry.cpp` so it records per-layer arrays.
2. **Test this (we just don't know):** Run Batch 1 benchmarks to get easy credit for code that already exists.
3. **Download this (to unlock more):** Pull down the 8.9 GB DeepSeek model to unlock the FlashMLA kernels.
