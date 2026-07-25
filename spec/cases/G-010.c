/* SUM_NEW が値の型だけからコンストラクタを選ぶ（正常系）。 */
#include "generic_sum_type.h"
typedef struct { int v; } IntBox;
typedef struct { const char *v; } StrBox;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, i, IntBox) X(NAME, EXTRA, s, StrBox)
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_NEW_GENERIC(T, V)
int main(void) {
    T a = SUM_NEW(T, V, ((IntBox){ 1 }));
    T b = SUM_NEW(T, V, ((StrBox){ "x" }));
    return (a.tag == b.tag) ? 1 : 0;
}
