#!/usr/bin/env python3
"""
Technique #6 Benchmark: Pinned Host Memory & Zero-Copy PCIe DMA (`cudaHostRegister`)
A/B interleaved benchmark measuring PCIe Host-to-Device DMA transfer bandwidth and latency:
- Arm A (Baseline): Standard pageable host memory (posix_memalign, unpinned)
- Arm B (Test):     Zero-copy pinned host memory (cudaHostRegister virtual memory pinning)
Protocol: n=5 interleaved iterations, 4s thermal cooling, performance governor.
"""

import subprocess
import time
import json
import math
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BIN_PATH = os.path.join(SCRIPT_DIR, "bench_dma_pinned_6")
SRC_PATH = os.path.join(SCRIPT_DIR, "bench_dma_pinned_6.cu")

def ensure_binary():
    if not os.path.exists(BIN_PATH):
        print(f"[Build] Compiling {SRC_PATH} with nvcc...")
        cmd = ["nvcc", "-O3", "-arch=sm_120", SRC_PATH, "-o", BIN_PATH]
        subprocess.run(cmd, check=True)
        print(f"[Build] Successfully built {BIN_PATH}")

def set_performance():
    try:
        subprocess.run(["powerprofilesctl", "set", "performance"], check=True)
    except Exception as e:
        print(f"Warning: Could not set performance profile: {e}")

def run_arm(pinned: bool, size_mb: int = 128, iters: int = 10):
    flag = "--pinned" if pinned else "--unpinned"
    cmd = [BIN_PATH, flag, "--size-mb", str(size_mb), "--iters", str(iters)]
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
    for line in p.stdout.splitlines():
        if line.strip().startswith("{") and "avg_latency_ms" in line:
            return json.loads(line)
    raise RuntimeError(f"Could not parse output: {p.stdout}")

