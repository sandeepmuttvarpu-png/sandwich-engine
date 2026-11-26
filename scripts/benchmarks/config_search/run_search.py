#!/usr/bin/env python3
"""Audit-grade config search runner for Qwen3.6-35B-A3B-UD-IQ3_XXS.

Per-run it records decode t/s, decode wall time, peak VRAM (cudaMemGetInfo,
50 ms), peak host VmHWM (/proc/<pid>/status), pure-token SHA256, and exit state.

Usage:
  run_search.py --spec spec.json --out raw.json
  run_search.py --name c4 --flags "-ctk turbo4 ..." --reps 5 --out raw.json

Spec schema:
  { "reps": 3, "cooldown": 4, "interleave": true, "timeout": 600,
    "configs": [ {"name": "...", "flags": ["-ctk","turbo4", ...]} ] }
"""
import argparse
import ctypes
import hashlib
import json
import os
import re
import statistics
import subprocess
import sys
import threading
import time

BIN = "/home/sandeep/dev/sandwich/sandwich-engine/build/bin/llama-cli"
MODEL = "/home/sandeep/ai-models/moe/Qwen3.6-35B-A3B-MTP-GGUF/Qwen3.6-35B-A3B-UD-IQ3_XXS.gguf"
DRAFTER = "/home/sandeep/ai-models/drafters/Qwen3.6-35B-A3B-DFlash2-GGUF/Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf"
PROMPT = "/home/sandeep/dev/the-mds/audit-20261112-131000/sandwich_bench_prompt.txt"
CANONICAL_PURE_SHA = "481b2a7aea43731e77979bca6631cfac9d32bd42d023d63c91552b245931a6aa"
VRAM_GUARD_MIB = 7800
VMHWM_GUARD_MIB = 16552

BASE = [
    "-m", MODEL, "-ngl", "14", "-t", "6", "-c", "4096", "-n", "64",
    "--ignore-eos", "-f", PROMPT, "--temp", "0", "--seed", "42",
    "--no-warmup", "--simple-io",
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
        self.peak_mib = 0
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
                            if v > self.peak_mib:
                                self.peak_mib = v
                            break
            except (FileNotFoundError, ProcessLookupError):
                return
            v = cuda_vram_mib()
            if v > self.peak_mib_vram:
                self.peak_mib_vram = v
            time.sleep(self.interval)

    def start(self):
        self.peak_mib = 0
        self.peak_mib_vram = cuda_vram_mib()
        self._run = True
        self._t = threading.Thread(target=self._loop, daemon=True)
        self._t.start()

    def stop(self):
        self._run = False
        if self._t:
            self._t.join(timeout=2.0)
        return self.peak_mib, self.peak_mib_vram


def check_env():
    problems = []
    used = cuda_vram_mib()
    if used > 200:
        problems.append(f"GPU not idle: {used} MiB in use")
    return problems


def parse_sha(stdout):
    start = stdout.find("[Start thinking]")
    end = stdout.find("[ Prompt:", start) if start != -1 else -1
    text = stdout[start:end].strip() if (start != -1 and end != -1) else ""
    return hashlib.sha256(text.encode()).hexdigest(), bool(text)


def run_one(name, flags, rep, extra_env=None):
    cmd = [BIN] + BASE + flags
    env = os.environ.copy()
    env["GGML_CUDA_PDL"] = "0"
    if extra_env:
        env.update(extra_env)

    p = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, text=True, env=env)
    s = Sampler(p.pid)
    s.start()
    t0 = time.time()
    try:
        out, err = p.communicate(timeout=600)
    except subprocess.TimeoutExpired:
        p.kill()
        out, err = p.communicate()
        return {"config": name, "rep": rep, "ok": False, "error": "timeout"}
    dur = time.time() - t0
    vmhwm, peak_vram = s.stop()

    rec = {
        "config": name, "rep": rep, "ok": p.returncode == 0,
        "rc": p.returncode, "wall_s": round(dur, 3),
        "peak_vram_mib": peak_vram, "vmhwm_mib": vmhwm,
    }
    if p.returncode != 0:
        tail = (err or "")[-2000:]
        rec["error"] = tail
        m = re.search(r"out of memory|CUDA error|failed to allocate|ggml_backend_cuda_buffer_type_alloc_buffer", err or "", re.I)
        if m:
            rec["error_class"] = m.group(0)
        return rec

    m = re.search(r"\[ Prompt:\s*([\d.]+)\s*t/s\s*\|\s*Generation:\s*([\d.]+)\s*t/s\s*\]", out)
    rec["prefill_ts"] = float(m.group(1)) if m else 0.0
    rec["decode_ts"] = float(m.group(2)) if m else 0.0

    m = re.search(r"\[ Wall-Time Breakdown:[^]]*prefill:\s*([\d.]+)s\s*\|\s*decode:\s*([\d.]+)s", out)
    rec["prefill_s"] = float(m.group(1)) if m else 0.0
    rec["decode_s"] = float(m.group(2)) if m else 0.0

    sha, has_text = parse_sha(out)
    rec["pure_sha"] = sha
    rec["sha_match"] = has_text and sha == CANONICAL_PURE_SHA
    return rec


