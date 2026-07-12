# 設定値ストア (config value) — 仕様

対応デモ: [`examples/demo.c`](../demo.c)

## 背景・要件
組み込み機器の設定項目は、項目ごとに値の型が異なる（整数のポート番号、文字列のホスト名、実数の係数など）。これらを型安全に1つの値型で保持し、型に応じて整形出力したい。設定の種類を増やしたとき、出力処理の更新漏れをコンパイル時に検出したい。

## 仕様
- `ConfigValue` は次のいずれか1つを保持する直和型: `i`(int32_t) / `s`(const char*) / `f`(double)。
- `ConfigValue_print()` は保持している型に応じた1行を出力する。
- variant を追加したら、`print` のハンドラ追加を強制する（渡し忘れはコンパイルエラー）。

## 実装対応（generic_sum_type.h の使い方）
- `DEFINE_SUM_TYPE(ConfigValue, CONFIG_VALUE)` … 型・コンストラクタ・ゲッター生成。
- `DEFINE_SUM_MATCH(..., ConfigValue_print, int)` … 網羅ディスパッチ。ハンドラは payload のみ受け取る。

## 受け入れ基準（このデモで確認できること）
- 3種の値を1つの型で保持し、型ごとに出力できる。
- `print_s` を渡し忘れる／順序を入れ替えると、`gcc -Werror` でコンパイルが落ちる（design_spec 3節）。
