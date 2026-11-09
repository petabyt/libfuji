#!/usr/bin/env bash
# Runs the C core against petabyt/vcam, an emulated Fujifilm camera.
# This validates the protocol code paths (setup, gallery, thumbnails, chunked
# download, cancel/abort, resume, liveview socket) -- it does NOT validate
# X-T30 III behaviour, which needs the real camera.
#
# Usage: VCAM=/path/to/vcam ios/scripts/vcam-integration.sh [phase...]
#   Build vcam: git clone https://github.com/petabyt/vcam && cmake -G Ninja -B build && cmake --build build
#   (needs libusb-1.0-0-dev libexif-dev)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
VCAM="${VCAM:?set VCAM to the vcam binary}"
VCAM_MODEL="${VCAM_MODEL:-fuji_x_s10}"
WORK="${WORK_DIR:-$ROOT/ios/.build-vcam}"
PHASES=("$@")
[ ${#PHASES[@]} -gt 0 ] || PHASES=(main setup-abort download-abort liveview)

mkdir -p "${WORK:?}/card/DCIM/100_FUJI"
VCAM_BIN_DIR="$(cd "$(dirname "$VCAM")/.." && pwd)/bin"
# Test card: a 3.9MB JPEG with an EXIF thumbnail (multi-chunk download) plus small files
cp "$VCAM_BIN_DIR/test.jpg" "$WORK/card/DCIM/100_FUJI/DSCF0001.JPG"
cp "$VCAM_BIN_DIR/fuji/jpeg-compressed.jpg" "$WORK/card/DCIM/100_FUJI/DSCF0002.JPG"
cp "$VCAM_BIN_DIR/fuji/lv2.jpg" "$WORK/card/DCIM/100_FUJI/DSCF0003.JPG"

NO_RUN=1 BUILD_DIR="$WORK/c" "$ROOT/ios/scripts/c-core-check.sh" "$ROOT/ios/FujiKit/Tests/CTests/vcam_integration.c"

wait_port_free() {
	if ! command -v ss >/dev/null 2>&1; then
		sleep 2 # no ss(8): give the previous vcam time to release its ports
		return 0
	fi
	for _ in $(seq 1 60); do
		if ! (ss -tan 2>/dev/null | grep -qE ":5574[0-2] .*(LISTEN|ESTAB|TIME-WAIT)"); then return 0; fi
		sleep 1
	done
	echo "ports 55740-55742 still busy" >&2
}

overall=0
for phase in "${PHASES[@]}"; do
	wait_port_free
	(cd "$(dirname "$VCAM")/.." && exec "$VCAM" "$VCAM_MODEL" tcp --ip 127.0.0.1 --fs "$WORK/card") >"$WORK/vcam-$phase.log" 2>&1 &
	vcam_pid=$!
	sleep 1.5
	echo "=== phase $phase (vcam $VCAM_MODEL, pid $vcam_pid)"
	if ! timeout 120 "$WORK/c/c-test" "$phase" "$WORK/card" 2>&1 | grep -v "PTP String does not have null terminator"; then
		overall=1
	fi
	kill "$vcam_pid" 2>/dev/null || true
	wait "$vcam_pid" 2>/dev/null || true
done
exit $overall
