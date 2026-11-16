#!/usr/bin/env python3
"""
Technique #19 Benchmark: Frequency-Based Cold-to-Hot Expert Dynamic Promotion (`--moe-cache-profile`)
A/B interleaved benchmark measuring MoE cache hit rate under non-uniform routing:
- Arm A (Baseline): Static default expert placement (no frequency promotion)
- Arm B (Test):     Frequency-based dynamic promotion (online telemetry ranks & repacks hot experts)
Protocol: n=5 interleaved iterations, 4s thermal cooling, performance governor.
"""

import subprocess
import time
import json
import math
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BIN_PATH = os.path.join(SCRIPT_DIR, "bench_dynamic_promotion_19")

def set_performance():
    try:
        subprocess.run(["powerprofilesctl", "set", "performance"], check=True)
    except Exception as e:
        print(f"Warning: Could not set performance profile: {e}")

def run_arm(dynamic: bool):
    flag = "--dynamic" if dynamic else "--static"
    cmd = [BIN_PATH, flag, "--experts", "64", "--slots", "16", "--iters", "20"]
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
    for line in p.stdout.splitlines():
        if line.strip().startswith("{") and "hit_rate_pct" in line:
            return json.loads(line)
    raise RuntimeError(f"Could not parse output: {p.stdout}")

def main():
    print("================================================================================")
    print("Technique #19: Frequency-Based Dynamic Promotion Benchmark (n=5)")
    print("================================================================================")
    set_performance()

    arm_a_hits = []
    arm_b_hits = []

    n = 5
    for i in range(n):
        print(f"\n--- Iteration {i+1}/{n} ---")

        # Arm A (Baseline: Static)
        print("  Running Arm A (Baseline: Static default expert placement)...")
        res_a = run_arm(dynamic=False)
        hit_a = res_a["hit_rate_pct"]
        arm_a_hits.append(hit_a)
        print(f"    Arm A Hit Rate: {hit_a:.2f}%")

        print("  Cooling for 4.0s...")
        time.sleep(4.0)

        # Arm B (Test: Dynamic Promotion)
        print("  Running Arm B (Test: Frequency-Based Dynamic Promotion)...")
        res_b = run_arm(dynamic=True)
        hit_b = res_b["hit_rate_pct"]
        arm_b_hits.append(hit_b)
        print(f"    Arm B Hit Rate: {hit_b:.2f}% (repack: {res_b['repack_ms']:.4f} ms)")

        if i < n - 1:
            print("  Cooling for 4.0s...")
            time.sleep(4.0)

    # Statistical Calculations on Cache Hit Rate
    mu_a = sum(arm_a_hits) / n
    sigma_a = math.sqrt(sum((x - mu_a) ** 2 for x in arm_a_hits) / (n - 1)) if n > 1 else 0.0

    mu_b = sum(arm_b_hits) / n
    sigma_b = math.sqrt(sum((x - mu_b) ** 2 for x in arm_b_hits) / (n - 1)) if n > 1 else 0.0

    delta_pct = mu_b - mu_a
    sigma_comb = math.sqrt(sigma_a ** 2 + sigma_b ** 2)
    two_sigma = max(0.01, 2.0 * sigma_comb)

    passed = delta_pct > two_sigma

    print("\n================================================================================")
    print("RESULTS SUMMARY: Technique #19 (Frequency-Based Dynamic Promotion)")
    print("================================================================================")
    print(f"Arm A (Baseline, Static):          {mu_a:.2f} ± {sigma_a:.2f}% hit rate")
    print(f"Arm B (Test, Dynamic Promotion):   {mu_b:.2f} ± {sigma_b:.2f}% hit rate")
    print(f"Hit Rate Delta:                    +{delta_pct:.2f}%")
    print(f"Combined 2-sigma Threshold:        {two_sigma:.2f}%")
    print(f"Statistical Significance:          {delta_pct / two_sigma:.2f}x threshold")
    verdict = "PROMOTED TO PROVEN" if passed else "KEPT PARTIAL (Inside Noise)"
    print(f"VERDICT:                           {verdict}")
    print("================================================================================")

    result_data = {
        "technique_id": 19,
        "name": "Frequency-Based Cold-to-Hot Expert Dynamic Promotion (`--moe-cache-profile`)",
        "arm_a_mu": mu_a,
        "arm_a_sigma": sigma_a,
        "arm_b_mu": mu_b,
        "arm_b_sigma": sigma_b,
        "delta_hit_rate_pct": delta_pct,
        "sigma_comb": sigma_comb,
        "two_sigma_threshold": two_sigma,
        "passed": passed,
        "verdict": verdict
    }

    out_path = os.path.join(SCRIPT_DIR, "result_dynamic_promotion_19.json")
    with open(out_path, "w") as f:
        json.dump(result_data, f, indent=2)

    return 0 if passed else 1

if __name__ == "__main__":
    sys.exit(main())
