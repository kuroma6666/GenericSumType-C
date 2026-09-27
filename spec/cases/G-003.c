/* 2 variant が同一のペイロード型を持つと、ハンドラの関数ポインタ型も一致するため
 * 順序を入れ替えてもコンパイラには区別できない。検出できないことの固定。 */
#include "generic_sum_type.h"
#include <stdint.h>
#define V(X, NAME, EXTRA) X(NAME, EXTRA, score, int32_t) X(NAME, EXTRA, penalty, int32_t)
SUM_DEFINE_TYPE(Cmd, V)
SUM_DEFINE_DISPATCH(Cmd, V, Cmd_dispatch, int)
static void on_score(int32_t *v, int *c) { (void)v; (void)c; }
static void on_penalty(int32_t *v, int *c) { (void)v; (void)c; }
int main(void) {
    Cmd c = Cmd_new_score(10);
    int ctx = 0;
    Cmd_dispatch(&c, &ctx, on_penalty, on_score);   /* 取り違えているが通る */
    return 0;
}
