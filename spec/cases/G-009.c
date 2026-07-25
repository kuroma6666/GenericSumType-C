/* ペイロード型が重複していると _Generic の連想リストが重複する。
 * DEFINE_SUM_NEW_GENERIC が生成する probe 関数がその場で _Generic を展開するため、
 * SUM_NEW を一度も呼ばなくても定義時点で落ちる。 */
#include "generic_sum_type.h"
typedef struct { int v; } IntBox;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, IntBox) X(NAME, EXTRA, b, IntBox)
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_NEW_GENERIC(T, V)
int main(void) { return 0; }   /* SUM_NEW は呼ばない */
