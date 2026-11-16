#!/usr/bin/env python3
"""
Ablation Benchmark for Technique #13: Intel Hybrid Topology P-Core Pinning
Tests CPU inference generation throughput and decode latency:
- Arm A (Baseline): -t 16 (Unpinned 16 threads, spilling to E-cores and SMT)
- Arm B (Test):     -t 6  (Strictly pinned to 6 physical P-cores)
Interleaved n=5 runs with 4s thermal cooling and performance power profile.
"""

import os
import sys
import time
import json
import subprocess
import numpy as np

BENCH_BIN = "/home/sandeep/dev/sandwich/sandwich-engine/build/bin/llama-bench"
MODEL_PATH = "/home/sandeep/ai-models/dense/NVIDIA-Nemotron-3-Nano-4B-GGUF/NVIDIA-Nemotron-3-Nano-4B-Q6_K.gguf"

def run_bench(threads):
    cmd = [
        BENCH_BIN,
        "-m", MODEL_PATH,
        "-ngl", "0",
        "-p", "64",
        "-n", "32",
        "-t", str(threads),
        "-r", "1",
        "-o", "json"
    ]
    res = subprocess.run(cmd, capture_output=True, text=True, check=True)
    stdout = res.stdout
    # llama-bench outputs CUDA init info before the JSON array
    json_start = stdout.find("[")
    if json_start == -1:
        raise RuntimeError(f"Failed to find JSON in output: {stdout}")
    data = json.loads(stdout[json_start:])
    
    # Second item is generation (n_gen > 0)
    gen_item = [d for d in data if d.get("n_gen", 0) > 0][0]
    tokens_per_sec = gen_item["avg_ts"]
    latency_ms = gen_item["avg_ns"] / 1e6
    return tokens_per_sec, latency_ms

def main():
    print("======================================================================")
    print("BENCHMARK: Technique #13 — Intel Hybrid Topology P-Core Pinning")
    print("Source: src/dispatch/hardware_profiler.cpp:276 & src/dispatch/router.cpp:194")
    print(f"Model: Nemotron-3-Nano-4B-Q6_K | CPU: i7-13620H | Interleaved n=5 | Cooldown: 4s")
    print("======================================================================")
    
    os.system("powerprofilesctl set performance 2>/dev/null || true")
    
    # Warmup
    print("[Warmup] Executing warmup...")
    run_bench(6)
    time.sleep(2)
    
    tps_a = []
    tps_b = []
    
    for i in range(1, 6):
        print(f"\n--- Iteration {i}/5 ---")
        
        # Arm A: -t 16
        print("  [Cooling 4s before Arm A]")
        time.sleep(4)
        t_a, lat_a = run_bench(16)
        tps_a.append(t_a)
        print(f"  Run A{i} (Baseline, -t 16 unpinned): {t_a:.3f} t/s ({lat_a:.1f} ms)")
        
        # Arm B: -t 6
        print("  [Cooling 4s before Arm B]")
        time.sleep(4)
        t_b, lat_b = run_bench(6)
        tps_b.append(t_b)
        print(f"  Run B{i} (Test, -t 6 P-core pinned): {t_b:.3f} t/s ({lat_b:.1f} ms)")

    mu_a = float(np.mean(tps_a))
    sigma_a = float(np.std(tps_a, ddof=1))
    mu_b = float(np.mean(tps_b))
    sigma_b = float(np.std(tps_b, ddof=1))
    
    sigma_comb = float(np.sqrt(sigma_a**2 + sigma_b**2))
    two_sigma = 2.0 * sigma_comb
    delta = mu_b - mu_a # throughput gain
    delta_pct = (delta / mu_a) * 100.0
    
    print("\n========================= STATISTICAL ANALYSIS =========================")
    print(f"Arm A (Baseline -t 16):    mu = {mu_a:.3f} t/s, sigma = {sigma_a:.3f} t/s")
    print(f"Arm B (Test -t 6 P-cores): mu = {mu_b:.3f} t/s, sigma = {sigma_b:.3f} t/s")
    print(f"Delta (Throughput Gain):   {delta:+.3f} t/s ({delta_pct:+.2f}%)")
    print(f"Combined Sigma:            {sigma_comb:.3f} t/s")
    print(f"2-Sigma Threshold:         {two_sigma:.3f} t/s")
    
    passed = (delta > two_sigma)
    verdict = "PROMOTED TO PROVEN" if passed else "KEPT PARTIAL (Inside Noise)"
    print(f"VERDICT: {verdict} (|Delta| = {abs(delta):.3f} vs 2sigma = {two_sigma:.3f})")
    print("========================================================================")
    
    result_data = {
        "technique_id": 13,
        "name": "Intel Hybrid Topology P-Core Pinning",
        "arm_a_mu": mu_a,
        "arm_a_sigma": sigma_a,
        "arm_b_mu": mu_b,
        "arm_b_sigma": sigma_b,
        "delta_tps": delta,
        "delta_pct": delta_pct,
        "sigma_comb": sigma_comb,
        "two_sigma_threshold": two_sigma,
        "passed": passed,
        "verdict": verdict
    }
    with open("/home/sandeep/dev/sandwich/sandwich-engine/tests/benchmarks/batch1/result_pcore_13.json", "w") as f:
        json.dump(result_data, f, indent=2)

    return 0 if passed else 1

if __name__ == "__main__":
    sys.exit(main())
