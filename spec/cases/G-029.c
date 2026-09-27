/* tag a の const ゲッター NAME_get_a_const と、tag a_const のゲッター NAME_get_a_const は
 * 同名になる。末尾が _const の tag 名は予約（docs/constraints.md C-14）。 */
#include "generic_sum_type.h"
typedef struct { int v; } P1;
typedef struct { int v; } P2;
#ifndef SPEC_FIXED
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, P1) X(NAME, EXTRA, a_const, P2)
#else
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, P1) X(NAME, EXTRA, a_fixed, P2)
#endif
SUM_DEFINE_TYPE(T, V)
int main(void) { return 0; }
