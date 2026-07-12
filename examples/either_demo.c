/*
 * Either イディオムの使用例。
 *
 * Either<L, R>（Rust の Result、関数型の Either に相当）は「left か right の
 * ちょうど2択」に固定された直和型なので、既存の DEFINE_SUM_TYPE をそのまま使い、
 * tag 名を left / right にするだけで表現できる。加えて DEFINE_EITHER_HELPERS で
 * is_left / is_right の述語が生える。
 *
 *   構築  : NAME_new_left / NAME_new_right
 *   述語  : NAME_is_left / NAME_is_right          (DEFINE_EITHER_HELPERS)
 *   取り出し: NAME_get_left / NAME_get_right (+_const)  (DEFINE_SUM_TYPE が生成)
 *   畳み込み: DEFINE_SUM_MATCH_CONST の左右2ハンドラ
 *
 * ビルド: gcc -std=c11 -pedantic -Wall -Wextra -Werror -Iinclude examples/either_demo.c -o either_demo
 */
#include <stdio.h>
#include "generic_sum_type.h"

/* payload は専用struct（3節ガイドライン: L と R が別型なら左右取り違えを検出できる） */
typedef struct { int code; const char *msg; } Err;  /* Left  */
typedef struct { int value; }                 Ok;   /* Right */

#define RESULT_EITHER(X, NAME, EXTRA) \
    X(NAME, EXTRA, left,  Err)        \
    X(NAME, EXTRA, right, Ok)

DEFINE_SUM_TYPE(Result, RESULT_EITHER)
DEFINE_EITHER_HELPERS(Result)
/* fold: 左右どちらの場合も1つの値へ畳み込む read-only な変換 */
DEFINE_SUM_MATCH_CONST(Result, RESULT_EITHER, Result_fold, int)

static int on_err(const Err *e) { printf("  Left  Err(%d, \"%s\")\n", e->code, e->msg); return -e->code; }
static int on_ok (const Ok  *o) { printf("  Right Ok(%d)\n", o->value);                return o->value; }

/* Result を書き換えずに検査するだけの関数（const Result* で受けられる） */
static void report(const Result *r) {
    printf("is_left=%d is_right=%d -> fold=%d\n",
           Result_is_left(r), Result_is_right(r),
           Result_fold(r, on_err, on_ok));
}

int main(void) {
    Result ok  = Result_new_right((Ok){ .value = 42 });
    Result err = Result_new_left((Err){ .code = 404, .msg = "not found" });

    report(&ok);
    report(&err);

    /* 型安全な取り出し（右のときだけ非NULL） */
    const Ok *v = Result_get_right_const(&ok);
    printf("unwrap_right(ok) = %d\n", v ? v->value : -1);
    return 0;
}
