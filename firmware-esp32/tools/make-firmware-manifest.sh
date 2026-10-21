#!/usr/bin/env bash
# Generate firmware.json (the OTA update manifest read by the Android app) from a
# built firmware binary. Computes the SHA-256 + size and reads the version from
# firmware/main/ble_server.c, then writes the manifest at the repo root.
#
# The app fetches this file at:
#   https://raw.githubusercontent.com/linquant/jnkwatch/main/firmware.json
# (see FirmwareRepo.MANIFEST_URL). The manifest's "url" points at the .bin, by
# default a GitHub release asset URL .../releases/download/<tag>/jnkwatch.bin.
#
# Usage:
#   tools/make-firmware-manifest.sh [--bin PATH] [--major N] [--minor N]
#                                   [--bin-url URL] [--out PATH]
# With no args it uses firmware/build/jnkwatch.bin, the FW_MAJOR/FW_MINOR from
# ble_server.c, and the release-asset URL for tag v<major>.<minor>.
set -euo pipefail

REPO="$(cd "$(dirname "$0")/../.." && pwd)"
PROJECT="github.com/linquant/jnkwatch"

BIN="$REPO/firmware-esp32/firmware/build/jnkwatch.bin"
OUT="$REPO/firmware.json"
MAJOR=""
MINOR=""
BIN_URL=""

while [ $# -gt 0 ]; do
  case "$1" in
    --bin)     BIN="$2"; shift 2 ;;
    --major)   MAJOR="$2"; shift 2 ;;
    --minor)   MINOR="$2"; shift 2 ;;
    --bin-url) BIN_URL="$2"; shift 2 ;;
    --out)     OUT="$2"; shift 2 ;;
    -h|--help) grep '^#' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "unknown arg: $1" >&2; exit 2 ;;
  esac
done

[ -f "$BIN" ] || { echo "firmware binary not found: $BIN (build it first with idf.py build)" >&2; exit 1; }

# Default version from the shared source of truth (core/version.h).
hdr="$REPO/firmware-esp32/core/version.h"
[ -n "$MAJOR" ] || MAJOR="$(grep -E '#define[[:space:]]+WF_FW_MAJOR' "$hdr" | awk '{print $3}')"
[ -n "$MINOR" ] || MINOR="$(grep -E '#define[[:space:]]+WF_FW_MINOR' "$hdr" | awk '{print $3}')"
[ -n "$MAJOR" ] && [ -n "$MINOR" ] || { echo "could not determine version; pass --major/--minor" >&2; exit 1; }

SIZE="$(stat -c %s "$BIN")"
SHA="$(sha256sum "$BIN" | awk '{print $1}')"
TAG="v${MAJOR}.${MINOR}"
[ -n "$BIN_URL" ] || BIN_URL="https://${PROJECT}/releases/download/${TAG}/jnkwatch.bin"

cat > "$OUT" <<JSON
{
  "major": ${MAJOR},
  "minor": ${MINOR},
  "size": ${SIZE},
  "sha256": "${SHA}",
  "url": "${BIN_URL}"
}
JSON

echo "Wrote $OUT:"
cat "$OUT"
cat <<EOF

Next steps to publish ${TAG}:
  1. Create a GitHub release tagged ${TAG} and attach ${BIN} as an asset named
     jnkwatch.bin (so the URL above resolves). Example with the gh CLI:
       gh release create ${TAG} "${BIN}#jnkwatch.bin" --repo linquant/jnkwatch \\
         --title "${TAG}" --notes-file NOTES.md
  2. Commit firmware.json to main so the app can read it:
       git add firmware.json && git commit -m "release: firmware ${TAG}" && git push
  3. Make sure FW_MAJOR/FW_MINOR in ble_server.c match ${MAJOR}.${MINOR} and that
     this is the build you attached, so the watch (which reports its own version)
     sees ${TAG} as newer.
EOF
