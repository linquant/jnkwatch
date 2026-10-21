#!/usr/bin/env bash
# Report the black/lit pixel ratio per face (AMOLED usage proxy), headless.
# Usage: ./pixels.sh [--check]   (--check exits non-zero if over the per-face budget)
set -euo pipefail
cd "$(dirname "$0")"
cmake --build build --target watch_pixels -j"$(nproc)" >/dev/null
./build/watch_pixels "$@"
