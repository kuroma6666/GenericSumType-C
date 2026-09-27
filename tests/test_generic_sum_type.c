/*
 * generic_sum_type.h の単体テスト。
 * 外部フレームワークは使わず、標準の assert() のみで完結させている
 * （本ライブラリ自体が依存ゼロを方針としているため、テストもそれに合わせた）。
 *
 * ビルド:  gcc -std=c11 -pedantic -Wall -Wextra -Werror -o test_generic_sum_type test_generic_sum_type.c
 * 実行:    ./test_generic_sum_type
 */
#define _POSIX_C_SOURCE 200809L
#undef NDEBUG
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* SUM_CTX_LOCK/UNLOCK フックの検証用: 呼び出し回数を記録するだけのフェイクロック */
static int g_lock_count = 0;
static int g_unlock_count = 0;
#define SUM_CTX_LOCK(ctx)   ((void)(ctx), g_lock_count++)
#define SUM_CTX_UNLOCK(ctx) ((void)(ctx), g_unlock_count++)

/* 不正 tag の検証用: フックの発火回数を数え、SUM_TRAP は longjmp で試験側へ戻す
 * (SUM_TRAP は「戻らない」ことが契約。longjmp は通常の復帰ではないので契約を満たす) */
static int g_unhandled = 0;
static jmp_buf g_trap_env;
#define SUM_UNHANDLED_TAG(self) ((void)(self), g_unhandled++)
#define SUM_TRAP() longjmp(g_trap_env, 1)
#include "generic_sum_type.h"

static int tests_run = 0;
#define RUN(test) do { printf("- %-55s ... ", #test); test(); tests_run++; printf("OK\n"); } while (0)

/* ============ テスト対象の型定義 ============ */
typedef struct { int32_t v; } IntBox;
typedef struct { char *v; } StrBox;   /* 所有する場合は strdup した領域を指す */

#define IOS_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, i, IntBox)        \
    X(NAME, EXTRA, s, StrBox)

typedef struct { int total; } SumCtx;

SUM_DEFINE_TYPE(IntOrStr, IOS_VARIANTS)
SUM_DEFINE_MATCH(IntOrStr, IOS_VARIANTS, IntOrStr_to_len, int)
SUM_DEFINE_DISPATCH(IntOrStr, IOS_VARIANTS, IntOrStr_dispatch, SumCtx)
SUM_DEFINE_DESTROY(IntOrStr, IOS_VARIANTS, IntOrStr_destroy)
SUM_DEFINE_COPY(IntOrStr, IOS_VARIANTS, IntOrStr_copy)
SUM_DEFINE_MATCH_CONST(IntOrStr, IOS_VARIANTS, IntOrStr_to_len_const, int)
SUM_DEFINE_DISPATCH_RET(IntOrStr, IOS_VARIANTS, IntOrStr_accumulate, SumCtx, int)
SUM_DEFINE_COPY_CHECKED(IntOrStr, IOS_VARIANTS, IntOrStr_try_copy)

/* 型ごとにロックを明示する DISPATCH_LOCKED の検証用 */
typedef struct { int total; int held; int max_held; } LockedCtx;
static void locked_ctx_lock(LockedCtx *c) {
    c->held++;
    if (c->held > c->max_held) { c->max_held = c->held; }
}
static void locked_ctx_unlock(LockedCtx *c) { c->held--; }
SUM_DEFINE_DISPATCH_LOCKED(IntOrStr, IOS_VARIANTS, IntOrStr_dispatch_locked, LockedCtx,
                           locked_ctx_lock, locked_ctx_unlock)
SUM_DEFINE_DISPATCH_LOCKED(IntOrStr, IOS_VARIANTS, IntOrStr_dispatch_unlocked, SumCtx,
                           SUM_NO_LOCK, SUM_NO_LOCK)

/* Either イディオム(left/right の2 variant + SUM_DEFINE_EITHER_HELPERS)の検証用 */
typedef struct { int code; } ELeft;
typedef struct { int val; }  ERight;
#define EITHER_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, left,  ELeft)        \
    X(NAME, EXTRA, right, ERight)
SUM_DEFINE_TYPE(MyEither, EITHER_VARIANTS)
SUM_DEFINE_EITHER_HELPERS(MyEither)

/* Result イディオム(ok/err の2 variant + SUM_DEFINE_RESULT_HELPERS)の検証用 */
typedef struct { int value; } ROk;
typedef struct { int code; }  RErr;
#define RESULT_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, ok,  ROk)            \
    X(NAME, EXTRA, err, RErr)
SUM_DEFINE_TYPE(MyResult, RESULT_VARIANTS)
SUM_DEFINE_RESULT_HELPERS(MyResult)

