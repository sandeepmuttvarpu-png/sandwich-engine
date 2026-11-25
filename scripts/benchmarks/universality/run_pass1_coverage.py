#!/usr/bin/env python3
"""
Pass 1 Coverage Sweep Runner for Sandwich Engine Universality Survey.
Runs one short generation run per model:
  --sandwich-auto, -c 4096 (or 512 for Supra), -n 16, --temp 0, --seed 42, --spec-type none, --simple-io, -st, -p "Hello"
Logs: loaded, auto plan, load_s, peak_vram, peak_vmhwm, first_16_tokens, exit code, error string, verdict.
Enforces 60s cooldown between models, logs GPU temperature at each model start.
"""

import os
import sys
import json
import time
import re
import threading
import subprocess

SANDWICH_BIN = "/home/sandeep/dev/sandwich/sandwich-engine/build/bin/sandwich"
OUTPUT_JSON = "/home/sandeep/dev/sandwich/sandwich-engine/scripts/benchmarks/universality/coverage.json"
INVENTORY_JSON = "/home/sandeep/dev/sandwich/sandwich-engine/scripts/benchmarks/universality/inventory.json"

RUN_ORDER_REL = [
    # 1. Flash Next (hardest, most informative)
    "moe/Qwen3.8-Flash-Next-GSQ-RCO-GGUF/IQ3_XXS/Qwen3.8-Flash-Next-GSQ-RCO-abliterated-IQ3_XXS-00001-of-00002.gguf",
    # 2. Unproven architectures
    "moe/flint-GGUF/flint-UD-IQ3_XXS.gguf",
    "moe/Inkling-2-Mini-GGUF/Inkling-2-Mini-UD-Q4_K_M.gguf",
    "moe/LFM2-8B-A1B-Instruct-Quantum-IQ1C-Qwen3.6-35B-A3B-DISTILL-i1-GGUF/LFM2-8B-A1B-Instruct-Quantum-IQ1C-Qwen3.6-35B-A3B-DISTILL.i1-IQ2_XXS.gguf",
    "dense/NVIDIA-Nemotron-Labs-Dory1.0-4B-GGUF/NVIDIA-Nemotron-Labs-Dory1.0-4B-Q5_K_M.gguf",
    "dense/Muse-Glimmer-30B-GGUF/Muse-Glimmer-30B-KQuant-17GB-Q4_K_M.gguf",
    # 3. Regression check
    "moe/Qwen3.6-35B-A3B-MTP-GGUF/Qwen3.6-35B-A3B-UD-IQ3_XXS.gguf",
    # 4. Standard suite
    "dense/dolphin-2.8-mistral-7b-v02-GGUF/dolphin-2.8-mistral-7b-v02-Q4_K_S.gguf",
    "dense/Qwen3.8-27B-GGUF/Qwen3.8-27B-UD-IQ3_XXS.gguf",
    "dense/gemma-4-31B-it-QAT-GGUF/gemma-4-31B-it-QAT-Q4_0.gguf",
    "dense/c4ai-command-r-08-2024-GGUF/c4ai-command-r-08-2024-IQ3_XS.gguf",
    "dense/NVIDIA-Nemotron-3-Nano-4B-GGUF/NVIDIA-Nemotron-3-Nano-4B-Q6_K.gguf",
    "dense/Bonsai-27B-GGUF/Bonsai-27B-Q1_0.gguf",
    "moe/DeepSeek-Coder-V2-Lite-Instruct-GGUF/DeepSeek-Coder-V2-Lite-Instruct-IQ3_XXS.gguf",
    "moe/GLM-4.7-Flash-GGUF/GLM-4.7-Flash-Q4_K_M.gguf",
    "moe/Nemotron-3-Nano-30B-A3B-GGUF/Nemotron-3-Nano-30B-A3B-UD-IQ3_XXS.gguf",
    # 5. Small model last
    "dense/Supra2.5-Pro-Instruct/Supra2.5-65M.F16.gguf"
]

def get_gpu_temp():
    try:
        out = subprocess.check_output(
            ["nvidia-smi", "--query-gpu=temperature.gpu", "--format=csv,noheader,nounits"]
        ).decode().strip()
        return float(out)
    except Exception:
        return 0.0

def sample_mem(pid, stop_event, mem_rec):
    peak_vram = 0
    peak_vmhwm = 0
    while not stop_event.is_set():
        try:
            with open(f"/proc/{pid}/status") as f:
                for line in f:
                    if line.startswith("VmHWM:"):
                        val = int(line.split()[1]) // 1024  # MB
                        if val > peak_vmhwm: peak_vmhwm = val
        except Exception:
            pass
        try:
            out = subprocess.check_output(
                ["nvidia-smi", "--query-gpu=memory.used", "--format=csv,noheader,nounits"]
            ).decode().strip()
            val = int(out)
            if val > peak_vram: peak_vram = val
        except Exception:
            pass
        time.sleep(0.05)
    mem_rec["vmhwm"] = peak_vmhwm
    mem_rec["vram"] = peak_vram

