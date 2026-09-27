# ADR-0016: 生成関数に `nonnull` 属性を付け、定数 NULL のハンドラを検出する

- **状態:** 採用
- **関連:** [G-021](../GUARANTEES.md#g-021), [G-022](../GUARANTEES.md#g-022), [C-07](../constraints.md#c-07)

## 文脈

網羅性の検査は引数の個数で強制している（[ADR-0001](./0001-x-macro-code-generation.md)）。ところがハンドラの代わりに `0` / `NULL` を渡すと、
`-Werror` を付けていてもコンパイルが通り、その variant が来た時点で実行時に落ちていた（SIGSEGV）。
variant を追加したときの「とりあえず NULL」は現実的なすり抜け方なのに、spec にも constraints にも載っていなかった。

## 決定

生成するすべての関数に、GCC / Clang の `__attribute__((nonnull))` を付ける。引数を指定しない形なので、すべてのポインタ引数が対象になる。

- 定数の NULL を渡すと、`-Wnonnull`（`-Wall` に含まれる）の警告が出て、`-Werror` でエラーになる（[G-021](../GUARANTEES.md#g-021)）。
- 変数を経由した NULL は検出できない（[G-022](../GUARANTEES.md#g-022)）。
- GCC / Clang 以外では、属性は空に展開される（`SUM__NONNULL`）。

## 帰結

- `self`（`SUM_DEFINE_COPY_CHECKED` では `dst` / `src`）と `ctx` も非 NULL が前提になる。v1 で `ctx` に `NULL` を渡していた利用者はコンパイルエラーになる。
  ctx が不要な用途は MATCH を使う（[ADR-0002](./0002-separate-match-and-dispatch.md)）。
- コンパイラは、これらの引数が非 NULL であることを前提に最適化してよくなる。生成関数の内部に NULL 検査は無いので、v1 から意味は変わらない。
