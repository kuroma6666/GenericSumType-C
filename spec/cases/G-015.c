/* Result ヘルパは ok/err という tag 規約に依存する（G-013 の Result 版）。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#ifndef SPEC_FIXED
#define V(X, NAME, EXTRA) X(NAME, EXTRA, foo, PA) X(NAME, EXTRA, bar, PB)
#else
#define V(X, NAME, EXTRA) X(NAME, EXTRA, ok, PA) X(NAME, EXTRA, err, PB)
#endif
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_RESULT_HELPERS(T)   /* T_TAG_ok / T_TAG_err が無い */
int main(void) { return 0; }
