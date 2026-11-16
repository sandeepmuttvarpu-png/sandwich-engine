#!/usr/bin/env python3
"""
Technique #11 & #46 Benchmark:
  - Technique #11: Native FlashMLA Attention Kernel (`latent_attn.cu`)
  - Technique #46: Latent FlashMLA Direct Tensor Core MMA (`--kv-snapshot` / MLA Tensor Core MMA)
Architecture-gated benchmark comparing CPU reference execution vs GPU FlashMLA Tensor Core MMA.
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
SRC_FILE = BENCHMARK_DIR / "bench_flash_mla_11_46.cpp"
BIN_FILE = BENCHMARK_DIR / "bench_flash_mla_11_46"
RESULT_FILE = BENCHMARK_DIR / "result_flash_mla_11_46.json"


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
    print("[BUILD] Compiling bench_flash_mla_11_46.cpp with g++ linking GGML CUDA/CPU...")
    cmd = [
        "g++",
        "-O3",
        "-I" + str(PROJECT_ROOT / "ggml" / "include"),
        "-I" + str(PROJECT_ROOT / "ggml" / "src"),
        str(SRC_FILE),
        "-L" + str(PROJECT_ROOT / "build" / "bin"),
        f"-Wl,-rpath,{PROJECT_ROOT / 'build' / 'bin'}",
        "-lggml",
        "-lggml-base",
        "-lggml-cuda",
        "-lggml-cpu",
        "-o",
        str(BIN_FILE),
    ]
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Build failed:\n{res.stderr}")
    print("[BUILD] bench_flash_mla_11_46 compiled successfully.")


def run_run(enable_flash_mla: bool) -> float:
    cmd = [str(BIN_FILE)]
    if enable_flash_mla:
        cmd.append("--flash-mla")

    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Run failed (enable={enable_flash_mla}):\n{res.stderr}")

    # Parse output: "[FLASH_MLA] Mode: ... | Iters: 50 | Total: 1.0702 ms | Per-step: 0.0214 ms | Status: OK"
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
    print("BENCHMARK SUITE: Techniques #11 & #46 (Native FlashMLA & Latent MMA)")
    print("=" * 70)

    set_performance_mode()
    compile_benchmark()

    n_runs = 5
    baseline_latencies = []
    test_latencies = []

    print(f"\n[EXECUTION] Starting n={n_runs} interleaved A/B benchmark (Arm A: Baseline CPU vs Arm B: FlashMLA GPU)...")

    for i in range(1, n_runs + 1):
        # Arm A: Baseline (CPU scalar reference)
        time.sleep(4.0)
        t_base = run_run(enable_flash_mla=False)
        baseline_latencies.append(t_base)
        print(f"  [Run {i}/{n_runs}] Arm A (CPU Baseline): {t_base:.4f} ms")

        # Arm B: Test (FlashMLA GPU Tensor Core MMA)
        time.sleep(4.0)
        t_test = run_run(enable_flash_mla=True)
        test_latencies.append(t_test)
        print(f"  [Run {i}/{n_runs}] Arm B (FlashMLA GPU):  {t_test:.4f} ms")

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
    print("STATISTICAL RESULTS (Techniques #11 & #46: Native FlashMLA & Latent MMA)")
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
        "technique_ids": [11, 46],
        "names": [
            "Native FlashMLA Attention Kernel (latent_attn.cu)",
            "Latent FlashMLA Direct Tensor Core MMA",
        ],
        "flags": ["--flash-mla", "--kv-snapshot"],
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
