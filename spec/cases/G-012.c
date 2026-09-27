/* const NAME* を保持したまま read-only な match / getter を呼べる
 * （可変版 SUM_DEFINE_MATCH は discards const で落ちる）。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_MATCH_CONST(T, V, T_area, int)
static int on_a(const PA *p) { return p->a; }
static int on_b(const PB *p) { return p->b; }
static int use(const T *t) {
    const PA *pa = T_get_a_const(t);
    (void)pa;
    return T_area(t, on_a, on_b);
}
int main(void) { T t = T_new_a((PA){ 7 }); return use(&t) == 7 ? 0 : 1; }
