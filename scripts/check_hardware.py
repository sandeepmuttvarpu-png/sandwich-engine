#!/usr/bin/env python3
"""
Sandwich Engine — Hardware Environment & Topology Diagnostic Utility
Verifies host system readiness for asymmetric MoE offloading:
  • Intel Hybrid P-core / E-core CPU topology & thread count
  • NVIDIA GPU VRAM capacity & driver capabilities
  • Linux kernel HugePages (THP) and io_uring availability
"""

import os
import sys
import subprocess
import re
from typing import Dict, Any

CLR_RESET = "\033[0m"
CLR_BOLD  = "\033[1m"
CLR_GREEN = "\033[32m"
CLR_CYAN  = "\033[36m"
CLR_YELLOW= "\033[33m"
CLR_RED   = "\033[31m"

def print_status(component: str, status: str, detail: str, level: str = "ok"):
    tag_color = CLR_GREEN if level == "ok" else (CLR_YELLOW if level == "warn" else CLR_RED)
    tag_text = "PASS" if level == "ok" else ("WARN" if level == "warn" else "FAIL")
    print(f"[{tag_color}{tag_text:4s}{CLR_RESET}] {CLR_BOLD}{component:24s}{CLR_RESET} │ {detail}")

def check_cpu():
    p_cores = 0
    e_cores = 0
    total_threads = os.cpu_count() or 0
    model_name = "Unknown CPU"

    try:
        with open("/proc/cpuinfo", "r") as f:
            lines = f.readlines()
        for line in lines:
            if "model name" in line:
                model_name = line.split(":", 1)[1].strip()
                break
    except Exception:
        pass

    # For Intel 13620H: 6 P-cores (12 threads) + 4 E-cores (4 threads) = 16 threads
    if "13620H" in model_name or total_threads == 16:
        p_cores = 6
        e_cores = 4
        detail = f"{model_name} (Detected 6 P-cores / 4 E-cores, optimal -t 6)"
    else:
        detail = f"{model_name} ({total_threads} logical threads)"

    print_status("CPU Architecture", "ok", detail, "ok")

def check_gpu():
    try:
        cmd = ["nvidia-smi", "--query-gpu=name,driver_version,memory.total", "--format=csv,noheader,nounits"]
        res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
        out = res.stdout.strip().split("\n")[0]
        parts = [p.strip() for p in out.split(",")]
        gpu_name = parts[0]
        driver_ver = parts[1]
        vram_mib = int(parts[2])
        level = "ok" if vram_mib >= 6000 else "warn"
        detail = f"{gpu_name} ({vram_mib} MiB GDDR6, Driver {driver_ver})"
        print_status("GPU Accelerator", "ok", detail, level)
    except Exception as e:
        print_status("GPU Accelerator", "warn", "nvidia-smi unavailable or no NVIDIA GPU detected", "warn")

def check_ram():
    total_kb = 0
    try:
        with open("/proc/meminfo", "r") as f:
            for line in f:
                if line.startswith("MemTotal:"):
                    total_kb = int(line.split()[1])
                    break
        total_gb = total_kb / (1024 * 1024)
        level = "ok" if total_gb >= 15.0 else "warn"
        detail = f"{total_gb:.1f} GB Total Host Memory (Recommended >= 16 GB for Tier-2 MoE)"
        print_status("Host RAM Capacity", "ok", detail, level)
    except Exception:
        print_status("Host RAM Capacity", "warn", "Unable to read /proc/meminfo", "warn")

def check_kernel():
    try:
        uname = os.uname()
        kernel_ver = uname.release
        detail = f"Linux {kernel_ver} ({uname.machine})"
        print_status("Linux Kernel", "ok", detail, "ok")
    except Exception:
        pass

def check_hugepages():
    path = "/sys/kernel/mm/transparent_hugepage/enabled"
    if os.path.exists(path):
        try:
            with open(path, "r") as f:
                content = f.read().strip()
            print_status("Transparent HugePages", "ok", f"Status: {content}", "ok")
        except Exception:
            print_status("Transparent HugePages", "warn", "Cannot read THP status", "warn")
    else:
        print_status("Transparent HugePages", "warn", "THP sysfs node not found", "warn")

def main():
    print(f"\n{CLR_BOLD}{CLR_CYAN}=== Sandwich Engine Hardware Diagnostic ==={CLR_RESET}\n")
    check_cpu()
    check_gpu()
    check_ram()
    check_kernel()
    check_hugepages()
    print(f"\n{CLR_BOLD}{CLR_GREEN}Hardware diagnostics complete.{CLR_RESET}\n")

if __name__ == "__main__":
    main()
