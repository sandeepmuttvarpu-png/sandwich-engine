#!/usr/bin/env python3
"""
Ablation Benchmark for Technique #60: Unrotated KV Staging (`--unrotated-kv`)
Tests sliding-window context shifting and generation decode latency:
- Arm A (Baseline): --no-unrotated-kv (Explicit graph shift & RoPE re-rotation)
- Arm B (Test):     --unrotated-kv    (O(1) zero-copy metadata position shifts)
Interleaved n=5 runs with 4s thermal cooling and performance power profile.
"""

import os
import sys
import re
import time
import json
import subprocess
import numpy as np

CLI_BIN = "/home/sandeep/dev/sandwich/sandwich-engine/build/bin/llama-cli"
MODEL_PATH = "/home/sandeep/ai-models/dense/NVIDIA-Nemotron-3-Nano-4B-GGUF/NVIDIA-Nemotron-3-Nano-4B-Q6_K.gguf"

PROMPT = "In the deep void of interstellar space, ancient probes drift silently between galaxies."
ANSI_RE = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')

def run_bench(flag):
    # Use setsid to disconnect /dev/tty and route stdout/stderr to pipes
    cmd = [
        "setsid",
        CLI_BIN,
        "-m", MODEL_PATH,
        "-ngl", "32",
        "-c", "32",
        "-n", "96",
        "-p", PROMPT,
        "-st",
        flag
    ]
    res = subprocess.run(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    raw = (res.stdout or "") + "\n" + (res.stderr or "")
    clean = ANSI_RE.sub('', raw)
    
    gen_m = re.search(r"Generation:\s+([\d\.]+)\s+t/s", clean)
    dec_m = re.search(r"decode:\s+([\d\.]+)s", clean)
    
    if not gen_m or not dec_m:
        raise RuntimeError(f"Could not parse stats from output:\n{clean[-500:]}")
        
    tps = float(gen_m.group(1))
    decode_ms = float(dec_m.group(1)) * 1000.0
    return tps, decode_ms

def main():
    print("======================================================================")
    print("BENCHMARK: Technique #60 — Unrotated KV Staging (--unrotated-kv)")
    print("Source: src/llama-kv-cache.cpp:1035 & common/arg.cpp:4844")
    print("Metric: Sliding Context Generation Throughput (t/s) | Interleaved n=5 | Cooldown: 4s")
    print("======================================================================")
    
    os.system("powerprofilesctl set performance 2>/dev/null || true")
    
    # Warmup
    print("[Warmup] Executing warmup...")
    run_bench("--no-unrotated-kv")
    run_bench("--unrotated-kv")
    time.sleep(2)
    
    tps_a = []
    tps_b = []
    
    for i in range(1, 6):
        print(f"\n--- Iteration {i}/5 ---")
        
        # Arm A: --no-unrotated-kv
        print("  [Cooling 4s before Arm A]")
        time.sleep(4)
        t_a, d_a = run_bench("--no-unrotated-kv")
        tps_a.append(t_a)
        print(f"  Run A{i} (Baseline, --no-unrotated-kv): {t_a:.2f} t/s ({d_a:.1f} ms decode)")
        
        # Arm B: --unrotated-kv
        print("  [Cooling 4s before Arm B]")
        time.sleep(4)
        t_b, d_b = run_bench("--unrotated-kv")
        tps_b.append(t_b)
        print(f"  Run B{i} (Test, --unrotated-kv):       {t_b:.2f} t/s ({d_b:.1f} ms decode)")

    mu_a = float(np.mean(tps_a))
    sigma_a = float(np.std(tps_a, ddof=1))
    mu_b = float(np.mean(tps_b))
    sigma_b = float(np.std(tps_b, ddof=1))
    
    sigma_comb = float(np.sqrt(sigma_a**2 + sigma_b**2))
    two_sigma = 2.0 * sigma_comb
    delta = mu_b - mu_a # throughput gain
    delta_pct = (delta / mu_a) * 100.0
    
    print("\n========================= STATISTICAL ANALYSIS =========================")
    print(f"Arm A (Baseline --no-unrotated-kv): mu = {mu_a:.3f} t/s, sigma = {sigma_a:.3f} t/s")
    print(f"Arm B (Test --unrotated-kv):       mu = {mu_b:.3f} t/s, sigma = {sigma_b:.3f} t/s")
    print(f"Delta (Throughput Gain):           {delta:+.3f} t/s ({delta_pct:+.2f}%)")
    print(f"Combined Sigma:                    {sigma_comb:.3f} t/s")
    print(f"2-Sigma Threshold:                 {two_sigma:.3f} t/s")
    
    passed = (delta > two_sigma)
    verdict = "PROMOTED TO PROVEN" if passed else "KEPT PARTIAL (Inside Noise)"
    print(f"VERDICT: {verdict} (|Delta| = {abs(delta):.3f} vs 2sigma = {two_sigma:.3f})")
    print("========================================================================")
    
    result_data = {
        "technique_id": 60,
        "name": "Unrotated KV Staging (`--unrotated-kv`)",
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
    with open("/home/sandeep/dev/sandwich/sandwich-engine/tests/benchmarks/batch1/result_unrotated_kv_60.json", "w") as f:
        json.dump(result_data, f, indent=2)

    return 0 if passed else 1

if __name__ == "__main__":
    sys.exit(main())
