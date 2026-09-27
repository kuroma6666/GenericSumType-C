#!/usr/bin/env bash
#
# CI と同じ検査を手元で実行する。CI（.github/workflows/ci.yml）はこのスクリプトを呼ぶだけ。
#
#   scripts/check.sh                       gcc / c11 / MODE=default
#   CC=clang STD=c99 scripts/check.sh      コンパイラ・C標準を切り替える
#   MODE=sanitize scripts/check.sh         -O2 + ASan + UBSan で examples / tests / spec を実行
#   MODE=cross CC=arm-none-eabi-gcc CROSS_CFLAGS="-mcpu=cortex-m4 -mthumb" scripts/check.sh
#                                          組込み向けクロスコンパイラでのコンパイルのみ（実行しない）
#   MODE=strict   scripts/check.sh         利用側が付けがちな厳格警告を足して examples / tests をビルド
#                                          （gcc では -fanalyzer も -Werror で実行する）
#
# examples/*.c の追加フラグは、ファイル内の `ci-flags: <flags>` 行で宣言する。
set -eu

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
STD="${STD:-c11}"
MODE="${MODE:-default}"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

BASE=(-std="$STD" -pedantic -Wall -Wextra -Werror -Iinclude)
STRICT=(-Wswitch-default -Wswitch-enum -Wshadow -Wconversion -Wcast-qual -Wundef
        -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls)
SAN=(-O2 -g -fsanitize=address,undefined -fno-sanitize-recover=all)

case "$MODE" in
    default)  EXTRA=();                 TEST_EXTRA=(-g -fsanitize=address) ;;
    sanitize) EXTRA=("${SAN[@]}");      TEST_EXTRA=() ;;
    strict)   EXTRA=(-O2 "${STRICT[@]}"); TEST_EXTRA=() ;;
    # shellcheck disable=SC2206
    cross)    EXTRA=(-Os "${STRICT[@]}" ${CROSS_CFLAGS:-}); TEST_EXTRA=() ;;
    *) echo "unknown MODE: $MODE" >&2; exit 2 ;;
esac

step() { printf '\n== %s ==\n' "$*"; }
ci_flags() { sed -n 's/.*ci-flags:[[:space:]]*\(.*\)$/\1/p' "$1" | head -1; }

step "$CC $STD MODE=$MODE"
"$CC" --version | head -1

if [ "$MODE" = cross ]; then
    step "cross: compile examples / tests (no run)"
    for f in examples/*.c tests/test_*.c; do
        # POSIX スレッドに依存するデモはベアメタル向けには対象外
        if ci_flags "$f" | grep -q pthread; then echo "skip $f"; continue; fi
        "$CC" "${BASE[@]}" "${EXTRA[@]}" -c -o "$OUT/cross.o" "$f"
        echo "ok   $f"
    done
    step "cross: spec (run-pass is compile only)"
    CC="$CC" STD="$STD" EXTRA_CFLAGS="${CROSS_CFLAGS:-}" COMPILE_ONLY=1 bash spec/run.sh
    echo
    echo "all checks passed ($CC $STD MODE=$MODE)"
    exit 0
fi

step "examples: build and run"
for f in examples/*.c; do
    bin="$OUT/$(basename "$f" .c)"
    # shellcheck disable=SC2046
    "$CC" "${BASE[@]}" ${EXTRA[@]+"${EXTRA[@]}"} $(ci_flags "$f") -o "$bin" "$f"
    "$bin" > "$bin.log" || { echo "FAIL: $f"; cat "$bin.log"; exit 1; }
    echo "ok   $f"
done

step "tests: build and run"
for f in tests/test_*.c; do
    bin="$OUT/$(basename "$f" .c)"
    # shellcheck disable=SC2046
    "$CC" "${BASE[@]}" ${EXTRA[@]+"${EXTRA[@]}"} ${TEST_EXTRA[@]+"${TEST_EXTRA[@]}"} $(ci_flags "$f") -o "$bin" "$f"
    "$bin" > "$bin.log" || { echo "FAIL: $f"; cat "$bin.log"; exit 1; }
    echo "ok   $f ($(tail -1 "$bin.log"))"
done

if [ "$MODE" = strict ] && "$CC" --version | grep -qi gcc && ! "$CC" --version | grep -qi clang; then
    step "gcc -fanalyzer: examples / tests"
    for f in examples/*.c tests/test_*.c; do
        # shellcheck disable=SC2046
        "$CC" "${BASE[@]}" -O1 -fanalyzer $(ci_flags "$f") -c -o "$OUT/analyzer.o" "$f"
        echo "ok   $f"
    done
fi

if [ "$MODE" != strict ]; then
    step "spec: compile-time guarantees"
    CC="$CC" STD="$STD" EXTRA_CFLAGS="${EXTRA[*]+"${EXTRA[*]}"}" bash spec/run.sh
fi

step "docs: GUARANTEES.md in sync / links"
bash spec/emit.sh --check
python3 spec/check_links.py

echo
echo "all checks passed ($CC $STD MODE=$MODE)"