def run_model(model_info, idx, total):
    rel_path = model_info["rel_path"]
    full_path = model_info["path"]
    arch = model_info["architecture"]
    size_gb = model_info["file_size_gb"]
    
    start_temp = get_gpu_temp()
    print(f"\n=======================================================")
    print(f"[{idx}/{total}] RUNNING: {rel_path}")
    print(f"Arch: {arch} | Size: {size_gb:.2f} GB | Start GPU Temp: {start_temp}°C")
    print(f"=======================================================")
    
    ctx = 512 if "Supra2.5" in rel_path else 4096
    timeout_s = 600 if "Qwen3.8-Flash-Next" in rel_path else 180
    
    cmd = [
        SANDWICH_BIN,
        "-m", full_path,
        "--sandwich-auto",
        "-c", str(ctx),
        "-n", "16",
        "--temp", "0",
        "--seed", "42",
        "--spec-type", "none",
        "--simple-io",
        "-st",
        "-p", "Hello"
    ]
    
    t0 = time.time()
    stop_event = threading.Event()
    mem_rec = {}
    
    try:
        p = subprocess.Popen(
            cmd,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        th = threading.Thread(target=sample_mem, args=(p.pid, stop_event, mem_rec))
        th.start()
        
        out, err = p.communicate(timeout=timeout_s)
        stop_event.set()
        th.join()
        rc = p.returncode
        timed_out = False
    except subprocess.TimeoutExpired:
        p.kill()
        stop_event.set()
        th.join()
        out, err = p.communicate()
        rc = -9
        timed_out = True
    except Exception as ex:
        stop_event.set()
        th.join()
        out, err = "", str(ex)
        rc = -1
        timed_out = False
        
    wall_s = round(time.time() - t0, 2)
    peak_vram = mem_rec.get("vram", 0)
    peak_vmhwm = mem_rec.get("vmhwm", 0)
    
    combined_log = (out or "") + "\n" + (err or "")
    
    # 1. Parse Auto-Plan
    m_plan = re.search(r"• Auto-Plan\s*:\s*(.*)", out or "")
    auto_plan = m_plan.group(1).strip() if m_plan else "N/A"
    
    # Also capture resolved model topology line from sandwich
    m_topo = re.search(r"• Model Topology:\s*(.*)", out or "")
    resolved_topo = m_topo.group(1).strip() if m_topo else "N/A"
    
    # 2. Parse Load seconds
    m_load = re.search(r"load:\s*([\d.]+)s", combined_log)
    load_s = float(m_load.group(1)) if m_load else 0.0
    
    # 3. Parse First 16 tokens verbatim
    first_16 = ""
    idx_p = (out or "").find("> Hello\n")
    if idx_p != -1:
        gen = out[idx_p + len("> Hello\n"):]
        idx_t = gen.find("\n[ Prompt:")
        if idx_t != -1:
            gen = gen[:idx_t]
        first_16 = gen.strip()
        
    # 4. Parse any error string
    error_str = ""
    if rc != 0 or timed_out:
        if timed_out:
            error_str = f"TIMED OUT after {timeout_s}s"
        else:
            # find error lines in stderr or stdout
            err_lines = [l for l in combined_log.splitlines() if any(k in l.lower() for k in ["error", "fatal", "failed", "abort", "exception", "cannot", "unknown"])]
            error_str = "\n".join(err_lines[-10:]) if err_lines else combined_log[-1000:].strip()
            
    # 5. Determine Verdict
    # Verdict per model: LOADS+GENERATES / LOADS+GARBAGE / FAILS.
    loaded = (rc == 0) and (load_s > 0 or "Wall-Time Breakdown:" in combined_log)
    if loaded and len(first_16) > 0 and not any(k in first_16.lower() for k in ["nan", "null"]):
        verdict = "LOADS+GENERATES"
    elif loaded and (len(first_16) == 0 or first_16.count(first_16[:3]) > 10):
        verdict = "LOADS+GARBAGE"
    else:
        verdict = "FAILS"
        
    print(f"VERDICT      : {verdict} (rc={rc})")
    print(f"Resolved Plan: {auto_plan} (Topology: {resolved_topo})")
    print(f"Load Time    : {load_s}s | Wall Time: {wall_s}s")
    print(f"Peak VRAM    : {peak_vram} MiB | Peak VmHWM: {peak_vmhwm} MiB")
    print(f"Tokens       : {repr(first_16[:80])}")
    if error_str:
        print(f"Error String : {error_str[:300]}")
        
    res = {
        "rel_path": rel_path,
        "name": model_info["name"],
        "architecture": arch,
        "parameter_count_str": model_info["parameter_count_str"],
        "quant_type": model_info["quant_type"],
        "file_size_gb": size_gb,
        "context_length": ctx,
        "start_temp_c": start_temp,
        "loaded": loaded,
        "resolved_plan": auto_plan,
        "resolved_topology": resolved_topo,
        "load_seconds": load_s,
        "wall_seconds": wall_s,
        "peak_vram_mib": peak_vram,
        "peak_vmhwm_mib": peak_vmhwm,
        "first_16_tokens": first_16,
        "exit_code": rc,
        "error_string": error_str,
        "verdict": verdict
    }
    return res

def main():
    inv = json.load(open(INVENTORY_JSON))
    targets_by_rel = {t["rel_path"]: t for t in inv["targets"]}
    
    ordered_targets = []
    for rel in RUN_ORDER_REL:
        if rel in targets_by_rel:
            ordered_targets.append(targets_by_rel[rel])
        else:
            print(f"WARNING: rel path not found in inventory targets: {rel}")
            
    total = len(ordered_targets)
    print(f"Starting Pass 1 Coverage Sweep on {total} targets in prioritized order.")
    
    results = []
    for idx, t in enumerate(ordered_targets, 1):
        res = run_model(t, idx, total)
        results.append(res)
        
        # Write incremental results after every model
        with open(OUTPUT_JSON, "w") as f:
            json.dump({"timestamp": time.strftime("%Y-%m-%dT%H:%M:%S%z"), "results": results}, f, indent=2)
            
        if idx < total:
            print(f"Cooldown 60s before next model...")
            time.sleep(60)
            
    print("\nPass 1 Coverage Sweep Completed Successfully.")

if __name__ == "__main__":
    main()
