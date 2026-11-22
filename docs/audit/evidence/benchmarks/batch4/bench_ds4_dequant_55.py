#!/usr/bin/env python3
"""
Technique #55 Benchmark: Sub-2-bit MoE Expert Dequantization (`--fused-expdec`)
Microbenchmark evaluating CPU scalar fallback path and in-memory row cache for Engram retrieval.
Rule K Compliance: This technique is strictly vetoed from PROVEN status until a fused CUDA kernel is implemented.
It remains PARTIAL in the ledger regardless of empirical delta.
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
SRC_FILE = BENCHMARK_DIR / "bench_ds4_dequant_55.cpp"
BIN_FILE = BENCHMARK_DIR / "bench_ds4_dequant_55"
RESULT_FILE = BENCHMARK_DIR / "result_ds4_dequant_55.json"


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
    print("[BUILD] Compiling bench_ds4_dequant_55.cpp with g++...")
    cmd = [
        "g++",
        "-O3",
        "-I" + str(PROJECT_ROOT),
        "-I" + str(PROJECT_ROOT / "include"),
        "-I" + str(PROJECT_ROOT / "ggml" / "include"),
        str(SRC_FILE),
        "-o",
        str(BIN_FILE),
    ]
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Build failed:\n{res.stderr}")
    print("[BUILD] bench_ds4_dequant_55 compiled successfully.")


def run_run(enable_cached: bool) -> float:
    cmd = [str(BIN_FILE)]
    if enable_cached:
        cmd.append("--fused-expdec")

    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Run failed (enable={enable_cached}):\n{res.stderr}")

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
    print("BENCHMARK SUITE: Technique #55 (Sub-2-bit MoE Expert Dequantization)")
    print("=" * 70)

    set_performance_mode()
    compile_benchmark()

    n_runs = 5
    baseline_latencies = []
    test_latencies = []

    print(f"\n[EXECUTION] Starting n={n_runs} interleaved A/B benchmark (Arm A: Cold Disk vs Arm B: Host Cache)...")

    for i in range(1, n_runs + 1):
        # Arm A: Cold disk read
        time.sleep(4.0)
        t_base = run_run(enable_cached=False)
        baseline_latencies.append(t_base)
        print(f"  [Run {i}/{n_runs}] Arm A (Cold Disk):  {t_base:.4f} ms")

        # Arm B: Host cache hit + CPU scalar fallback
        time.sleep(4.0)
        t_test = run_run(enable_cached=True)
        test_latencies.append(t_test)
        print(f"  [Run {i}/{n_runs}] Arm B (Host Cache): {t_test:.4f} ms")

    mu_a = mean(baseline_latencies)
    sigma_a = stdev(baseline_latencies, mu_a)
    mu_b = mean(test_latencies)
    sigma_b = stdev(test_latencies, mu_b)

    sigma_comb = math.sqrt(sigma_a**2 + sigma_b**2)
    threshold_2sigma = 2.0 * sigma_comb

    # Latency reduction metric: positive delta means test is faster (lower latency)
    delta = mu_a - mu_b

    # Mandatory Rule K check: Technique #55 is vetoed from PROVEN because no fused CUDA kernel exists
    verdict = "KEPT PARTIAL (Rule K Veto - No Fused CUDA Kernel)"

    print("\n" + "=" * 70)
    print("STATISTICAL RESULTS (Technique #55: Sub-2-bit MoE Expert Dequantization)")
    print(f"Arm A (Baseline) Mean: {mu_a:.4f} ms, Std: {sigma_a:.4f} ms")
    print(f"Arm B (Test)     Mean: {mu_b:.4f} ms, Std: {sigma_b:.4f} ms")
    print(f"Delta (Reduction):    {delta:.4f} ms ({(delta / mu_a) * 100:.2f}%)")
    print(f"Combined Sigma:       {sigma_comb:.4f} ms")
    print(f"2-Sigma Threshold:    {threshold_2sigma:.4f} ms")
    print(f"RULE K MANDATE:       Only CPU host reference exists in test-ds4-engram; CUDA kernel absent.")
    print(f"VERDICT: >>> {verdict} <<< (Score remains 0.45)")
    print("=" * 70)

    result = {
        "technique_id": 55,
        "name": "Sub-2-bit MoE Expert Dequantization",
        "flag": "--fused-expdec",
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
        "rule_k_veto": True,
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
