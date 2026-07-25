# ADR-0007: MSVC を対象外とする

- **状態:** 採用
- **関連:** [C-07](../constraints.md#c-07)

## 文脈

`SUM_UNREACHABLE()` は `__builtin_unreachable()`、`SUM_MAYBE_UNUSED` は `__attribute__((unused))` に
依存しており、いずれも GCC / Clang の処理系拡張である。MSVC に対応するには代替実装が要る。

## 決定

**GCC / Clang のみをサポートする。MSVC は対応せず、今後も検討しない。**

## 根拠

MSVC の利用が想定される環境では C++ が使える場合が多く、C++ であれば `std::variant` という
**標準の解が既に存在する**。そこにこのライブラリを移植するコストを払う理由がない。

「対応していない」ではなく「検討もしない」と明記するのは、この判断を再検討する提案が出るたびに
同じ議論を繰り返さないためである。

## 帰結

- GCC / Clang 拡張への依存を気兼ねなく増やせる。実際 [ADR-0008](./0008-generic-constructor-selection.md) や
  [ADR-0010](./0010-unhandled-tag-hook.md) はこの前提の上に立っている
- 非対応コンパイラでは `SUM_UNREACHABLE()` が `abort()` にフォールバックする。
  「コンパイル時の保証」ではなく「実行時にクラッシュする」形になる（[C-07](../constraints.md#c-07)）
- CI マトリクスは gcc / clang × c99 / c11 / c17 の6通りで足りる
