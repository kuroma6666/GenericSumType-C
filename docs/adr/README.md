# 設計判断の記録（ADR）

このライブラリで「なぜ A ではなく B を選んだか」を1判断1ファイルで記録する。

ADR は**追記型**である。決定が覆った場合も既存ファイルは書き換えず、`状態` を
`置き換え済み（ADR-NNNN による）` に変え、新しい番号で書き起こす。過去の判断が
なぜ妥当だったか（そして何が変わって覆ったか）が読めなくなるのを避けるため。

「今どういう制約があるか」を引きたい場合は [`../constraints.md`](../constraints.md)、
「何がコンパイル時に保証されるか」は [`../GUARANTEES.md`](../GUARANTEES.md) を見ること。

| # | 決定 |
|---|---|
| [0001](./0001-x-macro-code-generation.md) | 多相性の解決を X-Macro によるコンパイル時コード生成で行う |
| [0002](./0002-separate-match-and-dispatch.md) | 値を返す MATCH と副作用を伴う DISPATCH をマクロ名で分ける |
| [0003](./0003-lock-hook-injection.md) | ロックプリミティブを埋め込まずフックとして外出しする |
| [0004](./0004-destroy-copy-separate-macros.md) | 解放・コピーを DISPATCH の流用ではなく専用マクロにする |
| [0005](./0005-either-result-thin-helpers.md) | Either / Result を専用ジェネレータではなく薄いヘルパで実現する |
| [0006](./0006-no-recursive-sum-types.md) | 再帰的な直和型は対応しない |
| [0007](./0007-msvc-out-of-scope.md) | MSVC を対象外とする |
| [0008](./0008-generic-constructor-selection.md) | `_Generic` でコンストラクタを型から選択する（C11 以降） |
| [0009](./0009-non-breaking-const-support.md) | const 対応を破壊的変更ではなく追加で行う |
| [0010](./0010-unhandled-tag-hook.md) | 不正 tag の検知をフックとして提供する |
| [0011](./0011-reserved-internal-identifiers.md) | 生成コードの内部識別子を `sum_*_` の予約名に限定する |
