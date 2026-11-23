#!/usr/bin/env bash
# Installs git hooks for sandwich-engine
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
HOOKS_DIR="$REPO_ROOT/.git/hooks"

if [ ! -d "$HOOKS_DIR" ]; then
    echo "Error: .git/hooks directory not found at $HOOKS_DIR" >&2
    exit 1
fi

mkdir -p "$HOOKS_DIR"
cp "$SCRIPT_DIR/git-hooks/post-commit" "$HOOKS_DIR/post-commit"
chmod +x "$HOOKS_DIR/post-commit"
echo "Successfully installed post-commit hook to $HOOKS_DIR/post-commit"
