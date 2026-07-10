/*
 * DEFINE_SUM_NEW_GENERIC / SUM_NEW（C11以降限定のオプトイン機能）の単体テスト。
 * 他のtest_*.cと同様、外部フレームワークを使わずassert()のみで構成している。
 *
 * C99ビルドではDEFINE_SUM_NEW_GENERIC / SUM_NEW自体が存在しないため
 * （generic_sum_type.h内で__STDC_VERSION__によりガードされている）、
 * このファイルは丸ごと__STDC_VERSION__で分岐し、C99実行時は
 * テストをスキップした旨を表示するだけにしている。ci.ymlはtests/test_*.cを
 * c99/c11/c17全マトリクスで無条件にビルド・実行するため、この分岐が
 * ないとc99ジョブが壊れる（design_spec.md 4.13節で検証済みの制約）。
 *
 * 「渡し忘れ」「型取り違え」に相当する検証（コンパイルが失敗するべきケース）は
 * ランタイムのassertテストには書けないため、test_compile_guarantees.shの方に
 * ケースを追加している。
 *
 * ビルド:  gcc -std=c11 -pedantic -Wall -Wextra -Werror -o test_sum_new_generic test_sum_new_generic.c
 * 実行:    ./test_sum_new_generic
 */
#include <stdio.h>
#include "generic_sum_type.h"

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

#include <assert.h>
#include <stdint.h>
#include <string.h>

static int tests_run = 0;
#define RUN(test) do { printf("- %-55s ... ", #test); test(); tests_run++; printf("OK\n"); } while (0)

/* ============ テスト対象の型定義 ============ */
typedef struct { int32_t v; } IntBox;
typedef struct { const char *v; } StrBox;
typedef struct { double v; } FloatBox;

#define IOSF_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, i, IntBox)         \
    X(NAME, EXTRA, s, StrBox)         \
    X(NAME, EXTRA, f, FloatBox)

DEFINE_SUM_TYPE(IntOrStrOrFloat, IOSF_VARIANTS)
DEFINE_SUM_NEW_GENERIC(IntOrStrOrFloat, IOSF_VARIANTS)

/* ============ 1. 型ごとに正しいコンストラクタが自動選択されるか ============ */
static void test_selects_int_ctor(void) {
    IntOrStrOrFloat v = SUM_NEW(IntOrStrOrFloat, IOSF_VARIANTS, ((IntBox){ 42 }));
    assert(IntOrStrOrFloat_get_i(&v) != NULL);
    assert(IntOrStrOrFloat_get_i(&v)->v == 42);
    assert(IntOrStrOrFloat_get_s(&v) == NULL);
    assert(IntOrStrOrFloat_get_f(&v) == NULL);
}

static void test_selects_str_ctor(void) {
    IntOrStrOrFloat v = SUM_NEW(IntOrStrOrFloat, IOSF_VARIANTS, ((StrBox){ "hello" }));
    assert(IntOrStrOrFloat_get_s(&v) != NULL);
    assert(strcmp(IntOrStrOrFloat_get_s(&v)->v, "hello") == 0);
    assert(IntOrStrOrFloat_get_i(&v) == NULL);
    assert(IntOrStrOrFloat_get_f(&v) == NULL);
}

static void test_selects_float_ctor(void) {
    IntOrStrOrFloat v = SUM_NEW(IntOrStrOrFloat, IOSF_VARIANTS, ((FloatBox){ 3.5 }));
    assert(IntOrStrOrFloat_get_f(&v) != NULL);
    assert(IntOrStrOrFloat_get_f(&v)->v == 3.5);
    assert(IntOrStrOrFloat_get_i(&v) == NULL);
    assert(IntOrStrOrFloat_get_s(&v) == NULL);
}

/* ============ 2. SUM_NEWで作った値が通常のマクロ生成関数と等価に扱えるか ============ */
DEFINE_SUM_MATCH(IntOrStrOrFloat, IOSF_VARIANTS, IntOrStrOrFloat_to_tagchar, char)
static char tag_i(IntBox *b)   { (void)b; return 'i'; }
static char tag_s(StrBox *b)   { (void)b; return 's'; }
static char tag_f(FloatBox *b) { (void)b; return 'f'; }

static void test_sum_new_value_works_with_match(void) {
    IntOrStrOrFloat v = SUM_NEW(IntOrStrOrFloat, IOSF_VARIANTS, ((StrBox){ "x" }));
    assert(IntOrStrOrFloat_to_tagchar(&v, tag_i, tag_s, tag_f) == 's');
}

int main(void) {
    printf("DEFINE_SUM_NEW_GENERIC / SUM_NEW 単体テスト\n");
    RUN(test_selects_int_ctor);
    RUN(test_selects_str_ctor);
    RUN(test_selects_float_ctor);
    RUN(test_sum_new_value_works_with_match);
    printf("%d件全て成功\n", tests_run);
    return 0;
}

#else /* __STDC_VERSION__ < 201112L */

int main(void) {
    printf("DEFINE_SUM_NEW_GENERIC / SUM_NEW 単体テスト\n");
    printf("C11未満のためSUM_NEWは利用不可、このビルド(C99)ではテストをスキップします\n");
    return 0;
}

#endif /* __STDC_VERSION__ >= 201112L */
