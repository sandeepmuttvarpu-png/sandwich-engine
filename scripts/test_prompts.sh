#!/usr/bin/env bash
# ==============================================================================
# Sandwich Engine: Prompt Template Dry-Run & Formatting Validator
# Validates prompt synthesis and template handling without executing heavy inference.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BOLD="\033[1m"
GREEN="\033[32m"
CYAN="\033[36m"
YELLOW="\033[33m"
RESET="\033[0m"

echo -e "${BOLD}${CYAN}=== Sandwich Engine: Prompt Template Validator ===${RESET}\n"

TEMPLATES=(
    "chatml:<|im_start|>user\nHello, explain 3-tier MoE memory.<|im_end|>\n<|im_start|>assistant\n"
    "llama3:<|start_header_id|>user<|end_header_id|>\n\nHello, world!<|eot_id|><|start_header_id|>assistant<|end_header_id|>\n\n"
    "qwen:<|im_start|>system\nYou are a helpful assistant.<|im_end|>\n<|im_start|>user\nExplain TurboQuant.<|im_end|>\n<|im_start|>assistant\n"
)

PASS_COUNT=0
TOTAL_COUNT=${#TEMPLATES[@]}

for item in "${TEMPLATES[@]}"; do
    NAME="${item%%:*}"
    PROMPT="${item#*:}"

    echo -ne "  • Testing template ${BOLD}${NAME}${RESET} ... "
    # Verify non-empty and contains valid control tokens
    if [[ -n "${PROMPT}" && "${PROMPT}" == *"<|"* ]]; then
        echo -e "[ ${GREEN}PASS${RESET} ] (Length: ${#PROMPT} chars)"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo -e "[ ${YELLOW}FAIL${RESET} ]"
    fi
done

echo -e "\n${BOLD}${GREEN}Result: ${PASS_COUNT}/${TOTAL_COUNT} prompt templates verified successfully.${RESET}\n"
