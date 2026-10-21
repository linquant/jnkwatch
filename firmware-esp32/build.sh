#!/usr/bin/env bash
# Configure + build every available target (watch_shot always; watch_sim if SDL2 is present).
set -euo pipefail
cd "$(dirname "$0")"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
echo
echo "Built binaries in ./build :"
ls -1 build/watch_* 2>/dev/null || true
