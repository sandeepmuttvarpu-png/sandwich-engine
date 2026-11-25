#!/usr/bin/env python3
"""Diagnostic re-examination of universality open items after dispatch & autotune fixes."""
import json
import os
import re
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path("/home/sandeep/dev/sandwich/sandwich-engine")
LLAMA_CLI = ROOT / "build/bin/llama-cli"
PROMPT = ROOT / "scripts/benchmarks/universality/prompt_512.txt"
OUT_JSON = ROOT / "scripts/benchmarks/universality/open_items_check.json"

TARGETS = [
    {
        "id": "qwen36_repack_gate",
        "model": "/home/sandeep/ai-models/moe/Qwen3.6-35B-A3B-MTP-GGUF/Qwen3.6-35B-A3B-UD-IQ3_XXS.gguf",
        "extra_args": [
            "-md", "/home/sandeep/ai-models/drafters/Qwen3.6-35B-A3B-DFlash2-GGUF/Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf",
            "--spec-type", "draft-dflash",
            "-ngl", "14", "-ngld", "99",
            "--moe-cache-slots", "16",
            "--moe-warm-ram-mb", "8000",
            "-t", "6", "-c", "8192",
            "-ctk", "turbo4", "-ctv", "turbo4",
            "-fa", "on", "--io-uring", "--moe-2q",
        ],
        "timeout_s": 180,
    },
    {
        "id": "muse_glimmer_30b_fixed",
        "model": "/home/sandeep/ai-models/dense/Muse-Glimmer-30B-GGUF/Muse-Glimmer-30B-KQuant-17GB-Q4_K_M.gguf",
        "extra_args": ["--sandwich-auto", "--spec-type", "none"],
        "timeout_s": 300,
    },
    {
        "id": "command_r_32b_fixed",
        "model": "/home/sandeep/ai-models/dense/c4ai-command-r-08-2024-GGUF/c4ai-command-r-08-2024-IQ3_XS.gguf",
        "extra_args": ["--sandwich-auto", "--spec-type", "none"],
        "timeout_s": 600,
    },
]


def query_gpu():
    out = subprocess.check_output(
        ["nvidia-smi", "--query-gpu=memory.used,temperature.gpu", "--format=csv,noheader,nounits"],
        text=True,
    ).strip()
    mem_s, temp_s = [x.strip() for x in out.split(",")]
    return float(mem_s), float(temp_s)


def read_vmhwm_mib(pid: int) -> float:
    try:
        status = Path(f"/proc/{pid}/status").read_text()
        m = re.search(r"^VmHWM:\s+(\d+)\s+kB", status, re.M)
        if m:
            return int(m.group(1)) / 1024.0
    except OSError:
        pass
    return 0.0


def run_target(spec: dict) -> dict:
    cmd = [
        str(LLAMA_CLI),
        "-m", spec["model"],
        *spec["extra_args"],
        "-st", "--no-display-prompt", "--simple-io", "-lv", "3",
        "--temp", "0", "--seed", "42", "-n", "64",
        "-f", str(PROMPT),
    ]
    env = os.environ.copy()
    env["SANDWICH_AUTOTUNE_DISABLE"] = "1"

    print(f"\n=== Running {spec['id']} (timeout={spec['timeout_s']}s) ===", flush=True)
    t0 = time.monotonic()
    with tempfile.TemporaryFile(mode="w+b") as out_f, tempfile.TemporaryFile(mode="w+b") as err_f:
        proc = subprocess.Popen(
            cmd,
            stdout=out_f,
            stderr=err_f,
            env=env,
        )

        peak_vram = 0.0
        peak_vmhwm = 0.0
        peak_temp = 0.0
        timed_out = False

        while True:
            rc = proc.poll()
            vram, temp = query_gpu()
            vmhwm = read_vmhwm_mib(proc.pid)
            peak_vram = max(peak_vram, vram)
            peak_vmhwm = max(peak_vmhwm, vmhwm)
            peak_temp = max(peak_temp, temp)
            if rc is not None:
                break
            if time.monotonic() - t0 > spec["timeout_s"]:
                timed_out = True
                proc.kill()
                proc.wait()
                break
            time.sleep(0.25)

        elapsed = time.monotonic() - t0
        out_f.seek(0)
        err_f.seek(0)
        stdout = out_f.read().decode("utf-8", errors="replace")
        stderr = err_f.read().decode("utf-8", errors="replace")
    combined = stdout + "\n" + stderr

    repack_fired = "Online Warm-Up Auto-Tuning: Re-packed" in combined
    ngl_match = re.findall(r"n_gpu_layers\s*=\s*(-?\d+)", combined)
    t_match = re.findall(r"(?:Pinning\s+|threads\s*=\s*)(\d+)", combined)
    pp_match = re.search(r"Prompt:\s*([\d.]+)\s*t/s", combined)
    tg_match = re.search(r"Generation:\s*([\d.]+)\s*t/s", combined)

    # Extract actual assistant generation after the echoed prompt block
    body = stdout.split("[ Prompt:")[0] if "[ Prompt:" in stdout else stdout
    marker = "memory bandwidth efficiency."
    after_prompt = body.split(marker)[-1] if marker in body else body
    preview = after_prompt.strip()[-600:]

    res = {
        "id": spec["id"],
        "rc": proc.returncode,
        "timed_out": timed_out,
        "elapsed_s": round(elapsed, 2),
        "peak_vram_mib": round(peak_vram, 1),
        "peak_vmhwm_mib": round(peak_vmhwm, 1),
        "peak_temp_c": round(peak_temp, 2),
        "resolved_ngl": int(ngl_match[-1]) if ngl_match else None,
        "resolved_threads": int(t_match[-1]) if t_match else None,
        "prompt_tps": float(pp_match.group(1)) if pp_match else None,
        "gen_tps": float(tg_match.group(1)) if tg_match else None,
        "repack_fired": repack_fired,
        "guard_vram_ok": peak_vram <= 7100.0,
        "guard_vmhwm_ok": peak_vmhwm <= 16552.0,
        "preview": preview,
    }
    print(json.dumps({k: v for k, v in res.items() if k != "preview"}, indent=2), flush=True)
    print(f"Preview:\n{preview}\n", flush=True)
    return res


def main():
    vram_idle, temp_idle = query_gpu()
    if vram_idle > 200:
        print(f"ERROR: GPU not idle ({vram_idle} MiB used)", file=sys.stderr)
        sys.exit(1)

    results = []
    for i, spec in enumerate(TARGETS):
        results.append(run_target(spec))
        if i + 1 < len(TARGETS):
            print("Cooling down 30s...", flush=True)
            time.sleep(30)

    OUT_JSON.write_text(json.dumps({"results": results}, indent=2) + "\n")
    print(f"Saved {OUT_JSON}")


if __name__ == "__main__":
    main()
