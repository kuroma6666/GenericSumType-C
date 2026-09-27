/* SUM_NO_LEGACY_NAMES を定義すると v1 のマクロ名は提供されない。
 * 移行完了後に v1 名の残存をビルドで検出するための切り替え。 */
#define SUM_NO_LEGACY_NAMES
#include "generic_sum_type.h"
typedef struct { int v; } P1;
typedef struct { int v; } P2;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, P1) X(NAME, EXTRA, b, P2)
#ifndef SPEC_FIXED
DEFINE_SUM_TYPE(T, V)
#else
SUM_DEFINE_TYPE(T, V)
#endif
int main(void) { return 0; }
