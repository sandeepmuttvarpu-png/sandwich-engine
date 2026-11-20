#!/usr/bin/env bash
# ==============================================================================
# Sandwich Engine: Strata TUI Environment Checker & Launcher
# Validates host environment, GPU/VRAM availability, and launches sandwich-tui.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ANSI formatting
BOLD="\033[1m"
GREEN="\033[32m"
YELLOW="\033[33m"
CYAN="\033[36m"
RED="\033[31m"
RESET="\033[0m"

echo -e "${BOLD}${CYAN}=== Sandwich Engine: Strata TUI Launcher ===${RESET}"

# 1. Check Terminal Capability
if [ -t 1 ]; then
    TERM_COLOR="${COLORTERM:-}"
    if [[ "$TERM_COLOR" =~ ^(truecolor|24bit)$ ]]; then
        echo -e "[ ${GREEN}OK${RESET} ] Terminal: 24-bit TrueColor supported (${TERM})"
    else
        echo -e "[ ${YELLOW}WARN${RESET} ] Terminal: 24-bit TrueColor not advertised in COLORTERM, standard ANSI mode used."
    fi
else
    echo -e "[ ${RED}FAIL${RESET} ] Strata TUI requires an interactive TTY." >&2
    exit 1
fi

# 2. Check Host Hardware & GPU
echo -e "\n${BOLD}Probing Hardware & Accelerators:${RESET}"
if command -v nvidia-smi >/dev/null 2>&1; then
    GPU_NAME=$(nvidia-smi --query-gpu=name --format=csv,noheader,nounits | head -n 1 || echo "Unknown NVIDIA GPU")
    DRIVER_VER=$(nvidia-smi --query-gpu=driver_version --format=csv,noheader,nounits | head -n 1 || echo "Unknown Driver")
    VRAM_TOTAL=$(nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits | head -n 1 || echo "0")
    echo -e "[ ${GREEN}OK${RESET} ] GPU: ${GPU_NAME} (Driver: ${DRIVER_VER}, VRAM: ${VRAM_TOTAL} MiB)"
else
    echo -e "[ ${YELLOW}WARN${RESET} ] nvidia-smi not detected in PATH; GPU telemetry will fallback to CPU-only."
fi

# Check CPU Cores & RAM
CPU_MODEL=$(grep -m1 "model name" /proc/cpuinfo | cut -d: -f2 | sed 's/^[ \t]*//' || echo "Unknown CPU")
RAM_TOTAL_KB=$(grep MemTotal /proc/meminfo | awk '{print $2}' || echo "0")
RAM_TOTAL_GB=$((RAM_TOTAL_KB / 1024 / 1024))
echo -e "[ ${GREEN}OK${RESET} ] CPU: ${CPU_MODEL}"
echo -e "[ ${GREEN}OK${RESET} ] RAM: ${RAM_TOTAL_GB} GB System Memory"

# 3. Locate Binaries
BINARY_PATH=""
if [ -x "${ROOT_DIR}/build/bin/sandwich-tui" ]; then
    BINARY_PATH="${ROOT_DIR}/build/bin/sandwich-tui"
elif [ -x "${ROOT_DIR}/build/bin/sandwich" ]; then
    BINARY_PATH="${ROOT_DIR}/build/bin/sandwich --tui"
fi

if [ -z "${BINARY_PATH}" ]; then
    echo -e "\n[ ${RED}FAIL${RESET} ] Binary not found: neither build/bin/sandwich-tui nor build/bin/sandwich is built."
    echo -e "Please build Sandwich Engine first:"
    echo -e "    cmake -B build -DGGML_CUDA=ON && cmake --build build -j8 --target sandwich-tui"
    exit 1
fi

echo -e "\n[ ${GREEN}OK${RESET} ] Launching Strata TUI: ${BINARY_PATH}\n"
exec ${BINARY_PATH} "$@"
