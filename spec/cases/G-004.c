/* 網羅・順序ともに正しい呼び出し。誤検出（正しいコードが落ちる）がないことの確認。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_DISPATCH(T, V, T_dispatch, int)
static void ha(PA *p, int *c) { (void)p; (void)c; }
static void hb(PB *p, int *c) { (void)p; (void)c; }
int main(void) {
    T t = T_new_a((PA){ 1 });
    int ctx = 0;
    T_dispatch(&t, &ctx, ha, hb);
    return 0;
}
