#!/usr/bin/env python3
"""Pass 2 Throughput & Latency Sweep across narrowed 8 models.

Executes n=3 repetitions per model with 60s thermal cooldown.
Prompt: scripts/benchmarks/universality/prompt_512.txt (~512 tokens)
Generation: -n 128 tokens
Hardware Guards:
  Peak VRAM <= 7,100 MiB
  Peak VmHWM <= 16,552 MiB
"""

import argparse
import ctypes
import json
import math
import os
import re
import subprocess
import sys
import threading
import time

BIN = "/home/sandeep/dev/sandwich/sandwich-engine/build/bin/llama-cli"
PROMPT_FILE = "/home/sandeep/dev/sandwich/sandwich-engine/scripts/benchmarks/universality/prompt_512.txt"
OUTPUT_JSON = "/home/sandeep/dev/sandwich/sandwich-engine/scripts/benchmarks/universality/throughput.json"
VRAM_GUARD = 7100
VMHWM_GUARD = 16552
COOLDOWN_SEC = 60
REPS = 3
TIMEOUT_SEC = 600

TARGETS = [
    {
        "name": "Qwen3.6-35B-A3B (Certified)",
        "model_path": "/home/sandeep/ai-models/moe/Qwen3.6-35B-A3B-MTP-GGUF/Qwen3.6-35B-A3B-UD-IQ3_XXS.gguf",
        "architecture": "qwen35moe",
        "params": "35.51B",
        "quant": "IQ3_S",
        "flags": [
            "-ngl", "14", "-t", "6", "-c", "4096", "-n", "128",
            "--temp", "0", "--seed", "42", "--simple-io", "-st",
            "-ctk", "turbo4", "-ctv", "turbo4", "--turbo-kvran", "-fa",
            "-md", "/home/sandeep/ai-models/drafters/Qwen3.6-35B-A3B-DFlash2-GGUF/Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf",
            "-ngld", "99", "--io-uring", "--moe-2q", "--moe-cache-slots", "16",
            "--moe-warm-ram-mb", "8000"
        ],
        "env": {
            "SANDWICH_AUTOTUNE_DISABLE": "1"
        }
    },
    {
        "name": "Flint Preview",
        "model_path": "/home/sandeep/ai-models/moe/flint-GGUF/flint-UD-IQ3_XXS.gguf",
        "architecture": "flint",
        "params": "39.79B",
        "quant": "IQ3_XXS",
        "flags": [
            "--sandwich-auto", "-c", "4096", "-n", "128",
            "--temp", "0", "--seed", "42", "--spec-type", "none",
            "--simple-io", "-st"
        ],
        "env": {}
    },
    {
        "name": "Muse-Glimmer-30B",
        "model_path": "/home/sandeep/ai-models/dense/Muse-Glimmer-30B-GGUF/Muse-Glimmer-30B-KQuant-17GB-Q4_K_M.gguf",
        "architecture": "muse-glimmer",
        "params": "27.85B",
        "quant": "Q4_K_M",
        "flags": [
            "--sandwich-auto", "-c", "4096", "-n", "128",
            "--temp", "0", "--seed", "42", "--spec-type", "none",
            "--simple-io", "-st"
        ],
        "env": {}
    },
    {
        "name": "LFM2-8B-A1B",
        "model_path": "/home/sandeep/ai-models/moe/LFM2-8B-A1B-Instruct-Quantum-IQ1C-Qwen3.6-35B-A3B-DISTILL-i1-GGUF/LFM2-8B-A1B-Instruct-Quantum-IQ1C-Qwen3.6-35B-A3B-DISTILL.i1-IQ2_XXS.gguf",
        "architecture": "lfm2moe",
        "params": "8.34B",
        "quant": "IQ2_XXS",
        "flags": [
            "--sandwich-auto", "-c", "4096", "-n", "128",
            "--temp", "0", "--seed", "42", "--spec-type", "none",
            "--simple-io", "-st"
        ],
        "env": {}
    },
    {
        "name": "GLM-4.7-Flash",
        "model_path": "/home/sandeep/ai-models/moe/GLM-4.7-Flash-GGUF/GLM-4.7-Flash-Q4_K_M.gguf",
        "architecture": "deepseek2",
        "params": "29.94B",
        "quant": "Q4_K_M",
        "flags": [
            "--sandwich-auto", "-c", "4096", "-n", "128",
            "--temp", "0", "--seed", "42", "--spec-type", "none",
            "--simple-io", "-st"
        ],
        "env": {}
    },
    {
        "name": "Nemotron-3-Nano-30B",
        "model_path": "/home/sandeep/ai-models/moe/Nemotron-3-Nano-30B-A3B-GGUF/Nemotron-3-Nano-30B-A3B-UD-IQ3_XXS.gguf",
        "architecture": "nemotron_h_moe",
        "params": "31.58B",
        "quant": "IQ3_XXS",
        "flags": [
            "--sandwich-auto", "-c", "4096", "-n", "128",
            "--temp", "0", "--seed", "42", "--spec-type", "none",
            "--simple-io", "-st"
        ],
        "env": {}
    },
    {
        "name": "Gemma-4-31B-it QAT",
        "model_path": "/home/sandeep/ai-models/dense/gemma-4-31B-it-QAT-GGUF/gemma-4-31B-it-QAT-Q4_0.gguf",
        "architecture": "gemma4",
        "params": "30.70B",
        "quant": "Q4_0",
        "flags": [
            "--sandwich-auto", "-c", "4096", "-n", "128",
            "--temp", "0", "--seed", "42", "--spec-type", "none",
            "--simple-io", "-st"
        ],
        "env": {}
    },
    {
        "name": "Dolphin-2.8-Mistral-7B",
        "model_path": "/home/sandeep/ai-models/dense/dolphin-2.8-mistral-7b-v02-GGUF/dolphin-2.8-mistral-7b-v02-Q4_K_S.gguf",
        "architecture": "llama",
        "params": "7.24B",
        "quant": "Q4_K_S",
        "flags": [
            "--sandwich-auto", "-c", "4096", "-n", "128",
            "--temp", "0", "--seed", "42", "--spec-type", "none",
            "--simple-io", "-st"
        ],
        "env": {}
    }
]


