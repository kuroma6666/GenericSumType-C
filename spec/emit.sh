#!/usr/bin/env bash
#
# spec/manifest.tsv とケース断片から docs/GUARANTEES.md を生成する。
#
#   ./emit.sh                 標準出力に書き出す
#   ./emit.sh --write         docs/GUARANTEES.md を上書きする
#   ./emit.sh --check         docs/GUARANTEES.md が最新か検査する（差分があれば exit 1）
#
# --check を CI に置くことで、「ドキュメントに書いた保証」と「実際に検査されている
# 保証」の乖離が構造的に起きなくなる。GUARANTEES.md を手で編集してはならない。
set -u

SPEC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MANIFEST="$SPEC_DIR/manifest.tsv"
OUT="$SPEC_DIR/../docs/GUARANTEES.md"

expect_ja() {
    case "$1" in
        compile-fail) echo "コンパイル失敗" ;;
        compile-pass) echo "コンパイル成功" ;;
        run-pass)     echo "ビルド・実行成功" ;;
        *)            echo "$1" ;;
    esac
}

std_ja() {
    case "$1" in
        any)     echo "C99 以降すべて" ;;
        c11+)    echo "C11 以降のみ" ;;
        c99only) echo "C99 のみ" ;;
        *)       echo "$1" ;;
    esac
}

# ref（リポジトリルート相対）を、docs/ から辿れるリンクと短いラベルに変換する。
#   docs/adr/0008-foo.md      -> [ADR-0008](adr/0008-foo.md)
#   docs/constraints.md#c-01  -> [C-01](constraints.md#c-01)
ref_link() {
    local ref="$1" path="${1%%#*}" anchor="${1#*#}" label
    case "$ref" in
        */adr/*) label="ADR-$(basename "$path" | cut -d- -f1)" ;;
        *\#c-*)  label="$(echo "$anchor" | tr 'a-z' 'A-Z')" ;;
        *)       label="$(basename "$path")" ;;
    esac
    printf '[%s](%s)' "$label" "${ref#docs/}"
}

emit_table() { # emit_table <kind>
    local want="$1" n=0
    echo "| ID | 期待結果 | 対象 | 内容 | 根拠 |"
    echo "|---|---|---|---|---|"
    while IFS=$'\t' read -r id kind expect std ref title; do
        case "$id" in ''|\#*) continue ;; esac
        [ "$kind" = "$want" ] || continue
        printf '| [%s](#%s) | %s | %s | %s | %s |\n' \
            "$id" "$(echo "$id" | tr 'A-Z' 'a-z')" \
            "$(expect_ja "$expect")" "$(std_ja "$std")" "$title" \
            "$(ref_link "$ref")"
        n=$((n+1))
    done < "$MANIFEST"
    [ "$n" = 0 ] && echo "| — | — | — | 該当なし | — |"
    return 0
}

emit_details() {
    while IFS=$'\t' read -r id kind expect std ref title; do
        case "$id" in ''|\#*) continue ;; esac
        echo
        # 見出し由来のスラグ（g-001--タイトル）ではなく #g-001 で参照できるよう明示アンカーを置く。
        # 他文書からは ID で参照する規約なので、タイトルを変えてもリンクが切れない。
        printf '<a id="%s"></a>\n' "$(echo "$id" | tr 'A-Z' 'a-z')"
        echo
        echo "### $id — $title"
        echo
        echo "| 種別 | 期待結果 | 対象 | 根拠 |"
        echo "|---|---|---|---|"
        printf '| %s | %s | %s | %s |\n' \
            "$([ "$kind" = guarantee ] && echo '保証' || echo '既知の限界')" \
            "$(expect_ja "$expect")" "$(std_ja "$std")" \
            "$(ref_link "$ref")"
        echo
        local src="$SPEC_DIR/cases/$id.c" dir="$SPEC_DIR/cases/$id" f
        if [ -f "$src" ]; then
            echo '```c'; cat "$src"; echo '```'
            echo
            echo "<sub>ケース断片: [\`spec/cases/$id.c\`](../spec/cases/$id.c)</sub>"
        elif [ -d "$dir" ]; then
            for f in "$dir"/*.h "$dir"/*.c; do
                [ -e "$f" ] || continue
                echo "\`$(basename "$f")\`:"
                echo
                echo '```c'; cat "$f"; echo '```'
                echo
            done
            echo "<sub>ケース断片: [\`spec/cases/$id/\`](../spec/cases/$id)</sub>"
        fi
    done < "$MANIFEST"
}

generate() {
    cat <<'HEADER'
# コンパイル時保証の一覧

> **このファイルは `spec/emit.sh` が `spec/manifest.tsv` とケース断片から生成する。手で編集しないこと。**
> 内容を変えたい場合は `spec/manifest.tsv` と `spec/cases/` を編集して `spec/emit.sh --write` を実行する。

`generic_sum_type.h` のランタイム挙動は `switch` 一つ分しかなく、このライブラリが
実際に約束しているのは「**どんなコードがコンパイルを通り、どんなコードが通らないか**」である。
したがってこの一覧がこのライブラリの仕様の中核であり、その判定器はコンパイラそのものになる。

各項目は `spec/run.sh` が CI（gcc / clang × c99 / c11 / c17）で毎回実行して検証している。

- **保証** — ライブラリが約束する挙動。破れたらバグとして扱う。
- **既知の限界** — 約束しない（できない）ことを固定したもの。将来検出できるようになってもよいが、
  そのときはこの表と根拠文書をあわせて更新する必要がある。

ビルドフラグは `-std=$STD -pedantic -Wall -Wextra -Werror`。
**`-Werror` を外すと順序取り違えの検出（G-002）が警告止まりになり保証が成立しない。**

## 保証

HEADER
    emit_table guarantee
    cat <<'MID'

## 既知の限界

MID
    emit_table known-limit
    cat <<'DETAIL'

---

## 各ケースの詳細
DETAIL
    emit_details
}

case "${1:-}" in
    --write) generate > "$OUT"; echo "生成しました: docs/GUARANTEES.md" ;;
    --check)
        if diff -u "$OUT" <(generate) > /tmp/guarantees.diff 2>&1; then
            echo "OK  docs/GUARANTEES.md は spec/ と同期しています"
        else
            echo "NG  docs/GUARANTEES.md が spec/ と食い違っています。spec/emit.sh --write を実行してください"
            cat /tmp/guarantees.diff
            exit 1
        fi
        ;;
    *) generate ;;
esac
