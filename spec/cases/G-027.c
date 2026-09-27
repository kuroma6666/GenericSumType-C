/* 生成コードは -Wswitch-default / -Wswitch-enum を出さない（全 switch に default: と
 * 空状態の case を持つ）。MISRA C:2012 Rule 16.4 を課す利用側のビルドに入れられることの固定。 */
#pragma GCC diagnostic error "-Wswitch-default"
#pragma GCC diagnostic error "-Wswitch-enum"
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
typedef struct { int n; } Ctx;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_MATCH(T, V, T_m, int)
SUM_DEFINE_MATCH_CONST(T, V, T_mc, int)
SUM_DEFINE_DISPATCH(T, V, T_d, Ctx)
SUM_DEFINE_DISPATCH_RET(T, V, T_r, Ctx, int)
SUM_DEFINE_DESTROY(T, V, T_destroy)
SUM_DEFINE_COPY(T, V, T_copy)
SUM_DEFINE_COPY_CHECKED(T, V, T_try_copy)
int main(void) { return 0; }
