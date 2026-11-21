#!/usr/bin/env bash
# Builds the vendored C core + iOS shim on the host with clang, ASan/UBSan and
# Apple-clang-like hard errors (including the -Wshorten-64-to-32 and
# -Wunsequenced warnings Xcode reports), links the given test program and runs it.
#
# Usage: ios/scripts/c-core-check.sh [test.c] [args...]
#   default test: ios/FujiKit/Tests/CTests/smoke.c
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/ios/FujiKit/Sources/CFujiCore"
OUT="${BUILD_DIR:-$ROOT/ios/.build-c}"
TEST="${1:-$ROOT/ios/FujiKit/Tests/CTests/smoke.c}"
shift || true

mkdir -p "${OUT:?}"
find "${OUT:?}" -name '*.o' -delete

CFLAGS=(
	-std=gnu11 -g -O1
	-Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable
	-Werror=implicit-function-declaration -Werror=int-conversion
	-Werror=incompatible-pointer-types -Werror=return-type
	-Werror=shorten-64-to-32 -Werror=unsequenced
	-DEZXML_NOMMAP
	-I"$SRC/upstream/libpict" -I"$SRC/upstream/libfuji" -I"$SRC/upstream/fp" -I"$SRC/include"
	-fsanitize=address,undefined -fno-omit-frame-pointer
)

status=0
while IFS= read -r -d '' f; do
	obj="$OUT/$(echo "${f#"$SRC"/}" | tr '/' '_' | sed 's/\.c$/.o/')"
	if ! clang "${CFLAGS[@]}" -c "$f" -o "$obj" 2>"$obj.log"; then
		cat "$obj.log"
		status=1
	elif grep -q "warning" "$obj.log"; then
		grep -A2 "warning" "$obj.log" | head -20
	fi
done < <(find "$SRC/upstream" "$SRC/shim" -name '*.c' -print0 | sort -z)
[ "$status" -eq 0 ] || { echo "C core failed to compile" >&2; exit 1; }

clang "${CFLAGS[@]}" -c "$TEST" -o "$OUT/test_main.o"
clang -fsanitize=address,undefined "$OUT"/*.o -lpthread -o "$OUT/c-test"
echo "built $OUT/c-test"
if [ -z "${NO_RUN:-}" ]; then
	"$OUT/c-test" "$@"
fi
