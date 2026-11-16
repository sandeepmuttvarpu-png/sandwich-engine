#!/usr/bin/env python3
"""
Technique #36 Benchmark: Asynchronous CPU Split Worker Overlap (`--async-cpu`)
Microbenchmark evaluating concurrent execution of CPU graph splits via dedicated worker queue
versus synchronous execution blocking pipeline orchestration.
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
SRC_FILE = BENCHMARK_DIR / "bench_async_split_36.cpp"
BIN_FILE = BENCHMARK_DIR / "bench_async_split_36"
RESULT_FILE = BENCHMARK_DIR / "result_async_split_36.json"


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
    print("[BUILD] Compiling bench_async_split_36.cpp with g++...")
    cmd = [
        "g++",
        "-O3",
        "-pthread",
        str(SRC_FILE),
        "-o",
        str(BIN_FILE),
    ]
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Build failed:\n{res.stderr}")
    print("[BUILD] bench_async_split_36 compiled successfully.")


def run_run(enable_async: bool) -> float:
    cmd = [str(BIN_FILE)]
    if enable_async:
        cmd.append("--async-cpu")

    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Run failed (enable={enable_async}):\n{res.stderr}")

    # Parse output: "[MICROBENCHMARK] Mode: ... | Total: 287.7467 ms | Per-step: 5.7549 ms | Status: OK"
    match = re.search(r"Total:\s+([0-9.]+)\s+ms", res.stdout)
    if not match:
        raise ValueError(f"Could not parse total time from output:\n{res.stdout}")

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
    baseline_latencies = []
    test_latencies = []

    print(f"\n[BENCHMARK] Starting n={n_runs} interleaved A/B benchmark (Technique #36: Asynchronous CPU Split Worker Overlap)...")

    for i in range(1, n_runs + 1):
        print(f"\n--- Iteration {i}/{n_runs} ---")

        # Arm A: Baseline (Synchronous evaluation)
        print("Running Arm A (Baseline: Synchronous CPU execution)...")
        t_base = run_run(enable_async=False)
        baseline_latencies.append(t_base)
        print(f"Arm A Run {i}: {t_base:.4f} ms")

        print("Thermal cooling (4s)...")
        time.sleep(4)

        # Arm B: Test (--async-cpu)
        print("Running Arm B (Test: Asynchronous CPU Split Worker Overlap)...")
        t_test = run_run(enable_async=True)
        test_latencies.append(t_test)
        print(f"Arm B Run {i}: {t_test:.4f} ms")

        if i < n_runs:
            print("Thermal cooling (4s)...")
            time.sleep(4)

    mu_a, sigma_a = mean_std(baseline_latencies)
    mu_b, sigma_b = mean_std(test_latencies)
    sigma_comb = math.sqrt(sigma_a**2 + sigma_b**2)
    threshold_2sigma = 2.0 * sigma_comb

    # Latency reduction metric: positive delta means test is faster (lower latency)
    delta = mu_a - mu_b
    passed = delta > threshold_2sigma

    print("\n" + "=" * 60)
    print("STATISTICAL RESULTS (Technique #36: Asynchronous CPU Split Worker Overlap)")
    print(f"Arm A (Baseline) Mean: {mu_a:.4f} ms, Std: {sigma_a:.4f} ms")
    print(f"Arm B (Test)     Mean: {mu_b:.4f} ms, Std: {sigma_b:.4f} ms")
    print(f"Delta (Reduction):    {delta:.4f} ms ({(delta / mu_a) * 100:.2f}%)")
    print(f"Combined Sigma:       {sigma_comb:.4f} ms")
    print(f"2-Sigma Threshold:    {threshold_2sigma:.4f} ms")
    print(f"Significance Ratio:   {abs(delta) / threshold_2sigma:.2f}x threshold")

    if passed:
        verdict = "PROMOTED"
        print(f"VERDICT: >>> PROMOTED TO PROVEN <<< (Delta {delta:.4f} ms > 2σ {threshold_2sigma:.4f} ms)")
    else:
        verdict = "KEPT PARTIAL (Inside Noise)"
        print(f"VERDICT: >>> KEPT PARTIAL (Inside Noise) <<< (Delta {delta:.4f} ms <= 2σ {threshold_2sigma:.4f} ms)")
    print("=" * 60)

    result = {
        "technique_id": 36,
        "name": "Asynchronous CPU Split Worker Overlap",
        "flag": "--async-cpu",
        "baseline_runs": baseline_latencies,
        "test_runs": test_latencies,
        "baseline_mu": mu_a,
        "baseline_sigma": sigma_a,
        "test_mu": mu_b,
        "test_sigma": sigma_b,
        "delta_ms": delta,
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
