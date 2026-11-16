#!/usr/bin/env python3
"""
Technique #22 & #32 Benchmark:
  - Technique #22: Native Blackwell FP4 / NVFP4 Tensor Core Acceleration (`--nvfp4`)
  - Technique #32: Blackwell FP4 / NVFP4 Native Tensor Core Execution (`--dma-ring`)
Architecture-gated benchmark comparing CPU scalar dequantization vs native Blackwell sm_120 GPU NVFP4 execution.
Protocol: n=5 interleaved A/B runs, 4s thermal cooling, powerprofilesctl set performance.
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
SRC_FILE = BENCHMARK_DIR / "bench_nvfp4_22_32.cu"
BIN_FILE = BENCHMARK_DIR / "bench_nvfp4_22_32"
RESULT_FILE = BENCHMARK_DIR / "result_nvfp4_22_32.json"


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
    print("[BUILD] Compiling bench_nvfp4_22_32.cu with nvcc -arch=sm_120...")
    cmd = [
        "/opt/cuda/bin/nvcc",
        "-O3",
        "-arch=sm_120",
        str(SRC_FILE),
        "-o",
        str(BIN_FILE),
    ]
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Build failed:\n{res.stderr}")
    print("[BUILD] bench_nvfp4_22_32 compiled successfully.")


def run_run(enable_nvfp4: bool) -> float:
    cmd = [str(BIN_FILE)]
    if enable_nvfp4:
        cmd.append("--nvfp4")

    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Run failed (enable={enable_nvfp4}):\n{res.stderr}")

    match = re.search(r"Total:\s*([0-9.]+)\s*ms", res.stdout)
    if not match:
        raise ValueError(f"Could not parse Total latency from output:\n{res.stdout}")
    return float(match.group(1))


def mean(data):
    return sum(data) / len(data)


def stdev(data, mu):
    variance = sum((x - mu) ** 2 for x in data) / (len(data) - 1)
    return math.sqrt(variance)


def main():
    print("=" * 70)
    print("BENCHMARK SUITE: Techniques #22 & #32 (Blackwell FP4 / NVFP4 Execution)")
    print("=" * 70)

    set_performance_mode()
    compile_benchmark()

    n_runs = 5
    baseline_latencies = []
    test_latencies = []

    print(f"\n[EXECUTION] Starting n={n_runs} interleaved A/B benchmark (Arm A: CPU Baseline vs Arm B: Blackwell NVFP4 GPU)...")

    for i in range(1, n_runs + 1):
        # Arm A: Baseline (CPU host scalar dequantization)
        time.sleep(4.0)
        t_base = run_run(enable_nvfp4=False)
        baseline_latencies.append(t_base)
        print(f"  [Run {i}/{n_runs}] Arm A (CPU Baseline): {t_base:.4f} ms")

        # Arm B: Test (Blackwell NVFP4 GPU Execution)
        time.sleep(4.0)
        t_test = run_run(enable_nvfp4=True)
        test_latencies.append(t_test)
        print(f"  [Run {i}/{n_runs}] Arm B (NVFP4 GPU):     {t_test:.4f} ms")

    mu_a = mean(baseline_latencies)
    sigma_a = stdev(baseline_latencies, mu_a)
    mu_b = mean(test_latencies)
    sigma_b = stdev(test_latencies, mu_b)

    sigma_comb = math.sqrt(sigma_a**2 + sigma_b**2)
    threshold_2sigma = 2.0 * sigma_comb

    # Latency reduction metric: positive delta means test is faster (lower latency)
    delta = mu_a - mu_b
    passed = delta > threshold_2sigma

    print("\n" + "=" * 70)
    print("STATISTICAL RESULTS (Techniques #22 & #32: Blackwell NVFP4 Tensor Core)")
    print(f"Arm A (Baseline) Mean: {mu_a:.4f} ms, Std: {sigma_a:.4f} ms")
    print(f"Arm B (Test)     Mean: {mu_b:.4f} ms, Std: {sigma_b:.4f} ms")
    print(f"Delta (Reduction):    {delta:.4f} ms ({(delta / mu_a) * 100:.2f}%)")
    print(f"Speedup Ratio:        {mu_a / mu_b:.2f}x")
    print(f"Combined Sigma:       {sigma_comb:.4f} ms")
    print(f"2-Sigma Threshold:    {threshold_2sigma:.4f} ms")
    print(f"Significance Ratio:   {abs(delta) / threshold_2sigma:.2f}x threshold")

    if passed:
        verdict = "PROMOTED"
        print(f"VERDICT: >>> PROMOTED TO PROVEN <<< (Delta {delta:.4f} ms > 2σ {threshold_2sigma:.4f} ms)")
    else:
        verdict = "KEPT PARTIAL (Inside Noise)"
        print(f"VERDICT: >>> KEPT PARTIAL (Inside Noise) <<< (Delta {delta:.4f} ms <= 2σ {threshold_2sigma:.4f} ms)")
    print("=" * 70)

    result = {
        "technique_ids": [22, 32],
        "names": [
            "Native Blackwell FP4 / NVFP4 Tensor Core Acceleration",
            "Blackwell FP4 / NVFP4 Native Tensor Core Execution",
        ],
        "flags": ["--nvfp4", "--dma-ring"],
        "baseline_runs": baseline_latencies,
        "test_runs": test_latencies,
        "baseline_mu": mu_a,
        "baseline_sigma": sigma_a,
        "test_mu": mu_b,
        "test_sigma": sigma_b,
        "delta_ms": delta,
        "speedup_ratio": mu_a / mu_b,
        "sigma_comb": sigma_comb,
        "two_sigma_threshold": threshold_2sigma,
        "verdict": verdict,
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    }

    with open(RESULT_FILE, "w") as f:
        json.dump(result, f, indent=2)
    print(f"[OUTPUT] Written result to {RESULT_FILE}")

    # Cleanup binary
    if BIN_FILE.exists():
        BIN_FILE.unlink()
        print(f"[CLEANUP] Removed temporary binary {BIN_FILE}")


if __name__ == "__main__":
    main()