/* ============ 1. SUM_DEFINE_TYPE: コンストラクタ・ゲッター ============ */
static void test_ctor_and_getter(void) {
    IntOrStr a = IntOrStr_new_i((IntBox){ .v = 42 });
    assert(IntOrStr_get_i(&a) != NULL);
    assert(IntOrStr_get_i(&a)->v == 42);
    assert(IntOrStr_get_s(&a) == NULL); /* 違うタグなのでNULLを返す */

    IntOrStr b = IntOrStr_new_s((StrBox){ .v = "hello" });
    assert(IntOrStr_get_s(&b) != NULL);
    assert(strcmp(IntOrStr_get_s(&b)->v, "hello") == 0);
    assert(IntOrStr_get_i(&b) == NULL);
}

/* ============ 2. SUM_DEFINE_MATCH: 正しいハンドラに振り分けられるか ============ */
static int len_i(IntBox *b) { (void)b; return -1; }
static int len_s(StrBox *b) { return (int)strlen(b->v); }

static void test_match_dispatches_to_correct_handler(void) {
    IntOrStr a = IntOrStr_new_i((IntBox){ .v = 1 });
    IntOrStr b = IntOrStr_new_s((StrBox){ .v = "abcde" });
    assert(IntOrStr_to_len(&a, len_i, len_s) == -1);
    assert(IntOrStr_to_len(&b, len_i, len_s) == 5);
}

/* ============ 3. SUM_DEFINE_DISPATCH: ctxの書き換え + ロックフックの呼び出し ============ */
static void on_i(IntBox *b, SumCtx *ctx) { ctx->total += b->v; }
static void on_s(StrBox *b, SumCtx *ctx) { ctx->total += (int)strlen(b->v); }

static void test_dispatch_mutates_ctx_and_calls_lock_hooks(void) {
    SumCtx ctx = { .total = 0 };
    IntOrStr a = IntOrStr_new_i((IntBox){ .v = 10 });
    IntOrStr b = IntOrStr_new_s((StrBox){ .v = "abc" });

    int lock_before = g_lock_count, unlock_before = g_unlock_count;
    IntOrStr_dispatch(&a, &ctx, on_i, on_s);
    IntOrStr_dispatch(&b, &ctx, on_i, on_s);

    assert(ctx.total == 13); /* 10 + strlen("abc")=3 */
    assert(g_lock_count == lock_before + 2);     /* dispatch呼び出し回数と一致 */
    assert(g_unlock_count == unlock_before + 2);
    assert(g_lock_count == g_unlock_count);      /* lock/unlockの対応が崩れていない
                                                     (このassertが無ければ過去に実際に
                                                      作り込んだreturn/break バグを
                                                      再発検出できない) */
}

/* ============ 4. SUM_DEFINE_DESTROY: タグごとに正しいハンドラが呼ばれるか ============ */
static int g_destroy_i_called = 0;
static int g_destroy_s_called = 0;
static void destroy_i(IntBox *b) { (void)b; g_destroy_i_called++; }
static void destroy_s(StrBox *b) { free(b->v); g_destroy_s_called++; }

static void test_destroy_dispatches_per_tag(void) {
    IntOrStr a = IntOrStr_new_i((IntBox){ .v = 1 });
    IntOrStr b = IntOrStr_new_s((StrBox){ .v = strdup("owned") });

    g_destroy_i_called = 0;
    g_destroy_s_called = 0;
    IntOrStr_destroy(&a, destroy_i, destroy_s);
    IntOrStr_destroy(&b, destroy_i, destroy_s);

    assert(g_destroy_i_called == 1);
    assert(g_destroy_s_called == 1);
}

/* ============ 5. SUM_DEFINE_COPY: ポインタ資源のディープコピー ============ */
static IntBox copy_i(const IntBox *b) { return *b; }
static StrBox copy_s(const StrBox *b) { return (StrBox){ .v = strdup(b->v) }; }

static void test_copy_is_deep_for_pointer_payload(void) {
    IntOrStr original = IntOrStr_new_s((StrBox){ .v = strdup("copy-me") });
    IntOrStr copy = IntOrStr_copy(&original, copy_i, copy_s);

    StrBox *o = IntOrStr_get_s(&original);
    StrBox *c = IntOrStr_get_s(&copy);
    assert(o != NULL && c != NULL && o->v != NULL && c->v != NULL);
    assert(o->v != c->v);              /* 別アドレス = ディープコピーされている */
    assert(strcmp(o->v, c->v) == 0);   /* 内容は同一 */

    IntOrStr_destroy(&original, destroy_i, destroy_s);
    IntOrStr_destroy(&copy, destroy_i, destroy_s);
}

