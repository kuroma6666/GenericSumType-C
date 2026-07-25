/* Result ヘルパは ok/err という tag 規約に依存する（G-013 の Result 版）。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, foo, PA) X(NAME, EXTRA, bar, PB)
DEFINE_SUM_TYPE(T, V)
DEFINE_RESULT_HELPERS(T)   /* T_ok / T_err が無い */
int main(void) { return 0; }
