# Sandwich Engine — Host Universality Sweep & Verified Re-Run Report

**Host**: HP Victus 15 | Intel i7-13620H (6P+4E) | 24 GB DDR5-5200 | RTX 5050 Laptop 8 GB (`sm_120`, 7,767 MiB CUDA total) | Arch Linux `7.3.5` | Driver `620.73.04`
**Host Guards**: Peak VRAM $\le 7,100\text{ MiB}$ | Peak Host VmHWM $\le 16,552\text{ MiB}$ | Cooldown: 60s between runs.

---

## 1. Verified Phase 2 Matrix (`throughput_v2_verified.json` + Banked MoE Rows)

All four target models were run ($n=3$, 60s cooldown, Jinja chat templates via `llama-cli` $\to$ `/v1/chat/completions`) **after** fixing the dense auto-dispatch defects in `common/common.cpp` (`f960bab09` and `c23fb46df`):

| Model | Arch | Params / Quant | Prompt | Dispatch / Config | Prefill ($\mu \pm \sigma$) | Decode ($\mu \pm \sigma$) | Max VRAM ($\le 7,100$) | Max VmHWM ($\le 16,552$) | Output & Guard Verdict |
|---|---|---|---|---|---|---|---|---|---|
| **Qwen3.6-35B-A3B (Certified)** | `qwen35moe` | 35.51B / IQ3_S | `sandwich_bench_prompt.txt` (`3,771` raw / `3,780` evaluated) | Certified (`-ngl 14 -ngld 99`, slots 16, warm 8000, `SANDWICH_AUTOTUNE_DISABLE=1`) | **$424.97 \pm 56.70\text{ t/s}$** *(warm-2: 457.70)* | **$31.40 \pm 1.87\text{ t/s}$** *(warm-2: 32.45)* | **6,576 MiB** ✓ | **13,936 MiB** ✓ | **VALID + 3/3 SHA MATCH ✓** |
| **Dolphin-2.8-Mistral-7B** | `llama` | 7.24B / Q4_K_S | `prompt_512.txt` (`561` tok) | Fixed `--sandwich-auto` (`-ngl 99`, 33/33 layers, `turbo4`) | **$1,883.93 \pm 73.66\text{ t/s}$** | **$50.87 \pm 0.25\text{ t/s}$** | **4,342 MiB** ✓ | **4,357 MiB** ✓ | **VALID + CLEAN GUARDS ✓** |
| **Gemma-4-31B-it QAT** | `gemma4` | 30.70B / Q4_0 | `prompt_512.txt` (`469` tok) | Fixed `--sandwich-auto` (`-ngl 16`, 16/61 layers, `turbo4`) | **$110.23 \pm 3.59\text{ t/s}$** | **$2.03 \pm 0.55\text{ t/s}$** | **5,689 MiB** ✓ | **17,354 MiB** *(Rep 2 +802M)* | **VALID (Rep 2 VmHWM BREACH)** |
| **DeepSeek-Coder-V2-Lite** | `deepseek2` | 15.71B / IQ3_XXS | `prompt_512.txt` | Fixed `--sandwich-auto` (`-ngl 99`, 28/28 backbone, 3-tier slab) | **$76.50 \pm 3.30\text{ t/s}$** | **$15.20 \pm 0.17\text{ t/s}$** | **3,650 MiB** ✓ | **13,317 MiB** ✓ | **VALID + CLEAN GUARDS ✓** |
| **LFM2-8B-A1B** *(Banked MoE)* | `lfm2moe` | 8.34B / IQ2_XXS | `prompt_512.txt` | `--sandwich-auto` (`-ngl 32`, 3-tier slab, `turbo4`) | **$2,058.77 \pm 67.75\text{ t/s}$** | **$204.07 \pm 5.12\text{ t/s}$** | **2,470 MiB** ✓ | **2,551 MiB** ✓ | **VALID + CLEAN GUARDS ✓** |

### Raw Per-Run Metrics (`throughput_v2_verified.json`)
- **Qwen3.6-35B-A3B (Certified)**:
  - Rep 1 *(cold, 41.56 °C)*: `pp 359.5 t/s`, `tg 29.3 t/s`, `load 8.02s`, `VRAM 6,576 MiB`, `VmHWM 12,723 MiB`, `SHA=MATCH`
  - Rep 2 *(warm, 42.12 °C)*: `pp 457.3 t/s`, `tg 32.9 t/s`, `load 3.22s`, `VRAM 6,576 MiB`, `VmHWM 13,932 MiB`, `SHA=MATCH`
  - Rep 3 *(warm, 41.62 °C)*: `pp 458.1 t/s`, `tg 32.0 t/s`, `load 1.81s`, `VRAM 6,576 MiB`, `VmHWM 13,936 MiB`, `SHA=MATCH`
  - *Baseline Reconciliation*: Warm-2 decode `32.45 t/s` reproduces the certified `32.72 ± 0.61 t/s` baseline (`Δ = -0.27`, `2σ_comb ≈ 1.76`, not statistically significant). Raw tokenization of `sandwich_bench_prompt.txt` yields `3,771` tokens (`llama-tokenize`), while `/v1/chat/completions` Jinja wrapping yields `prompt_n = 3,780` (historically cited as `3,782` on the identical SHA-matched file).
