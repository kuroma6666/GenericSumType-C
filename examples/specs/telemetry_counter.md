# 共有テレメトリカウンタの並行更新 — 仕様

対応デモ: [`examples/threadsafe_dispatch_demo.c`](../threadsafe_dispatch_demo.c)

## 背景・要件
複数ワーカースレッドが1つの共有カウンタに加算/減算コマンドを適用する。カウンタ更新は read-modify-write であり、排他制御がないと lost update で値がずれる。ライブラリ本体を特定ロック機構に依存させず、環境（pthread / RTOS）に応じたロックを差し込みたい。

## 仕様
- `CounterCmd` は `increment{amount}` / `decrement{amount}`。
- dispatch はコマンドを共有 `Counter` に適用し、その区間を排他する。
- +1 と -1 を同数スレッドで回し、最終値が必ず 0 になること。

## 実装対応
- `DEFINE_SUM_DISPATCH` が switch 前後で呼ぶ `SUM_CTX_LOCK`/`SUM_CTX_UNLOCK` フックに、ヘッダ include 前に pthread_mutex を `#define` して差し込む。
- RTOS ではこのフックをセマフォ等に差し替えるだけ（ライブラリ本体は不変）。

## 受け入れ基準
- 8スレッド並行で最終カウンタが 0（ロックが lost update を防いでいる）。
- 注意: ハンドラ内から同一 ctx へ再 dispatch すると非再帰ロックでデッドロック（design_spec 4.8節）。本デモは再入しない。
