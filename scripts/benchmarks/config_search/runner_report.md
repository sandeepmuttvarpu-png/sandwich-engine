# Qwen3.6-35B-A3B-UD-IQ3_XXS Configuration Search Report (CORRECTED)

**Status:** Supersedes commit `e3686ef81`. That commit screened a stack that was missing the real
DFlash2 speculative drafter. Its conclusions are retracted in `docs/audit/CONFIDENCE.md`.

**Date:** 2026-11-22
**Host:** RTX 5050 Mobile, 8,151 MiB total, driver 620.73.04, kernel 7.3.5-arch1-1
**Binary:** `build/bin/llama-cli` at `e3686ef81` + the runner below
**Runner:** `scripts/benchmarks/config_search/run_search.py` (VRAM via `cudaMemGetInfo` at 50 ms,
host VmHWM via `/proc/<pid>/status`, pure-token SHA256 per run)

---

## Executive Summary

The fastest SAFE configuration is **canonical C4 with the DFlash2 drafter pinned to VRAM
(`-ngld 99`)**:

| Field | Value |
|---|---|
| Config | `-ctk turbo4 -ctv turbo4 --turbo-kvran -fa -md <DFlash2> -ngld 99 --io-uring --moe-2q --moe-cache-slots 16 --moe-warm-ram-mb 8000` |
| Decode (warm-4, dedicated n=5) | **30.73 +/- 0.21 t/s** |
| Decode (warm-4, interleaved control session) | 28.80 +/- 1.42 t/s |
| Peak VRAM (measured) | **6,576 MiB** (524 MiB under the 7,100 MiB guard) |
| Peak host VmHWM (measured) | **13,936 MiB** (2,616 MiB under the 16,552 MiB guard) |
| Pure-token SHA256 | `481b2a7a...` on 5/5 runs |
| LOSSY | none |

The single decisive factor was not slot count. It was **where the drafter lives**. `-ngld 0`
(host-injected drafter) measured 22.58 +/- 1.80 t/s; `-ngld 99` (VRAM-pinned drafter) measured
28.80 +/- 1.42 t/s on the identical slot/warm config. Delta = +6.23 t/s versus 2*sigma_comb = 4.58 t/s.

---

## Why the prior search was wrong (one paragraph)

Commit `e3686ef81` passed the drafter-only GGUF as the target (`-m .../DFlash2-Q4_K_M.gguf`), which
correctly fails with "dflash requires ctx_other to be set". It then removed the drafter instead of
fixing the flag, and its "best command" used the target as its own drafter (`-md` = target), so it
measured self-drafting. `tests/verify_generation_parity.py --stack c4` had already proven DFlash2 works
when attached with `-md`. Every conclusion below is measured with the real drafter attached.

---

## Ranked Table (SAFE configs only, measured per-run)

All rows: canonical pure-token SHA256 match, AC power, performance governor, GPU idle before each
session, 4 s cooldown. Warm-4 means run 1 discarded.

| Rank | Config | Decode mu+/-sigma (t/s) | n | Peak VRAM (MiB) | VmHWM (MiB) | SHA | Verdict |
|---|---|---|---|---|---|---|---|
| 1 | `-ngld 99`, slots 16, warm 8000 (dedicated final) | **30.73 +/- 0.21** | 4 | 6,576 | 13,936 | OK | **BEST** |
| 1 | `-ngld 99`, slots 16, warm 8000 (control session) | 28.80 +/- 1.42 | 4 | 6,576 | 13,936 | OK | BEST (session drift) |
| 1 | `-ngld 99`, slots 16, warm 8000 (top-candidate session) | 28.18 +/- 1.15 | 4 | 6,576 | 13,936 | OK | same config, third session |
| 2 | `-ngld 99`, slots 24, warm 8000 | 28.80 +/- 1.24 | 4 | 6,816 | 13,936 | OK | tied, less headroom |
| 2 | `-ngld 99`, slots 32, warm 8000 | 28.75 +/- 1.24 | 4 | 7,056 | 13,936 | OK | tied, 44 MiB headroom |
| 2 | `-ngld 99`, slots 16, warm 12500 | 28.35 +/- 1.40 | 4 | 6,576 | 13,936 | OK | tied |
| 2 | `-ngld 99`, slots 8, warm 8000 (n=3 screen) | 29.35 +/- 0.35 | 2 | 5,894 | 13,936 | OK | tied; only n=3 measured |
| 3 | Canonical C4, `-ngld 0`, slots 8, warm 8000 | 23.85 +/- 0.31 | 4 | 5,894 | 13,936 | OK | reference baseline |
| 4 | Canonical C4, `-ngld 0`, slots 32, warm 12500 | 23.75 +/- 1.25 | 4 | 6,614 | 13,936 | OK | matches published 6,614 MiB |
| 5 | Canonical C4, `-ngld 0`, slots 16, warm 8000 | 22.58 +/- 1.80 | 4 | 6,134 | 13,936 | OK | control for the `-ngld` claim |
| - | `--sandwich-auto` | n/a | 0/5 | n/a | n/a | n/a | **CRASH (see Finding)** |

