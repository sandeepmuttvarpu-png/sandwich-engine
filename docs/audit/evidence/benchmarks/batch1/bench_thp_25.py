#!/usr/bin/env python3
"""
Ablation Benchmark for Technique #25: 2MB Transparent HugePages (THP) Buffer Alignment
Tests memory allocation and throughput for 512MB RAM Cache Layer buffers:
- Arm A (Baseline): 4KB alignment, MADV_NOHUGEPAGE
- Arm B (Test): 2MB alignment, MADV_HUGEPAGE (RAMCacheLayer specification)
Interleaved n=5 runs with 4s thermal cooling and performance power profile.
"""

import os
import sys
import time
import subprocess
import numpy as np

BINARY = "/home/sandeep/dev/sandwich/sandwich-engine/tests/benchmarks/batch1/bench_thp_25_bin"

def run_arm(arm_id, size_mb=512):
    res = subprocess.run([BINARY, str(arm_id), str(size_mb)], capture_output=True, text=True, check=True)
    return float(res.stdout.strip())

def main():
    print("======================================================================")
    print("BENCHMARK: Technique #25 — 2MB Transparent HugePages Buffer Alignment")
    print("Source: src/dispatch/ram_cache_layer.cpp:56 & ggml/src/ggml.c:354")
    print("Governor: performance | Buffer: 512 MB | Interleaved n=5 | Cooldown: 4s")
    print("======================================================================")
    
    os.system("powerprofilesctl set performance 2>/dev/null || true")
    
    # Warmup
    print("[Warmup] Warming cache...")
    run_arm(0, 128)
    run_arm(1, 128)
    time.sleep(2)
    
    times_a = []
    times_b = []
    
    for i in range(1, 6):
        print(f"\n--- Iteration {i}/5 ---")
        
        # Arm A
        print(f"  [Cooling 4s before Arm A]")
        time.sleep(4)
        t_a = run_arm(0, 512)
        times_a.append(t_a)
        print(f"  Run A{i} (Baseline, 4KB unaligned): {t_a:.2f} ms")
        
        # Arm B
        print(f"  [Cooling 4s before Arm B]")
        time.sleep(4)
        t_b = run_arm(1, 512)
        times_b.append(t_b)
        print(f"  Run B{i} (Test, 2MB THP aligned):    {t_b:.2f} ms")

    mu_a = float(np.mean(times_a))
    sigma_a = float(np.std(times_a, ddof=1))
    mu_b = float(np.mean(times_b))
    sigma_b = float(np.std(times_b, ddof=1))
    
    sigma_comb = float(np.sqrt(sigma_a**2 + sigma_b**2))
    two_sigma = 2.0 * sigma_comb
    delta = mu_a - mu_b # latency reduction
    delta_pct = (delta / mu_a) * 100.0
    
    print("\n========================= STATISTICAL ANALYSIS =========================")
    print(f"Arm A (Baseline 4KB):  mu = {mu_a:.3f} ms, sigma = {sigma_a:.3f} ms")
    print(f"Arm B (Test 2MB THP):  mu = {mu_b:.3f} ms, sigma = {sigma_b:.3f} ms")
    print(f"Delta (Latency Reduc): {delta:.3f} ms ({delta_pct:+.2f}%)")
    print(f"Combined Sigma:        {sigma_comb:.3f} ms")
    print(f"2-Sigma Threshold:     {two_sigma:.3f} ms")
    
    passed = (delta > two_sigma)
    verdict = "PROMOTED TO PROVEN" if passed else "KEPT PARTIAL (Inside Noise)"
    print(f"VERDICT: {verdict} (|Delta| = {abs(delta):.3f} vs 2sigma = {two_sigma:.3f})")
    print("========================================================================")
    
    # Dump JSON artifact for ledger citation
    result_data = {
        "technique_id": 25,
        "name": "[P2] 2MB Transparent HugePages (THP) Buffer Alignment",
        "arm_a_mu": mu_a,
        "arm_a_sigma": sigma_a,
        "arm_b_mu": mu_b,
        "arm_b_sigma": sigma_b,
        "delta_ms": delta,
        "delta_pct": delta_pct,
        "sigma_comb": sigma_comb,
        "two_sigma_threshold": two_sigma,
        "passed": passed,
        "verdict": verdict
    }
    import json
    with open("/home/sandeep/dev/sandwich/sandwich-engine/tests/benchmarks/batch1/result_thp_25.json", "w") as f:
        json.dump(result_data, f, indent=2)

    return 0 if passed else 1

if __name__ == "__main__":
    sys.exit(main())
