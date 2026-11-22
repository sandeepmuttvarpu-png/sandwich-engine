#!/usr/bin/env python3
"""
Technique #7 [MICROBENCHMARK]: Early-Lookahead Router & Asynchronous Prefetcher (`--moe-lookahead`)
A/B interleaved benchmark measuring 32-layer MoE execution latency:
- Arm A (Baseline): On-demand expert fetching at layer boundary (fetch stall)
- Arm B (Test):     Early-lookahead router prefetching layer L+1 experts asynchronously
Protocol: n=5 interleaved iterations (n=10 if baseline sigma > 10%), 4s thermal cooling, performance governor.
"""

import subprocess
import time
import json
import math
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BIN_PATH = os.path.join(SCRIPT_DIR, "bench_lookahead_router_7")

def set_performance():
    try:
        subprocess.run(["powerprofilesctl", "set", "performance"], check=True)
    except Exception as e:
        print(f"Warning: Could not set performance profile: {e}")

def run_arm(use_lookahead: bool):
    flag = "--lookahead" if use_lookahead else "--no-lookahead"
    cmd = [BIN_PATH, flag, "--layers", "32", "--iters", "25"]
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
    for line in p.stdout.splitlines():
        if line.strip().startswith("{") and "avg_latency_ms" in line:
            return json.loads(line)
    raise RuntimeError(f"Could not parse output: {p.stdout}")

def main():
    print("================================================================================")
    print("Technique #7 [MICROBENCHMARK]: Early-Lookahead Router Benchmark (n=5)")
    print("================================================================================")
    set_performance()

    arm_a_times = []
    arm_b_times = []

    n = 5
    for i in range(n):
        print(f"\n--- Iteration {i+1}/{n} ---")

        # Arm A (Baseline: On-demand)
        print("  Running Arm A (Baseline: On-Demand Fetching)...")
        res_a = run_arm(use_lookahead=False)
        t_a = res_a["avg_latency_ms"]
        arm_a_times.append(t_a)
        print(f"    Arm A Latency: {t_a:.4f} ms ({res_a['avg_layer_us']:.1f} us/layer)")

        print("  Cooling for 4.0s...")
        time.sleep(4.0)

        # Arm B (Test: Early-Lookahead)
        print("  Running Arm B (Test: Early-Lookahead Prefetching)...")
        res_b = run_arm(use_lookahead=True)
        t_b = res_b["avg_latency_ms"]
        arm_b_times.append(t_b)
        print(f"    Arm B Latency: {t_b:.4f} ms ({res_b['avg_layer_us']:.1f} us/layer)")

        if i < n - 1:
            print("  Cooling for 4.0s...")
            time.sleep(4.0)

    # Statistical Calculations on Latency
    mu_a = sum(arm_a_times) / n
    var_a = sum((x - mu_a) ** 2 for x in arm_a_times) / (n - 1)
    sigma_a = math.sqrt(var_a)

    # Hygiene Rule
    if sigma_a > 0.10 * mu_a and n == 5:
        print("\n[HYGIENE RULE TRIGGERED]: Baseline sigma > 10% of mean. Running 5 additional iterations (n=10)...")
        for i in range(5, 10):
            print(f"\n--- Iteration {i+1}/10 ---")
            res_a = run_arm(use_lookahead=False)
            arm_a_times.append(res_a["avg_latency_ms"])
            time.sleep(4.0)
            res_b = run_arm(use_lookahead=True)
            arm_b_times.append(res_b["avg_latency_ms"])
            if i < 9:
                time.sleep(4.0)
        n = 10
        mu_a = sum(arm_a_times) / n
        sigma_a = math.sqrt(sum((x - mu_a) ** 2 for x in arm_a_times) / (n - 1))

    mu_b = sum(arm_b_times) / n
    sigma_b = math.sqrt(sum((x - mu_b) ** 2 for x in arm_b_times) / (n - 1))

    sigma_comb = math.sqrt(sigma_a ** 2 + sigma_b ** 2)
    two_sigma = 2.0 * sigma_comb
    delta_ms = mu_a - mu_b  # Positive means latency reduction

    passed = delta_ms > two_sigma

    print("\n================================================================================")
    print("RESULTS SUMMARY: Technique #7 [MICROBENCHMARK] (Early-Lookahead Router)")
    print("================================================================================")
    print(f"Arm A (Baseline, On-Demand):    {mu_a:.4f} ± {sigma_a:.4f} ms")
    print(f"Arm B (Test, Lookahead Pre):   {mu_b:.4f} ± {sigma_b:.4f} ms")
    print(f"Latency Delta:                 {delta_ms:.4f} ms reduction ({(delta_ms/mu_a)*100:.2f}%)")
    print(f"Combined 2-sigma Threshold:    {two_sigma:.4f} ms")
    print(f"Statistical Significance:      {delta_ms / two_sigma:.2f}x threshold")
    verdict = "PROMOTED TO PROVEN" if passed else "KEPT PARTIAL (Inside Noise)"
    print(f"VERDICT:                       {verdict}")
    print("================================================================================")

    result_data = {
        "technique_id": 7,
        "name": "Early-Lookahead Router & Asynchronous Prefetcher (`--moe-lookahead`) [MICROBENCHMARK]",
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

    out_path = os.path.join(SCRIPT_DIR, "result_lookahead_router_7.json")
    with open(out_path, "w") as f:
        json.dump(result_data, f, indent=2)

    return 0 if passed else 1

if __name__ == "__main__":
    sys.exit(main())
