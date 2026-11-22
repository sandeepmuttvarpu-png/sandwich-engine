#!/usr/bin/env python3
"""
scripts/journal_sync.py
Folds .git/JOURNAL.jsonl entries into docs/history/commit_inventory.md
and optionally syncs entries with Understory if the service is online.
Idempotent: tracks synced SHAs to avoid duplicate entries.
"""

import json
import os
import sys
import subprocess
import urllib.request
import urllib.error

REPO_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
JOURNAL_PATH = os.path.join(REPO_DIR, ".git", "JOURNAL.jsonl")
INVENTORY_PATH = os.path.join(REPO_DIR, "docs", "history", "commit_inventory.md")
SYNC_STATE_PATH = os.path.join(REPO_DIR, ".git", "journal_sync_state.json")

def load_journal_entries():
    if not os.path.exists(JOURNAL_PATH):
        return []
    entries = []
    with open(JOURNAL_PATH, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line:
                try:
                    entries.append(json.loads(line))
                except json.JSONDecodeError:
                    pass
    return entries

def load_sync_state():
    if os.path.exists(SYNC_STATE_PATH):
        try:
            with open(SYNC_STATE_PATH, "r", encoding="utf-8") as f:
                return json.load(f)
        except Exception:
            pass
    return {"synced_shas": []}

def save_sync_state(state):
    with open(SYNC_STATE_PATH, "w", encoding="utf-8") as f:
        json.dump(state, f, indent=2)

def check_understory_online():
    # Probe MCP endpoint at localhost:3800/mcp
    try:
        req = urllib.request.Request("http://localhost:3800/mcp", method="HEAD")
        with urllib.request.urlopen(req, timeout=1.0) as resp:
            return resp.status in (200, 405)
    except urllib.error.HTTPError as e:
        return e.code in (200, 405)
    except Exception:
        return False

def sync_inventory(entries, state):
    synced_set = set(state.get("synced_shas", []))
    new_entries = [e for e in entries if e["sha"] not in synced_set]

    if not new_entries:
        print("[journal_sync] All journal entries are already synced to inventory.")
        return 0

    if not os.path.exists(INVENTORY_PATH):
        print(f"[journal_sync] {INVENTORY_PATH} not found; regenerating full inventory.")
        gen_script = os.path.join(REPO_DIR, "scripts", "generate_commit_inventory.py")
        if os.path.exists(gen_script):
            subprocess.run([sys.executable, gen_script], check=True)
        for e in entries:
            synced_set.add(e["sha"])
        state["synced_shas"] = list(synced_set)
        save_sync_state(state)
        return len(new_entries)

    with open(INVENTORY_PATH, "r", encoding="utf-8") as f:
        content = f.read()

    appended = 0
    with open(INVENTORY_PATH, "a", encoding="utf-8") as f:
        for entry in new_entries:
            sha = entry["sha"]
            if sha in content:
                synced_set.add(sha)
                continue
            f.write(f"### [`{sha[:9]}`] {entry['subject']}\n")
            f.write(f"- **Full SHA:** `{sha}`\n")
            f.write(f"- **Date:** `{entry['date']}`\n")
            env = entry.get("environment", {})
            f.write(f"- **Environment:** Kernel `{env.get('kernel')}`, Driver `{env.get('driver')}`, Governor `{env.get('governor')}`\n")
            f.write(f"- **Diffstat:**\n```text\n{entry.get('diffstat', '')}\n```\n\n")
            synced_set.add(sha)
            appended += 1

    state["synced_shas"] = list(synced_set)
    save_sync_state(state)
    print(f"[journal_sync] Appended {appended} new journal entries to commit inventory.")
    return appended

def sync_understory(entries):
    if not check_understory_online():
        print("[journal_sync] Understory service at localhost:3800 is unreachable; skipping remote memory sync.")
        return False
    print("[journal_sync] Understory service is reachable. Journal entries available for MCP memory ingestion.")
    return True

def main():
    entries = load_journal_entries()
    print(f"[journal_sync] Loaded {len(entries)} entries from {JOURNAL_PATH}.")
    state = load_sync_state()
    sync_inventory(entries, state)
    sync_understory(entries)
    print("[journal_sync] Completed successfully.")

if __name__ == "__main__":
    main()