def summarize(runs, warm_discard=1):
    ok = [r for r in runs if r.get("ok")]
    if not ok:
        return {"n": 0, "fatal": "no successful runs"}
    warm = ok[warm_discard:] if len(ok) > warm_discard else ok
    dec = [r["decode_ts"] for r in warm]
    s = {
        "n_total": len(runs), "n_ok": len(ok), "n_warm": len(warm),
        "decode_mean": round(statistics.mean(dec), 3),
        "decode_sd": round(statistics.stdev(dec), 3) if len(dec) > 1 else 0.0,
        "decode_runs": dec,
        "peak_vram_mib": max(r["peak_vram_mib"] for r in ok),
        "vmhwm_mib": max(r["vmhwm_mib"] for r in ok),
        "sha_all_match": all(r.get("sha_match") for r in ok),
        "vram_gate": all(r["peak_vram_mib"] <= VRAM_GUARD_MIB for r in ok),
        "vmhwm_gate": all(r["vmhwm_mib"] <= VMHWM_GUARD_MIB for r in ok),
        "failed_runs": len(runs) - len(ok),
    }
    return s


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--spec")
    ap.add_argument("--name")
    ap.add_argument("--flags", default="")
    ap.add_argument("--reps", type=int, default=3)
    ap.add_argument("--cooldown", type=float, default=4.0)
    ap.add_argument("--out", required=True)
    ap.add_argument("--interleave", action="store_true")
    args = ap.parse_args()

    if args.spec:
        spec = json.load(open(args.spec))
        configs = spec["configs"]
        reps = spec.get("reps", args.reps)
        cooldown = spec.get("cooldown", args.cooldown)
        interleave = spec.get("interleave", args.interleave)
    else:
        configs = [{"name": args.name, "flags": args.flags.split()}]
        reps, cooldown, interleave = args.reps, args.cooldown, args.interleave

    problems = check_env()
    if problems:
        print("ENV PRECHECK FAILED:", problems)
        sys.exit(1)

    all_runs = {c["name"]: [] for c in configs}
    order = []
    if interleave:
        for _ in range(reps):
            for c in configs:
                order.append(c)
        # Round-robin is already balanced; one full cycle per rep.
    else:
        for c in configs:
            order.extend([c] * reps)

    print(f"spec: {len(configs)} config(s), reps={reps}, cooldown={cooldown}s, "
          f"interleave={interleave}, guard={VRAM_GUARD_MIB} MiB")
    for i, c in enumerate(order, 1):
        time.sleep(cooldown)
        rep = len(all_runs[c["name"]]) + 1
        print(f"[{i}/{len(order)}] {c['name']} rep{rep} ... ", end="", flush=True)
        r = run_one(c["name"], c["flags"], rep, c.get("env"))
        all_runs[c["name"]].append(r)
        if r.get("ok"):
            print(f"{r['decode_ts']:.2f} t/s | {r['peak_vram_mib']} MiB VRAM | "
                  f"{r['vmhwm_mib']} MiB VmHWM | sha={'OK' if r['sha_match'] else 'BAD'}")
        else:
            print(f"FAILED rc={r.get('rc')} {r.get('error_class', '')}")
        # Write incrementally so a crash keeps evidence.
        payload = {
            "generated": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "gpu": "RTX 5050 Mobile 8151 MiB, driver 620.73.04",
            "protocol": {"guard_vram_mib": VRAM_GUARD_MIB, "guard_vmhwm_mib": VMHWM_GUARD_MIB,
                         "cooldown_s": cooldown, "warm_discard": 1},
            "configs": {n: {"flags": next(x["flags"] for x in configs if x["name"] == n),
                            "env": next(x.get("env", {}) for x in configs if x["name"] == n),
                            "runs": rs, "summary": summarize(rs)} for n, rs in all_runs.items()},
        }
        json.dump(payload, open(args.out, "w"), indent=2)

    print("\n=== SUMMARY (warm-4 style: discard run 1) ===")
    for n, rs in all_runs.items():
        s = summarize(rs)
        print(f"{n}: {s.get('decode_mean')} +/- {s.get('decode_sd')} t/s "
              f"(n={s.get('n_warm')}) VRAM {s.get('peak_vram_mib')} MiB "
              f"VmHWM {s.get('vmhwm_mib')} MiB sha={s.get('sha_all_match')} "
              f"gates: vram={s.get('vram_gate')} vmhwm={s.get('vmhwm_gate')}")


if __name__ == "__main__":
    main()
