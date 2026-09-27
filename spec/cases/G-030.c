/* v1 のマクロ名は非推奨の別名として残り、SUM_LEGACY_TAG_NAMES を定義すれば v1 の
 * tag 定数名 NAME_<tag> も使える。v1 のコードを段階的に移行できることの固定。 */
#define SUM_LEGACY_TAG_NAMES
#include "generic_sum_type.h"
typedef struct { int v; } IntBox;
typedef struct { int code; } ErrBox;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, ok, IntBox) X(NAME, EXTRA, err, ErrBox)
DEFINE_SUM_TYPE(Res, V)
DEFINE_RESULT_HELPERS(Res)
DEFINE_SUM_MATCH(Res, V, Res_fold, int)
static int on_ok(IntBox *b) { return b->v; }
static int on_err(ErrBox *e) { return -e->code; }
int main(void) {
    Res r = Res_new_ok((IntBox){ 3 });
    int k = 0;
    switch (r.tag) {
        case Res_ok:  k = 1; break;             /* v1 の tag 定数名 */
        case Res_err: k = 2; break;
        default:      break;
    }
    return (k == 1 && Res_is_ok(&r) && Res_fold(&r, on_ok, on_err) == 3) ? 0 : 1;
}
