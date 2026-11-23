# Non-Destructive Laptop Archival: sandwich-cleanup-20261122

- **Archive Root:** `/home/sandeep/Archive/sandwich-cleanup-20261122/`
- **Device ID:** 56 (`/dev/nvme0n1p2`, identical filesystem, verified via `stat -c %d`)
- **Execution Date:** 2026-11-23T20:17:35+05:30
- **Approval Scope:** Explicit operator approval granted to move only `/home/sandeep/Downloads/TLauncher.jar`.

---

## 1. Archived Contents & Manifest

| Original Path | Archived Location | Class | Size (Bytes) | SHA256 |
| :--- | :--- | :---: | :---: | :--- |
| `/home/sandeep/Downloads/TLauncher.jar` | `Downloads/TLauncher.jar` | Old Installer | 10,189,227 | `77a85ddcf6b24ef72524d8615761f52a7970ce1a8b9db7302d91ad78cb66fe87` |

### Held In Place (Zero Deletions / No Moves)
- **28 Wallpaper & UI Decor Duplicates:** All 28 image/gif files in `~/Pictures/Wallpapers/` and `~/Pictures/ryodecors/` were kept in place because they are actively referenced by live desktop configurations (`quickshell` and `ryoku-arch`).
- **All Active Development & AI Files:** All git repositories, dotfiles, python virtual environments, and `.gguf` model weights were hard-excluded by safety guardrails.

---

## 2. Reversibility & Restoration Protocol

To restore archived files back to their original locations, use the provided reversible script:

```bash
# 1. Dry-run simulation (does not touch filesystem)
./docs/archive/restore_archive.sh --dry-run
# or from the archive root:
/home/sandeep/Archive/sandwich-cleanup-20261122/restore_archive.sh --dry-run

# 2. Live restore
./docs/archive/restore_archive.sh
# or from the archive root:
/home/sandeep/Archive/sandwich-cleanup-20261122/restore_archive.sh
```

Every restored file is verified against its declared original path in `MANIFEST.tsv`.