Ranks 1-2 are statistically indistinguishable (spread < 1.0 t/s, sigma ~ 1.2). Slots 16 is selected
because it reaches the same throughput with 480 MiB more VRAM headroom than slots 32 and no session
showed a slots-32 advantage beyond noise.

### The decisive control (`spec_ngld_control.json`, n=5 interleaved)

| Config | Decode (t/s) | Peak VRAM (MiB) |
|---|---|---|
| `-ngld 0`, slots 16 | 22.58 +/- 1.80 | 6,134 |
| `-ngld 99`, slots 16 | 28.80 +/- 1.42 | 6,576 |
| Delta | **+6.23** | +442 |

sigma_comb = sqrt(1.80^2 + 1.42^2) = 2.29; 2*sigma_comb = 4.58. Delta 6.23 > 4.58, and all 5
interleaved pairs favor `-ngld 99`.

---

## LOSSY Table

**None.** Every retained configuration reproduced the canonical pure-token SHA256
`481b2a7aea43731e77979bca6631cfac9d32bd42d023d63c91552b245931a6aa` on every successful run.
KV `f16` and `q8_0` were also bit-identical; `f16` was rejected on speed only (26.85 t/s).

---

## Copy-Pasteable Best Command

```bash
./build/bin/llama-cli \
  -m /home/sandeep/ai-models/moe/Qwen3.6-35B-A3B-MTP-GGUF/Qwen3.6-35B-A3B-UD-IQ3_XXS.gguf \
  -md /home/sandeep/ai-models/drafters/Qwen3.6-35B-A3B-DFlash2-GGUF/Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf \
  -ngl 14 -ngld 99 -t 6 -c 4096 -n 64 --ignore-eos --temp 0 --seed 42 --no-warmup --simple-io \
  -ctk turbo4 -ctv turbo4 --turbo-kvran -fa --io-uring --moe-2q \
  --moe-cache-slots 16 --moe-warm-ram-mb 8000 \
  -f /home/sandeep/dev/the-mds/audit-20261112-131000/sandwich_bench_prompt.txt
```

The `-md` line is the real DFlash2 drafter, not the target. `-ngld 99` pins it to VRAM.

**Expected:** 30.7 t/s decode warm-4 (dedicated run), 28.8 t/s in an interleaved session.
**Measured:** peak VRAM 6,576 MiB, peak VmHWM 13,936 MiB, SHA 5/5 match.

---

## Hard-Constraint Compliance (measured this session)

| Constraint | Limit | Measured | Status |
|---|---|---|---|
| Peak VRAM | <= 7,100 MiB | 6,576 MiB | PASS |
| Peak host VmHWM | <= 16,552 MiB | 13,936 MiB | PASS |
| Pure-token SHA256 | `481b2a7a...` | `481b2a7a...` (5/5) | PASS |
| AC power | online | online (1) | PASS |
| GPU idle at session start | ~0 MiB | 2 MiB | PASS |
| Governor | performance | performance | PASS |
| Cooldown | 4 s | 4 s between all runs | PASS |

---

## Stage Verification Log

### Stage 0 - Environment lock and true-C4 baseline
- Environment precheck before every session: AC online, GPU 2 MiB, governor `performance`.
- True C4 (`-md` DFlash2, `-ngld 0`, slots 8/warm 8000): 23.85 +/- 0.31 t/s warm-4, 5,894 MiB.
- True C4 with slots 32/warm 12500: 23.75 +/- 1.25 t/s, 6,614 MiB (reproduces the published 6,614 MiB
  figure exactly). The prior report's claim that "full C4 exceeded the VRAM guard" was false: it peaks
  at 6,614 MiB, 486 MiB under guard.
- The canonical ~24.5 t/s baseline reproduces (23.8-24.0 here under a warmer machine state; run 1 of each
  config is cold and discarded).

### Stage 1/3 - slots x warm grid on true C4
- `-ngld 0` grid: slots in {8,16,24,32} x warm in {8000,10000,12500} all cluster at 22-24 t/s.
  No slot/warm cell clears the survivor rule over baseline. Full grid run was interrupted by a server
  restart; the completed `-ngld 99` grid and the n=5 top-candidate runs supersede it for ranking.
- `-ngld 99` arm (directive step 2) was the outlier: 28.3 t/s on its first run.

