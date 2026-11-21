# Changelog

All notable changes to Sandwich Engine will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased] - UI Overhaul & Architecture Tooling

### Added
- **Developer & User Documentation:**
  - Added Strata TUI Backend C++ API reference in `docs/api/strata_backend_api.md`.
  - Added power-user CLI cheatsheet and flag reference in `docs/guides/cli_cheatsheet.md`.
  - Authored comprehensive 3-Tier MoE Memory Architecture specification in `docs/architecture/moe_3tier_memory.md`.
  - Added comprehensive user guide and keymap documentation in `docs/tui_user_guide.md`.
- **Hardware & Validation Tooling:**
  - Added automated system diagnostic tool `scripts/check_hardware.py` auditing CPU P/E topology, GPU VRAM capacity, and Transparent HugePages status.
  - Added prompt template dry-run validator in `scripts/test_prompts.sh`.
  - Added `scripts/bench_summary.py` CLI tool for computing warm-4 vs. all-5 throughput, cold start delta, sample standard deviation σ, and 2σ significance tests.
  - Added automated environment checker and launcher script `scripts/launch_tui.sh`.
  - Added dedicated one-line launcher script `scripts/launch_web.sh`.
- **Server Web UI Auto-Mount:**
  - Enhanced `llama-server` HTTP listener to automatically search and mount pre-compiled SvelteKit assets (`build/tools/ui/dist`) when `--path` is omitted.
- **Responsive Strata TUI Terminal Handling:**
  - Added terminal size geometry checks (`cols < 70 || rows < 20`) with warning banners in `src/tui/strata_tui.cpp` to prevent text truncation or ANSI escape wrapping on small terminal windows.
- **Strata Terminal User Interface (TUI):**
  - Integrated MIT-licensed Strata TUI modernized for Sandwich Engine (`sandwich-tui` and `sandwich --tui`).
  - Added ANSI 24-bit truecolor styling, raw POSIX termios non-blocking keyboard controls, model selector, and dual-mode inference screens.
  - Implemented real-time hardware telemetry HUD polling NVIDIA VRAM via `cudaMemGetInfo()`, host RSS via `/proc/self/status`, decode/prefill tokens/sec, and MoE cache hit-rate counters.
- **Auto-Dispatch & Manual Inference Backend:**
  - Integrated `sandwich_auto_dispatch()` under the hood for automatic hardware tuning (Intel hybrid P-core pinning, 3-tier MoE memory hierarchy, TurboQuant Lloyd-Max KV cache compression, and VRAM-pinned speculative assistant).
  - Implemented Manual Configuration mode for power users, allowing granular customization of `-ngl`, `-t`, `-c`, `-md`, `-ngld`, KV quantization, and `--io-uring`.
- **Test Suite Extensions:**
  - Added unit test suite `test_strata_backend()` in `tests/test-sandwich-dispatch.cpp` verifying backend telemetry, model discovery, whitespace path quoting, and parameter synthesis.
  - Added configuration invariant defaults validation and boundary tests.

### Removed
- **Legacy DUI/TUI Implementation:**
  - Completely purged broken legacy terminal wizard code (`tui_wizard.*`), removing dead code paths with zero residual references across `src/`, `tools/`, and `common/`.

### Verified
- Zero test exclusions under Rule W (all 76 CTest suites passing).
- 26/26 audit rules verified in forensic ledger validator (`docs/audit/validate_ledger.py`).