def main():
    print("================================================================================")
    print("Technique #6: Pinned Host Memory & Zero-Copy PCIe DMA (cudaHostRegister) Benchmark")
    print("Source: src/dispatch/gpudirect_storage.cpp:115")
    print("Metric: Host-to-Device PCIe DMA Bandwidth (GB/s) & Transfer Latency (ms)")
    print("Buffer: 128 MiB | 10 Iters/Run | Interleaved n=5 | Cooldown: 4s")
    print("================================================================================")
    
    ensure_binary()
    set_performance()

    arm_a_lat = []
    arm_a_bw = []
    arm_b_lat = []
    arm_b_bw = []

    # Warm-up run
    print("[Warmup] Warming PCIe interconnect and CUDA runtime...")
    run_arm(pinned=False, size_mb=64, iters=5)
    run_arm(pinned=True, size_mb=64, iters=5)
    time.sleep(2.0)

    n = 5
    for i in range(n):
        print(f"\n--- Iteration {i+1}/{n} ---")

        # Arm A (Baseline: Pageable / Unpinned)
        print(f"  [Cooling 4s before Arm A]")
        time.sleep(4.0)
        print("  Running Arm A (Baseline: Pageable host memory)...")
        res_a = run_arm(pinned=False, size_mb=128, iters=10)
        lat_a = res_a["avg_latency_ms"]
        bw_a = res_a["bandwidth_gb_s"]
        arm_a_lat.append(lat_a)
        arm_a_bw.append(bw_a)
        print(f"    Arm A latency: {lat_a:.4f} ms, bandwidth: {bw_a:.2f} GB/s")

        # Arm B (Test: cudaHostRegister Pinned)
        print(f"  [Cooling 4s before Arm B]")
        time.sleep(4.0)
        print("  Running Arm B (Test: cudaHostRegister pinned DMA)...")
        res_b = run_arm(pinned=True, size_mb=128, iters=10)
        lat_b = res_b["avg_latency_ms"]
        bw_b = res_b["bandwidth_gb_s"]
        arm_b_lat.append(lat_b)
        arm_b_bw.append(bw_b)
        print(f"    Arm B latency: {lat_b:.4f} ms, bandwidth: {bw_b:.2f} GB/s")

    # Statistical analysis on latency reduction and bandwidth expansion
    mu_a_lat = sum(arm_a_lat) / n
    var_a_lat = sum((x - mu_a_lat) ** 2 for x in arm_a_lat) / (n - 1)
    sigma_a_lat = math.sqrt(var_a_lat)

    mu_b_lat = sum(arm_b_lat) / n
    var_b_lat = sum((x - mu_b_lat) ** 2 for x in arm_b_lat) / (n - 1)
    sigma_b_lat = math.sqrt(var_b_lat)

    sigma_comb_lat = math.sqrt(sigma_a_lat ** 2 + sigma_b_lat ** 2)
    two_sigma_lat = 2.0 * sigma_comb_lat
    delta_lat = mu_a_lat - mu_b_lat  # Positive means latency reduction

    mu_a_bw = sum(arm_a_bw) / n
    sigma_a_bw = math.sqrt(sum((x - mu_a_bw) ** 2 for x in arm_a_bw) / (n - 1))
    mu_b_bw = sum(arm_b_bw) / n
    sigma_b_bw = math.sqrt(sum((x - mu_b_bw) ** 2 for x in arm_b_bw) / (n - 1))
    sigma_comb_bw = math.sqrt(sigma_a_bw ** 2 + sigma_b_bw ** 2)
    two_sigma_bw = 2.0 * sigma_comb_bw
    delta_bw = mu_b_bw - mu_a_bw  # Positive means bandwidth increase

    passed = delta_lat > two_sigma_lat and delta_bw > two_sigma_bw

    print("\n========================= STATISTICAL ANALYSIS =========================")
    print(f"Arm A (Baseline, Pageable):      {mu_a_lat:.4f} ± {sigma_a_lat:.4f} ms | {mu_a_bw:.2f} ± {sigma_a_bw:.2f} GB/s")
    print(f"Arm B (Test, cudaHostRegister):  {mu_b_lat:.4f} ± {sigma_b_lat:.4f} ms | {mu_b_bw:.2f} ± {sigma_b_bw:.2f} GB/s")
    print(f"Latency Reduction (Delta):       {delta_lat:.4f} ms ({(delta_lat/mu_a_lat)*100:.2f}%)")
    print(f"Bandwidth Expansion (Delta):     +{delta_bw:.2f} GB/s (+{(delta_bw/mu_a_bw)*100:.2f}%)")
    print(f"Combined Sigma (Latency):        {sigma_comb_lat:.4f} ms")
    print(f"2-Sigma Threshold (Latency):     {two_sigma_lat:.4f} ms")
    print(f"Statistical Delta Ratio:         {delta_lat / two_sigma_lat:.2f}x threshold")
    verdict = "PROMOTED TO PROVEN" if passed else "KEPT PARTIAL (Inside Noise)"
    print(f"VERDICT:                         {verdict}")
    print("========================================================================")

    result_data = {
        "technique_id": 6,
        "name": "Pinned Host Memory & Zero-Copy PCIe DMA (`cudaHostRegister`)",
        "arm_a_latency_mu": mu_a_lat,
        "arm_a_latency_sigma": sigma_a_lat,
        "arm_b_latency_mu": mu_b_lat,
        "arm_b_latency_sigma": sigma_b_lat,
        "delta_latency_ms": delta_lat,
        "delta_pct": (delta_lat / mu_a_lat) * 100.0,
        "arm_a_bandwidth_mu": mu_a_bw,
        "arm_a_bandwidth_sigma": sigma_a_bw,
        "arm_b_bandwidth_mu": mu_b_bw,
        "arm_b_bandwidth_sigma": sigma_b_bw,
        "delta_bandwidth_gb_s": delta_bw,
        "sigma_comb_lat": sigma_comb_lat,
        "two_sigma_threshold_lat": two_sigma_lat,
        "passed": passed,
        "verdict": verdict
    }

    out_path = os.path.join(SCRIPT_DIR, "result_dma_pinned_6.json")
    with open(out_path, "w") as f:
        json.dump(result_data, f, indent=2)
    print(f"Artifact saved to {out_path}")

    return 0 if passed else 1

if __name__ == "__main__":
    sys.exit(main())
