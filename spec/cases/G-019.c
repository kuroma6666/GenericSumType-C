/* 不正 tag（マクロ経由以外で破壊された tag）でディスパッチしたとき、
 * void を返す DISPATCH / DESTROY は SUM_UNHANDLED_TAG を呼んで正常復帰する。
 * ハンドラは一つも呼ばれない。値を返す MATCH / MATCH_CONST / COPY は
 * フック呼び出し後 SUM_UNREACHABLE() に進むため、ここでは検証しない。 */
#include <stdio.h>
static int sum_unhandled_fired = 0;
#define SUM_UNHANDLED_TAG(self) do { (void)(self); sum_unhandled_fired++; } while (0)
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
DEFINE_SUM_TYPE(T, V)
typedef struct { int calls; } Ctx;
DEFINE_SUM_DISPATCH(T, V, T_dispatch, Ctx)
DEFINE_SUM_DESTROY(T, V, T_destroy)
static void da(PA *p, Ctx *c) { (void)p; c->calls++; }
static void db(PB *p, Ctx *c) { (void)p; c->calls++; }
static void za(PA *p) { (void)p; }
static void zb(PB *p) { (void)p; }
int main(void) {
    T t = T_new_a((PA){ 1 });
    Ctx c = { 0 };
    T_dispatch(&t, &c, da, db);
    T_destroy(&t, za, zb);
    if (sum_unhandled_fired != 0 || c.calls != 1) {
        printf("正常 tag でフックが発火した: fired=%d calls=%d\n", sum_unhandled_fired, c.calls);
        return 1;
    }
    t.tag = (T_tag_t)99;
    T_dispatch(&t, &c, da, db);
    T_destroy(&t, za, zb);
    if (sum_unhandled_fired != 2 || c.calls != 1) {
        printf("不正 tag でフックが期待通り発火しない: fired=%d calls=%d\n", sum_unhandled_fired, c.calls);
        return 1;
    }
    return 0;
}
