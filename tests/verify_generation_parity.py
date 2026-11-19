#!/usr/bin/env python3
"""
verify_generation_parity.py
===========================
Executes deterministic canonical generation test on target model using llama-cli
and verifies bit-identical parity against canonical reference hashes:
  - Canonical Pure Token Text SHA256 : 481b2a7aea43731e77979bca6631cfac9d32bd42d023d63c91552b245931a6aa
  - Full Output SHA256 (Baseline)    : 77e2b15be215db920cd2bb86ece4bea713f42954e302c2e396bb26239f12d74b (build-banner normalized)

Supports two execution stacks:
  1. baseline: Upstream reference configuration (FP16 KV, no drafter, no MoE cache, historical baseline)
  2. c4: Full C4 stack (-ctk turbo4 -ctv turbo4 --turbo-kvran -fa -md <DFlash2> -ngld 0 --moe-cache-slots 32 --moe-warm-ram-mb 12500 --io-uring --moe-2q --moe-exchange-rot)

Monitors:
  - Pure Token Text SHA256 (Canonical invariant)
  - fattn_native_count (> 0 for C4 quantized KV attention)
  - Peak GPU VRAM (via nvidia-smi telemetry)
"""

import os
import sys
import re
import time
import subprocess
import threading
import hashlib
import argparse
import json
import ctypes

CANONICAL_PURE_SHA = "481b2a7aea43731e77979bca6631cfac9d32bd42d023d63c91552b245931a6aa"
CANONICAL_FULL_SHA_BASELINE = "77e2b15be215db920cd2bb86ece4bea713f42954e302c2e396bb26239f12d74b"
DEFAULT_PROMPT = "/home/sandeep/dev/the-mds/audit-20261112-131000/sandwich_bench_prompt.txt"
DEFAULT_DRAFTER = "/home/sandeep/ai-models/drafters/Qwen3.6-35B-A3B-DFlash2-GGUF/Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf"

