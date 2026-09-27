/* 値を返すディスパッチャ（MATCH / MATCH_CONST / COPY / COPY_CHECKED / DISPATCH_RET）は、
 * 不正 tag と空状態でフックを呼んだ後 SUM_TRAP() に進み、ハンドラを1つも呼ばない。
 * v1 ではここが __builtin_unreachable() で、-O2 で無関係なハンドラが呼ばれていた。
 * SUM_TRAP は「戻らない」ことが契約なので、ここでは longjmp で検査側へ戻す。 */
#include <setjmp.h>
#include <stdio.h>
static jmp_buf env;
static int fired = 0;
#define SUM_UNHANDLED_TAG(self) ((void)(self), fired++)
#define SUM_TRAP() longjmp(env, 1)
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_MATCH(T, V, T_m, int)
SUM_DEFINE_MATCH_CONST(T, V, T_mc, int)
SUM_DEFINE_COPY(T, V, T_copy)
SUM_DEFINE_COPY_CHECKED(T, V, T_try_copy)
typedef struct { int n; } Ctx;
SUM_DEFINE_DISPATCH_RET(T, V, T_run, Ctx, int)
static int calls = 0;
static int ma(PA *p) { (void)p; calls++; return 1; }
static int mb(PB *p) { (void)p; calls++; return 2; }
static int ca(PA const *p) { (void)p; calls++; return 1; }
static int cb(PB const *p) { (void)p; calls++; return 2; }
static PA pa(const PA *p) { calls++; return *p; }
static PB pb(const PB *p) { calls++; return *p; }
static int ka(PA *d, const PA *s) { calls++; *d = *s; return 0; }
static int kb(PB *d, const PB *s) { calls++; *d = *s; return 0; }
static int ra(PA *p, Ctx *c) { (void)p; (void)c; calls++; return 1; }
static int rb(PB *p, Ctx *c) { (void)p; (void)c; calls++; return 2; }
int main(void) {
    T bad = T_new_b((PB){ 1 });
    T empty = SUM_EMPTY(T);
    T dst = T_new_a((PA){ 0 });
    Ctx c = { 0 };
    volatile int traps = 0;
    bad.tag = (T_tag_t)99;
    if (setjmp(env) == 0) { (void)T_m(&bad, ma, mb); } else { traps++; }
    if (setjmp(env) == 0) { (void)T_m(&empty, ma, mb); } else { traps++; }
    if (setjmp(env) == 0) { (void)T_mc(&bad, ca, cb); } else { traps++; }
    if (setjmp(env) == 0) { (void)T_copy(&bad, pa, pb); } else { traps++; }
    if (setjmp(env) == 0) { (void)T_try_copy(&dst, &bad, ka, kb); } else { traps++; }
    if (setjmp(env) == 0) { (void)T_run(&bad, &c, ra, rb); } else { traps++; }
    if (traps != 6 || fired != 6 || calls != 0) {
        printf("traps=%d fired=%d calls=%d\n", traps, fired, calls);
        return 1;
    }
    return 0;
}
