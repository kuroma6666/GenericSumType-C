/* ハンドラを1つ渡し忘れる。生成関数の引数個数は variant 数と常に一致するため、
 * 通常の関数プロトタイプ検査（too few arguments）で落ちる。 */
#include "generic_sum_type.h"
typedef struct { int a, b; } PA;
typedef struct { int a, b; } PB;
typedef struct { int a, b; } PC;
#define V(X, NAME, EXTRA) \
    X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB) X(NAME, EXTRA, c, PC)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_DISPATCH(T, V, T_dispatch, int)
static void ha(PA *p, int *c) { (void)p; (void)c; }
static void hb(PB *p, int *c) { (void)p; (void)c; }
static void hc(PC *p, int *c) { (void)p; (void)c; }
int main(void) {
    T t = T_new_a((PA){ 0, 0 });
    int ctx = 0;
#ifndef SPEC_FIXED
    T_dispatch(&t, &ctx, ha);           /* b, c 用のハンドラが無い */
    (void)hb; (void)hc;
#else
    T_dispatch(&t, &ctx, ha, hb, hc);
#endif
    return 0;
}
