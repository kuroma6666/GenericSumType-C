/* 生成コードは「TYPE 識別子;」という宣言パターンを前提にしている。関数ポインタ型は
 * 識別子を型の内側（TYPE (*識別子)(...)）に置く必要があり、このパターンに合わない。
 * マクロ引数のコンマ分割が原因ではない点に注意（括弧で保護されている）。対照は G-006 の回避策。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
#ifndef SPEC_FIXED
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, cb, void (*)(int, int))
#else
typedef void (*Callback)(int, int);
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, cb, Callback)
#endif
SUM_DEFINE_TYPE(T, V)
int main(void) { return 0; }
