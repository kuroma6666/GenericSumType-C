# 共有テレメトリカウンタの並行更新 — 仕様

対応デモ: [`examples/threadsafe_dispatch_demo.c`](../threadsafe_dispatch_demo.c)

## 背景・要件
複数ワーカースレッドが1つの共有カウンタに加算/減算コマンドを適用する。カウンタ更新は read-modify-write であり、排他制御がないと lost update で値がずれる。ライブラリ本体を特定ロック機構に依存させず、環境（pthread / RTOS）に応じたロックを差し込みたい。

## 仕様
- `CounterCmd` は `increment{amount}` / `decrement{amount}`。
- dispatch はコマンドを共有 `Counter` に適用し、その区間を排他する。
- +1 と -1 を同数スレッドで回し、最終値が必ず 0 になること。

## 実装対応
- `SUM_DEFINE_DISPATCH_LOCKED` に pthread_mutex のロック/アンロック関数を渡す。ロックは定義箇所で型ごとに決まる（[ADR-0014](../../docs/adr/0014-dispatch-ret-and-locked.md)）。
- RTOS では渡す関数をセマフォ等に差し替えるだけ（ライブラリ本体は不変）。

## 受け入れ基準
- 8スレッド並行で最終カウンタが 0（ロックが lost update を防いでいる）。
- 注意: ハンドラ内から同一 ctx へ再 dispatch すると非再帰ロックでデッドロック（[C-09](../../docs/constraints.md#c-09)）。本デモは再入しない。