static void test_copy_is_identity_for_pod_payload(void) {
    IntOrStr original = IntOrStr_new_i((IntBox){ .v = 99 });
    IntOrStr copy = IntOrStr_copy(&original, copy_i, copy_s);
    assert(IntOrStr_get_i(&copy)->v == 99);
}

/* ============ 6. SUM_DEFINE_MATCH_CONST / getter_const: read-only アクセス ============ */
/* ハンドラは payload を const で受け取る（書き換え不可）。const IntOrStr* からも呼べる。 */
static int len_i_const(const IntBox *b) { (void)b; return -1; }
static int len_s_const(const StrBox *b) { return (int)strlen(b->v); }

static int const_total_len(const IntOrStr *self) {
    /* 引数が const IntOrStr* でも、可変版と違いキャストなしで呼べることが要点 */
    return IntOrStr_to_len_const(self, len_i_const, len_s_const);
}

static void test_const_match_and_getter(void) {
    IntOrStr a = IntOrStr_new_i((IntBox){ .v = 1 });
    IntOrStr b = IntOrStr_new_s((StrBox){ .v = "abcde" });

    const IntOrStr *pa = &a, *pb = &b;
    assert(const_total_len(pa) == -1);
    assert(const_total_len(pb) == 5);

    /* const getter: const IntOrStr* から const payload* を得る */
    const IntBox *ib = IntOrStr_get_i_const(pa);
    assert(ib != NULL && ib->v == 1);
    assert(IntOrStr_get_s_const(pa) == NULL);   /* タグ不一致でNULL */

    const StrBox *sb = IntOrStr_get_s_const(pb);
    assert(sb != NULL && strcmp(sb->v, "abcde") == 0);
    assert(IntOrStr_get_i_const(pb) == NULL);
}

/* ============ 7. SUM_DEFINE_EITHER_HELPERS: left/right 述語 ============ */
static void test_either_helpers(void) {
    MyEither l = MyEither_new_left((ELeft){ .code = 7 });
    MyEither r = MyEither_new_right((ERight){ .val = 9 });

    assert(MyEither_is_left(&l)  && !MyEither_is_right(&l));
    assert(MyEither_is_right(&r) && !MyEither_is_left(&r));

    /* 取り出しは既存の const ゲッターをそのまま使う */
    assert(MyEither_get_left_const(&l)->code == 7);
    assert(MyEither_get_right_const(&r)->val == 9);
    assert(MyEither_get_right_const(&l) == NULL); /* 左なので右取り出しはNULL */
}

/* ============ 8. SUM_DEFINE_RESULT_HELPERS: ok/err 述語 ============ */
static void test_result_helpers(void) {
    MyResult ok  = MyResult_new_ok((ROk){ .value = 5 });
    MyResult err = MyResult_new_err((RErr){ .code = 42 });

    assert(MyResult_is_ok(&ok)   && !MyResult_is_err(&ok));
    assert(MyResult_is_err(&err) && !MyResult_is_ok(&err));

    assert(MyResult_get_ok_const(&ok)->value == 5);
    assert(MyResult_get_err_const(&err)->code == 42);
    assert(MyResult_get_ok_const(&err) == NULL);
}


/* ============ 9. 空状態(tag 0): ゼロ初期化・destroy 後 ============ */
static void test_zero_initialized_value_is_empty(void) {
    IntOrStr z = SUM_EMPTY(IntOrStr);
    IntOrStr zz;
    memset(&zz, 0, sizeof zz);
    assert(z.tag == IntOrStr_TAG__EMPTY && zz.tag == IntOrStr_TAG__EMPTY);
    assert(IntOrStr_get_i(&zz) == NULL && IntOrStr_get_s(&zz) == NULL);

    int before = g_unhandled;
    IntOrStr_destroy(&zz, destroy_i, destroy_s);      /* 空状態の destroy は no-op */
    assert(g_unhandled == before);
}

static void test_destroy_resets_to_empty_and_second_destroy_is_noop(void) {
    IntOrStr b = IntOrStr_new_s((StrBox){ .v = strdup("owned") });
    g_destroy_s_called = 0;
    IntOrStr_destroy(&b, destroy_i, destroy_s);
    assert(b.tag == IntOrStr_TAG__EMPTY);
    IntOrStr_destroy(&b, destroy_i, destroy_s);       /* 2回目は解放しない(ASan が二重解放を見る) */
    assert(g_destroy_s_called == 1);
}

