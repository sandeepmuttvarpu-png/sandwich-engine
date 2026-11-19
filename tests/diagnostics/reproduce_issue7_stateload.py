#!/usr/bin/env python3
"""
Issue #7 Forensic Diagnostic Suite: Recurrent / Hybrid State Load Divergence Reproducer.
Systematically reproduces and catalogs NMSE drift during state_load across candidate models:
kolibri1-moe, mimo2-moe, muse-spark-moe, olmo2-dense, plamo3-dense, maple-moe, cohere2moe-moe, and flint-moe.
"""

import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
BIN_PATH = REPO_ROOT / "build" / "bin" / "test-llama-context"
MODELS_DIR = REPO_ROOT / "build" / "tests" / "test-models"
OUTPUT_JSON = Path(__file__).resolve().parent / "issue7_diagnostic_report.json"

TARGET_MODELS = [
    "kolibri1-moe.gguf",
    "mimo2-moe.gguf",
    "muse-spark-moe.gguf",
    "olmo2-dense.gguf",
    "plamo3-dense.gguf",
    "maple-moe.gguf",
    "cohere2moe-moe.gguf",
    "flint-moe.gguf",
]


def test_model(model_name: str) -> dict:
    model_path = MODELS_DIR / model_name
    if not model_path.exists():
        return {
            "model": model_name,
            "status": "MISSING",
            "error": "Model file not found",
        }

    cmd = [
        str(BIN_PATH),
        "-m", str(model_path),
        "--test", "baseline|state_load",
    ]

    res = subprocess.run(cmd, cwd=str(REPO_ROOT), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    # Clean any generated state dumps
    for dump in REPO_ROOT.glob(f"dump_state.{model_name}*"):
        try:
            dump.unlink()
        except Exception:
            pass

    # Strip ANSI color codes
    clean_stdout = re.sub(r"\x1b\[[0-9;]*m", "", res.stdout)

    # Parse NMSE divergence
    # Format: "generate_tokens_compare: error: NMSE at step 1 is 2.723098e-05 (threshold 1.0e-05)"
    nmse_match = re.search(r"NMSE at step (\d+) is ([0-9.eE+-]+)\s*\(threshold ([0-9.eE+-]+)\)", clean_stdout)
    step = int(nmse_match.group(1)) if nmse_match else None
    measured_nmse = float(nmse_match.group(2)) if nmse_match else None
    threshold = float(nmse_match.group(3)) if nmse_match else 1e-5

    status = "FAIL" if "state_load  FAIL" in clean_stdout else ("PASS" if "state_load  OK" in clean_stdout else "UNKNOWN")

    return {
        "model": model_name,
        "status": status,
        "step": step,
        "measured_nmse": measured_nmse,
        "threshold": threshold,
        "nmse_ratio": (measured_nmse / threshold) if (measured_nmse is not None and threshold > 0) else None,
        "raw_output": [l for l in res.stdout.splitlines() if "baseline" in l or "state_load" in l or "NMSE" in l],
    }


def main():
    print("=" * 80)
    print("🔬 [SANDWICH DIAGNOSTIC] ISSUE #7 RECURRENT STATE_LOAD REPRODUCER")
    print("=" * 80)

    results = []
    for model in TARGET_MODELS:
        print(f"Testing {model:<25} ... ", end="", flush=True)
        r = test_model(model)
        status = r["status"]
        if status == "FAIL":
            nmse_val = f"{r['measured_nmse']:.4e}" if r['measured_nmse'] is not None else "UNKNOWN"
            ratio_val = f"{r['nmse_ratio']:.1f}x" if r['nmse_ratio'] is not None else "N/A"
            print(f"FAILED (step {r['step']}, NMSE: {nmse_val}, {ratio_val} threshold)")
        else:
            print(f"{status}")
        results.append(r)

    print("=" * 80)
    print("ISSUE #7 FORENSIC MATRIX")
    print(f"{'Model':<24} | {'Status':<6} | {'Step':<5} | {'Measured NMSE':<18} | {'Threshold':<10} | {'Ratio':<8}")
    print("-" * 80)
    for r in results:
        step_str = str(r['step']) if r['step'] is not None else "N/A"
        nmse_str = f"{r['measured_nmse']:.4e}" if r['measured_nmse'] is not None else "PASS (not-emitted)"
        ratio_str = f"{r['nmse_ratio']:.1f}x" if r['nmse_ratio'] is not None else "N/A"
        print(f"{r['model']:<24} | {r['status']:<6} | {step_str:<5} | {nmse_str:<18} | {r['threshold']:<10.1e} | {ratio_str:<8}")
    print("=" * 80)

    report = {
        "issue_id": 7,
        "title": "Recurrent / Hybrid state_load NMSE Divergence",
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "target_count": len(results),
        "failures_detected": sum(1 for r in results if r["status"] == "FAIL"),
        "results": results,
    }

    OUTPUT_JSON.parent.mkdir(parents=True, exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(report, f, indent=2)
    print(f"[REPORT] Saved diagnostic report to {OUTPUT_JSON}")


if __name__ == "__main__":
    main()
