/* DISPATCH_LOCKED はロックを定義箇所で受け取るため、同じ翻訳単位の中で型ごとに
 * 異なるロック方針を持てる（ロックを持たない Ctx 型も同居できる）。 */
#include <stdio.h>
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
typedef struct { int n; int held; int acquired; } Shared;
typedef struct { int n; } Local;                 /* ロック用メンバを持たない */
static void shared_lock(Shared *s) { s->held++; s->acquired++; }
static void shared_unlock(Shared *s) { s->held--; }
SUM_DEFINE_DISPATCH_LOCKED(T, V, T_apply_shared, Shared, shared_lock, shared_unlock)
SUM_DEFINE_DISPATCH_LOCKED(T, V, T_apply_local, Local, SUM_NO_LOCK, SUM_NO_LOCK)
static void sa(PA *p, Shared *s) { s->n += p->a * s->held; }
static void sb(PB *p, Shared *s) { s->n += p->b * s->held; }
static void la(PA *p, Local *l) { l->n += p->a; }
static void lb(PB *p, Local *l) { l->n += p->b; }
int main(void) {
    T t = T_new_a((PA){ 5 });
    Shared s = { 0, 0, 0 };
    Local l = { 0 };
    T_apply_shared(&t, &s, sa, sb);
    T_apply_local(&t, &l, la, lb);
    if (s.n != 5 || s.held != 0 || s.acquired != 1 || l.n != 5) {
        printf("s.n=%d held=%d acquired=%d l.n=%d\n", s.n, s.held, s.acquired, l.n);
        return 1;
    }
    return 0;
}
