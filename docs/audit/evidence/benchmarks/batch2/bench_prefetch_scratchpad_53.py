#!/usr/bin/env python3
"""
Technique #53 Benchmark: Per-Layer Pinned Prefetch Scratchpads (`--prefetch-scratchpad`)
A/B interleaved benchmark measuring 32-layer pipelined staging latency:
- Arm A (Baseline): Synchronous staging without pinned prefetch scratchpads
- Arm B (Test):     Double-buffered pinned prefetch scratchpads (overlapped layer staging)
Protocol: n=5 interleaved iterations, 4s thermal cooling, performance governor.
"""

import subprocess
import time
import json
import math
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BIN_PATH = os.path.join(SCRIPT_DIR, "bench_prefetch_scratchpad_53")

def set_performance():
    try:
        subprocess.run(["powerprofilesctl", "set", "performance"], check=True)
    except Exception as e:
        print(f"Warning: Could not set performance profile: {e}")

def run_arm(scratchpad: bool):
    flag = "--scratchpad" if scratchpad else "--synchronous"
    cmd = [BIN_PATH, flag, "--layers", "32", "--iters", "10"]
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
    for line in p.stdout.splitlines():
        if line.strip().startswith("{") and "avg_pipeline_ms" in line:
            return json.loads(line)
    raise RuntimeError(f"Could not parse output: {p.stdout}")

def main():
    print("================================================================================")
    print("Technique #53: Per-Layer Pinned Prefetch Scratchpads Benchmark (n=5)")
    print("================================================================================")
    set_performance()

    arm_a_times = []
    arm_b_times = []

    n = 5
    for i in range(n):
        print(f"\n--- Iteration {i+1}/{n} ---")

        # Arm A (Baseline: Synchronous / No scratchpad)
        print("  Running Arm A (Baseline: Synchronous on-demand transfer)...")
        res_a = run_arm(scratchpad=False)
        t_a = res_a["avg_pipeline_ms"]
        arm_a_times.append(t_a)
        print(f"    Arm A time: {t_a:.4f} ms ({res_a['avg_layer_ms']:.4f} ms/layer)")

        print("  Cooling for 4.0s...")
        time.sleep(4.0)

        # Arm B (Test: Pinned Prefetch Scratchpad)
        print("  Running Arm B (Test: Double-Buffered Pinned Prefetch Scratchpad)...")
        res_b = run_arm(scratchpad=True)
        t_b = res_b["avg_pipeline_ms"]
        arm_b_times.append(t_b)
        print(f"    Arm B time: {t_b:.4f} ms ({res_b['avg_layer_ms']:.4f} ms/layer)")

        if i < n - 1:
            print("  Cooling for 4.0s...")
            time.sleep(4.0)

    # Statistical Calculations
    mu_a = sum(arm_a_times) / n
    var_a = sum((x - mu_a) ** 2 for x in arm_a_times) / (n - 1)
    sigma_a = math.sqrt(var_a)

    mu_b = sum(arm_b_times) / n
    var_b = sum((x - mu_b) ** 2 for x in arm_b_times) / (n - 1)
    sigma_b = math.sqrt(var_b)

    sigma_comb = math.sqrt(sigma_a ** 2 + sigma_b ** 2)
    two_sigma = 2.0 * sigma_comb
    delta_ms = mu_a - mu_b  # Positive means latency reduction

    passed = delta_ms > two_sigma

    print("\n================================================================================")
    print("RESULTS SUMMARY: Technique #53 (Per-Layer Pinned Prefetch Scratchpads)")
    print("================================================================================")
    print(f"Arm A (Baseline, Synchronous):    {mu_a:.4f} ± {sigma_a:.4f} ms")
    print(f"Arm B (Test, Pinned Scratchpad): {mu_b:.4f} ± {sigma_b:.4f} ms")
    print(f"Pipeline Latency Delta:          {delta_ms:.4f} ms reduction ({(delta_ms/mu_a)*100:.2f}%)")
    print(f"Combined 2-sigma Threshold:      {two_sigma:.4f} ms")
    print(f"Statistical Significance:        {delta_ms / two_sigma:.2f}x threshold")
    verdict = "PROMOTED TO PROVEN" if passed else "KEPT PARTIAL (Inside Noise)"
    print(f"VERDICT:                         {verdict}")
    print("================================================================================")

    result_data = {
        "technique_id": 53,
        "name": "Per-Layer Pinned Prefetch Scratchpads (`--prefetch-scratchpad`)",
        "arm_a_mu": mu_a,
        "arm_a_sigma": sigma_a,
        "arm_b_mu": mu_b,
        "arm_b_sigma": sigma_b,
        "delta_ms": delta_ms,
        "delta_pct": (delta_ms / mu_a) * 100.0,
        "sigma_comb": sigma_comb,
        "two_sigma_threshold": two_sigma,
        "passed": passed,
        "verdict": verdict
    }

    out_path = os.path.join(SCRIPT_DIR, "result_prefetch_scratchpad_53.json")
    with open(out_path, "w") as f:
        json.dump(result_data, f, indent=2)

    return 0 if passed else 1

if __name__ == "__main__":
    sys.exit(main())
