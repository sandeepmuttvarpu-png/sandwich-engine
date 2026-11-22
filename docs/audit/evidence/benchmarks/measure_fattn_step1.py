#!/usr/bin/env python3
"""
Step 1 Gate: Forensic CudaEvent Measurement Suite for Issue #4
Brackets launch_fattn with cudaEvent on identical KV state, sparse vs dense,
across all 12 active GPU decode layers (28-39).
Supports q1 query counts: q1=1, q1=4, q1=8.
Runs an n=5 interleaved A/B test with 4-second thermal cooling and performance governor.
Measures crossover point to confirm or retract the amortization/scratchpad hypothesis.
"""

import subprocess
import time
import json
import re
import math
import sys
import os
import argparse

BIN_PATH = "/home/sandeep/dev/sandwich/sandwich-engine/tests/benchmarks/measure_fattn_step1"

def set_performance_governor():
    try:
        subprocess.run(["powerprofilesctl", "set", "performance"], check=True)
        print("[SETUP] Pinned power profile to 'performance'.")
    except Exception as e:
        print(f"[WARN] Failed to set powerprofilesctl: {e}")

def run_arm(mode: str, q1: int):
    cmd = [BIN_PATH, f"--{mode}", "--q1", str(q1)]
    env = os.environ.copy()
    proc = subprocess.run(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if proc.returncode != 0:
        raise RuntimeError(f"Run failed (exit {proc.returncode}):\n{proc.stderr}")

    # Parse stdout JSON
    json_match = re.search(r'\{[\s\S]*\}', proc.stdout)
    if not json_match:
        raise ValueError(f"Could not parse JSON from stdout:\n{proc.stdout}")
    data = json.loads(json_match.group(0))

    # Parse stderr for per-layer timings
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

    # The measured run is the last 12 events
    if len(layer_events) >= 12:
        measured_layers = layer_events[-12:]
    else:
        measured_layers = layer_events

    total_batch_ms = data.get("total_batch_kernel_ms", data.get("total_token_kernel_ms", 0.0))
    per_token_ms = data.get("per_token_kernel_ms", total_batch_ms / q1)

    return {
        "mode": mode,
        "q1": q1,
        "total_batch_kernel_ms": total_batch_ms,
        "per_token_kernel_ms": per_token_ms,
        "avg_layer_kernel_ms": data.get("avg_layer_kernel_ms", total_batch_ms / 12.0),
        "layers": measured_layers
    }

def mean(arr):
    return sum(arr) / len(arr) if arr else 0.0

def std(arr):
    if len(arr) <= 1:
        return 0.0
    m = mean(arr)
    return math.sqrt(sum((x - m) ** 2 for x in arr) / (len(arr) - 1))

def benchmark_q1(q1: int, n: int = 5):
    print("\n" + "=" * 80)
    print(f" BENCHMARK RUN: q1 = {q1} (n={n} Interleaved A/B)")
    print(f" Target: Sparse MMA Fast-Path vs Dense MMA Fallback (12 GPU decode layers)")
    print("=" * 80)

    dense_results = []
    sparse_results = []

    for i in range(n):
        print(f"  [q1={q1}] Iteration {i+1}/{n}...")

        # Arm A: Dense
        print(f"    Arm A (Dense): cooling (4s)...")
        time.sleep(4)
        res_a = run_arm("dense", q1)
        dense_results.append(res_a)
        print(f"    -> Dense total: {res_a['total_batch_kernel_ms']:.4f} ms | per-token: {res_a['per_token_kernel_ms']:.4f} ms")

        # Arm B: Sparse
        print(f"    Arm B (Sparse): cooling (4s)...")
        time.sleep(4)
        res_b = run_arm("sparse", q1)
        sparse_results.append(res_b)
        print(f"    -> Sparse total: {res_b['total_batch_kernel_ms']:.4f} ms | per-token: {res_b['per_token_kernel_ms']:.4f} ms")

    dense_batch_times = [r["total_batch_kernel_ms"] for r in dense_results]
    sparse_batch_times = [r["total_batch_kernel_ms"] for r in sparse_results]

    dense_token_times = [r["per_token_kernel_ms"] for r in dense_results]
    sparse_token_times = [r["per_token_kernel_ms"] for r in sparse_results]

    mu_dense_batch = mean(dense_batch_times)
    std_dense_batch = std(dense_batch_times)
    mu_sparse_batch = mean(sparse_batch_times)
    std_sparse_batch = std(sparse_batch_times)

    mu_dense_token = mean(dense_token_times)
    std_dense_token = std(dense_token_times)
    mu_sparse_token = mean(sparse_token_times)
    std_sparse_token = std(sparse_token_times)

    # Delta = Baseline (Dense) - Test (Sparse). Positive means Sparse is faster.
    delta_batch = mu_dense_batch - mu_sparse_batch
    delta_token = mu_dense_token - mu_sparse_token
    sigma_comb_token = math.sqrt(std_dense_token**2 + std_sparse_token**2)
    two_sigma_token = 2.0 * sigma_comb_token

    is_significant = abs(delta_token) > two_sigma_token
    winner = "Sparse" if delta_token > 0 and is_significant else ("Dense" if delta_token < 0 and is_significant else "Indistinguishable (Inside 2σ Noise)")

    print(f"\n  --- Results for q1 = {q1} ---")
    print(f"  Dense Batch  : {mu_dense_batch:.4f} ± {std_dense_batch:.4f} ms")
    print(f"  Sparse Batch : {mu_sparse_batch:.4f} ± {std_sparse_batch:.4f} ms")
    print(f"  Dense / Token: {mu_dense_token:.4f} ± {std_dense_token:.4f} ms")
    print(f"  Sparse / Token: {mu_sparse_token:.4f} ± {std_sparse_token:.4f} ms")
    print(f"  Delta / Token: {delta_token:+.4f} ms ({winner})")
    print(f"  2σ Threshold : {two_sigma_token:.4f} ms | Significant: {is_significant}")

    return {
        "q1": q1,
        "n": n,
        "mu_dense_batch_ms": mu_dense_batch,
        "std_dense_batch_ms": std_dense_batch,
        "mu_sparse_batch_ms": mu_sparse_batch,
        "std_sparse_batch_ms": std_sparse_batch,
        "mu_dense_per_token_ms": mu_dense_token,
        "std_dense_per_token_ms": std_dense_token,
        "mu_sparse_per_token_ms": mu_sparse_token,
        "std_sparse_per_token_ms": std_sparse_token,
        "delta_per_token_ms": delta_token,
        "sigma_comb_token": sigma_comb_token,
        "two_sigma_token": two_sigma_token,
        "is_significant": is_significant,
        "winner": winner,
        "dense_batch_raw": dense_batch_times,
        "sparse_batch_raw": sparse_batch_times,
    }

def main():
    parser = argparse.ArgumentParser(description="Step 1 Gate Crossover Benchmark")
    parser.add_argument("--q1", type=int, default=None, help="Specific q1 to test (e.g. 1, 4, 8)")
    parser.add_argument("--n", type=int, default=5, help="Number of interleaved iterations")
    parser.add_argument("--sweep", action="store_true", help="Sweep q1 = [1, 4, 8]")
    args = parser.parse_args()

    set_performance_governor()

    q1_values = [1, 4, 8] if (args.sweep or args.q1 is None) else [args.q1]
    all_results = {}

    for q1 in q1_values:
        res = benchmark_q1(q1, n=args.n)
        all_results[f"q1_{q1}"] = res

    print("\n" + "=" * 80)
    print(" CROSSOVER SUMMARY ACROSS q1 ∈ {1, 4, 8}")
    print("=" * 80)
    print(f"{'q1':<6} | {'Dense ms/tok':<14} | {'Sparse ms/tok':<14} | {'Delta ms/tok':<14} | {'2σ Bound':<10} | {'Advantage'}")
    print("-" * 78)
    for q1 in q1_values:
        r = all_results[f"q1_{q1}"]
        print(f"{q1:<6} | {r['mu_dense_per_token_ms']:<14.4f} | {r['mu_sparse_per_token_ms']:<14.4f} | {r['delta_per_token_ms']:<+14.4f} | {r['two_sigma_token']:<10.4f} | {r['winner']}")

    # Save output
    out_file = "/home/sandeep/dev/sandwich/sandwich-engine/tests/benchmarks/measure_fattn_crossover_results.json"
    with open(out_file, "w") as f:
        json.dump(all_results, f, indent=2)
    print(f"\n[REPORT] Saved crossover results to: {out_file}")

if __name__ == "__main__":
    main()
