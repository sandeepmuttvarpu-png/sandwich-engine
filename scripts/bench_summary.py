#!/usr/bin/env python3
"""
Sandwich Engine — Benchmark Summary & Statistical Analyzer
Parses and formats benchmark throughput runs, calculates warm-4 vs. all-5 means,
computes sample standard deviation, and runs 2σ significance tests.
"""

import sys
import math
import argparse
import json
from typing import List, Dict, Any, Tuple

# ANSI Color formatting
CLR_RESET   = "\033[0m"
CLR_BOLD    = "\033[1m"
CLR_CYAN    = "\033[36m"
CLR_GREEN   = "\033[32m"
CLR_YELLOW  = "\033[33m"
CLR_RED     = "\033[31m"
CLR_MAGENTA = "\033[35m"
CLR_DIM     = "\033[2m"

def mean(values: List[float]) -> float:
    return sum(values) / len(values) if values else 0.0

def stddev(values: List[float]) -> float:
    if len(values) < 2:
        return 0.0
    m = mean(values)
    variance = sum((x - m) ** 2 for x in values) / (len(values) - 1)
    return math.sqrt(variance)

def format_row(cols: List[str], widths: List[int]) -> str:
    res = []
    for c, w in zip(cols, widths):
        res.append(c.ljust(w))
    return " │ ".join(res)

def print_table(headers: List[str], rows: List[List[str]], title: str = ""):
    widths = [len(h) for h in headers]
    for row in rows:
        for i, val in enumerate(row):
            # Strip ANSI escape codes for width calculation
            clean_val = ""
            in_esc = False
            for ch in val:
                if ch == "\033":
                    in_esc = True
                elif in_esc:
                    if ch == "m":
                        in_esc = False
                else:
                    clean_val += ch
            widths[i] = max(widths[i], len(clean_val))

    total_w = sum(widths) + 3 * (len(headers) - 1)
    if title:
        print(f"\n{CLR_BOLD}{CLR_CYAN}=== {title} ==={CLR_RESET}")
    print("┌" + "─" * (total_w + 2) + "┐")
    print("│ " + format_row([f"{CLR_BOLD}{h}{CLR_RESET}" for h in headers], widths) + " │")
    print("├" + "─" * (total_w + 2) + "┤")
    for r in rows:
        print("│ " + format_row(r, widths) + " │")
    print("└" + "─" * (total_w + 2) + "┘")

def analyze_runs(arm_name: str, throughputs: List[float], times: List[float]) -> Dict[str, Any]:
    n = len(throughputs)
    all_t_mean = mean(throughputs)
    all_t_std = stddev(throughputs)
    all_time_mean = mean(times)
    all_time_std = stddev(times)

    warm_t = throughputs[1:] if n > 1 else throughputs
    warm_times = times[1:] if n > 1 else times

    warm_t_mean = mean(warm_t)
    warm_t_std = stddev(warm_t)
    warm_time_mean = mean(warm_times)
    warm_time_std = stddev(warm_times)

    cold_delta_pct = ((warm_t_mean - throughputs[0]) / warm_t_mean * 100.0) if warm_t_mean > 0 else 0.0

    return {
        "arm": arm_name,
        "n_all": n,
        "all_t_mean": all_t_mean,
        "all_t_std": all_t_std,
        "all_time_mean": all_time_mean,
        "all_time_std": all_time_std,
        "warm_t_mean": warm_t_mean,
        "warm_t_std": warm_t_std,
        "warm_time_mean": warm_time_mean,
        "warm_time_std": warm_time_std,
        "cold_first_t": throughputs[0] if throughputs else 0.0,
        "cold_delta_pct": cold_delta_pct,
    }

