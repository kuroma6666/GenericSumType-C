/* ペイロード型が異なる2 variant のハンドラを入れ替えて渡す。
 * 関数ポインタの型が variant ごとに固有なので -Wincompatible-pointer-types（-Werror でエラー）。 */
#include "generic_sum_type.h"
typedef struct { int target, dmg; } CmdAttack;
typedef struct { int target, amt; } CmdHeal;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, attack, CmdAttack) X(NAME, EXTRA, heal, CmdHeal)
SUM_DEFINE_TYPE(Cmd, V)
SUM_DEFINE_DISPATCH(Cmd, V, Cmd_dispatch, int)
static void on_attack(CmdAttack *a, int *c) { (void)a; (void)c; }
static void on_heal(CmdHeal *h, int *c) { (void)h; (void)c; }
int main(void) {
    Cmd c = Cmd_new_attack((CmdAttack){ 1, 2 });
    int ctx = 0;
#ifndef SPEC_FIXED
    Cmd_dispatch(&c, &ctx, on_heal, on_attack);   /* 順序が逆 */
#else
    Cmd_dispatch(&c, &ctx, on_attack, on_heal);
#endif
    return 0;
}
