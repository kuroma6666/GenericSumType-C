/* SUM_NEW にどの variant のペイロード型とも一致しない値を渡す。
 * _Generic に default 連想を用意していないため選択できず落ちる。 */
#include "generic_sum_type.h"
typedef struct { int v; } IntBox;
typedef struct { const char *v; } StrBox;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, i, IntBox) X(NAME, EXTRA, s, StrBox)
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_NEW_GENERIC(T, V)
int main(void) {
    T t = SUM_NEW(T, V, 42);   /* IntBox でも StrBox でもない生の int */
    return t.tag;
}
