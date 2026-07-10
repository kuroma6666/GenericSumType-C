#!/usr/bin/env bash
#
# generic_sum_type.h が提供する「コンパイル時保証」(design_spec.md 3節) が
# 実際に機能し続けているかを検証する回帰テスト。
#
# test_generic_sum_type.c のような通常の単体テストでは「正しく動くこと」しか
# 確認できない。このライブラリの価値の半分は「間違えたらコンパイルが落ちること」
# にあるため、それ専用のテストとして分けている。
#
# 実行: ./test_compile_guarantees.sh
#       CC=clang ./test_compile_guarantees.sh   (別コンパイラで検証する場合)
#       STD=c99  ./test_compile_guarantees.sh   (別のC標準で検証する場合。既定はc11)
set -u
HEADER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../include" && pwd)"
CC="${CC:-gcc}"
STD="${STD:-c11}"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

pass=0
fail=0

expect_fail() {
    local name="$1" file="$2"
    if "$CC" -I"$HEADER_DIR" -std="$STD" -Wall -Wextra -Werror -c "$file" -o "$TMP/out.o" 2>"$TMP/err.log"; then
        echo "NG  $name: コンパイルが成功してしまった（失敗するはずだった）"
        fail=$((fail+1))
    else
        echo "OK  $name: 期待通りコンパイル失敗"
        pass=$((pass+1))
    fi
}

expect_pass() {
    local name="$1" file="$2"
    if "$CC" -I"$HEADER_DIR" -std="$STD" -Wall -Wextra -Werror -c "$file" -o "$TMP/out.o" 2>"$TMP/err.log"; then
        echo "OK  $name: 期待通りコンパイル成功"
        pass=$((pass+1))
    else
        echo "NG  $name: コンパイルが失敗してしまった（成功するはずだった）"
        cat "$TMP/err.log"
        fail=$((fail+1))
    fi
}

# --- 1. ハンドラの渡し忘れ: 失敗するべき（design_spec.md 3節） ---
cat > "$TMP/missing_handler.c" << 'EOF'
#include "generic_sum_type.h"
typedef struct { int a, b; } PA;
typedef struct { int a, b; } PB;
typedef struct { int a, b; } PC;
#define V(X, NAME, EXTRA) \
    X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB) X(NAME, EXTRA, c, PC)
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_DISPATCH(T, V, T_dispatch, int)
void h(PA *p, int *c){(void)p;(void)c;}
int main(void) {
    T t = T_new_a((PA){0,0});
    int ctx = 0;
    T_dispatch(&t, &ctx, h); /* b, c用のハンドラを渡し忘れ */
    return 0;
}
EOF
expect_fail "ハンドラ渡し忘れ" "$TMP/missing_handler.c"

# --- 2. 異なる型のハンドラを順序入れ替え: 失敗するべき（design_spec.md 3節） ---
cat > "$TMP/swapped_diff_type.c" << 'EOF'
#include "generic_sum_type.h"
typedef struct { int target, dmg; } CmdAttack;
typedef struct { int target, amt; } CmdHeal;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, attack, CmdAttack) X(NAME, EXTRA, heal, CmdHeal)
DEFINE_SUM_TYPE(Cmd, V)
DEFINE_SUM_DISPATCH(Cmd, V, Cmd_dispatch, int)
void on_attack(CmdAttack *a, int *c){(void)a;(void)c;}
void on_heal(CmdHeal *h, int *c){(void)h;(void)c;}
int main(void) {
    Cmd c = Cmd_new_attack((CmdAttack){1,2});
    int ctx = 0;
    Cmd_dispatch(&c, &ctx, on_heal, on_attack); /* 順序入れ替え */
    return 0;
}
EOF
expect_fail "型の異なるハンドラの順序取り違え" "$TMP/swapped_diff_type.c"

# --- 3. 同一型のハンドラを順序入れ替え: 既知の限界としてコンパイルは通ってしまう ---
#         (design_spec.md 3節「検出できない残存ケース」の回帰確認。
#          将来これが失敗するようになったら設計判断が変わったということなので
#          ドキュメントの更新を忘れないこと)
cat > "$TMP/swapped_same_type.c" << 'EOF'
#include "generic_sum_type.h"
#include <stdint.h>
#define V(X, NAME, EXTRA) X(NAME, EXTRA, score, int32_t) X(NAME, EXTRA, penalty, int32_t)
DEFINE_SUM_TYPE(Cmd, V)
DEFINE_SUM_DISPATCH(Cmd, V, Cmd_dispatch, int)
void on_score(int32_t *v, int *c){(void)v;(void)c;}
void on_penalty(int32_t *v, int *c){(void)v;(void)c;}
int main(void) {
    Cmd c = Cmd_new_score(10);
    int ctx = 0;
    Cmd_dispatch(&c, &ctx, on_penalty, on_score); /* 型が同一なので検出できない(既知の限界) */
    return 0;
}
EOF
expect_pass "同一型ハンドラの順序取り違え(検出不能な既知の限界。design_spec.md 3節参照)" "$TMP/swapped_same_type.c"

# --- 4. 正常系: 成功するべき ---
cat > "$TMP/ok.c" << 'EOF'
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_DISPATCH(T, V, T_dispatch, int)
void ha(PA *p, int *c){(void)p;(void)c;}
void hb(PB *p, int *c){(void)p;(void)c;}
int main(void) {
    T t = T_new_a((PA){1});
    int ctx = 0;
    T_dispatch(&t, &ctx, ha, hb);
    return 0;
}
EOF
expect_pass "正常系" "$TMP/ok.c"

echo
echo "$pass 件成功 / $fail 件失敗"
exit "$fail"
