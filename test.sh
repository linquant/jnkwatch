#!/usr/bin/env bash
# Build and run the unit tests (pure logic, no display needed).
set -euo pipefail
cd "$(dirname "$0")"
cmake -S . -B build >/dev/null
# Build every registered test binary (not just one) so ctest never runs a
# stale/missing binary after a source change.
cmake --build build -j"$(nproc)" \
  --target test_watchface_logic test_dotfont test_timer \
           test_settings test_wrist test_activity test_notify test_actions \
           test_pixelstats test_screentime test_weather watch_pixels >/dev/null
ctest --test-dir build --output-on-failure
