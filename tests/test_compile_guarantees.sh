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

# --- 5. TYPEに関数ポインタ型をtypedefせず直接渡す: 失敗するべき（design_spec.md 4.5節） ---
#         プリプロセッサのマクロ引数分割自体は括弧に保護されるため起きない。
#         実際に壊れる原因は「TYPE 識別子;」という宣言パターンが関数ポインタの
#         宣言子構文（TYPE (*識別子)(...)）と噛み合わないこと。この区別を
#         回帰テストとして固定する。
cat > "$TMP/raw_fnptr.c" << 'EOF'
#include "generic_sum_type.h"
typedef struct { int a; } PA;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, cb, void (*)(int,int))
DEFINE_SUM_TYPE(T, V)
int main(void) { return 0; }
EOF
expect_fail "TYPEに関数ポインタ型を直接渡す(typedefなし)" "$TMP/raw_fnptr.c"

# --- 6. TYPEを事前にtypedefしてから渡す: 成功するべき（design_spec.md 4.5節の回避策） ---
cat > "$TMP/typedef_fnptr.c" << 'EOF'
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef void (*Callback)(int, int);
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, cb, Callback)
DEFINE_SUM_TYPE(T, V)
static void my_cb(int x, int y) { (void)x; (void)y; }
int main(void) {
    T t = T_new_cb(my_cb);
    Callback *pc = T_get_cb(&t);
    return pc ? 0 : 1;
}
EOF
expect_pass "TYPEを事前にtypedefしてから渡す(関数ポインタ型の回避策)" "$TMP/typedef_fnptr.c"

# --- 7. 複数翻訳単位での共有: 成功するべき（design_spec.md 4.10節） ---
#         同じDEFINE_SUM_TYPEを1つの共通ヘッダ(インクルードガード付き)で定義し、
#         別々の.cファイルからincludeして値を受け渡しできるかを検証する。
#         単一ファイルの-cコンパイルでは検証できないため、専用の関数として分離している。
check_multi_tu() {
    local name="複数翻訳単位での共有(command_type.hを2つの.cからinclude)"
    local dir="$TMP/multitu"
    mkdir -p "$dir"

    cat > "$dir/command_type.h" << 'EOF'
#ifndef COMMAND_TYPE_H
#define COMMAND_TYPE_H
#include "generic_sum_type.h"

typedef struct { int dx, dy; } CmdMove;
typedef struct { int target; } CmdAttack;

#define CMD_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, move, CmdMove)    \
    X(NAME, EXTRA, attack, CmdAttack)

DEFINE_SUM_TYPE(Command, CMD_VARIANTS)

typedef struct { int hp; int x, y; } GameState;

DEFINE_SUM_DISPATCH(Command, CMD_VARIANTS, Command_dispatch, GameState)

#endif
EOF

    cat > "$dir/worker.c" << 'EOF'
#include "command_type.h"

static void on_move(CmdMove *m, GameState *gs) { gs->x += m->dx; gs->y += m->dy; }
static void on_attack(CmdAttack *a, GameState *gs) { gs->hp -= a->target; }

void worker_process(Command cmd, GameState *gs) {
    Command_dispatch(&cmd, gs, on_move, on_attack);
}
EOF

    cat > "$dir/main.c" << 'EOF'
#include "command_type.h"

void worker_process(Command cmd, GameState *gs);

int main(void) {
    GameState gs = { .hp = 100, .x = 0, .y = 0 };
    Command c1 = Command_new_move((CmdMove){ .dx = 3, .dy = 4 });
    Command c2 = Command_new_attack((CmdAttack){ .target = 20 });
    worker_process(c1, &gs);
    worker_process(c2, &gs);
    return (gs.hp == 80 && gs.x == 3 && gs.y == 4) ? 0 : 1;
}
EOF

    if ! "$CC" -I"$HEADER_DIR" -I"$dir" -std="$STD" -Wall -Wextra -Werror -c "$dir/worker.c" -o "$dir/worker.o" 2>"$TMP/err.log"; then
        echo "NG  $name: worker.cのコンパイルが失敗した"
        cat "$TMP/err.log"; fail=$((fail+1)); return
    fi
    if ! "$CC" -I"$HEADER_DIR" -I"$dir" -std="$STD" -Wall -Wextra -Werror -c "$dir/main.c" -o "$dir/main.o" 2>"$TMP/err.log"; then
        echo "NG  $name: main.cのコンパイルが失敗した"
        cat "$TMP/err.log"; fail=$((fail+1)); return
    fi
    if ! "$CC" "$dir/worker.o" "$dir/main.o" -o "$dir/multitu_demo" 2>"$TMP/err.log"; then
        echo "NG  $name: リンクが失敗した"
        cat "$TMP/err.log"; fail=$((fail+1)); return
    fi
    if "$dir/multitu_demo"; then
        echo "OK  $name: 期待通りビルド・リンク・実行成功"
        pass=$((pass+1))
    else
        echo "NG  $name: 実行結果が期待値と異なる(ビルドは通ったが値がおかしい)"
        fail=$((fail+1))
    fi
}
check_multi_tu

echo
echo "$pass 件成功 / $fail 件失敗"
exit "$fail"
