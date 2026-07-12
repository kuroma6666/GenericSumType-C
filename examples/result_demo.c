/*
 * Result イディオムの使用例（Result<T, E> = ok/err の2 variant）。
 *
 * Result は Either（left/right）の ok/err 版。tag 名を ok/err にした2 variant を
 * DEFINE_SUM_TYPE で定義し、DEFINE_RESULT_HELPERS で is_ok/is_err を生やす。
 *
 *   構築  : NAME_new_ok / NAME_new_err
 *   述語  : NAME_is_ok / NAME_is_err            (DEFINE_RESULT_HELPERS)
 *   取り出し: NAME_get_ok / NAME_get_err (+_const) (DEFINE_SUM_TYPE)
 *   畳み込み: DEFINE_SUM_MATCH_CONST の ok/err 2ハンドラ
 *
 * 【C での限界】map/and_then のような「payload 型が変わる」関数合成(ROP)は、
 * 変換先が別の具体 Result 型になり汎用化できない（design_spec 2.10節）。
 * 下の and_then は「同じ型 T -> Result<T,E>」に限れば書ける例。型を変える段は
 * その都度 目標の Result 型を作って手書きする必要がある。
 *
 * ビルド: gcc -std=c11 -pedantic -Wall -Wextra -Werror -Iinclude examples/result_demo.c -o result_demo
 */
#include <stdio.h>
#include "generic_sum_type.h"

typedef struct { int value; }               Ok;   /* T */
typedef struct { int code; const char *msg; } Err; /* E */

#define RESULT_V(X, NAME, EXTRA) \
    X(NAME, EXTRA, ok,  Ok)      \
    X(NAME, EXTRA, err, Err)

DEFINE_SUM_TYPE(Result, RESULT_V)
DEFINE_RESULT_HELPERS(Result)
DEFINE_SUM_MATCH_CONST(Result, RESULT_V, Result_fold, int)

static int f_ok (const Ok  *o){ return o->value; }
static int f_err(const Err *e){ printf("  err(%d, \"%s\")\n", e->code, e->msg); return -e->code; }

/* unwrap_or: 成功なら中身、失敗ならデフォルト（T 型を知っていれば書ける） */
static Ok Result_unwrap_or(const Result *r, Ok dflt){
    return r->tag == Result_ok ? r->as.ok : dflt;
}

/* and_then（同型 T -> Result<T,E> に限る）。Err はそのまま伝播する */
static Result Result_and_then(const Result *r, Result (*f)(const Ok*)){
    return r->tag == Result_ok ? f(&r->as.ok) : *r;
}
static Result reject_if_big(const Ok *o){
    if (o->value > 100) return Result_new_err((Err){ .code=1, .msg="too big" });
    return Result_new_ok((Ok){ .value = o->value * 2 });
}

int main(void){
    Result ok  = Result_new_ok((Ok){ .value = 21 });
    Result err = Result_new_err((Err){ .code=404, .msg="not found" });

    printf("is_ok(ok)=%d is_err(err)=%d\n", Result_is_ok(&ok), Result_is_err(&err));
    printf("fold(ok)=%d fold(err)=%d\n", Result_fold(&ok, f_ok, f_err), Result_fold(&err, f_ok, f_err));
    printf("unwrap_or(err,{-1})=%d\n", Result_unwrap_or(&err, (Ok){-1}).value);

    Result chained = Result_and_then(&ok, reject_if_big);   /* 21 -> 42 */
    printf("and_then(ok)=%d\n", Result_is_ok(&chained) ? chained.as.ok.value : -999);
    return 0;
}
