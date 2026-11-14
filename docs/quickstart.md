# Sandwich.cpp Quickstart Guide

This guide walks you through getting up and running with **Sandwich.cpp** on consumer hardware (such as laptops or single-GPU workstations with 8GB VRAM).

---

## 1. Quick Installation

Install prebuilt binaries and link the unified `sandwich` CLI directly into your path:

```bash
curl -fsSL https://raw.githubusercontent.com/sandeepmuttvarpu-png/sandwich-engine/main/install.sh | bash
```

To verify the installation:
```bash
sandwich --tune
```

---

## 2. Interactive TUI Wizard

Launch the smart-dispatch terminal wizard by running `sandwich` with no arguments:

```bash
sandwich
```

The TUI automatically detects:
- Available VRAM and live Wayland memory overhead
- Physical CPU cores vs E-cores (for thread affinity pinning)
- Host RAM availability and optimal 70% threshold ratio
- Local GGUF models and spec-decoding drafters in your system cache

---

## 3. Recommended CLI Configurations

### Small / Fast Dense Models (3B–8B)
Fits entirely in VRAM:
```bash
sandwich run -m /path/to/Llama-3.2-3B.gguf -c 8192
```

### Mid-Sized Hybrid Models (27B–35B)
Optimal for hybrid VRAM + DDR5 execution:
```bash
sandwich run \
  -m /path/to/Qwen3.6-35B-A3B.gguf \
  -md /path/to/Qwen3.6-35B-A3B-DFlash2.gguf \
  -c 32768 \
  -ctk turbo4 -ctv turbo4 \
  --sandwich-auto
```

### Ultra-Context 128k Workloads
Enables 3-tier memory tiering and 2Q ghost cache to handle 128k context without VRAM fragmentation:
```bash
sandwich run \
  -m /path/to/DeepSeek-Coder-V2-Lite.gguf \
  -c 131072 \
  -ctk turbo2 -ctv turbo2 \
  -fitt 7100 \
  --sandwich-auto
```

---

## 4. Running as a Background Daemon

Sandwich.cpp provides a systemd service unit:

```bash
systemctl --user enable --now sandwich.service
```

Check service status:
```bash
systemctl --user status sandwich.service
```
