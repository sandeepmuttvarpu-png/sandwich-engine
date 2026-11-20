#!/usr/bin/env python3
import os
import sys
import re
import time
import subprocess
import threading
import hashlib
import ctypes
import numpy as np

BIN_ARM_A = "/home/sandeep/dev/sandwich/sandwich-engine-pre-regression/build/bin/llama-cli"
BIN_ARM_B = "/home/sandeep/dev/sandwich/sandwich-engine/build/bin/llama-cli"
MODEL_PATH = "/home/sandeep/ai-models/moe/Qwen3.6-35B-A3B-MTP-GGUF/Qwen3.6-35B-A3B-UD-IQ3_XXS.gguf"
DRAFTER_PATH = "/home/sandeep/ai-models/drafters/Qwen3.6-35B-A3B-DFlash2-GGUF/Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf"
PROMPT_FILE = "/home/sandeep/dev/the-mds/audit-20261112-131000/sandwich_bench_prompt.txt"
CANONICAL_PURE_SHA = "481b2a7aea43731e77979bca6631cfac9d32bd42d023d63c91552b245931a6aa"

def get_cuda_vram_bytes():
    try:
        cuda = ctypes.CDLL('libcudart.so')
        free = ctypes.c_size_t()
        total = ctypes.c_size_t()
        if cuda.cudaMemGetInfo(ctypes.byref(free), ctypes.byref(total)) == 0:
            return total.value - free.value
    except Exception:
        pass
    try:
        out = subprocess.check_output(
            ["nvidia-smi", "--query-gpu=memory.used", "--format=csv,noheader,nounits"],
            stderr=subprocess.DEVNULL,
            text=True
        )
        return int(out.strip().splitlines()[0]) * 1024 * 1024
    except Exception:
        pass
    return 0

class VRAMSamplerBytes:
    def __init__(self, interval_sec=0.05):
        self.interval = interval_sec
        self.peak_bytes = 0
        self._running = False
        self._thread = None

    def _sample_once(self):
        val = get_cuda_vram_bytes()
        if val > self.peak_bytes:
            self.peak_bytes = val

    def _sample_loop(self):
        while self._running:
            self._sample_once()
            time.sleep(self.interval)

    def start(self):
        self.peak_bytes = 0
        self._sample_once()
        self._running = True
        self._thread = threading.Thread(target=self._sample_loop, daemon=True)
        self._thread.start()

    def stop(self):
        self._running = False
        if self._thread:
            self._thread.join(timeout=1.0)
        self._sample_once()
        return self.peak_bytes

def run_one(bin_path, arm_name, rep):
    cmd = [
        bin_path,
        "-m", MODEL_PATH,
        "-ngl", "14",
        "-t", "6",
        "-c", "4096",
        "-n", "64",
        "--ignore-eos",
        "-f", PROMPT_FILE,
        "--temp", "0",
        "--seed", "42",
        "--no-warmup",
        "-ctk", "turbo4",
        "-ctv", "turbo4",
        "--turbo-kvran",
        "-fa",
        "-md", DRAFTER_PATH,
        "-ngld", "0",
        "--moe-cache-slots", "32",
        "--moe-warm-ram-mb", "12500",
        "--io-uring",
        "--moe-2q",
        "--moe-exchange-rot"
    ]
    env = os.environ.copy()
    env["GGML_CUDA_PDL"] = "0"
    
    sampler = VRAMSamplerBytes(interval_sec=0.05)
    sampler.start()
    t0 = time.time()
    res = subprocess.run(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env)
    dur = time.time() - t0
    peak_b = sampler.stop()

    if res.returncode != 0:
        print(f"[{arm_name} Rep {rep}] FAILED with code {res.returncode}: {res.stderr[:200]}")
        return None

    stdout = res.stdout
    # Extract prompt t/s and decode t/s
    m_ts = re.search(r'\[ Prompt:\s*([\d\.]+)\s*t/s\s*\|\s*Generation:\s*([\d\.]+)\s*t/s\s*\]', stdout)
    prefill_ts = float(m_ts.group(1)) if m_ts else 0.0
    decode_ts = float(m_ts.group(2)) if m_ts else 0.0

    m_bd = re.search(r'\[ Wall-Time Breakdown:\s*load:\s*([\d\.]+)s\s*\|\s*init:\s*([\d\.]+)s\s*\|\s*prefill:\s*([\d\.]+)s\s*\|\s*decode:\s*([\d\.]+)s\s*\|\s*teardown:\s*([\d\.]+)s\s*\|\s*total:\s*([\d\.]+)s\s*\]', stdout)
    load_s = float(m_bd.group(1)) if m_bd else 0.0
    init_s = float(m_bd.group(2)) if m_bd else 0.0
    prefill_s = float(m_bd.group(3)) if m_bd else 0.0
    decode_s = float(m_bd.group(4)) if m_bd else 0.0
    teardown_s = float(m_bd.group(5)) if m_bd else 0.0
    total_s = float(m_bd.group(6)) if m_bd else 0.0

    # Extract pure token text sha
    start_idx = stdout.find("[Start thinking]")
    end_idx = stdout.find("[ Prompt:", start_idx) if start_idx != -1 else -1
    gen_text = stdout[start_idx:end_idx].strip() if (start_idx != -1 and end_idx != -1) else ""
    pure_sha = hashlib.sha256(gen_text.encode("utf-8")).hexdigest()

    return {
        "arm": arm_name,
        "rep": rep,
        "prefill_ts": prefill_ts,
        "decode_ts": decode_ts,
        "load_s": load_s,
        "init_s": init_s,
        "prefill_s": prefill_s,
        "decode_s": decode_s,
        "teardown_s": teardown_s,
        "total_s": total_s,
        "peak_vram_bytes": peak_b,
        "peak_vram_mib": peak_b / (1024 * 1024),
        "pure_sha": pure_sha,
        "sha_match": (pure_sha == CANONICAL_PURE_SHA)
    }

