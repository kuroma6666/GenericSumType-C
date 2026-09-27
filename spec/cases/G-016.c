/* Result ヘルパを規約通り ok/err の tag に使う（正常系）。 */
#include "generic_sum_type.h"
typedef struct { int v; }    Ok;
typedef struct { int code; } Err;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, ok, Ok) X(NAME, EXTRA, err, Err)
SUM_DEFINE_TYPE(Res, V)
SUM_DEFINE_RESULT_HELPERS(Res)
static int use(const Res *r) { return Res_is_ok(r) ? 0 : 1; }
int main(void) { Res r = Res_new_ok((Ok){ .v = 1 }); return use(&r); }
