#!/usr/bin/env python3
"""
Technique #56 Benchmark: Speculative Batch Cascade Verification (`--spec-cascade`)
A/B interleaved benchmark measuring token generation throughput and decode acceleration:
- Arm A (Baseline): Standard autoregressive token verification (--spec-type none)
- Arm B (Test):     Speculative batch cascade verification (--spec-type ngram-simple --draft-max 4)
Protocol: n=5 interleaved iterations, 4s thermal cooling, performance governor.
"""

import subprocess
import time
import json
import math
import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ENGINE_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "../../.."))
LLAMA_CLI = os.path.join(ENGINE_ROOT, "build/bin/llama-cli")
MODEL_PATH = "/home/sandeep/ai-models/dense/NVIDIA-Nemotron-3-Nano-4B-GGUF/NVIDIA-Nemotron-3-Nano-4B-Q6_K.gguf"

PROMPT = (
    "Repeat the lyrics: 1 little, 2 little, 3 little Indians. "
    "4 little, 5 little, 6 little Indians. "
    "7 little, 8 little, 9 little Indians. 10 little"
)

def set_performance():
    try:
        subprocess.run(["powerprofilesctl", "set", "performance"], check=True)
    except Exception as e:
        print(f"Warning: Could not set performance profile: {e}")

def run_arm(speculative: bool):
    cmd = [
        "setsid", LLAMA_CLI,
        "-m", MODEL_PATH,
        "-p", PROMPT,
        "-n", "64",
        "-ngl", "99",
        "-st"
    ]
    if speculative:
        cmd.extend(["--spec-type", "ngram-simple", "--draft-max", "4"])
    else:
        cmd.extend(["--spec-type", "none"])

    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    output = p.stdout + "\n" + p.stderr

    gen_match = re.search(r"Generation:\s*([0-9.]+)\s*t/s", output)
    decode_match = re.search(r"decode:\s*([0-9.]+)s", output)

    if not gen_match:
        raise RuntimeError(f"Could not parse Generation t/s from output:\n{output[-1000:]}")

    gen_tps = float(gen_match.group(1))
    decode_s = float(decode_match.group(1)) if decode_match else 0.0

    return {
        "generation_tps": gen_tps,
        "decode_s": decode_s
    }

def main():
    print("================================================================================")
    print("Technique #56: Speculative Batch Cascade Verification Benchmark (n=5)")
    print("================================================================================")
    set_performance()

    arm_a_tps = []
    arm_b_tps = []

    n = 5
    for i in range(n):
        print(f"\n--- Iteration {i+1}/{n} ---")

        # Arm A (Baseline: No speculative cascade)
        print("  Running Arm A (Baseline: Autoregressive / No Speculative Cascade)...")
        res_a = run_arm(speculative=False)
        tps_a = res_a["generation_tps"]
        arm_a_tps.append(tps_a)
        print(f"    Arm A Generation: {tps_a:.2f} t/s (decode: {res_a['decode_s']:.2f}s)")

        print("  Cooling for 4.0s...")
        time.sleep(4.0)

        # Arm B (Test: Speculative Batch Cascade Verification)
        print("  Running Arm B (Test: Speculative Batch Cascade Verification)...")
        res_b = run_arm(speculative=True)
        tps_b = res_b["generation_tps"]
        arm_b_tps.append(tps_b)
        print(f"    Arm B Generation: {tps_b:.2f} t/s (decode: {res_b['decode_s']:.2f}s)")

        if i < n - 1:
            print("  Cooling for 4.0s...")
            time.sleep(4.0)

    # Statistical Calculations on Generation Throughput
    mu_a = sum(arm_a_tps) / n
    var_a = sum((x - mu_a) ** 2 for x in arm_a_tps) / (n - 1)
    sigma_a = math.sqrt(var_a)

    mu_b = sum(arm_b_tps) / n
    var_b = sum((x - mu_b) ** 2 for x in arm_b_tps) / (n - 1)
    sigma_b = math.sqrt(var_b)

    sigma_comb = math.sqrt(sigma_a ** 2 + sigma_b ** 2)
    two_sigma = 2.0 * sigma_comb
    delta_tps = mu_b - mu_a

    passed = delta_tps > two_sigma

    print("\n================================================================================")
    print("RESULTS SUMMARY: Technique #56 (Speculative Batch Cascade Verification)")
    print("================================================================================")
    print(f"Arm A (Baseline, Autoregressive): {mu_a:.2f} ± {sigma_a:.2f} t/s")
    print(f"Arm B (Test, Spec Cascade):       {mu_b:.2f} ± {sigma_b:.2f} t/s")
    print(f"Throughput Delta:                 +{delta_tps:.2f} t/s (+{(delta_tps/mu_a)*100:.2f}%)")
    print(f"Combined 2-sigma Threshold:       {two_sigma:.2f} t/s")
    print(f"Statistical Significance:         {delta_tps / two_sigma:.2f}x threshold")
    verdict = "PROMOTED TO PROVEN" if passed else "KEPT PARTIAL (Inside Noise)"
    print(f"VERDICT:                          {verdict}")
    print("================================================================================")

    result_data = {
        "technique_id": 56,
        "name": "Speculative Batch Cascade Verification (`--spec-cascade`)",
        "arm_a_mu": mu_a,
        "arm_a_sigma": sigma_a,
        "arm_b_mu": mu_b,
        "arm_b_sigma": sigma_b,
        "delta_tps": delta_tps,
        "delta_pct": (delta_tps / mu_a) * 100.0,
        "sigma_comb": sigma_comb,
        "two_sigma_threshold": two_sigma,
        "passed": passed,
        "verdict": verdict
    }

    out_path = os.path.join(SCRIPT_DIR, "result_spec_cascade_56.json")
    with open(out_path, "w") as f:
        json.dump(result_data, f, indent=2)

    return 0 if passed else 1

if __name__ == "__main__":
    sys.exit(main())