def print_comparison(arm_a: Dict[str, Any], arm_b: Dict[str, Any]):
    headers = ["Metric / Basis", f"Arm A ({arm_a['arm']})", f"Arm B ({arm_b['arm']})", "Delta / Signif."]
    rows = []

    # 1. Warm-4 Basis
    dt_warm = arm_b["warm_t_mean"] - arm_a["warm_t_mean"]
    sigma_comb_warm = math.sqrt(arm_a["warm_t_std"]**2 + arm_b["warm_t_std"]**2)
    sig_warm = "Indistinguishable" if abs(dt_warm) < (2.0 * sigma_comb_warm) else f"{CLR_GREEN}Significant{CLR_RESET}"

    rows.append([
        "Warm-4 Throughput (tok/s)",
        f"{arm_a['warm_t_mean']:.2f} ± {arm_a['warm_t_std']:.2f}",
        f"{arm_b['warm_t_mean']:.2f} ± {arm_b['warm_t_std']:.2f}",
        f"Δ = {dt_warm:+.2f} ({sig_warm})"
    ])

    # 2. Warm-4 Wall Time
    dtime_warm = (arm_b["warm_time_mean"] - arm_a["warm_time_mean"]) * 1000.0
    sigma_time_warm = math.sqrt(arm_a["warm_time_std"]**2 + arm_b["warm_time_std"]**2) * 1000.0
    sig_time_warm = "Indistinguishable" if abs(dtime_warm) < (2.0 * sigma_time_warm) else f"{CLR_GREEN}Significant{CLR_RESET}"

    rows.append([
        "Warm-4 Wall Time (s)",
        f"{arm_a['warm_time_mean']:.4f} s",
        f"{arm_b['warm_time_mean']:.4f} s",
        f"Δ = {dtime_warm:+.1f} ms ({sig_time_warm})"
    ])

    # 3. All-5 Basis
    dt_all = arm_b["all_t_mean"] - arm_a["all_t_mean"]
    rows.append([
        "All-5 Throughput (tok/s)",
        f"{arm_a['all_t_mean']:.2f} ± {arm_a['all_t_std']:.2f}",
        f"{arm_b['all_t_mean']:.2f} ± {arm_b['all_t_std']:.2f}",
        f"Δ = {dt_all:+.2f}"
    ])

    # 4. Cold Start Penalty
    rows.append([
        "Cold Start 1st Gen Penalty",
        f"{arm_a['cold_delta_pct']:.1f}% ({arm_a['cold_first_t']:.1f} tok/s)",
        f"{arm_b['cold_delta_pct']:.1f}% ({arm_b['cold_first_t']:.1f} tok/s)",
        "Expected cache warm-up"
    ])

    print_table(headers, rows, title="Sandwich Engine — Statistical Arm Comparison")

def main():
    parser = argparse.ArgumentParser(description="Sandwich Engine Benchmark Summary & Analysis Tool")
    parser.add_argument("--demo", action="store_true", help="Run with validated forensic audit baseline data")
    parser.add_argument("--file", type=str, help="Load benchmark JSON runs file")
    args = parser.parse_args()

    print(f"{CLR_BOLD}🥪 Sandwich Engine Benchmark Analyzer v1.0{CLR_RESET}")
    print(f"Target Hardware: HP Victus 15 | Intel i7-13620H (6P+4E) | 24GB DDR5 | RTX 5050 Mobile 8GB")

    if args.file:
        with open(args.file, "r") as f:
            data = json.load(f)
        # Parse data
        pass
    else:
        # Default validated forensic dataset (Arm A: Prior / Clean, Arm B: Speculative / Turbo)
        # Arm A: [23.2, 24.6, 24.3, 24.3, 24.2], times: [2.72, 2.56, 2.59, 2.59, 2.61]
        arm_a = analyze_runs("Dense Baseline", [23.2, 24.6, 24.3, 24.3, 24.2], [2.72, 2.56, 2.59, 2.59, 2.61])
        arm_b = analyze_runs("Turbo4 / DFlash2", [23.4, 24.8, 24.5, 24.4, 24.4], [2.70, 2.54, 2.57, 2.58, 2.59])
        print_comparison(arm_a, arm_b)

if __name__ == "__main__":
    main()
