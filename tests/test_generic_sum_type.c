/*
 * generic_sum_type.h の単体テスト。
 * 外部フレームワークは使わず、標準の assert() のみで完結させている
 * （本ライブラリ自体が依存ゼロを方針としているため、テストもそれに合わせた）。
 *
 * ビルド:  gcc -std=c11 -pedantic -Wall -Wextra -Werror -o test_generic_sum_type test_generic_sum_type.c
 * 実行:    ./test_generic_sum_type
 */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* SUM_CTX_LOCK/UNLOCK フックの検証用: 呼び出し回数を記録するだけのフェイクロック */
static int g_lock_count = 0;
static int g_unlock_count = 0;
#define SUM_CTX_LOCK(ctx)   (g_lock_count++)
#define SUM_CTX_UNLOCK(ctx) (g_unlock_count++)
#include "generic_sum_type.h"

static int tests_run = 0;
#define RUN(test) do { printf("- %-55s ... ", #test); test(); tests_run++; printf("OK\n"); } while (0)

/* ============ テスト対象の型定義 ============ */
typedef struct { int32_t v; } IntBox;
typedef struct { const char *v; } StrBox;

#define IOS_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, i, IntBox)        \
    X(NAME, EXTRA, s, StrBox)

typedef struct { int total; } SumCtx;

DEFINE_SUM_TYPE(IntOrStr, IOS_VARIANTS)
DEFINE_SUM_MATCH(IntOrStr, IOS_VARIANTS, IntOrStr_to_len, int)
DEFINE_SUM_DISPATCH(IntOrStr, IOS_VARIANTS, IntOrStr_dispatch, SumCtx)
DEFINE_SUM_DESTROY(IntOrStr, IOS_VARIANTS, IntOrStr_destroy)
DEFINE_SUM_COPY(IntOrStr, IOS_VARIANTS, IntOrStr_copy)
DEFINE_SUM_MATCH_CONST(IntOrStr, IOS_VARIANTS, IntOrStr_to_len_const, int)

/* Either イディオム(left/right の2 variant + DEFINE_EITHER_HELPERS)の検証用 */
typedef struct { int code; } ELeft;
typedef struct { int val; }  ERight;
#define EITHER_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, left,  ELeft)        \
    X(NAME, EXTRA, right, ERight)
DEFINE_SUM_TYPE(MyEither, EITHER_VARIANTS)
DEFINE_EITHER_HELPERS(MyEither)

/* ============ 1. DEFINE_SUM_TYPE: コンストラクタ・ゲッター ============ */
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

/* ============ 2. DEFINE_SUM_MATCH: 正しいハンドラに振り分けられるか ============ */
static int len_i(IntBox *b) { (void)b; return -1; }
static int len_s(StrBox *b) { return (int)strlen(b->v); }

static void test_match_dispatches_to_correct_handler(void) {
    IntOrStr a = IntOrStr_new_i((IntBox){ .v = 1 });
    IntOrStr b = IntOrStr_new_s((StrBox){ .v = "abcde" });
    assert(IntOrStr_to_len(&a, len_i, len_s) == -1);
    assert(IntOrStr_to_len(&b, len_i, len_s) == 5);
}

/* ============ 3. DEFINE_SUM_DISPATCH: ctxの書き換え + ロックフックの呼び出し ============ */
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

/* ============ 4. DEFINE_SUM_DESTROY: タグごとに正しいハンドラが呼ばれるか ============ */
static int g_destroy_i_called = 0;
static int g_destroy_s_called = 0;
static void destroy_i(IntBox *b) { (void)b; g_destroy_i_called++; }
static void destroy_s(StrBox *b) { free((void *)b->v); g_destroy_s_called++; }

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

/* ============ 5. DEFINE_SUM_COPY: ポインタ資源のディープコピー ============ */
static IntBox copy_i(const IntBox *b) { return *b; }
static StrBox copy_s(const StrBox *b) { return (StrBox){ .v = strdup(b->v) }; }

static void test_copy_is_deep_for_pointer_payload(void) {
    IntOrStr original = IntOrStr_new_s((StrBox){ .v = strdup("copy-me") });
    IntOrStr copy = IntOrStr_copy(&original, copy_i, copy_s);

    StrBox *o = IntOrStr_get_s(&original);
    StrBox *c = IntOrStr_get_s(&copy);
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

/* ============ 6. DEFINE_SUM_MATCH_CONST / getter_const: read-only アクセス ============ */
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

/* ============ 7. DEFINE_EITHER_HELPERS: left/right 述語 ============ */
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
    printf("%d件全て成功\n", tests_run);
    return 0;
}
