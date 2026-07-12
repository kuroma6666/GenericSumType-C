# 注文処理の Result — 仕様

対応デモ: [`examples/result_demo.c`](../result_demo.c)

## 背景・要件
注文作成は在庫確認・決済などで失敗しうる。これらは想定内のドメインエラーなので、例外ではなく Result の「値」として返し、呼び出し側で分岐・変換したい（DB 接続断のような技術的エラーは別途 例外で扱う設計の、Result 側）。

## 仕様
- `OrderResult` は `ok`=OrderId / `err`=OrderError（在庫不足・決済失敗など）。
- `is_ok`/`is_err` で分岐、fold で「注文ID or エラーコード」に畳み込む。
- `unwrap_or` でデフォルト、同型 `and_then`（OrderId -> Result）で段階適用。

## 実装対応
- `DEFINE_SUM_TYPE(tag=ok/err)` + `DEFINE_RESULT_HELPERS`（is_ok/is_err）。
- fold は `DEFINE_SUM_MATCH_CONST`、取り出しは const ゲッター。

## 受け入れ基準
- 成功/失敗を値で分岐でき、fold・unwrap_or・同型 and_then が期待どおり動く。

## 既知の限界（ROP）
型が変わる関数合成（`map: Result<T,E>->Result<U,E>` / `map_err`）は C では汎用化できない（design_spec 2.10節）。書けるのは同型変換（`T->Result<T,E>`）か、目標型を明示した手書き合成のいずれか。
