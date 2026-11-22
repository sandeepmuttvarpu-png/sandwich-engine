import subprocess
import os

repo_dir = "/home/sandeep/dev/sandwich/sandwich-engine"
docs_dir = os.path.join(repo_dir, "docs/history")
os.makedirs(docs_dir, exist_ok=True)

out = subprocess.check_output([
    "git", "log", "--reverse", "--stat",
    "--format=@@@%H|%ad|%an <%ae>|%s",
    "--date=iso-strict",
    "3837bf0bc02ea5d08f8ee3585fc8c7784a81bce8..HEAD"
], cwd=repo_dir).decode("utf-8", errors="replace")

raw_commits = [c for c in out.split("@@@") if c.strip()]

output_file = os.path.join(docs_dir, "commit_inventory.md")
with open(output_file, "w", encoding="utf-8") as f:
    f.write("# Mechanical Commit Inventory: sandwich-engine\n\n")
    f.write("This document records every sandwich-authored commit from upstream fork point `3837bf0bc02ea5d08f8ee3585fc8c7784a81bce8` to `HEAD`.\n\n")
    f.write("- **Upstream Repository:** `https://github.com/ggerganov/llama.cpp.git`\n")
    f.write("- **Fork Base SHA:** `3837bf0bc02ea5d08f8ee3585fc8c7784a81bce8` (vulkan: Divide GROUP_NORM workgroup to 2D to avoid maxComputeWorkGroupCount limit, 2026-11-10)\n")
    f.write(f"- **Total Sandwich Commits:** {len(raw_commits)}\n\n")
    f.write("---\n\n")

    for i, item in enumerate(raw_commits):
        lines = item.strip().split("\n")
        header = lines[0]
        stat_lines = lines[1:] if len(lines) > 1 else []
        parts = header.split("|", 3)
        sha, date, author, subj = parts[0], parts[1], parts[2], parts[3]

        f.write(f"### {i+1}. [`{sha[:9]}`] {subj}\n")
        f.write(f"- **Full SHA:** `{sha}`\n")
        f.write(f"- **Date:** `{date}`\n")
        f.write(f"- **Author:** `{author}`\n")
        f.write("- **Diffstat:**\n```text\n")
        stat_text = "\n".join(l for l in stat_lines if l.strip())
        f.write(stat_text + "\n```\n\n")

print(f"Generated {output_file} with {len(raw_commits)} commits.")