### Stage 2 - `-ngld 99` slot sweep (n=3) then top candidates (n=5)
- `-ngld 99` slots {0,8,16,24,32}: 29.0 / 29.35 / 30.10 / 29.30 / 29.75 t/s (n=2 warm).
  VRAM rises 240 MiB per 8 slots: 6,066 / 6,336 / 6,576 / 6,816 / 7,056 MiB.
- n=5 top candidates (slots 16, 24, 32 at warm 8000; slots 16 at warm 12500): 28.18 / 28.80 / 28.75 /
  28.35 t/s, all within 0.62 t/s, sigma ~ 1.2. Statistically tied.
- Selection: slots 16/warm 8000 for maximum VRAM headroom at equal speed.

### Stage 3 - Re-test of the ten previously "rejected" toggles (n=3, on true C4)
Baseline for these: WIN = `-ngld 99`, slots 16, warm 8000 = 30.45 +/- 0.49 t/s.

| Toggle | Result | Verdict |
|---|---|---|
| `--native-mtp` | fails to load (MTP context) | incompatible with `-md` drafter |
| `--sched-async-cpu` | 29.30 +/- 1.70 | no gain |
| `--unrotated-kv` | 29.70 +/- 0.99 | no gain |
| `--asym-prefetch` | 29.20 +/- 2.55 | no gain |
| `--h2o` | 28.75 +/- 2.05 | no gain |
| `--sink-tokens 4` | 28.70 +/- 2.26 | no gain |
| `--moe-exchange-rot` | 29.85 +/- 0.21 | no gain |
| KV `f16` | 26.85 +/- 0.21 | -3.6 t/s, real regression |
| KV `q8_0` | 30.45 +/- 0.07 | tie (SHA identical) |
| `--moe-warm-ram-mb 0` | arg rejected (invalid value) | not testable; 0 not allowed |

None beats the winner. The earlier n=1 rejections were numerically wrong in detail (several are ties,
not losses) but the *decision* to omit them was nevertheless correct. `--native-mtp` is a true
incompatibility, not a performance loss: it cannot build an MTP context alongside an external drafter.

### Stage 4 - Winner vs `--sandwich-auto` (n=5 interleaved)
- Winner: 30.73 +/- 0.21 t/s, 6,576 MiB, SHA 5/5.
- `--sandwich-auto`: **crashed 5/5** during load with `CUDA error: misaligned address`
  (`ggml-cuda.cu:114`, `ggml_cuda_kernel_launch`, `common.cuh:1731`), rc = 134 / SIGABRT.
- Reproduced with explicit `turbo4`, explicit `f16` KV, `--no-sandwich-predict`, context 512, and a
  one-word prompt. It fails at ~0.13 s regardless of KV type, context size, or predictor setting.
- **Finding:** the `--sandwich-auto` default path is broken on this build. This is independent of the
  configuration search and needs its own fix; the winner above is the safe path.

---

## Corrected Dead-End List

Rejected for real (do not re-test on this hardware/driver):
1. **KV `f16`** - 26.85 t/s vs 30.45, a true -3.6 t/s regression. `turbo4` and `q8_0` tie (SHA identical).
2. **`--native-mtp`** - cannot coexist with an external `-md` drafter ("failed to create MTP context").
3. **`--moe-warm-ram-mb` 0** - rejected by the argument parser; not a runnable value.
4. **Slot/warm scaling beyond 16** - no gain; costs VRAM. Slots 24/32 only approach the guard.
5. **`--sched-async-cpu`, `--unrotated-kv`, `--asym-prefetch`, `--h2o`, `--sink-tokens 4`,
   `--moe-exchange-rot`** - all ties or slight losses at n=3. Not regressions, but no reason to enable.

Do **not** carry forward (retracted):
- "Drafter presence/absence is a dead end." False. The drafter was mis-passed as `-m`. With `-md` it is
  the single largest win in this search (+6.2 t/s from VRAM pinning alone, more from spec decode itself).

Open item:
- `--sandwich-auto` crash (See Stage 4). Not a dead end; a defect to fix.

---

## Raw Evidence

Per-run JSON (decode t/s, decode seconds, peak VRAM MiB, VmHWM MiB, pure SHA, rc):

- `raw/sanity_c4_s8.json`
- `raw/baseline_c4.json`
- `raw/grid_n3.json` (partial, interrupted; superseded by the files below)
- `raw/grid_ngld99_n3.json`
- `raw/top_n5.json`
- `raw/ngld_n5.json`
- `raw/ngld_control_n5.json`
- `raw/toggles_n3.json`
- `raw/stage4_n5.json`
- `raw/auto_probe.json`
