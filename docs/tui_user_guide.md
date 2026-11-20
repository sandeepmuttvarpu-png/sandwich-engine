# Sandwich Engine: Strata TUI User Guide & Keymap Reference

**Engine:** Sandwich Engine (`sandwich.cpp`)  
**Interface:** Strata Terminal User Interface (`sandwich-tui`)  
**Target Hardware:** HP Victus 15-fa1xxx | Intel Core i7-13620H (6P + 4E) | 24 GB DDR5-5200 | NVIDIA RTX 5050 Mobile 8GB (sm_120 Blackwell) | Linux (Kernel 7.3.5)

---

## 1. Quick Start & Invocation

The Strata TUI provides a terminal interface for Sandwich Engine, featuring real-time hardware telemetry and dual execution paths (Auto-Dispatch vs. Manual Mode).

### Launch Methods

```bash
# Method 1: Default launch via unified Sandwich router
./build/bin/sandwich

# Method 2: Explicit TUI flag
./build/bin/sandwich --tui

# Method 3: Direct binary invocation
./build/bin/sandwich-tui
```

---

## 2. Interface Navigation & Keymap Reference

The interface operates with zero-latency non-blocking terminal I/O using raw POSIX termios mode and ANSI 24-bit truecolor styling.

### 2.1 Model Selection & Setup Screen (Screen 1)

| Key Binding | Action | Description |
| :---: | :--- | :--- |
| **`[Enter]`** | **Start Inference** | Dispatches configured model to inference execution with streaming output |
| **`[A]`** | **Auto Mode** | Selects recommended auto-tuning via `sandwich_auto_dispatch()` |
| **`[M]`** | **Manual Mode** | Unlocks advanced power-user flag customization |
| **`[↑]` / `[↓]`** | **Navigate Models** | Scrolls through discovered `.gguf` models in local directories |
| **`[Q]`** | **Quit** | Restores terminal cursor and exits cleanly |

### 2.2 Live Inference Dashboard Screen (Screen 2)

| Key Binding | Action | Description |
| :---: | :--- | :--- |
| **`[S]`** | **Stop / Back** | Gracefully halts generation (SIGINT/SIGTERM) and returns to setup menu |
| **`[C]`** | **Clear Output** | Clears the live streaming token window |
| **`[Q]`** | **Quit Application** | Terminates inference subprocess, restores terminal, and exits |

---

## 3. Execution Modes

### 3.1 Auto-Dispatch Mode (`[●] AUTO-DISPATCH`)
Auto-Dispatch is the default, recommended configuration for consumer workstations:
* **Intel Hybrid P-Core Pinning:** Automatically detects physical Performance-cores (`i7-13620H`: 6 P-cores) and sets thread pool and CPU affinity mask to prevent E-core synchronization stalls.
* **3-Tier MoE Hierarchy:** Configures Tier 1 (VRAM micro-expert slab), Tier 2 (DDR5 Transparent HugePages warm pool), and Tier 3 (asynchronous Linux `io_uring` direct NVMe storage).
* **TurboQuant Lloyd-Max KV Compression:** Automatically activates `turbo4` polar spherical quantization for key and value tensors, reducing KV cache footprint by 75% with zero reasoning drift.
* **VRAM-Pinned Speculative Assistant:** Scans model trees for matching drafters (e.g. `Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf`) and pins them 100% in GPU VRAM (`-ngld 99`) for 180+ tok/s draft generation.

### 3.2 Manual Mode (`[●] MANUAL MODE`)
Manual Mode provides complete low-level command synthesis for ablation testing and profiling:
* `-ngl <layers>`: Main model GPU layer offload.
* `-t <threads>`: Inference compute worker threads.
* `-c <ctx>`: Context length window.
* `-md <path> -ngld <layers>`: Speculative drafter path and GPU offload level (0 = host, 99 = VRAM).
* `--moe-cache-slots <n>`: Dynamic expert VRAM slot allocation (0, 16, 32).
* `--moe-warm-ram-mb <n>`: Host DDR5 warm pool capacity.
* `-ctk / -ctv <type>`: Quantized KV cache precision (`turbo4`, `turbo2`, `q8_0`, `f16`).
* `--io-uring`: Direct kernel async I/O streaming.
* `--moe-2q`: Two-Queue (2Q) probation and ghost cache replacement policy.
* `--moe-exchange-rot`: Zero-copy rotational buffer pointer exchange.

---

## 4. Real-Time Telemetry HUD (Technique #18)

During generation, the top panel updates at 20 Hz with live hardware and cache metrics:
1. **GPU VRAM:** Real-time physical allocation polled directly from the NVIDIA driver via `cudaMemGetInfo()`.
2. **Host RAM:** Resident Set Size (RSS) parsed dynamically from `/proc/self/status` `VmRSS`.
3. **Decode & Prefill Throughput:** Measured tokens per second and single-token latency in milliseconds.
4. **MoE Cache Hit-Rate:** Cumulative hit-rate percentage and hot hits vs. cold misses tracked via Technique #18 atomic counters (`sandwich_telemetry_get_stats()`).
5. **Drafter & Pipeline Status:** Real-time indicators of speculative decoding state and io_uring activity.
