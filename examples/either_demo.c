/*
 * キャッシュ参照結果 Hit/Miss（エラーではない対等な二分岐の Either）
 *
 * 【要件】
 *   キャッシュ参照は「ヒット（値がある）」か「ミス（無いのでフェッチが要る）」の
 *   二択。これは失敗/成功ではなく対等な二分岐なので、Result ではなく
 *   Either<Miss, Hit> で表したい（left=Miss, right=Hit）。
 *
 * 【仕様】
 *   - Lookup は left=miss（要フェッチのキーを持つ）/ right=hit（値を持つ）。
 *   - is_left/is_right で分岐。fold で「ヒットなら値、ミスなら別処理」に畳み込む。
 *
 * 【実装方針】
 *   - DEFINE_SUM_TYPE(tag=left/right) + DEFINE_EITHER_HELPERS（is_left/is_right）。
 *   - fold は DEFINE_SUM_MATCH_CONST の左右2ハンドラ、取り出しは const ゲッター。
 *   - L と R を別 struct にすることで左右ハンドラ取り違えを型で検出（design_spec 3節）。
 *
 * 仕様の詳細: examples/specs/cache_lookup.md
 */
#include <stdio.h>
#include "generic_sum_type.h"

typedef struct { int key; }   Miss;  /* Left : 要フェッチのキー */
typedef struct { int value; } Hit;   /* Right: キャッシュ済みの値 */

#define LOOKUP(X, NAME, EXTRA)   \
    X(NAME, EXTRA, left,  Miss)  \
    X(NAME, EXTRA, right, Hit)

DEFINE_SUM_TYPE(Lookup, LOOKUP)
DEFINE_EITHER_HELPERS(Lookup)
DEFINE_SUM_MATCH_CONST(Lookup, LOOKUP, Lookup_value_or_fetch, int)

/* miss のときはキーから「フェッチした値」を得る想定（ここでは key*10 で代用） */
static int on_miss(const Miss *m) { printf("  miss key=%d -> fetch\n", m->key); return m->key * 10; }
static int on_hit (const Hit  *h) { printf("  hit value=%d\n", h->value);      return h->value; }

static Lookup cache_get(int key) {
    /* 偶数キーだけキャッシュ済み、という単純化 */
    return (key % 2 == 0) ? Lookup_new_right((Hit){ .value = key * 10 })
                          : Lookup_new_left((Miss){ .key = key });
}

int main(void) {
    for (int key = 1; key <= 3; ++key) {
        Lookup r = cache_get(key);
        printf("key=%d is_hit=%d\n", key, Lookup_is_right(&r));
        int v = Lookup_value_or_fetch(&r, on_miss, on_hit);
        printf("  resolved value = %d\n", v);
    }
    return 0;
}
