/*
 * デバイス設定値ストア（config value）
 *
 * 【要件】
 *   組み込み機器の設定項目は、項目ごとに値の型が異なる（ポート番号などの整数、
 *   ホスト名などの文字列、係数などの実数）。これらを型安全に1つの値型で保持し、
 *   型に応じて整形出力したい。設定の種類を増やしたとき、出力処理の更新漏れを
 *   コンパイル時に検出したい。
 *
 * 【仕様】
 *   - ConfigValue は次のいずれか1つを保持する直和型:
 *       i: int32_t（整数）, s: const char*（文字列）, f: double（実数）
 *   - ConfigValue_print(): 保持している型に応じた1行を出力する。
 *   - variant を追加したら print のハンドラ追加を強制する（渡し忘れ=コンパイルエラー）。
 *
 * 【実装方針】
 *   - DEFINE_SUM_TYPE で ConfigValue と new_/get_ を生成。
 *   - DEFINE_SUM_MATCH で print 用の網羅ディスパッチを生成（ハンドラは payload のみ）。
 *   - 網羅性は関数プロトタイプの引数個数チェックで強制される（design_spec 3節）。
 *
 * 仕様の詳細: examples/specs/config_value.md
 */
#include <stdint.h>
#include <stdio.h>
#include "generic_sum_type.h"

#define CONFIG_VALUE(X, NAME, EXTRA) \
    X(NAME, EXTRA, i, int32_t)       \
    X(NAME, EXTRA, s, const char*)   \
    X(NAME, EXTRA, f, double)

DEFINE_SUM_TYPE(ConfigValue, CONFIG_VALUE)
DEFINE_SUM_MATCH(ConfigValue, CONFIG_VALUE, ConfigValue_print, int)

static int print_i(int32_t *v)     { printf("  int    = %d\n", *v); return 0; }
static int print_s(const char **v) { printf("  string = %s\n", *v); return 0; }
static int print_f(double *v)      { printf("  double = %g\n", *v); return 0; }

int main(void) {
    /* 例: 3つの設定項目を1つの値型で保持する */
    ConfigValue items[] = {
        ConfigValue_new_i(8080),               /* listen_port         */
        ConfigValue_new_s("device-01.local"),  /* hostname            */
        ConfigValue_new_f(1.5),                /* calibration_factor  */
    };
    const char *keys[] = { "listen_port", "hostname", "calibration_factor" };

    for (size_t k = 0; k < sizeof items / sizeof items[0]; ++k) {
        printf("%s:\n", keys[k]);
        ConfigValue_print(&items[k], print_i, print_s, print_f);
    }
    return 0;
}
