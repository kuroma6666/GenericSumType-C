# design_spec.md — 移設マップ

> **この文書は役割を終え、内容は目的別に分割された。以下は旧節番号から新しい置き場所への対応表である。**

## なぜ分割したか

この文書は3つの責務を1ファイルに混載していた。

| 内容 | 更新頻度 |
|---|---|
| 設計判断の根拠（なぜ A ではなく B か） | 一度決まれば不変。追記型 |
| 既知の制約カタログ | 実装が進むと減る。更新型 |
| テスト・CI・バグ・今後の課題といったプロジェクト状況 | 常に変動 |

更新頻度の違うものが同居していると、どれか一つを直すたびに全体を読み直すことになる。
保証の一覧は現在 `spec/manifest.tsv` を単一ソースとし、`docs/GUARANTEES.md` はその生成物である。

## 移設先

| 旧節 | 内容 | 移設先 |
|---|---|---|
| 冒頭・1章 | 対応環境、MSVC 非対応 | [ADR-0007](./docs/adr/0007-msvc-out-of-scope.md), [requirements.md](./docs/requirements.md) |
| 1 | 方式選択（X-Macro 採用と代替案の比較） | [ADR-0001](./docs/adr/0001-x-macro-code-generation.md) |
| 2.1〜2.3 | 型リスト・`DEFINE_SUM_TYPE`・`DEFINE_SUM_MATCH` の仕様 | [api_reference.md](./api_reference.md) |
| 2.4 | MATCH と DISPATCH を分けた判断 | [ADR-0002](./docs/adr/0002-separate-match-and-dispatch.md) |
| 2.5, 2.6 | DESTROY / COPY を専用マクロにした判断 | [ADR-0004](./docs/adr/0004-destroy-copy-separate-macros.md) |
| 2.7 | ロックフックの外出し | [ADR-0003](./docs/adr/0003-lock-hook-injection.md) |
| 2.8 | `_Generic` によるコンストラクタ選択 | [ADR-0008](./docs/adr/0008-generic-constructor-selection.md) |
| 2.9, 2.10 | Either / Result を薄いヘルパにした判断 | [ADR-0005](./docs/adr/0005-either-result-thin-helpers.md) |
| 2.10 後半 | ROP が汎用化できない件 | [C-13](./docs/constraints.md#c-13) |
| 3 | 網羅性検査の仕組みと限界 | [ADR-0001](./docs/adr/0001-x-macro-code-generation.md), [C-01](./docs/constraints.md#c-01), [GUARANTEES.md](./docs/GUARANTEES.md) |
| 4.1 | 所有権・呼び出し忘れ | [C-03](./docs/constraints.md#c-03) |
| 4.2 | 再帰的直和型 | [C-04](./docs/constraints.md#c-04), [ADR-0006](./docs/adr/0006-no-recursive-sum-types.md) |
| 4.3 | カプセル化がない | [C-05](./docs/constraints.md#c-05) |
| 4.4 | 1 variant = 1 型 | [C-06](./docs/constraints.md#c-06) |
| 4.5 | 関数ポインタ型・配列型の制約 | [C-02](./docs/constraints.md#c-02) |
| 4.6 | コンパイラ依存部分 | [C-07](./docs/constraints.md#c-07) |
| 4.7 | `RetType = void` が使えない | [C-08](./docs/constraints.md#c-08) |
| 4.8 | スレッドセーフティ・再入デッドロック | [C-09](./docs/constraints.md#c-09) |
| 4.9 | メモリレイアウト | [C-10](./docs/constraints.md#c-10) |
| 4.10 | 複数翻訳単位での共有 | [C-11](./docs/constraints.md#c-11), [G-007](./docs/GUARANTEES.md#g-007) |
| 4.11 | デバッグ容易性 | [C-12](./docs/constraints.md#c-12) |
| 4.12 | 規格書との突き合わせ | [VERIFICATION_LOG.md](./VERIFICATION_LOG.md) 4.12節 |
| 4.13 | `SUM_NEW` の制約 | [ADR-0008](./docs/adr/0008-generic-constructor-selection.md), [C-01](./docs/constraints.md#c-01) |
| 4.14 | const 正しさ | [ADR-0009](./docs/adr/0009-non-breaking-const-support.md) |
| 5.1 | ランタイム単体テスト | [`tests/`](./tests/) |
| 5.2 | コンパイル時保証の一覧 | **[`spec/`](./spec/) が単一ソース。[docs/GUARANTEES.md](./docs/GUARANTEES.md) はその生成物** |
| 6 | CI | [`.github/workflows/ci.yml`](./.github/workflows/ci.yml)（設定ファイル自体が正） |
| 7 | 見つかったバグ | [VERIFICATION_LOG.md](./VERIFICATION_LOG.md) 7節 |
| 8 | 今後の検討課題 | GitHub Issues（Markdown のタスクリストは腐るため） |
| — | 何を解くライブラリか（当時は不在） | [requirements.md](./docs/requirements.md) |

## 現在の文書構成

| 文書 | 役割 | 読者 |
|---|---|---|
| [README.md](./README.md) | 入口・クイックスタート | はじめて見る人 |
| [docs/requirements.md](./docs/requirements.md) | 何を解くか・採用条件・非目標 | 採用を検討する人 |
| [api_reference.md](./api_reference.md) | シグネチャと使い方 | 利用者 |
| [docs/GUARANTEES.md](./docs/GUARANTEES.md) | コンパイル時に何が保証されるか（**生成物**） | 利用者・メンテナ |
| [docs/constraints.md](./docs/constraints.md) | できないことの一覧 | 利用者・メンテナ |
| [docs/adr/](./docs/adr/) | なぜそう決めたか | メンテナ・将来の自分 |
| [spec/](./spec/) | 保証の実行可能仕様（**単一ソース**） | メンテナ |
| [VERIFICATION_LOG.md](./VERIFICATION_LOG.md) | 結論に至るまでの過程・訂正履歴 | 設計の読み物として |

`VERIFICATION_LOG.md` の節番号はこの文書の旧節番号に対応している。上の表で読み替えること。
