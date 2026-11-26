#!/usr/bin/env python3
"""Phase 3: Run Qwen3.8-Flash-Next under unified planner with smaps_rollup decomposition."""
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
MODEL = "/home/sandeep/ai-models/moe/Qwen3.8-Flash-Next-GSQ-RCO-GGUF/IQ3_XXS/Qwen3.8-Flash-Next-GSQ-RCO-abliterated-IQ3_XXS-00001-of-00002.gguf"
PROMPT = ROOT / "scripts/benchmarks/universality/prompt_512.txt"
OUT_JSON = ROOT / "scripts/benchmarks/universality/flash_next_unified.json"
TIMEOUT_S = 600


def query_gpu():
    out = subprocess.check_output(
        ["nvidia-smi", "--query-gpu=memory.used,temperature.gpu", "--format=csv,noheader,nounits"],
        text=True,
    ).strip()
    mem_s, temp_s = [x.strip() for x in out.split(",")]
    return float(mem_s), float(temp_s)


def read_proc_mem(pid: int) -> dict:
    res = {}
    try:
        status = Path(f"/proc/{pid}/status").read_text()
        for key in ("VmHWM", "VmRSS"):
            m = re.search(rf"^{key}:\s+(\d+)\s+kB", status, re.M)
            if m:
                res[key.lower() + "_mib"] = round(int(m.group(1)) / 1024.0, 2)
    except OSError:
        pass
    try:
        smaps = Path(f"/proc/{pid}/smaps_rollup").read_text()
        for key in ("Rss", "Pss", "Shared_Clean", "Shared_Dirty", "Private_Clean", "Private_Dirty", "Anonymous"):
            m = re.search(rf"^{key}:\s+(\d+)\s+kB", smaps, re.M)
            if m:
                res[key.lower() + "_mib"] = round(int(m.group(1)) / 1024.0, 2)
    except OSError:
        pass
    return res


def main():
    vram_idle, temp_idle = query_gpu()
    if vram_idle > 200:
        print(f"ERROR: GPU not idle ({vram_idle} MiB used)", file=sys.stderr)
        sys.exit(1)

    env = os.environ.copy()
    env["SANDWICH_AUTOTUNE_DISABLE"] = "1"

    # 1. Capture resolved plan via SANDWICH_DISPATCH_PRINT=1
    env_print = env.copy()
    env_print["SANDWICH_DISPATCH_PRINT"] = "1"
    p_print = subprocess.run(
        [str(LLAMA_CLI), "-m", MODEL, "--sandwich-auto", "--spec-type", "none", "-c", "4096"],
        capture_output=True,
        text=True,
        env=env_print,
        timeout=30,
    )
    print_combined = p_print.stdout + "\n" + p_print.stderr
    m_bytes = re.search(r"SANDWICH_DISPATCH_PRINT total_model_bytes:\s*(\d+)", print_combined)
    m_argv = re.search(r"SANDWICH_DISPATCH_PRINT argv:\s+([^\n]+)", print_combined)

    # 2. Run actual 64-token generation with smaps_rollup sampling
    cmd = [
        str(LLAMA_CLI),
        "-m", MODEL,
        "--sandwich-auto", "--spec-type", "none", "-c", "4096",
        "-st", "--no-display-prompt", "--simple-io",
        "--temp", "0", "--seed", "42", "-n", "64",
        "-f", str(PROMPT),
    ]
    print("=== Running Qwen3.8-Flash-Next (timeout=600s) ===", flush=True)
    t0 = time.monotonic()
    peak_vram = 0.0
    peak_vmhwm = 0.0
    peak_temp = 0.0
    peak_smaps = {}
    last_smaps = {}
    timed_out = False

    with tempfile.TemporaryFile(mode="w+b") as out_f, tempfile.TemporaryFile(mode="w+b") as err_f:
        proc = subprocess.Popen(cmd, stdout=out_f, stderr=err_f, env=env)
        while True:
            rc = proc.poll()
            vram, temp = query_gpu()
            pmem = read_proc_mem(proc.pid)
            peak_vram = max(peak_vram, vram)
            peak_temp = max(peak_temp, temp)
            vmhwm = pmem.get("vmhwm_mib", 0.0)
            if pmem:
                last_smaps = pmem
                if pmem.get("rss_mib", 0.0) >= peak_smaps.get("rss_mib", 0.0):
                    peak_smaps = pmem
            if vmhwm >= peak_vmhwm:
                peak_vmhwm = vmhwm
            if rc is not None:
                break
            if time.monotonic() - t0 > TIMEOUT_S:
                timed_out = True
                proc.kill()
                proc.wait()
                break
            time.sleep(0.05)

        elapsed = time.monotonic() - t0
        out_f.seek(0)
        err_f.seek(0)
        stdout = out_f.read().decode("utf-8", errors="replace")
        stderr = err_f.read().decode("utf-8", errors="replace")

    combined = stdout + "\n" + stderr
    pp_match = re.search(r"Prompt:\s*([\d.]+)\s*t/s", combined)
    tg_match = re.search(r"Generation:\s*([\d.]+)\s*t/s", combined)

    body = stdout.split("[ Prompt:")[0] if "[ Prompt:" in stdout else stdout
    marker = "memory bandwidth efficiency."
    after_prompt = body.split(marker)[-1] if marker in body else body
    preview = after_prompt.strip()[-600:]

    result = {
        "model": MODEL,
        "rc": proc.returncode,
        "timed_out": timed_out,
        "elapsed_s": round(elapsed, 2),
        "total_model_bytes_line": m_bytes.group(1).strip() if m_bytes else None,
        "resolved_argv": m_argv.group(1).strip() if m_argv else None,
        "peak_vram_mib": round(peak_vram, 1),
        "peak_vmhwm_mib": round(peak_vmhwm, 1),
        "peak_temp_c": round(peak_temp, 2),
        "guard_vram_ok": peak_vram <= 7100.0,
        "guard_vmhwm_ok": peak_vmhwm <= 16552.0,
        "smaps_at_peak_vmhwm": peak_smaps,
        "smaps_last": last_smaps,
        "prompt_tps": float(pp_match.group(1)) if pp_match else None,
        "gen_tps": float(tg_match.group(1)) if tg_match else None,
        "preview": preview,
    }
    OUT_JSON.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2), flush=True)


if __name__ == "__main__":
    main()
