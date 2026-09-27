/* DISPATCH_RET も他のディスパッチャと同じく、ハンドラの渡し忘れが too few arguments で落ちる。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
typedef struct { int n; } Ctx;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_DISPATCH_RET(T, V, T_run, Ctx, int)
static int ra(PA *p, Ctx *c) { return p->a + c->n; }
static int rb(PB *p, Ctx *c) { return p->b + c->n; }
int main(void) {
    T t = T_new_a((PA){ 1 });
    Ctx c = { 0 };
#ifndef SPEC_FIXED
    (void)rb;
    return T_run(&t, &c, ra);
#else
    return T_run(&t, &c, ra, rb) == 1 ? 0 : 1;
#endif
}
