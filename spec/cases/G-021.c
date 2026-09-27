/* ハンドラの代わりに定数 NULL を渡す。生成関数は __attribute__((nonnull)) 付きなので
 * -Wnonnull（-Wall に含まれる。-Werror でエラー）で落ちる。v1 では通って実行時に落ちていた。 */
#include <stddef.h>
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_MATCH(T, V, T_m, int)
static int ma(PA *p) { return p->a; }
static int mb(PB *p) { return p->b; }
int main(void) {
    T t = T_new_a((PA){ 1 });
#ifndef SPEC_FIXED
    (void)mb;
    return T_m(&t, ma, NULL);            /* variant 追加時の「とりあえず NULL」 */
#else
    return T_m(&t, ma, mb);
#endif
}
