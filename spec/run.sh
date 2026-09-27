#!/usr/bin/env bash
#
# spec/manifest.tsv に書かれたコンパイル時保証を実際に検査する。
#
# このライブラリの価値の中核は「間違えたらコンパイルが落ちること」であり、
# それは通常の単体テストでは確認できない。判定器はコンパイラそのものであり、
# manifest.tsv がその述語の一覧＝仕様の実体である。
#
#   ./run.sh                    デフォルト: gcc, -std=c11
#   CC=clang STD=c17 ./run.sh   コンパイラ / C標準を切り替える
#   EXTRA_CFLAGS=-O2 ./run.sh   追加フラグ（最適化・サニタイザ等）
#   COMPILE_ONLY=1 ./run.sh     run-pass もコンパイルだけ行う（実行できないクロスコンパイラ用）
#   ./run.sh G-009 G-017        指定した id だけ実行する
#
# compile-fail のケースは対照コンパイルを必須とする。ケース断片は誤り箇所を
# `#ifndef SPEC_FIXED` で囲み、`-DSPEC_FIXED` を付けるとコンパイルが通らなければならない。
# これにより「そのケースが意図した1箇所だけが原因で落ちている」ことを、
# 診断メッセージの文言に依存せずに確かめる（無関係な typo で落ちても OK にならない）。
set -u

SPEC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HEADER_DIR="$(cd "$SPEC_DIR/../include" && pwd)"
MANIFEST="$SPEC_DIR/manifest.tsv"
CC="${CC:-gcc}"
STD="${STD:-c11}"
# shellcheck disable=SC2206
CFLAGS=(-std="$STD" -pedantic -Wall -Wextra -Werror -I"$HEADER_DIR" ${EXTRA_CFLAGS:-})

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

pass=0; fail=0; skip=0
only=("$@")

# 引数で id が指定されていれば、その id だけを対象にする
selected() {
    [ ${#only[@]} -eq 0 ] && return 0
    local id="$1" want
    for want in "${only[@]}"; do [ "$id" = "$want" ] && return 0; done
    return 1
}

# manifest の std 列と、いま検査している STD が噛み合うか
std_applies() {
    case "$1" in
        any)     return 0 ;;
        c11+)    [ "$STD" != "c99" ] && return 0 || return 1 ;;
        c99only) [ "$STD"  = "c99" ] && return 0 || return 1 ;;
        *)       echo "manifest: 未知の std 値 '$1'" >&2; return 1 ;;
    esac
}

report() { # report OK|NG id title detail
    if [ "$1" = OK ]; then pass=$((pass+1)); else fail=$((fail+1)); fi
    printf '%s  %-6s %s\n' "$1" "$2" "$3"
    [ -n "${4:-}" ] && printf '%s\n' "$4"
    return 0
}

run_case() {
    local id="$1" kind="$2" expect="$3" title="$4"
    local src="$SPEC_DIR/cases/$id.c"
    local dir="$SPEC_DIR/cases/$id"

    case "$expect" in
    compile-pass)
        if [ ! -f "$src" ]; then
            report NG "$id" "$title" "  ケース断片が無い: cases/$id.c"; return
        fi
        if "$CC" "${CFLAGS[@]}" -c "$src" -o "$TMP/out.o" 2>"$TMP/err.log"; then
            report OK "$id" "$title"
        else
            report NG "$id" "$title" "$(sed 's/^/  /' "$TMP/err.log" | head -20)"
        fi
        ;;
    compile-fail)
        if [ ! -f "$src" ]; then
            report NG "$id" "$title" "  ケース断片が無い: cases/$id.c"; return
        fi
        if ! grep -q 'SPEC_FIXED' "$src"; then
            report NG "$id" "$title" "  対照が無い: 誤り箇所を #ifndef SPEC_FIXED で囲むこと"; return
        fi
        if "$CC" "${CFLAGS[@]}" -c "$src" -o "$TMP/out.o" 2>"$TMP/err.log"; then
            report NG "$id" "$title" "  コンパイルが成功してしまった（失敗するはずだった）"; return
        fi
        if ! "$CC" "${CFLAGS[@]}" -DSPEC_FIXED -c "$src" -o "$TMP/out.o" 2>"$TMP/fix.log"; then
            report NG "$id" "$title" "  対照（-DSPEC_FIXED）も失敗した。落ちた原因が意図した箇所ではない:
$(sed 's/^/  /' "$TMP/fix.log" | head -20)"; return
        fi
        report OK "$id" "$title"
        ;;
    run-pass)
        local build="$TMP/$id"; mkdir -p "$build"
        local srcs=() inc=()
        if [ -d "$dir" ]; then
            srcs=("$dir"/*.c); inc=(-I"$dir")
        elif [ -f "$src" ]; then
            srcs=("$src")
        else
            report NG "$id" "$title" "  ケース断片が無い: cases/$id.c も cases/$id/ も存在しない"; return
        fi
        if [ "${COMPILE_ONLY:-0}" = 1 ]; then
            local f
            for f in "${srcs[@]}"; do
                if ! "$CC" "${CFLAGS[@]}" "${inc[@]}" -c "$f" -o "$build/$(basename "$f").o" 2>"$TMP/err.log"; then
                    report NG "$id" "$title" "$(sed 's/^/  /' "$TMP/err.log" | head -20)"; return
                fi
            done
            report OK "$id" "$title (compile only)"; return
        fi
        if ! "$CC" "${CFLAGS[@]}" "${inc[@]}" "${srcs[@]}" -o "$build/a.out" 2>"$TMP/err.log"; then
            report NG "$id" "$title" "$(sed 's/^/  /' "$TMP/err.log" | head -20)"; return
        fi
        if out=$("$build/a.out" 2>&1); then
            report OK "$id" "$title"
        else
            report NG "$id" "$title" "  ビルドは通ったが実行が失敗した${out:+
$(printf '%s' "$out" | sed 's/^/  /')}"
        fi
        ;;
    *)
        report NG "$id" "$title" "  manifest: 未知の expect 値 '$expect'"
        ;;
    esac
}

echo "CC=$CC STD=$STD  flags: ${CFLAGS[*]}"
echo

while IFS=$'\t' read -r id kind expect std ref title; do
    case "$id" in ''|\#*) continue ;; esac
    selected "$id" || continue
    if ! std_applies "$std"; then
        skip=$((skip+1))
        printf 'SKIP  %-6s %s (std=%s)\n' "$id" "$title" "$std"
        continue
    fi
    run_case "$id" "$kind" "$expect" "$title"
done < "$MANIFEST"

echo
echo "$pass 件成功 / $fail 件失敗 / $skip 件スキップ"
[ "$fail" -eq 0 ]
