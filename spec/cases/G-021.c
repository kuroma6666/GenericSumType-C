/* 生成される switch は意図的に default: を持たない（ADR-0010: NAME_tag_t を手で拡張された
 * ときに -Wswitch の網羅性警告を殺さないため）。その帰結として、-Wswitch-default を error に
 * 昇格した TU では DEFINE_SUM_* を呼んだ時点で落ちる。これを既知の限界として固定する。
 * 呼び出し側での回避策（pragma で囲む）が機能することは G-022 で確かめる。
 * 注意: -Wswitch-default を知らない Clang 17 以前では -Wunknown-warning-option が -Werror で
 * error になり、別の理由でコンパイルが失敗する（結果だけ見ると同じ）。CI の Clang は 18。 */
#pragma GCC diagnostic error "-Wswitch-default"
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_MATCH(T, V, T_match, int)   /* 生成 switch に default: が無いので error */
static int ma(PA *p) { return p->a; }
static int mb(PB *p) { return p->b; }
int main(void) { T t = T_new_a((PA){ 1 }); return T_match(&t, ma, mb) - 1; }