def get_gpu_temp():
    try:
        out = subprocess.check_output(
            ["nvidia-smi", "--query-gpu=temperature.gpu", "--format=csv,noheader,nounits"],
            encoding="utf-8"
        )
        return float(out.strip())
    except Exception:
        return 0.0


def cuda_vram_mib():
    try:
        cuda = ctypes.CDLL("libcudart.so")
        free = ctypes.c_size_t()
        total = ctypes.c_size_t()
        if cuda.cudaMemGetInfo(ctypes.byref(free), ctypes.byref(total)) == 0:
            return int((total.value - free.value) // (1024 * 1024))
    except Exception:
        pass
    try:
        out = subprocess.check_output(
            ["nvidia-smi", "--query-gpu=memory.used", "--format=csv,noheader,nounits"],
            encoding="utf-8"
        )
        return int(out.strip().split("\n")[0])
    except Exception:
        return 0


class MemorySampler:
    def __init__(self, pid, interval=0.05):
        self.pid = pid
        self.interval = interval
        self.peak_vmhwm = 0
        self.peak_vram = 0
        self._running = False
        self._thread = None

    def _loop(self):
        while self._running:
            try:
                with open(f"/proc/{self.pid}/status") as f:
                    for line in f:
                        if line.startswith("VmHWM:"):
                            val = int(line.split()[1]) // 1024
                            if val > self.peak_vmhwm:
                                self.peak_vmhwm = val
                            break
            except Exception:
                pass
            vram = cuda_vram_mib()
            if vram > self.peak_vram:
                self.peak_vram = vram
            time.sleep(self.interval)

    def start(self):
        self.peak_vram = cuda_vram_mib()
        self._running = True
        self._thread = threading.Thread(target=self._loop, daemon=True)
        self._thread.start()

    def stop(self):
        self._running = False
        if self._thread:
            self._thread.join(timeout=2.0)
        return self.peak_vmhwm, self.peak_vram


def stats(values):
    if not values:
        return 0.0, 0.0
    mean = sum(values) / len(values)
    if len(values) == 1:
        return round(mean, 2), 0.0
    var = sum((x - mean) ** 2 for x in values) / (len(values) - 1)
    return round(mean, 2), round(math.sqrt(var), 2)


def run_single(target, rep):
    cmd = [BIN, "-m", target["model_path"]] + target["flags"] + ["-f", PROMPT_FILE]
    env = os.environ.copy()
    env["GGML_CUDA_PDL"] = "0"
    if target.get("env"):
        env.update(target["env"])

    start_temp = get_gpu_temp()
    t0 = time.time()
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env, text=True)
    sampler = MemorySampler(p.pid)
    sampler.start()

    try:
        out, _ = p.communicate(timeout=TIMEOUT_SEC)
    except subprocess.TimeoutExpired:
        p.kill()
        out, _ = p.communicate()
        peak_vmhwm, peak_vram = sampler.stop()
        return {
            "rep": rep, "exit_code": -9, "wall_seconds": round(time.time() - t0, 2),
            "start_temp_c": start_temp, "peak_vram_mib": peak_vram, "peak_vmhwm_mib": peak_vmhwm,
            "prefill_ts": 0.0, "decode_ts": 0.0, "tokens": "", "error": "TIMED OUT after 600s"
        }

    wall_sec = round(time.time() - t0, 2)
    peak_vmhwm, peak_vram = sampler.stop()

    # Parse Prompt and Generation t/s
    m_ts = re.search(r"\[ Prompt:\s*([\d.]+)\s*t/s\s*\|\s*Generation:\s*([\d.]+)\s*t/s\s*\]", out)
    prefill_ts = float(m_ts.group(1)) if m_ts else 0.0
    decode_ts = float(m_ts.group(2)) if m_ts else 0.0

    m_bd = re.search(r"\[ Wall-Time Breakdown:[^]]*load:\s*([\d.]+)s[^]]*prefill:\s*([\d.]+)s\s*\|\s*decode:\s*([\d.]+)s", out)
    load_sec = float(m_bd.group(1)) if m_bd else 0.0

    # Extract tokens verbatim
    tokens = ""
    start_pos = out.find("[Start thinking]")
    if start_pos == -1:
        # Check standard user prompt echo
        start_pos = out.find("> The design of modern operating systems")
        if start_pos != -1:
            start_pos = out.find("\n", start_pos)
    if start_pos != -1:
        end_pos = out.find("[ Prompt:", start_pos)
        if end_pos != -1:
            tokens = out[start_pos:end_pos].strip()
        else:
            tokens = out[start_pos:start_pos+500].strip()

    vram_breach = peak_vram > VRAM_GUARD
    vmhwm_breach = peak_vmhwm > VMHWM_GUARD

    return {
        "rep": rep,
        "exit_code": p.returncode,
        "wall_seconds": wall_sec,
        "load_seconds": load_sec,
        "start_temp_c": start_temp,
        "peak_vram_mib": peak_vram,
        "peak_vmhwm_mib": peak_vmhwm,
        "vram_breach": vram_breach,
        "vmhwm_breach": vmhwm_breach,
        "prefill_ts": prefill_ts,
        "decode_ts": decode_ts,
        "tokens_verbatim": tokens,
        "raw_snippet": (out[-1500:] if p.returncode != 0 else "")
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--resume", action="store_true", help="Load existing results from throughput.json and skip completed models")
    ap.add_argument("--only", type=int, default=0, help="Run only 1-based target index (e.g. 7)")
    args = ap.parse_args()

    print(f"Starting Pass 2 Throughput Sweep across {len(TARGETS)} models ({REPS} reps each).")
    print(f"Prompt: {PROMPT_FILE}")
    print(f"Guards: VRAM <= {VRAM_GUARD} MiB | VmHWM <= {VMHWM_GUARD} MiB | Cooldown: {COOLDOWN_SEC}s\n")

    results = []
    if args.resume and os.path.exists(OUTPUT_JSON):
        with open(OUTPUT_JSON) as f:
            results = json.load(f).get("results", [])

    completed_names = {r["name"] for r in results if r.get("completed_reps", 0) >= REPS}

    selected = [(i, t) for i, t in enumerate(TARGETS, 1) if args.only in (0, i)]
    for pos, (t_idx, target) in enumerate(selected):
        if args.resume and target["name"] in completed_names:
            print(f"[{t_idx}/{len(TARGETS)}] SKIPPING completed: {target['name']}")
            continue

        print("=" * 65)
        print(f"[{t_idx}/{len(TARGETS)}] MODEL: {target['name']} ({target['architecture']}, {target['params']})")
        print("=" * 65)

        model_runs = []
        for rep in range(1, REPS + 1):
            cur_temp = get_gpu_temp()
            print(f"  Rep {rep}/{REPS} starting (GPU Temp: {cur_temp:.1f}°C)...")
            run_data = run_single(target, rep)
            model_runs.append(run_data)

            breaches = []
            if run_data["vram_breach"]:
                breaches.append(f"VRAM OVER +{run_data['peak_vram_mib'] - VRAM_GUARD}M")
            if run_data["vmhwm_breach"]:
                breaches.append(f"VmHWM OVER +{run_data['peak_vmhwm_mib'] - VMHWM_GUARD}M")
            b_str = f" [GUARDS: {', '.join(breaches)}]" if breaches else " [GUARDS: OK]"

            print(f"    -> Prefill: {run_data['prefill_ts']:>6.1f} t/s | Decode: {run_data['decode_ts']:>5.1f} t/s | VRAM: {run_data['peak_vram_mib']:>4d}M | VmHWM: {run_data['peak_vmhwm_mib']:>5d}M{b_str}")

            # Persist after every run
            summary_runs = [r for r in model_runs if r["exit_code"] == 0]
            pp_mean, pp_sd = stats([r["prefill_ts"] for r in summary_runs])
            tg_mean, tg_sd = stats([r["decode_ts"] for r in summary_runs])
            
            # Temporary record update
            current_target_entry = {
                "name": target["name"],
                "architecture": target["architecture"],
                "params": target["params"],
                "quant": target["quant"],
                "reps": REPS,
                "completed_reps": len(model_runs),
                "pp512_mean": pp_mean,
                "pp512_sd": pp_sd,
                "tg128_mean": tg_mean,
                "tg128_sd": tg_sd,
                "peak_vram_max_mib": max(r["peak_vram_mib"] for r in model_runs),
                "peak_vmhwm_max_mib": max(r["peak_vmhwm_mib"] for r in model_runs),
                "runs": model_runs
            }

            temp_results = [r for r in results if r["name"] != target["name"]] + [current_target_entry]
            with open(OUTPUT_JSON, "w") as f:
                json.dump({"timestamp": time.strftime("%Y-%m-%dT%H:%M:%S%z"), "results": temp_results}, f, indent=2)

            if rep < REPS:
                print(f"  Cooldown {COOLDOWN_SEC}s before next rep...")
                time.sleep(COOLDOWN_SEC)

        results = [r for r in results if r["name"] != target["name"]] + [current_target_entry]
        print(f"\n  Summary: pp512 = {pp_mean} ± {pp_sd} t/s | tg128 = {tg_mean} ± {tg_sd} t/s\n")

        if pos + 1 < len(selected):
            print(f"Cooldown {COOLDOWN_SEC}s before next model...\n")
            time.sleep(COOLDOWN_SEC)

    print("Pass 2 Throughput Sweep Completed Successfully.")


if __name__ == "__main__":
    main()
