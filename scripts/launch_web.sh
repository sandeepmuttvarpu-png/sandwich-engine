#!/usr/bin/env bash
# ==============================================================================
# Sandwich Engine: Web UI (DUI) Launcher
# Launches llama-server with embedded SvelteKit frontend mounted on --path
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
UI_DIST="${ROOT_DIR}/build/tools/ui/dist"

MODEL="${1:-/home/sandeep/ai-models/dense/dolphin-2.8-mistral-7b-v02-GGUF/dolphin-2.8-mistral-7b-v02-Q4_K_S.gguf}"
PORT="${2:-8080}"

if [ ! -d "${UI_DIST}" ]; then
    echo "Error: UI dist directory not found at ${UI_DIST}" >&2
    exit 1
fi

echo "=========================================================="
echo " Starting Sandwich Web UI (DUI)"
echo " Server URL : http://127.0.0.1:${PORT}"
echo " Model      : ${MODEL}"
echo " UI Assets  : ${UI_DIST}"
echo "=========================================================="

exec "${ROOT_DIR}/build/bin/llama-server" \
    -m "${MODEL}" \
    -ngl 99 \
    -t 6 \
    -c 4096 \
    --port "${PORT}" \
    --host 127.0.0.1 \
    --path "${UI_DIST}"
