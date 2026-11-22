#!/usr/bin/env python3
"""
Ablation Benchmark for Technique #10: Persistent Prompt Prefix Caching (OpenCode Acceleration)
Tests prompt prefill latency (prompt_ms) on repeated long prompt prefixes:
- Arm A (Baseline): -cram 0 (Prompt prefix caching completely disabled)
- Arm B (Test):     -cram 512 (Prompt prefix caching active with 512MB RAM buffer)
Interleaved n=5 runs with 4s thermal cooling and performance power profile.
"""

import os
import sys
import time
import json
import signal
import urllib.request
import subprocess
import numpy as np

SERVER_BIN = "/home/sandeep/dev/sandwich/sandwich-engine/build/bin/llama-server"
MODEL_PATH = "/home/sandeep/ai-models/dense/NVIDIA-Nemotron-3-Nano-4B-GGUF/NVIDIA-Nemotron-3-Nano-4B-Q6_K.gguf"
PORT = 8094
URL = f"http://127.0.0.1:{PORT}"

PREFIX = (
    "You are an expert systems software engineer and AI acceleration researcher specializing in "
    "low-latency tensor graph compilers, heterogeneous memory tiering across HBM, DDR5 RAM, and "
    "NVMe Gen4 storage, and speculative decoding algorithms. In this context, consider the following "
    "system architecture details: The machine has an Intel 13th Gen Core processor with hybrid P-core "
    "and E-core architecture, an NVIDIA RTX 50-series laptop GPU with 8GB VRAM, and unified memory bandwidth "
    "constraints. When servicing large language models that exceed device VRAM, traditional implementations "
    "suffer from severe PCIe bottlenecking and excessive TLB misses due to 4KB small page paging. Sandwich "
    "engine solves this by introducing a 3-tier memory hierarchy, 2MB transparent hugepage alignment, "
    "dynamic layer offloading, and persistent prompt prefix caching in RAM. Explain the mathematical formulation "
    "of KV cache memory footprint per token given sequence length S, hidden dimension H, number of layers L, "
    "and head count K."
)

def wait_for_server(timeout=30):
    start = time.time()
    while time.time() - start < timeout:
        try:
            req = urllib.request.Request(f"{URL}/health")
            with urllib.request.urlopen(req, timeout=2) as resp:
                if resp.status == 200:
                    return True
        except Exception:
            time.sleep(0.5)
    return False

def query_completion(prompt, cache_prompt=True):
    data = json.dumps({
        "prompt": prompt,
        "n_predict": 4,
        "temperature": 0.0,
        "cache_prompt": cache_prompt,
    }).encode("utf-8")
    req = urllib.request.Request(f"{URL}/completion", data=data, headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=30) as resp:
        res = json.loads(resp.read().decode("utf-8"))
        return res

def run_single_arm(cram_val):
    # Launch server
    cmd = [
        SERVER_BIN,
        "-m", MODEL_PATH,
        "-ngl", "99",
        "-c", "2048",
        "-cram", str(cram_val),
        "--port", str(PORT),
        "-t", "6"
    ]
    proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        if not wait_for_server():
            raise RuntimeError(f"Server failed to start on port {PORT}")
        
        # 1. First query: establish the prefix
        query_completion(PREFIX, cache_prompt=True)
        time.sleep(0.5)
        
        # 2. Second query: identical prefix + slight suffix (evaluates prompt cache hit/miss)
        res = query_completion(PREFIX + " Furthermore, provide concrete examples.", cache_prompt=True)
        prompt_ms = res["timings"]["prompt_ms"]
        return prompt_ms
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()

def main():
    print("======================================================================")
    print("BENCHMARK: Technique #10 — Persistent Prompt Prefix Caching (-cram)")
    print("Source: common/arg.cpp:1745 & tools/server/server-context.cpp:1610")
    print(f"Model: Nemotron-3-Nano-4B-Q6_K | Port: {PORT} | Cooldown: 4s")
    print("======================================================================")
    
    os.system("powerprofilesctl set performance 2>/dev/null || true")
    
    times_a = [] # cram 0
    times_b = [] # cram 512
    
    for i in range(1, 6):
        print(f"\n--- Iteration {i}/5 ---")
        
        # Arm A: cram 0 (Disabled)
        print("  [Cooling 4s before Arm A]")
        time.sleep(4)
        t_a = run_single_arm(0)
        times_a.append(t_a)
        print(f"  Run A{i} (Baseline, -cram 0 disabled):  {t_a:.2f} ms prefill")
        
        # Arm B: cram 512 (Enabled)
        print("  [Cooling 4s before Arm B]")
        time.sleep(4)
        t_b = run_single_arm(512)
        times_b.append(t_b)
        print(f"  Run B{i} (Test, -cram 512 enabled):    {t_b:.2f} ms prefill")

    mu_a = float(np.mean(times_a))
    sigma_a = float(np.std(times_a, ddof=1))
    mu_b = float(np.mean(times_b))
    sigma_b = float(np.std(times_b, ddof=1))
    
    sigma_comb = float(np.sqrt(sigma_a**2 + sigma_b**2))
    two_sigma = 2.0 * sigma_comb
    delta = mu_a - mu_b # latency reduction
    delta_pct = (delta / mu_a) * 100.0
    
    print("\n========================= STATISTICAL ANALYSIS =========================")
    print(f"Arm A (Baseline -cram 0):   mu = {mu_a:.3f} ms, sigma = {sigma_a:.3f} ms")
    print(f"Arm B (Test -cram 512):     mu = {mu_b:.3f} ms, sigma = {sigma_b:.3f} ms")
    print(f"Delta (Prefill Reduction):  {delta:.3f} ms ({delta_pct:+.2f}%)")
    print(f"Combined Sigma:             {sigma_comb:.3f} ms")
    print(f"2-Sigma Threshold:          {two_sigma:.3f} ms")
    
    passed = (delta > two_sigma)
    verdict = "PROMOTED TO PROVEN" if passed else "KEPT PARTIAL (Inside Noise)"
    print(f"VERDICT: {verdict} (|Delta| = {abs(delta):.3f} vs 2sigma = {two_sigma:.3f})")
    print("========================================================================")
    
    result_data = {
        "technique_id": 10,
        "name": "Persistent Prompt Prefix Caching (OpenCode Acceleration)",
        "arm_a_mu": mu_a,
        "arm_a_sigma": sigma_a,
        "arm_b_mu": mu_b,
        "arm_b_sigma": sigma_b,
        "delta_ms": delta,
        "delta_pct": delta_pct,
        "sigma_comb": sigma_comb,
        "two_sigma_threshold": two_sigma,
        "passed": passed,
        "verdict": verdict
    }
    with open("/home/sandeep/dev/sandwich/sandwich-engine/tests/benchmarks/batch1/result_cram_10.json", "w") as f:
        json.dump(result_data, f, indent=2)

    return 0 if passed else 1

if __name__ == "__main__":
    sys.exit(main())
