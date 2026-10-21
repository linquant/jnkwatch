#!/usr/bin/env bash
# Render one watchface frame to a PNG (headless, no display needed).
# Usage: ./shot.sh [output.png]
set -euo pipefail
cd "$(dirname "$0")"
cmake --build build --target watch_shot -j"$(nproc)" >/dev/null
OUT="${1:-frame.png}"
./build/watch_shot "$OUT"
