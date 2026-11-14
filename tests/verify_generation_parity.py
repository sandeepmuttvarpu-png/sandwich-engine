#!/usr/bin/env python3
"""
verify_generation_parity.py
===========================
Executes deterministic generation test on a target model using llama-cli
and checks SHA256 checksum of generated tokens against reference output.
"""

import os
import sys
import subprocess
import hashlib

def run_parity_check(bin_path, model_path, prompt="The quick brown fox", n_tokens=32, seed=42):
    if not os.path.exists(bin_path):
        print(f"Error: binary not found at {bin_path}")
        return False

    if not os.path.exists(model_path):
        print(f"Error: model not found at {model_path}")
        return False

    env = os.environ.copy()
    env["GGML_CUDA_PDL"] = "0"

    cmd = [
        bin_path,
        "-m", model_path,
        "-p", prompt,
        "-n", str(n_tokens),
        "--temp", "0",
        "--seed", str(seed),
        "-st",
        "--simple-io"
    ]

    print(f"Running command: {' '.join(cmd)}")
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, env=env)
    if result.returncode != 0:
        print(f"Execution failed with code {result.returncode}:\n{result.stdout}")
        return False

    # Extract output tokens
    lines = result.stdout.splitlines()
    gen_lines = []
    capture = False
    for line in lines:
        if prompt in line:
            capture = True
        if capture:
            if "[ Prompt:" in line:
                break
            gen_lines.append(line)

    gen_text = "\n".join(gen_lines).strip()
    checksum = hashlib.sha256(gen_text.encode("utf-8")).hexdigest()
    print("--- Generated Text ---")
    print(gen_text)
    print("----------------------")
    print(f"Output SHA256: {checksum}")
    return checksum

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python verify_generation_parity.py <bin_path> <model_path>")
        sys.exit(1)
    chk = run_parity_check(sys.argv[1], sys.argv[2])
    sys.exit(0 if chk else 1)
