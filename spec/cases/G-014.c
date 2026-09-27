/* Either ヘルパを規約通り left/right の tag に使う（正常系）。 */
#include "generic_sum_type.h"
typedef struct { int code; } Err;
typedef struct { int val; }  Ok;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, left, Err) X(NAME, EXTRA, right, Ok)
SUM_DEFINE_TYPE(Result, V)
SUM_DEFINE_EITHER_HELPERS(Result)
static int use(const Result *r) { return Result_is_left(r) ? 0 : 1; }
int main(void) { Result r = Result_new_right((Ok){ .val = 1 }); return use(&r); }
