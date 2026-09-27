/* tag 値 0 は空状態（NAME_TAG__EMPTY）。ゼロ初期化した値はどの variant でもなく、
 * ゲッターは NULL を返し、DESTROY は何もしない（フックも呼ばない）。
 * DESTROY は解放後に空状態へ戻すため、同じ値への2回目の DESTROY は no-op になる。 */
#include <stdio.h>
#include <string.h>
static int fired = 0;
#define SUM_UNHANDLED_TAG(self) ((void)(self), fired++)
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_DESTROY(T, V, T_destroy)
static int freed = 0;
static void da(PA *p) { (void)p; freed++; }
static void db(PB *p) { (void)p; freed++; }
static T g_static;                               /* 静的記憶域はゼロ初期化される */
int main(void) {
    T z;
    T t = T_new_b((PB){ 1 });
    memset(&z, 0, sizeof z);
    if (z.tag != T_TAG__EMPTY || g_static.tag != T_TAG__EMPTY) { puts("zero != EMPTY"); return 1; }
    if (T_get_a(&z) != NULL || T_get_b(&z) != NULL) { puts("getter on EMPTY"); return 1; }
    T_destroy(&z, da, db);
    T_destroy(&t, da, db);
    T_destroy(&t, da, db);
    if (freed != 1 || fired != 0 || t.tag != T_TAG__EMPTY) {
        printf("freed=%d fired=%d tag=%d\n", freed, fired, (int)t.tag);
        return 1;
    }
    return 0;
}
