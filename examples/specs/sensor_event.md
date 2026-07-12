# 型からのイベント構築 — 仕様

対応デモ: [`examples/generic_ctor_demo.c`](../generic_ctor_demo.c)

## 背景・要件
センサハブは温度・スイッチ・パルスカウントなど複数種のイベントを1つのイベント型に格納する。生成側でタグ名を毎回綴るのは間違いやすいので、値の型からコンストラクタを自動選択して構築したい。

## 仕様
- `SensorEvent` は `temp`(TempReading) / `sw`(SwitchEvent) / `pulse`(PulseCount)。
- 渡した値の型から対応コンストラクタを自動選択して構築できる。
- C11 の `_Generic` を使うため C11 以降限定。C99 ではスキップ（ファイル自身を `__STDC_VERSION__` で自己ガード）。

## 実装対応
- `DEFINE_SUM_NEW_GENERIC` + `SUM_NEW`、推奨イディオム `#define SensorEvent_new(x) SUM_NEW(...)`。

## 受け入れ基準
- 値の型に応じて正しい variant が選択される（tag が一致）。
- C99 ビルドではスキップメッセージのみ（CI の c99 ジョブでもビルド・実行が通る）。

## 既知の落とし穴（プリプロセッサ）
複数フィールドの複合リテラルを `SUM_NEW` / `SensorEvent_new` へその場で渡すと、波括弧内のカンマがマクロ引数区切りと誤認されて壊れる（括弧はカンマを守るが波括弧は守らない）。payload をいったん名前付き変数に代入して渡すのが安全。
