#!/usr/bin/env python3
"""
Technique #38 Benchmark: 2Q & Ghost-Cache Replacement Policy (`--moe-2q`)
Microbenchmark evaluating cache hit-rate under scan pollution (A1in probation + Am frequent + A1out ghost)
versus naive single-queue LRU.
Protocol: n=5 interleaved A/B runs, 4s cooling, powerprofilesctl set performance.
"""

import json
import math
import os
import re
import subprocess
import time
from pathlib import Path

BENCHMARK_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = BENCHMARK_DIR.parents[2]
SRC_FILE = BENCHMARK_DIR / "bench_moe_2q_38.cpp"
BIN_FILE = BENCHMARK_DIR / "bench_moe_2q_38"
RESULT_FILE = BENCHMARK_DIR / "result_moe_2q_38.json"


def set_performance_mode():
    try:
        subprocess.run(
            ["powerprofilesctl", "set", "performance"],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        print("[SETUP] powerprofilesctl set to performance.")
    except Exception as e:
        print(f"[WARN] Failed to set power profile: {e}")


def compile_benchmark():
    print("[BUILD] Compiling bench_moe_2q_38.cpp with g++...")
    cmd = [
        "g++",
        "-O3",
        "-I" + str(PROJECT_ROOT),
        "-I" + str(PROJECT_ROOT / "src"),
        "-I" + str(PROJECT_ROOT / "common"),
        "-I" + str(PROJECT_ROOT / "ggml" / "include"),
        str(SRC_FILE),
        "-o",
        str(BIN_FILE),
    ]
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Build failed:\n{res.stderr}")
    print("[BUILD] bench_moe_2q_38 compiled successfully.")


def run_run(enable_2q: bool) -> float:
    cmd = [str(BIN_FILE)]
    if not enable_2q:
        cmd.append("--no-moe-2q")

    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Run failed (enable_2q={enable_2q}):\n{res.stderr}")

    # Parse output: "[MICROBENCHMARK] Policy: ... | HitRate: 13.4286% (1880/14000) | Total: 0.5403 ms | Status: OK"
    match = re.search(r"HitRate:\s+([0-9.]+)%", res.stdout)
    if not match:
        raise ValueError(f"Could not parse HitRate from output:\n{res.stdout}")

    return float(match.group(1))


def mean_std(values):
    n = len(values)
    mean = sum(values) / n
    variance = sum((x - mean) ** 2 for x in values) / (n - 1) if n > 1 else 0.0
    return mean, math.sqrt(variance)


def main():
    set_performance_mode()
    compile_benchmark()

    n_runs = 5
    baseline_hitrates = []
    test_hitrates = []

    print(f"\n[BENCHMARK] Starting n={n_runs} interleaved A/B benchmark (Technique #38: 2Q & Ghost-Cache Replacement Policy)...")

    for i in range(1, n_runs + 1):
        print(f"\n--- Iteration {i}/{n_runs} ---")

        # Arm A: Baseline (Naive single-queue LRU: --no-moe-2q)
        print("Running Arm A (Baseline: Naive LRU)...")
        hr_base = run_run(enable_2q=False)
        baseline_hitrates.append(hr_base)
        print(f"Arm A Run {i}: {hr_base:.4f}% hit-rate")

        print("Thermal cooling (4s)...")
        time.sleep(4)

        # Arm B: Test (2Q & Ghost Cache: --moe-2q)
        print("Running Arm B (Test: 2Q & Ghost Cache)...")
        hr_test = run_run(enable_2q=True)
        test_hitrates.append(hr_test)
        print(f"Arm B Run {i}: {hr_test:.4f}% hit-rate")

        if i < n_runs:
            print("Thermal cooling (4s)...")
            time.sleep(4)

    mu_a, sigma_a = mean_std(baseline_hitrates)
    mu_b, sigma_b = mean_std(test_hitrates)
    sigma_comb = math.sqrt(sigma_a**2 + sigma_b**2)
    threshold_2sigma = 2.0 * sigma_comb

    # Hit-rate improvement metric: positive delta means 2Q achieves higher hit rate under scan pollution
    delta = mu_b - mu_a
    passed = delta > threshold_2sigma

    print("\n" + "=" * 60)
    print("STATISTICAL RESULTS (Technique #38: 2Q & Ghost-Cache Replacement Policy)")
    print(f"Arm A (Baseline: Naive LRU) Mean: {mu_a:.4f}%, Std: {sigma_a:.4f}%")
    print(f"Arm B (Test: 2Q Cache)      Mean: {mu_b:.4f}%, Std: {sigma_b:.4f}%")
    print(f"Delta (Hit-Rate Boost):           +{delta:.4f}%")
    print(f"Combined Sigma:                   {sigma_comb:.4f}%")
    print(f"2-Sigma Threshold:                {threshold_2sigma:.4f}%")
    if threshold_2sigma > 0:
        print(f"Significance Ratio:               {delta / threshold_2sigma:.2f}x threshold")
    else:
        print(f"Significance Ratio:               INF (deterministic zero-variance difference)")

    if passed:
        verdict = "PROMOTED"
        print(f"VERDICT: >>> PROMOTED TO PROVEN <<< (Delta +{delta:.4f}% > 2σ {threshold_2sigma:.4f}%)")
    else:
        verdict = "KEPT PARTIAL (Inside Noise)"
        print(f"VERDICT: >>> KEPT PARTIAL (Inside Noise) <<< (Delta +{delta:.4f}% <= 2σ {threshold_2sigma:.4f}%)")
    print("=" * 60)

    result = {
        "technique_id": 38,
        "name": "2Q & Ghost-Cache Replacement Policy",
        "flag": "--moe-2q",
        "baseline_runs": baseline_hitrates,
        "test_runs": test_hitrates,
        "baseline_mu": mu_a,
        "baseline_sigma": sigma_a,
        "test_mu": mu_b,
        "test_sigma": sigma_b,
        "delta_hitrate": delta,
        "sigma_comb": sigma_comb,
        "two_sigma_threshold": threshold_2sigma,
        "verdict": verdict,
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    }

    with open(RESULT_FILE, "w") as f:
        json.dump(result, f, indent=2)
    print(f"[OUTPUT] Written result to {RESULT_FILE}")


if __name__ == "__main__":
    main()
