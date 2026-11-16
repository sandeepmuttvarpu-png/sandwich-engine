#!/usr/bin/env python3
"""
Technique #52 Benchmark: Static Graph Hot GEMM Substitution (`--cuda-graph-hot-gemm`)
A/B interleaved benchmark measuring token generation throughput and decode latency:
- Arm A (Baseline): Static CUDA graphs disabled (GGML_CUDA_DISABLE_GRAPHS=1, individual kernel launches)
- Arm B (Test):     Static CUDA graph hot GEMM substitution active (GGML_CUDA_DISABLE_GRAPHS=0 / default)
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

def set_performance():
    try:
        subprocess.run(["powerprofilesctl", "set", "performance"], check=True)
    except Exception as e:
        print(f"Warning: Could not set performance profile: {e}")

def run_arm(graphs_enabled: bool):
    env = os.environ.copy()
    if not graphs_enabled:
        env["GGML_CUDA_DISABLE_GRAPHS"] = "1"
    else:
        env.pop("GGML_CUDA_DISABLE_GRAPHS", None)

    cmd = [
        "setsid", LLAMA_CLI,
        "-m", MODEL_PATH,
        "-p", "Explain quantum computing in detail:",
        "-n", "128",
        "-ngl", "99",
        "-st"
    ]

    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env)
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
    print("Technique #52: Static Graph Hot GEMM Substitution Benchmark (n=5 interleaved)")
    print("================================================================================")
    set_performance()

    arm_a_tps = []
    arm_a_dec = []
    arm_b_tps = []
    arm_b_dec = []

    n = 5
    for i in range(n):
        print(f"\n--- Iteration {i+1}/{n} ---")

        # Arm A (Baseline: Graphs Disabled)
        print("  Running Arm A (Baseline: CUDA Graphs Disabled)...")
        res_a = run_arm(graphs_enabled=False)
        tps_a = res_a["generation_tps"]
        dec_a = res_a["decode_s"]
        arm_a_tps.append(tps_a)
        arm_a_dec.append(dec_a)
        print(f"    Arm A Generation: {tps_a:.2f} t/s (decode: {dec_a:.2f}s)")

        print("  Cooling for 4.0s...")
        time.sleep(4.0)

        # Arm B (Test: Graphs Enabled)
        print("  Running Arm B (Test: Static CUDA Graph Hot GEMM Enabled)...")
        res_b = run_arm(graphs_enabled=True)
        tps_b = res_b["generation_tps"]
        dec_b = res_b["decode_s"]
        arm_b_tps.append(tps_b)
        arm_b_dec.append(dec_b)
        print(f"    Arm B Generation: {tps_b:.2f} t/s (decode: {dec_b:.2f}s)")

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
    print("RESULTS SUMMARY: Technique #52 (Static Graph Hot GEMM Substitution)")
    print("================================================================================")
    print(f"Arm A (Baseline, Graphs Disabled): {mu_a:.2f} ± {sigma_a:.2f} t/s")
    print(f"Arm B (Test, Graphs Enabled):     {mu_b:.2f} ± {sigma_b:.2f} t/s")
    print(f"Throughput Delta:                 +{delta_tps:.2f} t/s (+{(delta_tps/mu_a)*100:.2f}%)")
    print(f"Combined 2-sigma Threshold:       {two_sigma:.2f} t/s")
    print(f"Statistical Significance:         {delta_tps / two_sigma:.2f}x threshold")
    verdict = "PROMOTED TO PROVEN" if passed else "KEPT PARTIAL (Inside Noise)"
    print(f"VERDICT:                          {verdict}")
    print("================================================================================")

    result_data = {
        "technique_id": 52,
        "name": "Static Graph Hot GEMM Substitution (`--cuda-graph-hot-gemm`)",
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

    out_path = os.path.join(SCRIPT_DIR, "result_cuda_graph_52.json")
    with open(out_path, "w") as f:
        json.dump(result_data, f, indent=2)

    return 0 if passed else 1

if __name__ == "__main__":
    sys.exit(main())
