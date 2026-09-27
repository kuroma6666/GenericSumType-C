/* SUM_DEFINE_TYPE は struct にタグ名 NAME を付けるため、payload から struct NAME* で
 * 自身の型を指せる（値としての自己埋め込みは言語上不可。C-04）。所有権は管理しない。 */
#include "generic_sum_type.h"
typedef struct { int v; } Num;
typedef struct { struct Expr *lhs, *rhs; } Add;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, num, Num) X(NAME, EXTRA, add, Add)
SUM_DEFINE_TYPE(Expr, V)
SUM_DEFINE_MATCH_CONST(Expr, V, Expr_eval, int)
static int eval_num(const Num *n) { return n->v; }
static int eval_add(const Add *a) {
    return Expr_eval(a->lhs, eval_num, eval_add) + Expr_eval(a->rhs, eval_num, eval_add);
}
int main(void) {
    Expr one = Expr_new_num((Num){ 1 });
    Expr two = Expr_new_num((Num){ 2 });
    Expr sum = Expr_new_add((Add){ &one, &two });
    return Expr_eval(&sum, eval_num, eval_add) == 3 ? 0 : 1;
}
