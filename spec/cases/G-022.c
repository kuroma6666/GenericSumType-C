/* 変数を経由した NULL は nonnull 属性では検出できない（コンパイラは値を追えない）。
 * 検出できないことの固定。 */
#include <stddef.h>
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_MATCH(T, V, T_m, int)
static int ma(PA *p) { return p->a; }
int use(int (*hb)(PB *));
int use(int (*hb)(PB *)) {
    T t = T_new_a((PA){ 1 });
    return T_m(&t, ma, hb);              /* hb が NULL でも通る */
}
