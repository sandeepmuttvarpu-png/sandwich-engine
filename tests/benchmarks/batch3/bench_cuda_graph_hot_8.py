#!/usr/bin/env python3
"""
Technique #8 Benchmark: CUDA Graph Capture Hot Path (`--cuda-graph-hot`)
Microbenchmark comparing uncaptured host stream kernel dispatches vs captured CUDA Graph execution.
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
SRC_FILE = BENCHMARK_DIR / "bench_cuda_graph_hot_8.cu"
BIN_FILE = BENCHMARK_DIR / "bench_cuda_graph_hot_8"
RESULT_FILE = BENCHMARK_DIR / "result_cuda_graph_hot_8.json"


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
    print("[BUILD] Compiling bench_cuda_graph_hot_8.cu with nvcc...")
    cmd = [
        "/opt/cuda/bin/nvcc",
        "-O3",
        str(SRC_FILE),
        "-o",
        str(BIN_FILE),
    ]
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Build failed:\n{res.stderr}")
    print("[BUILD] bench_cuda_graph_hot_8 compiled successfully.")


def run_run(enable_graph: bool) -> float:
    cmd = [str(BIN_FILE)]
    if enable_graph:
        cmd.append("--cuda-graph-hot")

    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Run failed (enable={enable_graph}):\n{res.stderr}")

    # Parse output: "[MICROBENCHMARK] Mode: ... | Total: 10.8848 ms | Per-step: 54.42 us | Status: OK"
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

    print(f"\n[BENCHMARK] Starting n={n_runs} interleaved A/B benchmark (Technique #8: CUDA Graph Capture Hot Path)...")

    for i in range(1, n_runs + 1):
        print(f"\n--- Iteration {i}/{n_runs} ---")

        # Arm A: Baseline (Direct host dispatch)
        print("Running Arm A (Baseline: Direct Host Dispatch)...")
        t_base = run_run(enable_graph=False)
        baseline_latencies.append(t_base)
        print(f"Arm A Run {i}: {t_base:.4f} ms")

        print("Thermal cooling (4s)...")
        time.sleep(4)

        # Arm B: Test (--cuda-graph-hot)
        print("Running Arm B (Test: CUDA Graph Capture Hot Path)...")
        t_test = run_run(enable_graph=True)
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
    print("STATISTICAL RESULTS (Technique #8: CUDA Graph Capture Hot Path)")
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
        "technique_id": 8,
        "name": "CUDA Graph Capture for the Hot-Expert Path",
        "flag": "--cuda-graph-hot",
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
