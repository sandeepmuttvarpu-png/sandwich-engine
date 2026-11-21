# Sandwich Engine: CLI & Tooling Cheat Sheet

Quick-reference commands, flag mappings, and launcher scripts for Sandwich Engine on consumer workstations (reference: HP Victus 15, Intel Core i7-13620H, RTX 5050 Mobile 8GB VRAM).

---

## 1. Quick Launchers

| Interface | Command | Notes |
| :--- | :--- | :--- |
| **Strata TUI** | `./scripts/launch_tui.sh` | Interactive terminal UI with 20 Hz live HUD |
| **Unified Router** | `./build/bin/sandwich` | Automatically launches TUI in interactive terminals |
| **Web UI (DUI)** | `./scripts/launch_web.sh` | Starts HTTP server & auto-mounts SvelteKit frontend at `http://localhost:8080` |
| **Hardware Check** | `./scripts/check_hardware.py` | Audits CPU P/E cores, GPU VRAM, and HugePages |
| **Benchmark Stats**| `./scripts/bench_summary.py` | Formats warm-4 vs. all-5 throughput tables & 2σ significance tests |

---

## 2. Command Line Recipes

### 2.1 MoE Speculative Decoding (Qwen 35B + DFlash2 Drafter)
```bash
./build/bin/llama-cli \
  -m /home/sandeep/ai-models/moe/Qwen3.6-35B-A3B-MTP-GGUF/Qwen3.6-35B-A3B-UD-IQ3_XXS.gguf \
  -md /home/sandeep/ai-models/drafters/Qwen3.6-35B-A3B-DFlash2-GGUF/Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf \
  -ngl 14 \
  -ngld 99 \
  -t 6 \
  -c 4096 \
  --moe-cache-slots 16 \
  --moe-warm-ram-mb 16552 \
  -ctk turbo4 \
  -ctv turbo4 \
  --io-uring \
  --moe-2q \
  --moe-exchange-rot
```

### 2.2 Dense Model 100% GPU Offload (Dolphin Mistral 7B)
```bash
./build/bin/llama-cli \
  -m /home/sandeep/ai-models/dense/dolphin-2.8-mistral-7b-v02-GGUF/dolphin-2.8-mistral-7b-v02-Q4_K_S.gguf \
  -ngl 99 \
  -t 6 \
  -c 4096
```

### 2.3 Web Server & REST API
```bash
./build/bin/llama-server \
  -m /home/sandeep/ai-models/dense/dolphin-2.8-mistral-7b-v02-GGUF/dolphin-2.8-mistral-7b-v02-Q4_K_S.gguf \
  -ngl 99 \
  -t 6 \
  -c 4096 \
  --port 8080 \
  --host 127.0.0.1
```

---

## 3. Key Flag Reference

* **`-t 6`**: Pins 6 worker threads to physical Performance-cores (P-cores) on the i7-13620H, preventing E-core latency jitter.
* **`-ngl 14`**: Fits main model base layers into RTX 5050 8GB VRAM envelope.
* **`-ngld 99`**: Pins 100% of the speculative drafter in GPU VRAM for zero-latency drafting.
* **`-ctk turbo4 -ctv turbo4`**: TurboQuant polar 4-bit spherical quantization for KV tensors (75% context footprint reduction).
* **`--moe-cache-slots 16`**: Dynamic hot expert cache allocation in VRAM.
* **`--moe-warm-ram-mb 16552`**: Host DDR5 warm pool capacity using Transparent HugePages.
* **`--io-uring`**: Direct kernel asynchronous DMA streaming from NVMe storage.
