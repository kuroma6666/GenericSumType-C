/* tag 定数は NAME_TAG_<tag>、生成関数は NAME_new_ / NAME_get_ / NAME_is_ で始まるため、
 * v1 で衝突していた tag 名（get_a, new_a, tag_t, is_a）を同じ型に並べても衝突しない。 */
#include "generic_sum_type.h"
typedef struct { int v; } P1;
typedef struct { int v; } P2;
typedef struct { int v; } P3;
typedef struct { int v; } P4;
typedef struct { int v; } P5;
#define V(X, NAME, EXTRA) \
    X(NAME, EXTRA, a, P1) X(NAME, EXTRA, get_a, P2) X(NAME, EXTRA, new_a, P3) \
    X(NAME, EXTRA, tag_t, P4) X(NAME, EXTRA, is_a, P5)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_PREDICATE(T, a)
int main(void) {
    T t = T_new_get_a((P2){ 1 });
    return (T_get_get_a(&t) != NULL && !T_is_a(&t) && t.tag == T_TAG_get_a) ? 0 : 1;
}
