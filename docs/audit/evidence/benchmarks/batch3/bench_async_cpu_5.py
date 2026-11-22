#!/usr/bin/env python3
"""
Technique #5 [MICROBENCHMARK]: Asynchronous CPU Cold-Expert Execution (`sched_async_cpu`)
A/B interleaved benchmark measuring concurrent CPU-GPU branch execution latency:
- Arm A (Baseline): Synchronous CPU cold-expert execution (serial execution)
- Arm B (Test):     Asynchronous CPU cold-expert execution (overlapped on worker queue)
Protocol: n=5 interleaved iterations (n=10 if baseline sigma > 10%), 4s thermal cooling, performance governor.
"""

import subprocess
import time
import json
import math
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BIN_PATH = os.path.join(SCRIPT_DIR, "bench_async_cpu_5")

def set_performance():
    try:
        subprocess.run(["powerprofilesctl", "set", "performance"], check=True)
    except Exception as e:
        print(f"Warning: Could not set performance profile: {e}")

def run_arm(use_async: bool):
    flag = "--async" if use_async else "--sync"
    cmd = [BIN_PATH, flag, "--dim", "512", "--iters", "25"]
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
    for line in p.stdout.splitlines():
        if line.strip().startswith("{") and "avg_latency_ms" in line:
            return json.loads(line)
    raise RuntimeError(f"Could not parse output: {p.stdout}")

def main():
    print("================================================================================")
    print("Technique #5 [MICROBENCHMARK]: Async CPU Cold-Expert Execution (n=5)")
    print("================================================================================")
    set_performance()

    arm_a_times = []
    arm_b_times = []

    n = 5
    for i in range(n):
        print(f"\n--- Iteration {i+1}/{n} ---")

        # Arm A (Baseline: Serial)
        print("  Running Arm A (Baseline: Synchronous / Serial Execution)...")
        res_a = run_arm(use_async=False)
        t_a = res_a["avg_latency_ms"]
        arm_a_times.append(t_a)
        print(f"    Arm A Latency: {t_a:.4f} ms")

        print("  Cooling for 4.0s...")
        time.sleep(4.0)

        # Arm B (Test: Overlapped Async)
        print("  Running Arm B (Test: Overlapped Async Worker Thread)...")
        res_b = run_arm(use_async=True)
        t_b = res_b["avg_latency_ms"]
        arm_b_times.append(t_b)
        print(f"    Arm B Latency: {t_b:.4f} ms")

        if i < n - 1:
            print("  Cooling for 4.0s...")
            time.sleep(4.0)

    # Statistical Calculations on Latency (ms)
    mu_a = sum(arm_a_times) / n
    var_a = sum((x - mu_a) ** 2 for x in arm_a_times) / (n - 1)
    sigma_a = math.sqrt(var_a)

    # Check Hygiene Rule
    if sigma_a > 0.10 * mu_a and n == 5:
        print("\n[HYGIENE RULE TRIGGERED]: Baseline sigma > 10% of mean. Running 5 additional iterations (n=10)...")
        # Run additional 5
        for i in range(5, 10):
            print(f"\n--- Iteration {i+1}/10 ---")
            res_a = run_arm(use_async=False)
            arm_a_times.append(res_a["avg_latency_ms"])
            time.sleep(4.0)
            res_b = run_arm(use_async=True)
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
    print("RESULTS SUMMARY: Technique #5 [MICROBENCHMARK] (Async CPU Cold-Expert Execution)")
    print("================================================================================")
    print(f"Arm A (Baseline, Serial):       {mu_a:.4f} ± {sigma_a:.4f} ms")
    print(f"Arm B (Test, Async Overlap):   {mu_b:.4f} ± {sigma_b:.4f} ms")
    print(f"Latency Delta:                 {delta_ms:.4f} ms reduction ({(delta_ms/mu_a)*100:.2f}%)")
    print(f"Combined 2-sigma Threshold:    {two_sigma:.4f} ms")
    print(f"Statistical Significance:      {delta_ms / two_sigma:.2f}x threshold")
    verdict = "PROMOTED TO PROVEN" if passed else "KEPT PARTIAL (Inside Noise)"
    print(f"VERDICT:                       {verdict}")
    print("================================================================================")

    result_data = {
        "technique_id": 5,
        "name": "Asynchronous CPU Cold-Expert Execution (`sched_async_cpu`) [MICROBENCHMARK]",
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

    out_path = os.path.join(SCRIPT_DIR, "result_async_cpu_5.json")
    with open(out_path, "w") as f:
        json.dump(result_data, f, indent=2)

    return 0 if passed else 1

if __name__ == "__main__":
    sys.exit(main())
