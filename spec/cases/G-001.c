/* ハンドラを1つ渡し忘れる。生成関数の引数個数は variant 数と常に一致するため、
 * 通常の関数プロトタイプ検査（too few arguments）で落ちる。 */
#include "generic_sum_type.h"
typedef struct { int a, b; } PA;
typedef struct { int a, b; } PB;
typedef struct { int a, b; } PC;
#define V(X, NAME, EXTRA) \
    X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB) X(NAME, EXTRA, c, PC)
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_DISPATCH(T, V, T_dispatch, int)
static void h(PA *p, int *c) { (void)p; (void)c; }
int main(void) {
    T t = T_new_a((PA){ 0, 0 });
    int ctx = 0;
    T_dispatch(&t, &ctx, h);   /* b, c 用のハンドラが無い */
    return 0;
}