def main():
    n_reps = 5
    print(f"=== SIDE-BY-SIDE BENCHMARK: Arm A (a7c9e6e26) vs Arm B (Current main) ===")
    print(f"Protocol: {n_reps} interleaved repetitions with 4s cooling delay")
    print(f"GPU status check:")
    subprocess.run(["nvidia-smi"], check=True)

    results_a = []
    results_b = []

    for r in range(1, n_reps + 1):
        print(f"\n--- Repetition {r}/{n_reps} ---")
        # Cool down
        time.sleep(4)
        print(f"Running Arm A (Pre-Regression a7c9e6e26)...")
        res_a = run_one(BIN_ARM_A, "Arm_A_PreRegression", r)
        if res_a:
            print(f"  Arm A: Decode {res_a['decode_ts']:.2f} t/s | Decode time: {res_a['decode_s']:.3f} s | Peak VRAM: {res_a['peak_vram_bytes']} B ({res_a['peak_vram_mib']:.2f} MiB) | SHA match: {res_a['sha_match']}")
            results_a.append(res_a)

        time.sleep(4)
        print(f"Running Arm B (Current main)...")
        res_b = run_one(BIN_ARM_B, "Arm_B_CurrentMain", r)
        if res_b:
            print(f"  Arm B: Decode {res_b['decode_ts']:.2f} t/s | Decode time: {res_b['decode_s']:.3f} s | Peak VRAM: {res_b['peak_vram_bytes']} B ({res_b['peak_vram_mib']:.2f} MiB) | SHA match: {res_b['sha_match']}")
            results_b.append(res_b)

    print("\n" + "=" * 80)
    print(" STATISTICAL SUMMARY (n=5 interleaved repetitions)")
    print("=" * 80)

    def stats(vals):
        arr = np.array(vals)
        return np.mean(arr), np.std(arr, ddof=1) if len(arr) > 1 else 0.0

    a_dec_ts_m, a_dec_ts_s = stats([x["decode_ts"] for x in results_a])
    b_dec_ts_m, b_dec_ts_s = stats([x["decode_ts"] for x in results_b])

    a_dec_s_m, a_dec_s_s = stats([x["decode_s"] for x in results_a])
    b_dec_s_m, b_dec_s_s = stats([x["decode_s"] for x in results_b])

    a_pref_ts_m, a_pref_ts_s = stats([x["prefill_ts"] for x in results_a])
    b_pref_ts_m, b_pref_ts_s = stats([x["prefill_ts"] for x in results_b])

    a_vram_b = np.max([x["peak_vram_bytes"] for x in results_a])
    b_vram_b = np.max([x["peak_vram_bytes"] for x in results_b])

    print(f"Arm A (Pre-Regression a7c9e6e26):")
    print(f"  Decode Throughput : {a_dec_ts_m:.2f} ± {a_dec_ts_s:.2f} t/s")
    print(f"  Decode Wall-Time  : {a_dec_s_m:.3f} ± {a_dec_s_s:.3f} s")
    print(f"  Prefill Throughput: {a_pref_ts_m:.2f} ± {a_pref_ts_s:.2f} t/s")
    print(f"  Peak VRAM (Bytes) : {a_vram_b:,} bytes ({a_vram_b/(1024*1024):.2f} MiB)")

    print(f"\nArm B (Current Main post-fix):")
    print(f"  Decode Throughput : {b_dec_ts_m:.2f} ± {b_dec_ts_s:.2f} t/s")
    print(f"  Decode Wall-Time  : {b_dec_s_m:.3f} ± {b_dec_s_s:.3f} s")
    print(f"  Prefill Throughput: {b_pref_ts_m:.2f} ± {b_pref_ts_s:.2f} t/s")
    print(f"  Peak VRAM (Bytes) : {b_vram_b:,} bytes ({b_vram_b/(1024*1024):.2f} MiB)")

    delta_dec_ts = b_dec_ts_m - a_dec_ts_m
    delta_dec_time_ms = (b_dec_s_m - a_dec_s_m) * 1000.0
    sigma_comb = np.sqrt(a_dec_ts_s**2 + b_dec_ts_s**2)

    print(f"\nComparative Delta (Arm B - Arm A):")
    print(f"  Decode Throughput Delta : {delta_dec_ts:+.2f} t/s (sigma_comb = {sigma_comb:.2f} t/s)")
    print(f"  Decode Wall-Time Delta  : {delta_dec_time_ms:+.1f} ms")
    print(f"  VRAM Delta (Bytes)      : {b_vram_b - a_vram_b:+,} bytes")

if __name__ == "__main__":
    main()
