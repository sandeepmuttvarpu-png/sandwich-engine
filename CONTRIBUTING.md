# Contributing to Sandwich.cpp

Thank you for your interest in contributing to **Sandwich.cpp**! This document provides guidelines for contributing code, architecture enhancements, optimizations, and documentation.

---

## 1. Core Principles

- **Zero-Regressions Policy**: Any addition to the core inference pipeline must not regress throughput, token-to-token latency, or perplexity.
- **Hardware-Conscious Design**: We prioritize bare-metal efficiency on constrained hardware (specifically sub-8GB VRAM consumer GPUs with DDR5 host RAM and NVMe storage).
- **Attribution & Upstream Respect**: Sandwich.cpp is derived from `llama.cpp`. Upstream copyright headers and licenses must always remain intact.

---

## 2. Development Setup

### Prerequisites
- Modern C++17 compiler (GCC 12+, Clang 16+, or MSVC 2022)
- CMake 3.14+
- CUDA Toolkit 12.0+ (for NVIDIA GPU acceleration, with support for SM_120 Blackwell architectures)
- `liburing` (for asynchronous NVMe kernel DMA streaming)

### Building from Source
```bash
git clone https://github.com/sandeepmuttvarpu-png/sandwich-engine.git
cd sandwich-engine
cmake -B build -S . \
  -DGGML_CUDA=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DGGML_CUDA_FA=ON \
  -DGGML_CUDA_GRAPHS=ON
cmake --build build --config Release -j$(nproc) --target sandwich llama-cli
```

---

## 3. Coding Standards

- **C++ Standard**: C++17 required (matching upstream ggml core). Prefer RAII, modern value semantics, and structured concurrency.
- **Naming Conventions**:
  - Types and Classes: `PascalCase` (e.g. `TUIWizard`, `MemoryWatermark`)
  - Functions and Methods: `snake_case` (e.g. `compute_forward`, `recompute_tiering`)
  - Variables: `snake_case`
  - Global variables: Prefix with `g_` (e.g. `g_active_wizard`)
  - Constants: `kPascalCase` or `ALL_CAPS`
- **Memory Safety**: No raw pointers for ownership. Use `std::unique_ptr` or reference handles. When dealing with hardware DMA, ensure page-alignment.
- **Blackwell Compatibility**: Avoid unaligned warps or non-portable PDL directives.

---

## 4. Git Commit Guidelines

We adhere to the Conventional Commits specification:

```
<type>(<scope>): <short description>

[optional body]

[optional footer(s)]
```

Common types:
- `feat`: A new feature or technique
- `fix`: A bug fix or stability patch
- `perf`: Performance or latency optimization
- `docs`: Documentation updates or additions
- `refactor`: Code restructuring without behavioral changes
- `test`: Adding or updating test suites
- `chore`: Maintenance tasks, dependencies, or build config

---

## 5. Pull Request Process

1. Fork or branch from `main`.
2. Ensure your changes compile cleanly with zero warnings (`-Wall -Wextra`).
3. Run test suites (`ctest --output-on-failure`).
4. Validate deterministic parity where applicable.
5. Submit a pull request with a clear description of the motivation and measurements.