/* ============ 10. DISPATCH_RET: 戻り値とロックの対応 ============ */
static int acc_i(IntBox *b, SumCtx *ctx) { ctx->total += b->v; return ctx->total; }
static int acc_s(StrBox *b, SumCtx *ctx) { ctx->total += (int)strlen(b->v); return -ctx->total; }

static void test_dispatch_ret_returns_handler_value(void) {
    SumCtx ctx = { .total = 0 };
    IntOrStr a = IntOrStr_new_i((IntBox){ .v = 4 });
    IntOrStr b = IntOrStr_new_s((StrBox){ .v = "xy" });
    int lock_before = g_lock_count;
    assert(IntOrStr_accumulate(&a, &ctx, acc_i, acc_s) == 4);
    assert(IntOrStr_accumulate(&b, &ctx, acc_i, acc_s) == -6);
    assert(g_lock_count == lock_before + 2 && g_lock_count == g_unlock_count);
}

/* ============ 11. DISPATCH_LOCKED: 型ごとのロック ============ */
static void lk_i(IntBox *b, LockedCtx *ctx) { assert(ctx->held == 1); ctx->total += b->v; }
static void lk_s(StrBox *b, LockedCtx *ctx) { assert(ctx->held == 1); ctx->total += (int)strlen(b->v); }

static void test_dispatch_locked_uses_given_lock_only(void) {
    LockedCtx lc = { 0, 0, 0 };
    SumCtx sc = { .total = 0 };
    IntOrStr a = IntOrStr_new_i((IntBox){ .v = 3 });
    int global_before = g_lock_count;

    IntOrStr_dispatch_locked(&a, &lc, lk_i, lk_s);
    assert(lc.total == 3 && lc.held == 0 && lc.max_held == 1);

    IntOrStr_dispatch_unlocked(&a, &sc, on_i, on_s);
    assert(sc.total == 3);
    assert(g_lock_count == global_before);           /* グローバルフックは使われない */
}

/* ============ 12. COPY_CHECKED: 成功・失敗 ============ */
static int g_fail_copy_s = 0;
static int try_copy_i(IntBox *dst, const IntBox *src) { *dst = *src; return 0; }
static int try_copy_s(StrBox *dst, const StrBox *src) {
    if (g_fail_copy_s) { return 12; }                /* 例: ENOMEM */
    char *p = strdup(src->v);
    if (p == NULL) { return 12; }
    dst->v = p;
    return 0;
}

static void test_copy_checked_success_and_failure(void) {
    IntOrStr src = IntOrStr_new_s((StrBox){ .v = "deep" });
    IntOrStr dst = IntOrStr_new_i((IntBox){ .v = 77 });

    g_fail_copy_s = 1;
    assert(IntOrStr_try_copy(&dst, &src, try_copy_i, try_copy_s) == 12);
    assert(dst.tag == IntOrStr_TAG_i && IntOrStr_get_i(&dst)->v == 77); /* 失敗時は不変 */

    g_fail_copy_s = 0;
    assert(IntOrStr_try_copy(&dst, &src, try_copy_i, try_copy_s) == 0);
    assert(dst.tag == IntOrStr_TAG_s);
    assert(IntOrStr_get_s(&dst)->v != src.as.s.v);
    assert(strcmp(IntOrStr_get_s(&dst)->v, "deep") == 0);
    IntOrStr_destroy(&dst, destroy_i, destroy_s);
}

/* ============ 13. 不正 tag: 値を返すものはフック後に SUM_TRAP ============ */
static int g_handler_called = 0;
static int spy_len_i(IntBox *b) { (void)b; g_handler_called++; return 1; }
static int spy_len_s(StrBox *b) { (void)b; g_handler_called++; return 2; }

static void test_bad_tag_traps_in_value_returning_dispatchers(void) {
    IntOrStr bad = IntOrStr_new_i((IntBox){ .v = 1 });
    bad.tag = (IntOrStr_tag_t)99;
    IntOrStr empty = SUM_EMPTY(IntOrStr);
    SumCtx ctx = { .total = 0 };
    volatile int trapped = 0;
    int before = g_unhandled;
    g_handler_called = 0;

    if (setjmp(g_trap_env) == 0) { (void)IntOrStr_to_len(&bad, spy_len_i, spy_len_s); }
    else { trapped++; }
    if (setjmp(g_trap_env) == 0) { (void)IntOrStr_to_len(&empty, spy_len_i, spy_len_s); }
    else { trapped++; }
    if (setjmp(g_trap_env) == 0) { (void)IntOrStr_copy(&bad, copy_i, copy_s); }
    else { trapped++; }
    if (setjmp(g_trap_env) == 0) { (void)IntOrStr_accumulate(&bad, &ctx, acc_i, acc_s); }
    else { trapped++; }
    g_lock_count = g_unlock_count;   /* 上の DISPATCH_RET はロック保持のまま trap した */

    assert(trapped == 4);
    assert(g_unhandled == before + 4);
    assert(g_handler_called == 0);
}