- **Dolphin-2.8-Mistral-7B**:
  - Rep 1 *(41.06 °C)*: `pp 1,799.1 t/s`, `tg 50.9 t/s`, `load 4.62s`, `VRAM 4,342 MiB`, `VmHWM 4,213 MiB`
  - Rep 2 *(39.81 °C)*: `pp 1,921.1 t/s`, `tg 51.1 t/s`, `load 1.81s`, `VRAM 4,342 MiB`, `VmHWM 4,357 MiB`
  - Rep 3 *(38.88 °C)*: `pp 1,931.6 t/s`, `tg 50.6 t/s`, `load 0.83s`, `VRAM 4,342 MiB`, `VmHWM 4,357 MiB`
  - *Before vs After*: `12.03 t/s` @ `358 MiB` VRAM (`-\n\n-` under `-ngl 0` + silent `Q2_K`) $\to$ **`50.87 t/s` @ `4,342 MiB` VRAM (+4.23× speedup, coherent prose)**.
- **Gemma-4-31B-it QAT**:
  - Rep 1 *(38.47 °C)*: `pp 112.1 t/s`, `tg 2.4 t/s`, `load 14.26s`, `VRAM 5,600 MiB`, `VmHWM 14,539 MiB` ✓
  - Rep 2 *(41.97 °C)*: `pp 106.1 t/s`, `tg 1.4 t/s`, `load 5.10s`, `VRAM 5,689 MiB`, `VmHWM 17,354 MiB` *(+802 MiB breach)*
  - Rep 3 *(42.22 °C)*: `pp 112.5 t/s`, `tg 2.3 t/s`, `load 7.07s`, `VRAM 5,600 MiB`, `VmHWM 16,324 MiB` ✓
  - *Before vs After*: `1.57 t/s` @ `1,021 MiB` VRAM (`The same same...` under `-ngl 0` + silent `Q2_K`) $\to$ **`2.03 t/s` mean (+29% over `1.57`, coherent 3/3 reasoning)** @ `5,600–5,689 MiB` VRAM.
- **DeepSeek-Coder-V2-Lite**:
  - Rep 1 *(41.97 °C)*: `pp 72.7 t/s`, `tg 15.4 t/s`, `load 8.42s`, `VRAM 3,650 MiB`, `VmHWM 12,796 MiB`
  - Rep 2 *(42.50 °C)*: `pp 78.7 t/s`, `tg 15.1 t/s`, `load 3.81s`, `VRAM 3,650 MiB`, `VmHWM 13,308 MiB`
  - Rep 3 *(42.03 °C)*: `pp 78.1 t/s`, `tg 15.1 t/s`, `load 4.02s`, `VRAM 3,650 MiB`, `VmHWM 13,317 MiB`
  - *VRAM Breakdown Reconciliation*: `2,400 MiB` reported by `--sandwich-auto` is the Tier 1 MoE VRAM expert slab alone (`common.cpp:1845` when `free_vram > 6,000 MiB`); total peak VRAM `3,650 MiB` comprises the `2,400 MiB` MoE slab + `474.20 MiB` CUDA0 non-expert model buffer (`28/28` backbone layers) + `334.25 MiB` Turbo4 KV cache + `113.13 MiB` compute buffer + `0.84 MiB` rotation buffer + CUDA context (`~328 MiB`).

---

## 2. Superseded Telemetry Scope & Architecture Map

- **`throughput.json` (`INSTRUMENT-INVALID`)**: Covers **every dense model row** (`Dolphin-2.8-Mistral-7B`, `Gemma-4-31B-it QAT`, `Muse-Glimmer-30B`) plus the mismatched 512-tok `Qwen3.6-35B-A3B` row. MoE rows (`LFM2-8B-A1B`, `Flint Preview`, `GLM-4.7-Flash`, `Nemotron-3-Nano-30B`) took the MoE dispatch branch and are unaffected by the dense `Q2_K` / `-1` sentinel defects (though `Flint`, `GLM-4.7-Flash`, and `Nemotron-3-Nano-30B` breached the `16,552 MiB` host `VmHWM` guard).
- **15-Architecture Local Corpus Map (`inventory.json`, `coverage.json`)**:
  - Unsupported upstream architectures: `inkling2_loop` (`Inkling-2-Mini`), `dory` (`NVIDIA-Nemotron-Labs-Dory1.0-4B`).
  - Dense `qwen35` loader assert (`src/llama-model-loader.cpp:1954: GGML_ASSERT(buf_mmap || cur->data)`): `Qwen3.8-27B` and `Bonsai-27B-Q1_0` (`qwen35moe` unaffected).
  - Multi-shard estimator bug: `qwen4exp` (`Qwen3.8-Flash-Next` 125B, 70.91 GB across 2 shards) sized against shard 1 (`44.09 GB`), breaching both `7,100 MiB` VRAM (`7,220 MiB`) and `16,552 MiB` VmHWM (`18,018 MiB`).
  - Open follow-ups filed in `docs/history/notes.md` (Sections 7–9): `SANDWICH_AUTOTUNE_DISABLE` vs MoE warm-up repack gating (`src/llama-context.cpp:3588`), Gemma-4 Rep 2 nondeterministic `VmHWM` spike (`2,815 MiB` spread), and re-testing `Muse-Glimmer-30B`, `C4AI Command-R` (`600s` timeout), and `Flint Preview` under fixed dispatch.
