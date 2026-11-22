#!/usr/bin/env python3
"""
Sandwich Engine: Unified Cross-Batch Benchmark Runner & Report Generator.
Aggregates and formats all isolated A/B benchmark results across Batches 1, 2, 3, and 4.
Can also execute benchmarks sequentially with safety cooling and power profile management.
"""

import argparse
import glob
import json
import os
import subprocess
import sys
import time
from pathlib import Path

BENCHMARKS_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = BENCHMARKS_DIR.parents[1]


def collect_results():
    pattern = str(BENCHMARKS_DIR / "batch*" / "result_*.json")
    files = sorted(glob.glob(pattern))
    results = []

    for fpath in files:
        with open(fpath, "r") as f:
            data = json.load(f)
            data["_source_file"] = os.path.relpath(fpath, PROJECT_ROOT)
            data["_batch"] = Path(fpath).parent.name
            results.append(data)

    return results


def format_table(results):
    rows = []
    headers = [
        "Batch",
        "ID(s)",
        "Technique Name",
        "Baseline (μ ± σ)",
        "Test (μ ± σ)",
        "Delta",
        "2σ Gate",
        "Verdict",
    ]

    for r in results:
        batch = r.get("_batch", "N/A")
        if "technique_ids" in r:
            tids = ", ".join(f"#{x}" for x in r["technique_ids"])
            name = " & ".join(r.get("names", []))
        else:
            tids = f"#{r.get('technique_id', 'N/A')}"
            name = r.get("name", "N/A")

        # Handle various metrics (ms, t/s, GB/s, hit-rate, etc.)
        mu_a = r.get("baseline_mu", r.get("arm_a_mu", 0.0))
        sig_a = r.get("baseline_sigma", r.get("arm_a_sigma", 0.0))
        mu_b = r.get("test_mu", r.get("arm_b_mu", 0.0))
        sig_b = r.get("test_sigma", r.get("arm_b_sigma", 0.0))
        two_sig = r.get("two_sigma_threshold", 0.0)
        verdict = r.get("verdict", "N/A")

        # Find delta key
        delta_val = None
        for k in ["delta_ms", "delta_tps", "delta_hit_rate_pct", "delta_rate_pct", "delta_gbps", "delta_overhead_ms"]:
            if k in r:
                delta_val = r[k]
                break
        if delta_val is None:
            delta_val = abs(mu_a - mu_b)

        rows.append([
            batch,
            tids,
            name[:40] + ("..." if len(name) > 40 else ""),
            f"{mu_a:.3f} ± {sig_a:.3f}",
            f"{mu_b:.3f} ± {sig_b:.3f}",
            f"{delta_val:+.3f}",
            f"{two_sig:.3f}",
            verdict,
        ])

    return headers, rows


def print_summary(headers, rows):
    print("=" * 110)
    print("🥪 [PROJECT SANDWICH] UNIFIED CROSS-BATCH BENCHMARK SUMMARY (BATCHES 1 - 4)")
    print("=" * 110)

    # Calculate column widths
    col_widths = [len(h) for h in headers]
    for row in rows:
        for i, val in enumerate(row):
            col_widths[i] = max(col_widths[i], len(str(val)))

    header_line = " | ".join(f"{h:<{col_widths[i]}}" for i, h in enumerate(headers))
    sep_line = "-+-".join("-" * col_widths[i] for i in range(len(headers)))

    print(header_line)
    print(sep_line)
    for row in rows:
        print(" | ".join(f"{str(v):<{col_widths[i]}}" for i, v in enumerate(row)))
    print("=" * 110)


def generate_markdown_report(results, output_path):
    headers, rows = format_table(results)

    lines = [
        "# Sandwich Engine Unified Cross-Batch Benchmark Report",
        "",
        f"**Generated:** {time.strftime('%Y-%m-%d %H:%M:%S UTC', time.gmtime())}",
        f"**Total Benchmark Entries:** {len(results)}",
        "",
        "| " + " | ".join(headers) + " |",
        "| " + " | ".join(["---"] * len(headers)) + " |",
    ]

    for row in rows:
        lines.append("| " + " | ".join(str(v) for v in row) + " |")

    lines.append("")
    with open(output_path, "w") as f:
        f.write("\n".join(lines))
    print(f"[REPORT] Saved Markdown report to {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Cross-Batch Benchmark Aggregator")
    parser.add_argument("--markdown", "-m", type=str, default="", help="Path to write Markdown summary report")
    parser.add_argument("--json", "-j", type=str, default="", help="Path to write JSON aggregate dump")
    args = parser.parse_args()

    results = collect_results()
    headers, rows = format_table(results)
    print_summary(headers, rows)

    if args.markdown:
        generate_markdown_report(results, args.markdown)
    else:
        default_md = BENCHMARKS_DIR / "aggregate_summary.md"
        generate_markdown_report(results, str(default_md))

    if args.json:
        with open(args.json, "w") as f:
            json.dump(results, f, indent=2)
        print(f"[REPORT] Saved JSON dump to {args.json}")


if __name__ == "__main__":
    main()
