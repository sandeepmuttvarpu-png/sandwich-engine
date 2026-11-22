#!/usr/bin/env python3
"""
Technique #21 Benchmark: Streaming Attention Sink / Sliding-Window KV Compaction (`--sink-tokens`)
A/B interleaved benchmark measuring prompt sink token preservation and compaction latency:
- Arm A (Baseline): Attention sinks disabled (--no-sinks / sink_tokens = 0)
- Arm B (Test):     Streaming Attention Sinks active (--sink-tokens 4)
Protocol: n=5 interleaved iterations, 4s thermal cooling, performance governor.
"""

import subprocess
import time
import json
import math
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BIN_PATH = os.path.join(SCRIPT_DIR, "bench_sink_tokens_21")

def set_performance():
    try:
        subprocess.run(["powerprofilesctl", "set", "performance"], check=True)
    except Exception as e:
        print(f"Warning: Could not set performance profile: {e}")

def run_arm(use_sinks: bool):
    flag = "--sink-tokens" if use_sinks else "--no-sinks"
    cmd = [BIN_PATH, flag, "--cycles", "500"]
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
    for line in p.stdout.splitlines():
        if line.strip().startswith("{") and "sink_retention_pct" in line:
            return json.loads(line)
    raise RuntimeError(f"Could not parse output: {p.stdout}")

def main():
    print("================================================================================")
    print("Technique #21: Streaming Attention Sink / KV Compaction Benchmark (n=5)")
    print("================================================================================")
    set_performance()

    arm_a_ret = []
    arm_a_lat = []
    arm_b_ret = []
    arm_b_lat = []

    n = 5
    for i in range(n):
        print(f"\n--- Iteration {i+1}/{n} ---")

        # Arm A (Baseline: No Sinks)
        print("  Running Arm A (Baseline: Sinks Disabled)...")
        res_a = run_arm(use_sinks=False)
        ret_a = res_a["sink_retention_pct"]
        lat_a = res_a["avg_us_per_cycle"]
        arm_a_ret.append(ret_a)
        arm_a_lat.append(lat_a)
        print(f"    Arm A Retention: {ret_a:.1f}% ({lat_a:.2f} us/cycle, {res_a['sinks_preserved']} sinks preserved)")

        print("  Cooling for 4.0s...")
        time.sleep(4.0)

        # Arm B (Test: Sinks Enabled)
        print("  Running Arm B (Test: Streaming Attention Sinks Active)...")
        res_b = run_arm(use_sinks=True)
        ret_b = res_b["sink_retention_pct"]
        lat_b = res_b["avg_us_per_cycle"]
        arm_b_ret.append(ret_b)
        arm_b_lat.append(lat_b)
        print(f"    Arm B Retention: {ret_b:.1f}% ({lat_b:.2f} us/cycle, {res_b['sinks_preserved']} sinks preserved)")

        if i < n - 1:
            print("  Cooling for 4.0s...")
            time.sleep(4.0)

    # Statistical Calculations on Sink Retention Rate
    mu_a_ret = sum(arm_a_ret) / n
    sigma_a_ret = math.sqrt(sum((x - mu_a_ret) ** 2 for x in arm_a_ret) / (n - 1)) if n > 1 else 0.0

    mu_b_ret = sum(arm_b_ret) / n
    sigma_b_ret = math.sqrt(sum((x - mu_b_ret) ** 2 for x in arm_b_ret) / (n - 1)) if n > 1 else 0.0

    delta_ret = mu_b_ret - mu_a_ret
    sigma_comb_ret = math.sqrt(sigma_a_ret ** 2 + sigma_b_ret ** 2)
    two_sigma_ret = max(0.01, 2.0 * sigma_comb_ret)

    mu_a_lat = sum(arm_a_lat) / n
    mu_b_lat = sum(arm_b_lat) / n

    passed = (delta_ret > two_sigma_ret) and (mu_b_ret == 100.0)

    print("\n================================================================================")
    print("RESULTS SUMMARY: Technique #21 (Streaming Attention Sink / KV Compaction)")
    print("================================================================================")
    print(f"Arm A (Baseline, No Sinks):       {mu_a_ret:.1f} ± {sigma_a_ret:.1f}% retention ({mu_a_lat:.2f} us/cycle)")
    print(f"Arm B (Test, 4 Sinks Preserved):  {mu_b_ret:.1f} ± {sigma_b_ret:.1f}% retention ({mu_b_lat:.2f} us/cycle)")
    print(f"Retention Delta:                  +{delta_ret:.1f}% (strictly 4 sinks preserved)")
    print(f"Combined 2-sigma Threshold:       {two_sigma_ret:.2f}%")
    print(f"Statistical Significance:         {delta_ret / two_sigma_ret:.2f}x threshold")
    verdict = "PROMOTED TO PROVEN" if passed else "KEPT PARTIAL (Inside Noise)"
    print(f"VERDICT:                          {verdict}")
    print("================================================================================")

    result_data = {
        "technique_id": 21,
        "name": "Streaming Attention Sink / Sliding-Window KV Compaction (`--sink-tokens`)",
        "arm_a_retention_mu": mu_a_ret,
        "arm_a_retention_sigma": sigma_a_ret,
        "arm_b_retention_mu": mu_b_ret,
        "arm_b_retention_sigma": sigma_b_ret,
        "delta_retention_pct": delta_ret,
        "arm_a_latency_us": mu_a_lat,
        "arm_b_latency_us": mu_b_lat,
        "sigma_comb": sigma_comb_ret,
        "two_sigma_threshold": two_sigma_ret,
        "passed": passed,
        "verdict": verdict
    }

    out_path = os.path.join(SCRIPT_DIR, "result_sink_tokens_21.json")
    with open(out_path, "w") as f:
        json.dump(result_data, f, indent=2)

    return 0 if passed else 1

if __name__ == "__main__":
    sys.exit(main())
