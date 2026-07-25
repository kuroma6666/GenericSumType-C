/* MATCH_CONST のハンドラはペイロードを Type const* で受け取るため、書き換えられない。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_MATCH_CONST(T, V, T_area, int)
static int on_a(const PA *p) { return p->a; }
static int on_b(const PB *p) { p->b = 9; return p->b; }   /* const への代入 */
int main(void) { T t = T_new_a((PA){ 1 }); return T_area(&t, on_a, on_b); }
