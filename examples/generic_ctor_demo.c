/*
 * generic_ctor_demo.c
 *
 * DEFINE_SUM_NEW_GENERIC / SUM_NEW（C11以降限定のオプトイン機能）の使用例。
 *
 * DEFINE_SUM_TYPEが生成する NAME_new_<tag>() は、呼び出し側がタグ名
 * （このデモなら i / s / f）を正しく覚えて綴る必要がある。C11の _Generic を使うと、
 * 渡す値の「型」だけからコンストラクタを自動選択できるようになり、タグ名という
 * 語彙を呼び出し側の記憶から追い出せる。
 *
 *   IntOrStrOrFloat_new_i((IntBox){42})                        // 従来: タグ名を書く
 *   SUM_NEW(IntOrStrOrFloat, IOSF_VARIANTS, ((IntBox){42}))    // 今回: 型から自動選択
 *
 * ただしDEFINE_SUM_TYPE等と違い、NAME_new(x)のような専用マクロは生成できない
 * （Cのプリプロセッサはマクロ展開結果から新しい#defineを起こせないため。
 * design_spec.md 4.13節）。そのためSUM_NEWはNAMEとVARIANTSを毎回明示する
 * 単一の共通マクロになっている。
 *
 * ペイロードをdemo.cのように生の int32_t / const char* / double のまま使わず、
 * 専用structでラップしているのは、3節が既に推奨している設計ガイドラインに加えて
 * SUM_NEW固有の理由もある。_Generic は制御式の型が完全一致しないと選択できず、
 * 例えば生の const char* をキーにすると文字列リテラル（減衰後 char*）が
 * マッチしない、という落とし穴が実機検証で見つかっている（design_spec.md 4.13節）。
 * 専用structでラップしておけば、_Genericが見るのは外側のstruct型だけになるため
 * この落とし穴を避けられる。
 *
 * このファイル自体はC99でもビルドできる（SUM_NEWを使う部分だけ__STDC_VERSION__で
 * 分岐し、C99実行時は案内メッセージを出すだけにしている）。これはci.ymlが
 * examples配下の全 .c ファイルをc99/c11/c17全マトリクスでビルドする既存の運用を
 * 変更せずに済ませるための構成であり、design_spec.md 4.13節で検証した
 * 「C11限定機能をCIに乗せる実務コスト」への対応そのものである。
 */
#include <stdint.h>
#include <stdio.h>
#include "generic_sum_type.h"

typedef struct { int32_t v; } IntBox;
typedef struct { const char *v; } StrBox;
typedef struct { double v; } FloatBox;

#define IOSF_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, i, IntBox)         \
    X(NAME, EXTRA, s, StrBox)         \
    X(NAME, EXTRA, f, FloatBox)

DEFINE_SUM_TYPE(IntOrStrOrFloat, IOSF_VARIANTS)

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
DEFINE_SUM_NEW_GENERIC(IntOrStrOrFloat, IOSF_VARIANTS)
#endif

int main(void) {
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    /* 型だけを見てコンストラクタが自動選択される。タグ名(i/s/f)を書く必要がない */
    IntOrStrOrFloat a = SUM_NEW(IntOrStrOrFloat, IOSF_VARIANTS, ((IntBox){ 42 }));
    IntOrStrOrFloat b = SUM_NEW(IntOrStrOrFloat, IOSF_VARIANTS, ((StrBox){ "hello" }));
    IntOrStrOrFloat c = SUM_NEW(IntOrStrOrFloat, IOSF_VARIANTS, ((FloatBox){ 3.14 }));

    printf("a: tag=%d v=%d\n", a.tag, IntOrStrOrFloat_get_i(&a)->v);
    printf("b: tag=%d v=%s\n", b.tag, IntOrStrOrFloat_get_s(&b)->v);
    printf("c: tag=%d v=%f\n", c.tag, IntOrStrOrFloat_get_f(&c)->v);
#else
    printf("generic_ctor_demo: SUM_NEWはC11以降限定のため、このビルド(C99)ではスキップします\n");
#endif
    return 0;
}