def get_cuda_vram_used_mib():
    try:
        cuda = ctypes.CDLL('libcudart.so')
        free = ctypes.c_size_t()
        total = ctypes.c_size_t()
        if cuda.cudaMemGetInfo(ctypes.byref(free), ctypes.byref(total)) == 0:
            return int((total.value - free.value) // (1024 * 1024))
    except Exception:
        pass
    try:
        out = subprocess.check_output(
            ["nvidia-smi", "--query-gpu=memory.used", "--format=csv,noheader,nounits"],
            stderr=subprocess.DEVNULL,
            text=True
        )
        return int(out.strip().splitlines()[0])
    except Exception:
        pass
    return 0

class VRAMSampler:
    def __init__(self, interval_sec=0.05):
        self.interval = interval_sec
        self.peak_vram_mib = 0
        self._running = False
        self._thread = None

    def _sample_once(self):
        val = get_cuda_vram_used_mib()
        if val > self.peak_vram_mib:
            self.peak_vram_mib = val

    def _sample_loop(self):
        while self._running:
            self._sample_once()
            time.sleep(self.interval)

    def start(self):
        self.peak_vram_mib = 0
        self._sample_once()
        self._running = True
        self._thread = threading.Thread(target=self._sample_loop, daemon=True)
        self._thread.start()

    def stop(self):
        self._running = False
        if self._thread:
            self._thread.join(timeout=1.0)
        self._sample_once()
        return self.peak_vram_mib

def run_parity_check(bin_path, model_path, stack="baseline", prompt_file=DEFAULT_PROMPT, drafter_path=DEFAULT_DRAFTER,
                     ngl=14, threads=6, ctx=4096, n_tokens=64, seed=42):
    if not os.path.exists(bin_path):
        print(f"Error: binary not found at {bin_path}")
        return False, {}

    if not os.path.exists(model_path):
        print(f"Error: model not found at {model_path}")
        return False, {}

    if not os.path.exists(prompt_file):
        print(f"Error: prompt file not found at {prompt_file}")
        return False, {}

    env = os.environ.copy()
    env["GGML_CUDA_PDL"] = "0"
    env["SANDWICH_LOG_FATTN_COUNT"] = "1"

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

    if stack == "c4":
        if not os.path.exists(drafter_path):
            print(f"Error: drafter model not found at {drafter_path}")
            return False, {}
        cmd.extend([
            "-ctk", "turbo4",
            "-ctv", "turbo4",
            "--turbo-kvran",
            "-fa",
            "-md", drafter_path,
            "-ngld", "0",
            "--moe-cache-slots", "32",
            "--moe-warm-ram-mb", "12500",
            "--io-uring",
            "--moe-2q",
            "--moe-exchange-rot"
        ])

    print("=" * 80)
    print(f" GENERATION PARITY TEST: Stack = {stack.upper()}")
    print("=" * 80)
    print(f"Executing command:\n{' '.join(cmd)}\n")

    vram_sampler = VRAMSampler(interval_sec=0.05)
    vram_sampler.start()

    start_time = time.time()
    result = subprocess.run(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env)
    elapsed_sec = time.time() - start_time

    peak_vram = vram_sampler.stop()

    if result.returncode != 0:
        print(f"Execution failed with code {result.returncode}:\n{result.stderr}")
        return False, {}

    stdout = result.stdout
    stderr = result.stderr

    # Extract fattn_native_count
    fattn_native_count = 0
    m_native = re.search(r'\[FATTN_TELEMETRY\]\s+fattn_native_count\s*=\s*(\d+)', stdout + "\n" + stderr)
    if m_native:
        fattn_native_count = int(m_native.group(1))

    # Locate generated token text
    start_idx = stdout.find("[Start thinking]")
    if start_idx == -1:
        # Fallback to general generation output
        start_idx = stdout.find("========== Output ==========")
        if start_idx == -1:
            start_idx = 0

    end_idx = stdout.find("[ Prompt:", start_idx)
    if end_idx == -1:
        end_idx = stdout.find("=======================================================", start_idx)

    gen_text = stdout[start_idx:end_idx].strip() if end_idx != -1 else stdout[start_idx:].strip()
    pure_sha = hashlib.sha256(gen_text.encode("utf-8")).hexdigest()

    # Build-banner normalized full output sha
    raw = stdout[stdout.find("Loading model..."):] if "Loading model..." in stdout else stdout
    trimmed_full = re.sub(r"\[ Prompt: [\d\.]+ t/s \| Generation: [\d\.]+ t/s \][\s\S]*", "", raw).strip()
    normalized_full = re.sub(r"build\s*:\s*[^\n]+", "build      : b12409-fe76f62b8", trimmed_full)
    full_sha = hashlib.sha256(normalized_full.encode("utf-8")).hexdigest()

    # Telemetry report
    print("\n--- Verified Generated Text ---")
    print(gen_text)
    print("--------------------------------\n")
    print(f"Execution Elapsed Time   : {elapsed_sec:.2f} s")
    print(f"Peak VRAM                : {peak_vram} MiB")
    print(f"fattn_native_count       : {fattn_native_count}")
    print(f"Pure Token Text SHA256   : {pure_sha}")
    print(f"Canonical Pure SHA256    : {CANONICAL_PURE_SHA}")
    print(f"Pure Token SHA Match     : {pure_sha == CANONICAL_PURE_SHA}")

    if stack == "baseline":
        print(f"Normalized Full SHA256   : {full_sha}")
        print(f"Expected Baseline Full   : {CANONICAL_FULL_SHA_BASELINE}")
        print(f"Baseline Full SHA Match  : {full_sha == CANONICAL_FULL_SHA_BASELINE}")

    report = {
        "stack": stack,
        "elapsed_sec": elapsed_sec,
        "peak_vram_mib": peak_vram,
        "fattn_native_count": fattn_native_count,
        "pure_token_sha256": pure_sha,
        "canonical_pure_sha256": CANONICAL_PURE_SHA,
        "pure_token_match": (pure_sha == CANONICAL_PURE_SHA),
        "full_output_sha256": full_sha,
        "gen_text": gen_text
    }

    if stack == "c4":
        # Parity assertion for C4 stack
        c4_valid = (pure_sha == CANONICAL_PURE_SHA)
        print(f"\n[C4 STACK PARITY ASSERTION] pure_token_match: {'PASSED' if c4_valid else 'FAILED'}")
        print(f"[C4 STACK TELEMETRY] fattn_native_count = {fattn_native_count}")
        return c4_valid, report
    else:
        baseline_valid = (pure_sha == CANONICAL_PURE_SHA and full_sha == CANONICAL_FULL_SHA_BASELINE)
        print(f"\n[BASELINE PARITY ASSERTION] match: {'PASSED' if baseline_valid else 'FAILED'}")
        return baseline_valid, report

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Verify generation parity against canonical reference.")
    parser.add_argument("bin_path", help="Path to llama-cli binary")
    parser.add_argument("model_path", help="Path to target GGUF model")
    parser.add_argument("--stack", choices=["baseline", "c4", "both"], default="both", help="Which execution stack to test")
    parser.add_argument("-f", "--prompt-file", default=DEFAULT_PROMPT, help="Path to prompt file")
    parser.add_argument("-md", "--drafter-path", default=DEFAULT_DRAFTER, help="Path to drafter model")
    args = parser.parse_args()

    results = {}
    all_ok = True

    stacks_to_run = ["baseline", "c4"] if args.stack == "both" else [args.stack]

    for st in stacks_to_run:
        ok, rep = run_parity_check(args.bin_path, args.model_path, stack=st,
                                  prompt_file=args.prompt_file, drafter_path=args.drafter_path)
        results[st] = rep
        if not ok:
            all_ok = False

    out_file = "/home/sandeep/dev/sandwich/sandwich-engine/tests/diagnostics/generation_parity_report.json"
    with open(out_file, "w") as f:
        json.dump(results, f, indent=2)
    print(f"\n[REPORT] Saved generation parity telemetry to: {out_file}")

    sys.exit(0 if all_ok else 1)
