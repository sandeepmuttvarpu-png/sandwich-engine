#!/usr/bin/env python3
"""Phase 3: n=3 verification of dynamic 7800 MiB ceiling, MoE full-fit, and fill-to-ceiling."""
import hashlib
import json
import math
import os
import re
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path("/home/sandeep/dev/sandwich/sandwich-engine")
LLAMA_CLI = ROOT / "build/bin/llama-cli"
PROMPT_512 = ROOT / "scripts/benchmarks/universality/prompt_512.txt"
PROMPT_CERT = Path("/tmp/sandwich_bench_prompt.txt")
CERT_SHA = "481b2a7aea43731e77979bca6631cfac9d32bd42d023d63c91552b245931a6aa"
OUT_JSON = ROOT / "scripts/benchmarks/universality/phase3_ceiling_v3.json"

MODELS = [
    {
        "id": "deepseek_v2_lite",
        "model": "/home/sandeep/ai-models/moe/DeepSeek-Coder-V2-Lite-Instruct-GGUF/DeepSeek-Coder-V2-Lite-Instruct-IQ3_XXS.gguf",
        "args": ["--sandwich-auto", "--spec-type", "none", "-c", "4096"],
        "prompt": str(PROMPT_512),
        "n_predict": 64,
        "reps": 3,
        "timeout_s": 180,
    },
    {
        "id": "gemma4_31b_qat",
        "model": "/home/sandeep/ai-models/dense/gemma-4-31B-it-QAT-GGUF/gemma-4-31B-it-QAT-Q4_0.gguf",
        "args": ["--sandwich-auto", "--spec-type", "none", "-c", "4096"],
        "prompt": str(PROMPT_512),
        "n_predict": 64,
        "reps": 3,
        "timeout_s": 300,
    },
    {
        "id": "glm47_flash",
        "model": "/home/sandeep/ai-models/moe/GLM-4.7-Flash-GGUF/GLM-4.7-Flash-UD-IQ3_XXS.gguf",
        "args": ["--sandwich-auto", "--spec-type", "none", "-c", "4096"],
        "prompt": str(PROMPT_512),
        "n_predict": 64,
        "reps": 3,
        "timeout_s": 300,
    },
    {
        "id": "qwen36_certified",
        "model": "/home/sandeep/ai-models/moe/Qwen3.6-35B-A3B-MTP-GGUF/Qwen3.6-35B-A3B-UD-IQ3_XXS.gguf",
        "args": [
            "-ngl", "14", "-ngld", "99", "-t", "6", "-c", "4096",
            "-ctk", "turbo4", "-ctv", "turbo4", "--turbo-kvran", "-fa",
            "--io-uring", "--moe-2q", "--moe-cache-slots", "16", "--moe-warm-ram-mb", "8000",
            "-md", "/home/sandeep/ai-models/drafters/Qwen3.6-35B-A3B-DFlash2-GGUF/Qwen3.6-35B-A3B-DFlash2-Q4_K_M.gguf",
        ],
        "prompt": str(PROMPT_CERT if PROMPT_CERT.exists() else PROMPT_512),
        "n_predict": 64,
        "reps": 3,
        "timeout_s": 180,
        "check_sha": True,
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


def mean_sd(xs):
    xs = [x for x in xs if x is not None]
    if not xs:
        return None, None
    m = sum(xs) / len(xs)
    sd = math.sqrt(sum((x - m) ** 2 for x in xs) / (len(xs) - 1)) if len(xs) > 1 else 0.0
    return round(m, 2), round(sd, 2)


def run_one(spec: dict, rep: int) -> dict:
    env = os.environ.copy()
    env["SANDWICH_AUTOTUNE_DISABLE"] = "1"
    cmd = [
        str(LLAMA_CLI),
        "-m", spec["model"],
        *spec["args"],
        "-st", "--no-display-prompt", "--simple-io",
        "--temp", "0", "--seed", "42", "-n", str(spec["n_predict"]),
        "-f", spec["prompt"],
    ]

    t0 = time.monotonic()
    peak_vram = 0.0
    peak_vmhwm = 0.0
    peak_temp = 0.0
    timed_out = False

    with tempfile.TemporaryFile(mode="w+b") as out_f, tempfile.TemporaryFile(mode="w+b") as err_f:
        proc = subprocess.Popen(cmd, stdout=out_f, stderr=err_f, env=env)
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
            time.sleep(0.1)

        elapsed = time.monotonic() - t0
        out_f.seek(0)
        err_f.seek(0)
        stdout = out_f.read().decode("utf-8", errors="replace")
        stderr = err_f.read().decode("utf-8", errors="replace")

    combined = stdout + "\n" + stderr
    pp_match = re.search(r"Prompt:\s*([\d.]+)\s*t/s", combined)
    tg_match = re.search(r"Generation:\s*([\d.]+)\s*t/s", combined)

    body = stdout.split("[ Prompt:")[0] if "[ Prompt:" in stdout else stdout
    preview = body.strip()[-300:]

    sha_ok = None
    if spec.get("check_sha"):
        # Extract generated text after prompt block for SHA comparison
        pure = body.split("> ")[-1] if "> " in body else body
        # Also match raw stdout pure-token hash if run_search prompt was used
        if "memory bandwidth" not in spec["prompt"]:
            # Best effort: check against raw lines after prompt
            lines = body.splitlines()
            gen_start = 0
            for idx, ln in enumerate(lines):
                if ln.startswith("> "):
                    gen_start = idx + 1
            pure_txt = "\n".join(lines[gen_start:]).strip()
            h = hashlib.sha256(pure_txt.encode("utf-8", errors="replace")).hexdigest()
            sha_ok = (h == CERT_SHA)

    return {
        "rep": rep,
        "rc": proc.returncode,
        "timed_out": timed_out,
        "elapsed_s": round(elapsed, 2),
        "peak_vram_mib": round(peak_vram, 1),
        "peak_vmhwm_mib": round(peak_vmhwm, 1),
        "peak_temp_c": round(peak_temp, 2),
        "prompt_tps": float(pp_match.group(1)) if pp_match else None,
        "gen_tps": float(tg_match.group(1)) if tg_match else None,
        "preview": preview,
        "sha_ok": sha_ok,
    }


def main():
    vram_idle, temp_idle = query_gpu()
    if vram_idle > 200:
        print(f"ERROR: GPU not idle ({vram_idle} MiB used)", file=sys.stderr)
        sys.exit(1)

    report = {"models": {}}
    for idx, spec in enumerate(MODELS):
        print(f"\n=== Model {idx+1}/{len(MODELS)}: {spec['id']} ===", flush=True)
        # Print resolved plan if --sandwich-auto
        plan_line = None
        if "--sandwich-auto" in spec["args"]:
            env_p = os.environ.copy()
            env_p["SANDWICH_DISPATCH_PRINT"] = "1"
            env_p["SANDWICH_AUTOTUNE_DISABLE"] = "1"
            pr = subprocess.run(
                [str(LLAMA_CLI), "-m", spec["model"], *spec["args"]],
                capture_output=True, text=True, env=env_p, timeout=30,
            )
            m = re.search(r"SANDWICH_DISPATCH_PRINT argv:\s+([^\n]+)", pr.stdout + "\n" + pr.stderr)
            if m:
                plan_line = m.group(1).strip()
                print(f"Plan: {plan_line}", flush=True)

        runs = []
        for r in range(1, spec["reps"] + 1):
            res = run_one(spec, r)
            runs.append(res)
            print(f"  Rep {r}: pp={res['prompt_tps']} t/s, tg={res['gen_tps']} t/s, VRAM={res['peak_vram_mib']} MiB, VmHWM={res['peak_vmhwm_mib']} MiB", flush=True)
            if r < spec["reps"]:
                time.sleep(4)

        pp_m, pp_s = mean_sd([x["prompt_tps"] for x in runs])
        tg_m, tg_s = mean_sd([x["gen_tps"] for x in runs])
        report["models"][spec["id"]] = {
            "resolved_argv": plan_line,
            "runs": runs,
            "pp_mean": pp_m,
            "pp_sd": pp_s,
            "tg_mean": tg_m,
            "tg_sd": tg_s,
            "peak_vram_mib": max(x["peak_vram_mib"] for x in runs),
            "peak_vmhwm_mib": max(x["peak_vmhwm_mib"] for x in runs),
        }

        if idx + 1 < len(MODELS):
            print("Cooling down 60s...", flush=True)
            time.sleep(60)

    OUT_JSON.write_text(json.dumps(report, indent=2) + "\n")
    print(f"\nSaved {OUT_JSON}", flush=True)


if __name__ == "__main__":
    main()
