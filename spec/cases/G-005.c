/* 生成コードは「TYPE 識別子;」という宣言パターンを前提にしている。関数ポインタ型は
 * 識別子を型の内側（TYPE (*識別子)(...)）に置く必要があり、このパターンに合わない。
 * マクロ引数のコンマ分割が原因ではない点に注意（括弧で保護されている）。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, cb, void (*)(int, int))
DEFINE_SUM_TYPE(T, V)
int main(void) { return 0; }
