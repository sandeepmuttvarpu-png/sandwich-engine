#!/usr/bin/env python3
import ctypes
import hashlib
import json
import os
import re
import subprocess
import sys
import threading
import time

BIN = "/home/sandeep/dev/sandwich/sandwich-engine/build/bin/llama-cli"
MODEL = "/home/sandeep/ai-models/moe/Qwen3.6-35B-A3B-MTP-GGUF/Qwen3.6-35B-A3B-UD-IQ3_XXS.gguf"
DRAFTER = "/home/sandeep/ai-models/drafters/Qwen3.6-35B-A3B-DFlash2-GGUF/Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf"
PROMPT = "/home/sandeep/dev/the-mds/audit-20261112-131000/sandwich_bench_prompt.txt"
CANONICAL_PURE_SHA = "481b2a7aea43731e77979bca6631cfac9d32bd42d023d63c91552b245931a6aa"

CMD = [
    BIN,
    "-m", MODEL,
    "-ngl", "14",
    "-t", "6",
    "-c", "4096",
    "-n", "64",
    "--ignore-eos",
    "-f", PROMPT,
    "--temp", "0",
    "--seed", "42",
    "--no-warmup",
    "--simple-io",
    "-ctk", "turbo4",
    "-ctv", "turbo4",
    "--turbo-kvran",
    "-fa",
    "-md", DRAFTER,
    "-ngld", "0",
    "--moe-cache-slots", "32",
    "--moe-warm-ram-mb", "12500",
    "--io-uring",
    "--moe-2q",
    "--moe-exchange-rot"
]

def cuda_vram_mib():
    try:
        cuda = ctypes.CDLL("libcudart.so")
        free = ctypes.c_size_t()
        total = ctypes.c_size_t()
        if cuda.cudaMemGetInfo(ctypes.byref(free), ctypes.byref(total)) == 0:
            return int((total.value - free.value) // (1024 * 1024))
    except Exception:
        pass
    return 0

class Sampler:
    def __init__(self, pid, interval=0.05):
        self.pid = pid
        self.interval = interval
        self.peak_vmhwm = 0
        self.peak_vram = 0
        self._run = False
        self._t = None

    def _loop(self):
        path = f"/proc/{self.pid}/status"
        while self._run:
            try:
                with open(path) as f:
                    for line in f:
                        if line.startswith("VmHWM:"):
                            v = int(line.split()[1]) // 1024
                            if v > self.peak_vmhwm:
                                self.peak_vmhwm = v
                            break
            except Exception:
                return
            v = cuda_vram_mib()
            if v > self.peak_vram:
                self.peak_vram = v
            time.sleep(self.interval)

    def start(self):
        self.peak_vmhwm = 0
        self.peak_vram = cuda_vram_mib()
        self._run = True
        self._t = threading.Thread(target=self._loop, daemon=True)
        self._t.start()

    def stop(self):
        self._run = False
        if self._t:
            self._t.join(timeout=2.0)
        return self.peak_vmhwm, self.peak_vram

def parse_sha(stdout):
    start = stdout.find("[Start thinking]")
    end = stdout.find("[ Prompt:", start) if start != -1 else -1
    text = stdout[start:end].strip() if (start != -1 and end != -1) else ""
    return hashlib.sha256(text.encode()).hexdigest(), bool(text)

runs = []
print("Running n=5 benchmark with 4s cooldown...")
for rep in range(1, 6):
    if rep > 1:
        time.sleep(4.0)
    env = os.environ.copy()
    env["GGML_CUDA_PDL"] = "0"

    p = subprocess.Popen(CMD, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, text=True, env=env)
    s = Sampler(p.pid)
    s.start()
    t0 = time.time()
    out, err = p.communicate()
    dur = time.time() - t0
    vmhwm, peak_vram = s.stop()

    m = re.search(r"\[ Prompt:\s*([\d.]+)\s*t/s\s*\|\s*Generation:\s*([\d.]+)\s*t/s\s*\]", out)
    prefill_ts = float(m.group(1)) if m else 0.0
    decode_ts = float(m.group(2)) if m else 0.0

    sha, has_text = parse_sha(out)
    match = (sha == CANONICAL_PURE_SHA)

    rec = {
        "rep": rep,
        "prefill_ts": prefill_ts,
        "decode_ts": decode_ts,
        "wall_s": round(dur, 2),
        "peak_vram_mib": peak_vram,
        "vmhwm_mib": vmhwm,
        "sha": sha,
        "sha_match": match
    }
    runs.append(rec)
    print(f"Rep {rep}: Decode = {decode_ts:.2f} t/s | Prefill = {prefill_ts:.2f} t/s | VRAM = {peak_vram} MiB | VmHWM = {vmhwm} MiB | Match = {match}")

warm_dec = [r["decode_ts"] for r in runs[1:]]
mean_dec = sum(warm_dec) / len(warm_dec)
var_dec = sum((x - mean_dec) ** 2 for x in warm_dec) / max(1, len(warm_dec) - 1)
std_dec = var_dec ** 0.5

summary = {
    "runs": runs,
    "warm_mean_decode_ts": round(mean_dec, 2),
    "warm_std_decode_ts": round(std_dec, 2),
    "all_sha_matched": all(r["sha_match"] for r in runs)
}

with open("/home/sandeep/dev/sandwich/sandwich-engine/tests/diagnostics/bench_n5_prune_results.json", "w") as f:
    json.dump(summary, f, indent=2)

print("\n--- Summary (Warm-4) ---")
print(f"Decode: {mean_dec:.2f} +/- {std_dec:.2f} t/s")
print(f"All SHA matched: {summary['all_sha_matched']}")
