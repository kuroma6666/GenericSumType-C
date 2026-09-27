/* COPY_CHECKED のコピー関数は (dst, src) の順に variant 固有の型を取る。
 * 型が異なる2 variant のコピー関数を入れ替えると -Wincompatible-pointer-types で落ちる。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_COPY_CHECKED(T, V, T_try_copy)
static int ka(PA *d, const PA *s) { *d = *s; return 0; }
static int kb(PB *d, const PB *s) { *d = *s; return 0; }
int main(void) {
    T src = T_new_a((PA){ 1 });
    T dst = SUM_EMPTY(T);
#ifndef SPEC_FIXED
    return T_try_copy(&dst, &src, kb, ka);
#else
    return T_try_copy(&dst, &src, ka, kb);
#endif
}
