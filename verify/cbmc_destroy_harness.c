/*
 * cbmc_destroy_harness.c
 *
 * DEFINE_SUM_DESTROY呼び出し忘れ/cleanup属性の二重解放について、
 * CBMC(Bounded Model Checker for C)で検証するためのハーネス。
 * design_spec.md 8節「DEFINE_SUM_DESTROY/DEFINE_SUM_COPYの呼び出し忘れ自体を
 * 検出する手段」の検証で使ったもの。CIには組み込んでおらず、CBMCが使える
 * 環境での任意の追加検証という位置づけ(通常のビルド・テストには影響しない)。
 *
 * 検証したい3つの性質:
 *   1. safe_case        : 正しく1回だけdestroyを呼ぶ場合
 *                          -> エラーなし(サニティチェック)
 *   2. leaked_case       : destroyを呼び忘れる場合
 *                          -> --memory-leak-check がリークを検出するはず
 *                          (design_spec.md 8節: gccの-fanalyzerでは検出できなかった問題)
 *   3. double_free_case  : 値コピー後にcleanup相当の解放を2回呼んでしまう場合
 *                          -> --pointer-check が二重解放を検出するはず
 *                          (design_spec.md 8節: ASanで再現した問題を別ツールで裏付け)
 *
 * 注意: __attribute__((cleanup(...))) 自体をCBMCが正しくモデル化できるかは
 * 未確認のため、double_free_case ではcleanup属性そのものは使わず、GCCの
 * ドキュメントが規定する「cleanup関数はスコープ終了時に宣言と逆順で呼ばれる」
 * という規約を明示的な関数呼び出しとして書き下している。これは実際にgccが
 * 生成するコードと意味的に同一であり、cleanup属性のCBMCでのモデル化可否に
 * 依存せず「二重解放という結果」だけを検証できる、より頑健なハーネス設計である。
 *
 * 実行方法(実機Linux、Debian/Ubuntu系の例。verify/ディレクトリで実行):
 *   sudo apt-get install cbmc
 *   ./cbmc_verify_destroy.sh
 *
 * または個別に:
 *   cbmc --pointer-check --memory-leak-check --bounds-check -I ../include \
 *        --function safe_case        cbmc_destroy_harness.c
 *
 * 期待される結果:
 *   safe_case        -> VERIFICATION SUCCESSFUL
 *   leaked_case      -> VERIFICATION FAILED (dynamically allocated memory ... leak)
 *   double_free_case -> VERIFICATION FAILED (double free)
 *
 * 3関数ともループ・再帰を含まないため、CBMCのシンボリック実行は近似(bounded)
 * ではなく状態空間を網羅的に探索できる。実機Linux(CBMC 5.95.1)で実行し、
 * 上記3件とも期待通りの結果になることを確認済み(design_spec.md 8節参照)。
 *
 * 期待と違う結果が出た場合(特にsafe_caseが失敗する場合)は、
 * ハーネス自体かCBMCのバージョン差異を疑うこと。
 */

#include <stdlib.h>
#include <string.h>
#include "generic_sum_type.h"

#define MSG_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, text, char*)

DEFINE_SUM_TYPE(Msg, MSG_VARIANTS)

static void destroy_text(char **s) {
    free(*s);
}

DEFINE_SUM_DESTROY(Msg, MSG_VARIANTS, Msg_destroy)

/* __attribute__((cleanup(Msg_auto_destroy))) が実際に呼ぶ関数と同じもの */
static void Msg_auto_destroy(Msg *m) {
    Msg_destroy(m, destroy_text);
}

/* 1) 正しく1回だけdestroyを呼ぶケース */
void safe_case(void) {
    char *p = malloc(6);
    if (p == NULL) return;
    memcpy(p, "hello", 6);
    Msg a = Msg_new_text(p);
    Msg_auto_destroy(&a);
}

/* 2) destroyを呼び忘れるケース */
void leaked_case(void) {
    char *p = malloc(6);
    if (p == NULL) return;
    memcpy(p, "hello", 6);
    Msg a = Msg_new_text(p);
    (void)a;
    /* Msg_auto_destroy(&a); を意図的に呼んでいない */
}

/* 3) 値をコピーしてcleanup相当を2回呼んでしまうケース。
 *    __attribute__((cleanup(...)))をa, b両方に付けた場合、GCCは
 *    スコープ終了時に「宣言と逆順」でcleanup関数を呼ぶ(bが先、aが後)。
 *    それを明示的に書き下したもの。 */
void double_free_case(void) {
    char *p = malloc(6);
    if (p == NULL) return;
    memcpy(p, "hello", 6);
    Msg a = Msg_new_text(p);
    {
        Msg b = a; /* 値コピー。Cにmove semanticsがないため両方が同じポインタを所有していると誤認される */
        Msg_auto_destroy(&b); /* 内側スコープのcleanup相当(bの解放)が先に発火 */
    }
    Msg_auto_destroy(&a); /* 外側スコープのcleanup相当(同じポインタをもう一度解放) */
}

int main(void) {
    return 0;
}
