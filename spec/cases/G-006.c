/* typedef で型名を単一トークンにすれば、関数ポインタ型もペイロードに使える（G-005 の回避策）。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef void (*Callback)(int, int);
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, cb, Callback)
SUM_DEFINE_TYPE(T, V)
static void my_cb(int x, int y) { (void)x; (void)y; }
int main(void) {
    T t = T_new_cb(my_cb);
    Callback *pc = T_get_cb(&t);
    return pc ? 0 : 1;
}
