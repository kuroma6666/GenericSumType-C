/* C-14 の回避策。生成マクロの呼び出し箇所だけを診断 pragma で囲むと、-Wswitch-default を
 * error に昇格した TU でも通る。診断にはマクロ定義側（ヘッダ）ではなく展開位置（呼び出し側）の
 * pragma 状態が適用される、という前提の確認でもある。DEFINE_SUM_TYPE は switch を生成しないので
 * 囲む必要がない。利用側自身の switch（main 内）は規約どおり default を持つ。
 * 注意: Clang 17 以前は -Wswitch-default を知らず、このケースは -Wunknown-warning-option で落ちる。
 * その場合は Clang 18 以降で再実行すること。 */
#pragma GCC diagnostic error "-Wswitch-default"
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
DEFINE_SUM_TYPE(T, V)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-default"
DEFINE_SUM_MATCH(T, V, T_match, int)
DEFINE_SUM_DISPATCH(T, V, T_dispatch, int)
#pragma GCC diagnostic pop
static int ma(PA *p) { return p->a; }
static int mb(PB *p) { return p->b; }
static void da(PA *p, int *c) { (void)p; (*c)++; }
static void db(PB *p, int *c) { (void)p; (*c)++; }
int main(void) {
    T t = T_new_a((PA){ 1 });
    int calls = 0;
    T_dispatch(&t, &calls, da, db);
    switch (calls) {
        case 1:  return T_match(&t, ma, mb) - 1;
        default: return 1;
    }
}
