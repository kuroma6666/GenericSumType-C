/* 生成コードの仮引数・ローカル変数は sum_ で始まり _ で終わる名前に限られる。
 * 利用側が self / ctx / v や NAME_on_<tag> / NAME_copy_<tag> という名前を大域に
 * 持っていても、生成関数の仮引数がそれを隠して -Wshadow を出すことはない。
 * -Wshadow は -Wall / -Wextra に含まれないため、この TU だけ error に昇格して検査する。 */
#pragma GCC diagnostic error "-Wshadow"
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
typedef struct { int n; } Ctx;
static Ctx *ctx;   /* 生成コードが仮引数名に使いそうな名前を大域に置く */
static int self;
static int v;
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_MATCH(T, V, T_match, int)
DEFINE_SUM_MATCH_CONST(T, V, T_match_const, int)
DEFINE_SUM_DISPATCH(T, V, T_dispatch, Ctx)
DEFINE_SUM_DESTROY(T, V, T_destroy)
DEFINE_SUM_COPY(T, V, T_copy)
SUM_DEFINE_PREDICATE(T, a)
static void T_on_a(PA *p, Ctx *c) { (void)p; c->n++; }   /* NAME_on_<tag> という自然な命名 */
static void T_on_b(PB *p, Ctx *c) { (void)p; c->n++; }
SUM_DEFINE_IDENTITY_COPY(T_copy_a, PA)                    /* NAME_copy_<tag> も同様 */
SUM_DEFINE_IDENTITY_COPY(T_copy_b, PB)
static int ma(PA *p) { return p->a; }
static int mb(PB *p) { return p->b; }
static int ca(const PA *p) { return p->a; }
static int cb(const PB *p) { return p->b; }
static void za(PA *p) { (void)p; }
static void zb(PB *p) { (void)p; }
int main(void) {
    Ctx c = { 0 };
    ctx = &c; self = 0; v = 0;
    T t = T_new_a((PA){ 1 });
    T_dispatch(&t, &c, T_on_a, T_on_b);
    T u = T_copy(&t, T_copy_a, T_copy_b);
    T_destroy(&u, za, zb);
    return T_match(&t, ma, mb) + T_match_const(&t, ca, cb) + T_is_a(&t) + c.n + self + v - 4;
}
