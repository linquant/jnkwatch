#!/usr/bin/env bash
# Pull the battery log off the watch over USB and decode it.
# Usage: tools/batlog.sh [port]   (default /dev/ttyACM0)
# Writes logs/batlog-<date>.bin + .csv and prints a summary. Note: esptool
# resets the watch (it reboots afterwards; the log itself is untouched).
set -euo pipefail
cd "$(dirname "$0")/.."
PORT="${1:-/dev/ttyACM0}"
[ -n "${IDF_PATH:-}" ] || . ~/esp/esp-idf/export.sh >/dev/null 2>&1
mkdir -p logs
STAMP="$(date +%Y%m%d-%H%M%S)"
BIN="logs/batlog-$STAMP.bin"
python "$IDF_PATH/components/partition_table/parttool.py" --port "$PORT" \
    --partition-table-file firmware/partitions.csv \
    read_partition --partition-name batlog --output "$BIN"
python3 tools/batlog_decode.py "$BIN" "logs/batlog-$STAMP.csv"
echo "CSV: logs/batlog-$STAMP.csv"
