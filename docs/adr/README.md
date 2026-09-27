# 設計判断の記録（ADR）

このライブラリで「なぜ A ではなく B を選んだか」を1判断1ファイルで記録する。

ADR は**追記型**である。決定が覆った場合も既存ファイルは書き換えず、`状態` を
`置き換え済み（ADR-NNNN による）` に変え、新しい番号で書き起こす。過去の判断が
なぜ妥当だったか（そして何が変わって覆ったか）が読めなくなるのを避けるため。

「今どういう制約があるか」を引きたい場合は [`../constraints.md`](../constraints.md)、
「何がコンパイル時に保証されるか」は [`../GUARANTEES.md`](../GUARANTEES.md) を見ること。

| # | 決定 |
|---|---|
| [0001](./0001-x-macro-code-generation.md) | 多相性の解決を X-Macro によるコンパイル時コード生成で行う（帰結の一部を 0011, 0018 が訂正） |
| [0002](./0002-separate-match-and-dispatch.md) | 値を返す MATCH と副作用を伴う DISPATCH をマクロ名で分ける |
| [0003](./0003-lock-hook-injection.md) | ロックプリミティブを埋め込まずフックとして外出しする |
| [0004](./0004-destroy-copy-separate-macros.md) | 解放・コピーを DISPATCH の流用ではなく専用マクロにする |
| [0005](./0005-either-result-thin-helpers.md) | Either / Result を専用ジェネレータではなく薄いヘルパで実現する |
| [0006](./0006-no-recursive-sum-types.md) | 再帰的な直和型は対応しない |
| [0007](./0007-msvc-out-of-scope.md) | MSVC を対象外とする |
| [0008](./0008-generic-constructor-selection.md) | `_Generic` でコンストラクタを型から選択する（C11 以降） |
| [0009](./0009-non-breaking-const-support.md) | const 対応を破壊的変更ではなく追加で行う |
| [0010](./0010-unhandled-tag-hook.md) | 不正 tag の検知をフックとして提供する（置き換え済み: 0011） |
| [0011](./0011-default-label-and-trap.md) | 全 switch に `default:` を置き、値を返すディスパッチャは不正 tag で停止する |
| [0012](./0012-naming-v2.md) | 公開名を `SUM_` 接頭辞に統一し、tag 定数を `NAME_TAG_<tag>` にする |
| [0013](./0013-empty-tag-zero.md) | tag 値 0 を空状態として予約し、DESTROY 後は空状態に戻す |
| [0014](./0014-dispatch-ret-and-locked.md) | 戻り値付き DISPATCH とロック明示版 DISPATCH を追加する |
| [0015](./0015-copy-checked.md) | 失敗を返せるディープコピー `SUM_DEFINE_COPY_CHECKED` を追加する |
| [0016](./0016-nonnull-handlers.md) | 生成関数に `nonnull` 属性を付け、定数 NULL のハンドラを検出する |
| [0017](./0017-self-reference-by-pointer.md) | struct にタグ名を付け、payload から自身の型をポインタで指せるようにする |
| [0018](./0018-exhaustiveness-by-arity.md) | 網羅性を引数の個数で強制する方式を維持する（`-Wswitch` 方式との比較） |
