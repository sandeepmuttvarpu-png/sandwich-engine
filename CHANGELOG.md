# Changelog

All notable changes to Sandwich Engine will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased] - UI Overhaul

### Added
- **Strata Terminal User Interface (TUI):**
  - Integrated MIT-licensed Strata TUI modernized for Sandwich Engine (`sandwich-tui` and `sandwich --tui`).
  - Added ANSI 24-bit truecolor styling, raw POSIX termios non-blocking keyboard controls, model selector, and dual-mode inference screens.
  - Implemented real-time hardware telemetry HUD polling NVIDIA VRAM via `cudaMemGetInfo()`, host RSS via `/proc/self/status`, decode/prefill tokens/sec, and MoE cache hit-rate counters.
- **Auto-Dispatch & Manual Inference Backend:**
  - Integrated `sandwich_auto_dispatch()` under the hood for automatic hardware tuning (Intel hybrid P-core pinning, 3-tier MoE memory hierarchy, TurboQuant Lloyd-Max KV cache compression, and VRAM-pinned speculative assistant).
  - Implemented Manual Configuration mode for power users, allowing granular customization of `-ngl`, `-t`, `-c`, `-md`, `-ngld`, KV quantization, and `--io-uring`.
- **Unified Engine Dispatch & Tools:**
  - Re-wired primary `sandwich` binary to dynamically dispatch to `sandwich-tui` when invoked without arguments or with `--tui`.
  - Added automated environment checker and launcher script `scripts/launch_tui.sh`.
  - Added comprehensive user guide and keymap documentation in `docs/tui_user_guide.md`.
  - Added unit test suite `test_strata_backend()` in `tests/test-sandwich-dispatch.cpp` verifying backend telemetry, model discovery, and parameter synthesis.

### Removed
- **Legacy DUI/TUI Implementation:**
  - Completely purged broken legacy terminal wizard code (`tui_wizard.*`), removing dead code paths with zero residual references across `src/`, `tools/`, and `common/`.

### Verified
- Zero test exclusions under Rule W (all 76 CTest suites passing).
- 26/26 audit rules verified in forensic ledger validator (`docs/audit/validate_ledger.py`).
