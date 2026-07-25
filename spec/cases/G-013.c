/* Either ヘルパは left/right という tag 規約に依存する。
 * 別の tag 名に使うと enum 定数が存在せず落ちる（規約違反がそのまま検出される）。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, foo, PA) X(NAME, EXTRA, bar, PB)
DEFINE_SUM_TYPE(T, V)
DEFINE_EITHER_HELPERS(T)   /* T_left / T_right が無い */
int main(void) { return 0; }
