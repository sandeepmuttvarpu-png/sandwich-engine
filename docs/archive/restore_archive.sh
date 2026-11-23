#!/usr/bin/env bash
# Reversible Restore Script for sandwich-cleanup-20261122
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -f "$SCRIPT_DIR/MANIFEST.tsv" ] && [ -d "$SCRIPT_DIR/Downloads" ]; then
    ARCHIVE_DIR="$SCRIPT_DIR"
else
    ARCHIVE_DIR="${ARCHIVE_DIR:-/home/sandeep/Archive/sandwich-cleanup-20261122}"
fi

MANIFEST="${MANIFEST:-$ARCHIVE_DIR/MANIFEST.tsv}"
if [ ! -f "$MANIFEST" ]; then
    MANIFEST="$SCRIPT_DIR/MANIFEST.tsv"
fi

DRY_RUN=0

for arg in "$@"; do
    case "$arg" in
        --dry-run)
            DRY_RUN=1
            ;;
        *)
            echo "Unknown option: $arg" >&2
            echo "Usage: $0 [--dry-run]" >&2
            exit 1
            ;;
    esac
done

if [ ! -f "$MANIFEST" ]; then
    echo "Error: Manifest not found at $MANIFEST" >&2
    exit 1
fi

if [ "$DRY_RUN" -eq 1 ]; then
    echo "DRY-RUN MODE: Simulating restore from $ARCHIVE_DIR based on $MANIFEST..."
else
    echo "Restoring files from $ARCHIVE_DIR based on $MANIFEST..."
fi

# Skip header and iterate
tail -n +2 "$MANIFEST" | while IFS=$'\t' read -r orig proposed cls size mtime sha same_dev; do
    if [ -f "$proposed" ]; then
        if [ "$DRY_RUN" -eq 1 ]; then
            echo "[DRY-RUN] Would restore: $proposed -> $orig"
        else
            mkdir -p "$(dirname "$orig")"
            echo "Restoring: $proposed -> $orig"
            mv -n "$proposed" "$orig"
        fi
    else
        echo "Notice: Archive copy does not exist: $proposed"
    fi
done

if [ "$DRY_RUN" -eq 1 ]; then
    echo "Dry-run verification complete. All operations simulated cleanly."
else
    echo "Restore complete."
fi
