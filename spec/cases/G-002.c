/* ペイロード型が異なる2 variant のハンドラを入れ替えて渡す。
 * 関数ポインタの型が variant ごとに固有なので -Wincompatible-pointer-types。 */
#include "generic_sum_type.h"
typedef struct { int target, dmg; } CmdAttack;
typedef struct { int target, amt; } CmdHeal;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, attack, CmdAttack) X(NAME, EXTRA, heal, CmdHeal)
DEFINE_SUM_TYPE(Cmd, V)
DEFINE_SUM_DISPATCH(Cmd, V, Cmd_dispatch, int)
static void on_attack(CmdAttack *a, int *c) { (void)a; (void)c; }
static void on_heal(CmdHeal *h, int *c) { (void)h; (void)c; }
int main(void) {
    Cmd c = Cmd_new_attack((CmdAttack){ 1, 2 });
    int ctx = 0;
    Cmd_dispatch(&c, &ctx, on_heal, on_attack);   /* 順序が逆 */
    return 0;
}
