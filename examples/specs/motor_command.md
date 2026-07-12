# ステッピングモータ制御コマンド — 仕様

対応デモ: [`examples/command_demo.c`](../command_demo.c)

## 背景・要件
モータ制御器は「速度設定・相対移動・停止」コマンドを受け取り、共有のモータ状態（現在位置・速度）を更新する。コマンド適用は状態への書き込み（副作用）を伴う。コマンド種別を増やしたら適用処理の更新漏れをコンパイル時に検出したい。

## 仕様
- `MotorCmd` は `set_speed{rpm}` / `move_by{steps}` / `stop{}`。
- 共有 `MotorState{pos, rpm}` に対し dispatch で適用: set_speed→rpm設定, move_by→pos加算, stop→rpm=0。

## 実装対応
- 副作用を伴うため `DEFINE_SUM_DISPATCH(MotorCmd, ..., MotorCmd_apply, MotorState)`（ハンドラは payload と ctx を受け取り戻り値 void）。
- データを持たない `stop` は「1 variant = 1 型」制約によりダミー1フィールド struct で表す（design_spec 4.4節）。

## 受け入れ基準
- コマンド列を順に適用して状態が期待どおり遷移する（pos=250, rpm=0）。
- コマンド追加時のハンドラ渡し忘れがコンパイルエラーになる。
