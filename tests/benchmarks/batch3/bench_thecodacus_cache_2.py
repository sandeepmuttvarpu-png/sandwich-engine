#!/usr/bin/env python3
"""
Technique #2 Benchmark: Thecodacus Dual-Path MoE Expert Cache (`--moe-cache-slots`)
A/B interleaved benchmark measuring Host-Resident MoE cache hot-hit rate under partial offloading:
- Arm A (Baseline): Caching disabled (--moe-cache-slots 0)
- Arm B (Test):     Thecodacus Dual-Path MoE cache enabled (--moe-cache-slots 36)
Protocol: n=5 interleaved iterations, 4s thermal cooling, performance governor.
"""

import json
import math
import os
import re
import subprocess
import sys
import time

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ENGINE_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "../../.."))
LLAMA_CLI = os.path.join(ENGINE_ROOT, "build/bin/llama-cli")
MODEL_PATH = "/home/sandeep/ai-models/moe/DeepSeek-Coder-V2-Lite-Instruct-GGUF/DeepSeek-Coder-V2-Lite-Instruct-IQ3_XXS.gguf"

PROMPT = "Write a python function to compute fibonacci numbers"


def set_performance():
    try:
        subprocess.run(["powerprofilesctl", "set", "performance"], check=True)
    except Exception as e:
        print(f"Warning: Could not set performance profile: {e}")


def run_arm(enable_cache: bool):
    slots = "36" if enable_cache else "0"
    cmd = [
        "setsid",
        LLAMA_CLI,
        "-m",
        MODEL_PATH,
        "-ngl",
        "12",
        "-c",
        "512",
        "-n",
        "32",
        "-p",
        PROMPT,
        "--moe-cache-slots",
        slots,
        "-st",
    ]

    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    output = p.stdout + "\n" + p.stderr

    # Parse telemetry:
    # "Hot-Pack Hit Rate (Host-Resident Layers 13..40):      58.65%"
    # or "Decode Host Hot Hits           :       1795 (60.44%)"
    hit_match = re.search(r"Hot-Pack Hit Rate.*:\s*([0-9.]+)%", output)
    decode_hit_match = re.search(r"Decode Host Hot Hits\s*:\s*\d+\s*\(([0-9.]+)%\)", output)
    gen_match = re.search(r"Generation:\s*([0-9.]+)\s*t/s", output)

    if not hit_match:
        # If cache slots is 0, telemetry may output 0.00%
        hit_rate = 0.0
    else:
        hit_rate = float(hit_match.group(1))

    decode_hit_rate = float(decode_hit_match.group(1)) if decode_hit_match else 0.0
    gen_tps = float(gen_match.group(1)) if gen_match else 0.0

    return {
        "hit_rate": hit_rate,
        "decode_hit_rate": decode_hit_rate,
        "generation_tps": gen_tps,
    }


def main():
    print("================================================================================")
    print("Technique #2: Thecodacus Dual-Path MoE Expert Cache Benchmark (n=5)")
    print("================================================================================")
    set_performance()

    arm_a_hits = []
    arm_b_hits = []
    arm_a_tps = []
    arm_b_tps = []

    n = 5
    for i in range(n):
        print(f"\n--- Iteration {i+1}/{n} ---")

        # Arm A (Baseline: Cache slots = 0)
        print("  Running Arm A (Baseline: --moe-cache-slots 0)...")
        res_a = run_arm(enable_cache=False)
        arm_a_hits.append(res_a["hit_rate"])
        arm_a_tps.append(res_a["generation_tps"])
        print(f"    Arm A Hot Hit Rate: {res_a['hit_rate']:.2f}% (Generation: {res_a['generation_tps']:.2f} t/s)")

        print("  Cooling for 4.0s...")
        time.sleep(4.0)

        # Arm B (Test: Cache slots = 36)
        print("  Running Arm B (Test: --moe-cache-slots 36)...")
        res_b = run_arm(enable_cache=True)
        arm_b_hits.append(res_b["hit_rate"])
        arm_b_tps.append(res_b["generation_tps"])
        print(f"    Arm B Hot Hit Rate: {res_b['hit_rate']:.2f}% (Decode Hits: {res_b['decode_hit_rate']:.2f}%, Generation: {res_b['generation_tps']:.2f} t/s)")

        if i < n - 1:
            print("  Cooling for 4.0s...")
            time.sleep(4.0)

    # Statistical Calculations on Hot Hit Rate
    mu_a = sum(arm_a_hits) / n
    var_a = sum((x - mu_a) ** 2 for x in arm_a_hits) / (n - 1) if n > 1 else 0.0
    sigma_a = math.sqrt(var_a)

    mu_b = sum(arm_b_hits) / n
    var_b = sum((x - mu_b) ** 2 for x in arm_b_hits) / (n - 1) if n > 1 else 0.0
    sigma_b = math.sqrt(var_b)

    sigma_comb = math.sqrt(sigma_a**2 + sigma_b**2)
    two_sigma = 2.0 * sigma_comb
    delta_hits = mu_b - mu_a

    passed = delta_hits > two_sigma

    print("\n================================================================================")
    print("RESULTS SUMMARY: Technique #2 (Thecodacus Dual-Path MoE Expert Cache)")
    print("================================================================================")
    print(f"Arm A (Baseline: slots=0):  {mu_a:.2f} ± {sigma_a:.2f}%")
    print(f"Arm B (Test: slots=36):     {mu_b:.2f} ± {sigma_b:.2f}%")
    print(f"Hot Hit Rate Delta:         +{delta_hits:.2f}%")
    print(f"Combined 2-sigma Threshold: {two_sigma:.2f}%")
    if two_sigma > 0:
        print(f"Statistical Significance:   {delta_hits / two_sigma:.2f}x threshold")
    else:
        print("Statistical Significance:   INF (deterministic difference)")
    verdict = "PROMOTED TO PROVEN" if passed else "KEPT PARTIAL (Inside Noise)"
    print(f"VERDICT:                    {verdict}")
    print("================================================================================")

    result_data = {
        "technique_id": 2,
        "name": "Thecodacus Dual-Path MoE Expert Cache",
        "flag": "--moe-cache-slots",
        "arm_a_runs": arm_a_hits,
        "arm_b_runs": arm_b_hits,
        "arm_a_mu": mu_a,
        "arm_a_sigma": sigma_a,
        "arm_b_mu": mu_b,
        "arm_b_sigma": sigma_b,
        "delta_hits": delta_hits,
        "sigma_comb": sigma_comb,
        "two_sigma_threshold": two_sigma,
        "passed": passed,
        "verdict": verdict,
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    }

    out_path = os.path.join(SCRIPT_DIR, "result_thecodacus_cache_2.json")
    with open(out_path, "w") as f:
        json.dump(result_data, f, indent=2)

    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
