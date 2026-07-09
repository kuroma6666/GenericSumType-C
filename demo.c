#include <stdint.h>
#include <stdio.h>
#include "generic_sum_type.h"

/* 型リストは1箇所だけで定義する（ここを1行足すだけでvariantを増やせる） */
#define INT_OR_STR_OR_FLOAT(X, NAME, EXTRA) \
    X(NAME, EXTRA, i, int32_t)              \
    X(NAME, EXTRA, s, const char*)          \
    X(NAME, EXTRA, f, double)

DEFINE_SUM_TYPE(IntOrStrOrFloat, INT_OR_STR_OR_FLOAT)
DEFINE_SUM_MATCH(IntOrStrOrFloat, INT_OR_STR_OR_FLOAT, IntOrStrOrFloat_print, int)

static int print_i(int32_t *v) { printf("int:   %d\n", *v);   return 0; }
static int print_s(const char **v) { printf("str:   %s\n", *v); return 0; }
static int print_f(double *v) { printf("float: %f\n", *v);    return 0; }

int main(void) {
    IntOrStrOrFloat values[] = {
        IntOrStrOrFloat_new_i(42),
        IntOrStrOrFloat_new_s("hello"),
        IntOrStrOrFloat_new_f(3.14),
    };

    for (size_t idx = 0; idx < 3; ++idx) {
        IntOrStrOrFloat_print(&values[idx], print_i, print_s, print_f);
    }

    /* ctor/getter単体の動作確認 */
    IntOrStrOrFloat v = IntOrStrOrFloat_new_i(7);
    int32_t *pi = IntOrStrOrFloat_get_i(&v);
    const char **ps = IntOrStrOrFloat_get_s(&v);
    printf("get_i -> %s (%d)\n", pi ? "非NULL" : "NULL", pi ? *pi : -1);
    printf("get_s -> %s\n", ps ? "非NULL" : "NULL");

    return 0;
}
