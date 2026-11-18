#!/usr/bin/env python3
"""
verify_generation_parity.py
===========================
Executes deterministic canonical generation test on target model using llama-cli
and verifies bit-identical parity against canonical reference hashes:
  - Pure Token Text SHA256 : 481b2a7aea43731e77979bca6631cfac9d32bd42d023d63c91552b245931a6aa
  - Full Output SHA256     : 77e2b15be215db920cd2bb86ece4bea713f42954e302c2e396bb26239f12d74b (build-banner normalized)
"""

import os
import sys
import re
import subprocess
import hashlib
import argparse

CANONICAL_PURE_SHA = "481b2a7aea43731e77979bca6631cfac9d32bd42d023d63c91552b245931a6aa"
CANONICAL_FULL_SHA = "77e2b15be215db920cd2bb86ece4bea713f42954e302c2e396bb26239f12d74b"
DEFAULT_PROMPT = "/home/sandeep/dev/the-mds/audit-20261112-131000/sandwich_bench_prompt.txt"

def run_parity_check(bin_path, model_path, prompt_file=DEFAULT_PROMPT, ngl=14, threads=6, ctx=4096, n_tokens=64, seed=42):
    if not os.path.exists(bin_path):
        print(f"Error: binary not found at {bin_path}")
        return False

    if not os.path.exists(model_path):
        print(f"Error: model not found at {model_path}")
        return False

    if not os.path.exists(prompt_file):
        print(f"Error: prompt file not found at {prompt_file}")
        return False

    env = os.environ.copy()
    env["GGML_CUDA_PDL"] = "0"

    cmd = [
        bin_path,
        "-m", model_path,
        "-ngl", str(ngl),
        "-t", str(threads),
        "-c", str(ctx),
        "-n", str(n_tokens),
        "--ignore-eos",
        "-f", prompt_file,
        "--temp", "0",
        "--seed", str(seed),
        "--no-warmup",
        "--simple-io"
    ]

    print(f"Executing canonical command:\n{' '.join(cmd)}")
    result = subprocess.run(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env)
    if result.returncode != 0:
        print(f"Execution failed with code {result.returncode}:\n{result.stderr}")
        return False

    stdout = result.stdout
    start_idx = stdout.find("[Start thinking]")
    if start_idx == -1:
        print("Error: Could not locate start of generation token sequence [Start thinking]")
        return False

    end_idx = stdout.find("[ Prompt:", start_idx)
    if end_idx == -1:
        end_idx = stdout.find("=======================================================", start_idx)
    
    gen_text = stdout[start_idx:end_idx].strip() if end_idx != -1 else stdout[start_idx:].strip()
    pure_sha = hashlib.sha256(gen_text.encode("utf-8")).hexdigest()

    # Full output reconciliation
    raw = stdout[stdout.find("Loading model..."):]
    trimmed_full = re.sub(r"\[ Prompt: [\d\.]+ t/s \| Generation: [\d\.]+ t/s \][\s\S]*", "", raw).strip()
    # Normalize build banner to canonical reference build (b12409-fe76f62b8)
    normalized_full = re.sub(r"build\s*:\s*[^\n]+", "build      : b12409-fe76f62b8", trimmed_full)
    full_sha = hashlib.sha256(normalized_full.encode("utf-8")).hexdigest()

    print("\n--- Verified Generated Text ---")
    print(gen_text)
    print("--------------------------------\n")
    print(f"Pure Token Text SHA256 : {pure_sha}")
    print(f"Expected Pure SHA256   : {CANONICAL_PURE_SHA}")
    print(f"Pure Token Match       : {pure_sha == CANONICAL_PURE_SHA}")
    print(f"\nNormalized Full SHA256 : {full_sha}")
    print(f"Expected Full SHA256   : {CANONICAL_FULL_SHA}")
    print(f"Full Output Match      : {full_sha == CANONICAL_FULL_SHA}")

    if pure_sha == CANONICAL_PURE_SHA and full_sha == CANONICAL_FULL_SHA:
        print("\n✅ BIT-IDENTICAL CANONICAL PARITY CONFIRMED (100% Match).")
        return True
    else:
        print("\n❌ CANONICAL PARITY MISMATCH DETECTED.")
        return False

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Verify generation parity against canonical reference.")
    parser.add_argument("bin_path", help="Path to llama-cli binary")
    parser.add_argument("model_path", help="Path to target GGUF model")
    parser.add_argument("-f", "--prompt-file", default=DEFAULT_PROMPT, help="Path to prompt file")
    args = parser.parse_args()

    ok = run_parity_check(args.bin_path, args.model_path, prompt_file=args.prompt_file)
    sys.exit(0 if ok else 1)
