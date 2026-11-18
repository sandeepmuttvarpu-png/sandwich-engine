#!/usr/bin/env bash
# ci_guard_harness_integrity.sh — Prevents test tampering and false-green regressions
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "=== [CI GUARD] Verifying Test Harness Integrity ==="

# 1. Assert NMSE_THRESHOLD in tests/llama-context/test-util.cpp is exactly 1e-5
if ! grep -q "const double NMSE_THRESHOLD = 1e-5;" "${REPO_ROOT}/tests/llama-context/test-util.cpp"; then
    echo "🚨 ERROR: NMSE_THRESHOLD tampering detected in tests/llama-context/test-util.cpp! Baseline must be 1e-5." >&2
    exit 1
fi
echo "✓ NMSE_THRESHOLD = 1e-5 verified."

# 2. Assert -fa 0 is NOT passed to test-llama-context in tests/CMakeLists.txt
if grep -q "\-fa 0" "${REPO_ROOT}/tests/CMakeLists.txt"; then
    echo "🚨 ERROR: -fa 0 suppression flag detected in tests/CMakeLists.txt! FlashAttention must not be disabled." >&2
    exit 1
fi
echo "✓ FlashAttention flags intact (no -fa 0 suppression)."

# 3. Assert zero test exclusions (-E or --exclude-regex) in scripts/ or tests/
if grep -rnE "(^|[ ;])ctest [^|]*(-E|--exclude-regex)" "${REPO_ROOT}/scripts/" "${REPO_ROOT}/tests/" 2>/dev/null; then
    echo "🚨 ERROR: Test exclusion flag (-E / --exclude-regex) detected! Zero test exclusions permitted." >&2
    exit 1
fi
echo "✓ Zero test exclusions verified across scripts and tests."

# 4. Assert baseline lineage and Rule W in docs/audit/technique_ledger_v4.9.json
python3 "${REPO_ROOT}/docs/audit/validate_ledger.py"

echo "=== [CI GUARD] All Integrity Checks Passed Successfully ==="
