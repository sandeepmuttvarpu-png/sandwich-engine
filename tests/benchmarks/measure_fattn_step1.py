#!/usr/bin/env python3
"""
Step 1 Gate: Forensic CudaEvent Measurement Suite for Issue #4
Brackets launch_fattn with cudaEvent on identical KV state, sparse vs dense,
across all 12 active GPU decode layers (28-39).
Runs an n=5 interleaved A/B test with 4-second thermal cooling and performance governor.
Compares measured Δ per token against the implied 3.49 ms/token (220 ms / 63).
"""

import subprocess
import time
import json
import re
import math
import sys
import os

BIN_PATH = "/home/sandeep/dev/sandwich/sandwich-engine/tests/benchmarks/measure_fattn_step1"

def set_performance_governor():
    try:
        subprocess.run(["powerprofilesctl", "set", "performance"], check=True)
        print("[SETUP] Pinned power profile to 'performance'.")
    except Exception as e:
        print(f"[WARN] Failed to set powerprofilesctl: {e}")

def run_arm(mode: str):
    cmd = [BIN_PATH, f"--{mode}"]
    env = os.environ.copy()
    proc = subprocess.run(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if proc.returncode != 0:
        raise RuntimeError(f"Run failed (exit {proc.returncode}):\n{proc.stderr}")

    # Parse stdout JSON
    json_match = re.search(r'\{[\s\S]*\}', proc.stdout)
    if not json_match:
        raise ValueError(f"Could not parse JSON from stdout:\n{proc.stdout}")
    data = json.loads(json_match.group(0))

    # Parse stderr for per-layer timings (last 12 events)
    layer_events = []
    pattern = re.compile(r'\[FATTN_CUDAEVENT\]\s+layer=(\w+)\s+q1=(\d+)\s+n_kv=(\d+)\s+sparse=(\d+)\s+ms=([0-9.]+)')
    for line in proc.stderr.splitlines():
        m = pattern.search(line)
        if m:
            layer_events.append({
                "layer": m.group(1),
                "q1": int(m.group(2)),
                "n_kv": int(m.group(3)),
                "sparse": int(m.group(4)),
                "ms": float(m.group(5)),
            })

    # The measured run is the last 12 events (after 2 warm-up runs of 12 events each)
    if len(layer_events) >= 12:
        measured_layers = layer_events[-12:]
    else:
        measured_layers = layer_events

    return {
        "mode": mode,
        "total_token_kernel_ms": data["total_token_kernel_ms"],
        "avg_layer_kernel_ms": data["avg_layer_kernel_ms"],
        "layers": measured_layers
    }

def mean(arr):
    return sum(arr) / len(arr) if arr else 0.0

def std(arr):
    if len(arr) <= 1:
        return 0.0
    m = mean(arr)
    return math.sqrt(sum((x - m) ** 2 for x in arr) / (len(arr) - 1))

def main():
    print("=" * 80)
    print(" STEP 1 GATE: CUDAEVENT BRACKET BENCHMARK (n=5 Interleaved A/B)")
    print(" Target: Issue #4 - Sparse vs Dense FlashAttention on 12 GPU Layers (28-39)")
    print(" Metric: Kernel Latency per token (ms) and per layer (ms)")
    print(" Baseline (Arm A): Dense Fallback (--dense)")
    print(" Test     (Arm B): Sparse Fast Path (--sparse)")
    print(" Enforcing: powerprofilesctl=performance, 4s thermal cooling")
    print("=" * 80)

    set_performance_governor()

    n = 5
    dense_results = []
    sparse_results = []

    for i in range(n):
        print(f"\n--- Iteration {i+1} / {n} ---")

        # Arm A: Dense
        print("[ARM A: Dense] Thermal cooling (4s)...")
        time.sleep(4)
        res_a = run_arm("dense")
        dense_results.append(res_a)
        print(f"  -> Total token ms: {res_a['total_token_kernel_ms']:.4f} ms | Avg layer ms: {res_a['avg_layer_kernel_ms']:.4f} ms")

        # Arm B: Sparse
        print("[ARM B: Sparse] Thermal cooling (4s)...")
        time.sleep(4)
        res_b = run_arm("sparse")
        sparse_results.append(res_b)
        print(f"  -> Total token ms: {res_b['total_token_kernel_ms']:.4f} ms | Avg layer ms: {res_b['avg_layer_kernel_ms']:.4f} ms")

    # Statistical Evaluation
    dense_totals = [r["total_token_kernel_ms"] for r in dense_results]
    sparse_totals = [r["total_token_kernel_ms"] for r in sparse_results]

    mu_dense = mean(dense_totals)
    std_dense = std(dense_totals)

    mu_sparse = mean(sparse_totals)
    std_sparse = std(sparse_totals)

    # Note: If dense is slower than sparse, delta = mu_dense - mu_sparse is positive (savings from sparse).
    # If delta is negative, dense is actually faster or identical.
    delta_token = mu_dense - mu_sparse
    sigma_comb = math.sqrt(std_dense**2 + std_sparse**2)
    two_sigma = 2.0 * sigma_comb

    print("\n" + "=" * 80)
    print(" PER-LAYER BREAKDOWN ACROSS 12 GPU DECODE LAYERS (28 - 39)")
    print("=" * 80)
    print(f"{'Layer':<12} | {'Dense Mean (ms)':<16} | {'Sparse Mean (ms)':<16} | {'Delta (ms)':<12}")
    print("-" * 62)

    layer_names = [r["layer"] for r in dense_results[0]["layers"]]
    for idx, l_name in enumerate(layer_names):
        l_dense_vals = [r["layers"][idx]["ms"] for r in dense_results if idx < len(r["layers"])]
        l_sparse_vals = [r["layers"][idx]["ms"] for r in sparse_results if idx < len(r["layers"])]
        m_d = mean(l_dense_vals)
        m_s = mean(l_sparse_vals)
        d_l = m_d - m_s
        print(f"{l_name:<12} | {m_d:<16.4f} | {m_s:<16.4f} | {d_l:<+12.4f}")

    print("\n" + "=" * 80)
    print(" STATISTICAL SUMMARY (Total 12-Layer Token Kernel Latency)")
    print("=" * 80)
    print(f"Dense (Baseline) : {mu_dense:.4f} ± {std_dense:.4f} ms")
    print(f"Sparse (Fast-Path): {mu_sparse:.4f} ± {std_sparse:.4f} ms")
    print(f"Measured Delta   : {delta_token:+.4f} ms per token")
    print(f"Sigma Combined   : {sigma_comb:.4f} ms")
    print(f"2-Sigma Threshold: {two_sigma:.4f} ms")
    print(f"|Delta| > 2-Sigma : {'YES' if abs(delta_token) > two_sigma else 'NO (Inside Noise)'}")

    # Comparison against the Implied 3.49 ms/token
    IMPLIED_DELTA = 3.492  # 220 ms / 63 tokens
    print("\n" + "=" * 80)
    print(" COMPARISON AGAINST IMPLIED REGRESSION (220 ms / 63 = 3.492 ms/token)")
    print("=" * 80)
    print(f"Implied Delta per token : {IMPLIED_DELTA:.4f} ms/token (from 220 ms over 63 tokens)")
    print(f"Measured Delta per token: {delta_token:+.4f} ms/token (exact cudaEvent bracket)")
    discrepancy = abs(delta_token - IMPLIED_DELTA)
    ratio = (delta_token / IMPLIED_DELTA) * 100.0 if delta_token != 0 else 0.0

    print(f"Difference              : {discrepancy:.4f} ms/token")
    print(f"Kernel Share of Delta   : {ratio:.1f}%")

    print("\n[VERDICT & GATE RESOLUTION]")
    if abs(delta_token - IMPLIED_DELTA) < 0.5:
        print("  -> Measured Δ ≈ 3.49 ms: Sparse restoration is the primary root cause.")
        print("  -> Proceed to Step 2.")
    else:
        print("  -> Measured Δ is MATERIALLY SMALLER than the implied 3.49 ms/token.")
        print(f"     Specifically, the isolated GPU kernel difference accounts for {delta_token:+.4f} ms/token,")
        print(f"     leaving {IMPLIED_DELTA - delta_token:.4f} ms/token to HOST-SIDE / OCCUPANCY / GRAPH OVERHEAD.")
        print("  -> The 'implied average' label is FORMALLY RETIRED with empirical measurement.")

    report_path = "/home/sandeep/dev/sandwich/sandwich-engine/tests/benchmarks/measure_fattn_step1_results.json"
    with open(report_path, "w") as f:
        json.dump({
            "mu_dense": mu_dense,
            "std_dense": std_dense,
            "mu_sparse": mu_sparse,
            "std_sparse": std_sparse,
            "delta_token": delta_token,
            "sigma_comb": sigma_comb,
            "two_sigma": two_sigma,
            "implied_delta": IMPLIED_DELTA,
            "kernel_share_pct": ratio,
            "n": n,
            "dense_totals": dense_totals,
            "sparse_totals": sparse_totals
        }, f, indent=2)
    print(f"\n[REPORT] Saved full JSON results to: {report_path}")

if __name__ == "__main__":
    main()