/* ============ 14. 自己参照(struct NAME* 経由) ============ */
typedef struct { int v; } ExprNum;
typedef struct { struct Expr *lhs, *rhs; } ExprAdd;
#define EXPR_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, num, ExprNum)      \
    X(NAME, EXTRA, add, ExprAdd)
SUM_DEFINE_TYPE(Expr, EXPR_VARIANTS)
SUM_DEFINE_MATCH_CONST(Expr, EXPR_VARIANTS, Expr_eval, int)

static int eval_num(const ExprNum *n) { return n->v; }
static int eval_add(const ExprAdd *a) {
    return Expr_eval(a->lhs, eval_num, eval_add) + Expr_eval(a->rhs, eval_num, eval_add);
}

static void test_self_reference_through_struct_pointer(void) {
    Expr one = Expr_new_num((ExprNum){ 1 });
    Expr two = Expr_new_num((ExprNum){ 2 });
    Expr sum = Expr_new_add((ExprAdd){ &one, &two });
    Expr top = Expr_new_add((ExprAdd){ &sum, &two });
    assert(Expr_eval(&top, eval_num, eval_add) == 5);
}

/* ============ 15. tag 値の全域走査: 正しい tag は対応ハンドラ1回、それ以外はハンドラ0回 ============ */
static int g_scan_i = 0, g_scan_s = 0;
static void scan_i(IntBox *b, SumCtx *ctx) { (void)b; (void)ctx; g_scan_i++; }
static void scan_s(StrBox *b, SumCtx *ctx) { (void)b; (void)ctx; g_scan_s++; }
static int scan_len_i(IntBox *b) { (void)b; g_scan_i++; return 0; }
static int scan_len_s(StrBox *b) { (void)b; g_scan_s++; return 0; }

static void test_tag_value_scan(void) {
    /* 負値・空状態・有効値・範囲外を含む tag 値を順に試す */
    static const int values[] = { -1000, -1, 0, 1, 2, 3, 4, 255, 256, 65535, 1 << 30 };
    SumCtx ctx = { .total = 0 };
    for (volatile size_t k = 0; k < sizeof values / sizeof values[0]; ++k) {
        IntOrStr v = IntOrStr_new_s((StrBox){ .v = "x" });
        v.tag = (IntOrStr_tag_t)values[k];
        int valid_i = (values[k] == (int)IntOrStr_TAG_i);
        int valid_s = (values[k] == (int)IntOrStr_TAG_s);

        g_scan_i = g_scan_s = 0;
        int hook_before = g_unhandled;
        IntOrStr_dispatch(&v, &ctx, scan_i, scan_s);
        assert(g_scan_i == valid_i && g_scan_s == valid_s);
        assert(g_unhandled == hook_before + (valid_i || valid_s ? 0 : 1));

        g_scan_i = g_scan_s = 0;
        volatile int trapped = 0;
        if (setjmp(g_trap_env) == 0) { (void)IntOrStr_to_len(&v, scan_len_i, scan_len_s); }
        else { trapped = 1; }
        assert(g_scan_i == valid_i && g_scan_s == valid_s);
        assert(trapped == !(valid_i || valid_s));
    }
    assert(g_lock_count == g_unlock_count);
}

int main(void) {
    printf("generic_sum_type.h 単体テスト\n");
    RUN(test_ctor_and_getter);
    RUN(test_match_dispatches_to_correct_handler);
    RUN(test_dispatch_mutates_ctx_and_calls_lock_hooks);
    RUN(test_destroy_dispatches_per_tag);
    RUN(test_copy_is_deep_for_pointer_payload);
    RUN(test_copy_is_identity_for_pod_payload);
    RUN(test_const_match_and_getter);
    RUN(test_either_helpers);
    RUN(test_result_helpers);
    RUN(test_zero_initialized_value_is_empty);
    RUN(test_destroy_resets_to_empty_and_second_destroy_is_noop);
    RUN(test_dispatch_ret_returns_handler_value);
    RUN(test_dispatch_locked_uses_given_lock_only);
    RUN(test_copy_checked_success_and_failure);
    RUN(test_bad_tag_traps_in_value_returning_dispatchers);
    RUN(test_self_reference_through_struct_pointer);
    RUN(test_tag_value_scan);
    printf("%d件全て成功\n", tests_run);
    return 0;
}
