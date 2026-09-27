# ADR-0018: 網羅性を引数の個数で強制する方式を維持する（`-Wswitch` 方式との比較）

- **状態:** 採用（[ADR-0001](./0001-x-macro-code-generation.md) の「網羅性がビルドフラグに依存しない」という主張を訂正する）
- **関連:** [G-001](../GUARANTEES.md#g-001), [G-003](../GUARANTEES.md#g-003), [C-01](../constraints.md#c-01), [C-08](../constraints.md#c-08), [requirements.md](../requirements.md#既存の選択肢との比較)

## 文脈

C で直和型の網羅性を検査する方式は、次の2つしかない。

| 方式 | 採用例 | 仕組み |
|---|---|---|
| **引数の個数** | 本ライブラリ | 各 variant のハンドラを関数引数として渡させる。variant が増えると必須引数が増える |
| **`-Wswitch`** | [datatype99](https://github.com/Hirrolot/datatype99) | 名前付きの腕（`of(Variant, x) ...`）を switch に展開し、`case` 漏れをコンパイラの警告で検出する |

ADR-0001 と README は、引数個数方式の利点を「`-Wswitch` 等のビルドフラグの運用に依存しない」と説明していた。
しかし本ライブラリ自身が `-Wall -Wextra -Werror` を必須条件にしている（[requirements.md](../requirements.md)）。
実測すると、この条件の下では datatype99 も網羅漏れをエラーにする（gcc 13 / clang 18、`enumeration value ... not handled in switch [-Werror=switch]`）。
**必須条件の下では、この利点は差別化点として成立しない。**

## 決定

引数個数方式を維持する。そのうえで、利点と代償を次のとおり定義し直す。

**引数個数方式に残る利点**

- ワイルドカード（`otherwise` / `default`）を書く手段が無く、すべての呼び出し箇所ですべての variant の処理が必須になる。
  `-Wswitch` 方式では、`otherwise` を1つ書くとその箇所の検出が無効になる。Rust の `_` を禁止した状態に相当する。
- 検査がプロトタイプ検査（`too few arguments`）なので、`-Werror` が無くてもエラーになる。
  ただし順序取り違えの検出（G-002）には `-Werror` が要るので、ライブラリ全体としては依然として `-Werror` が前提。

**引数個数方式の代償**（いずれも「ハンドラが位置引数の関数である」ことから来る）

| 代償 | 現れ方 |
|---|---|
| 位置で対応付けるので、順序を取り違え得る | 型が同じ variant 間では検出できない（[C-01](../constraints.md#c-01), [G-003](../GUARANTEES.md#g-003)） |
| ハンドラからローカル変数に触れない | 状態は ctx 経由になる。戻り値と ctx を併用する形は [ADR-0014](./0014-dispatch-ret-and-locked.md) で補った |
| `RET=void` の MATCH が書けない | [C-08](../constraints.md#c-08) |

## 検討した代替案

| 案 | 不採用の理由 |
|---|---|
| `-Wswitch` 方式に切り替える | ワイルドカードの禁止という、残る唯一の強い利点を失う。既存の利用者コードもすべて書き直しになる |
| 文形式の `match`（名前付きの腕）を併設する | ローカル変数を使える利便性はあるが、網羅性の強度が異なる2つの API が混在する。「ワイルドカードを許したい」という要求が出たときに再検討する |

## 帰結

- README と requirements.md の差別化の説明を、上記の「残る利点」に合わせて書き直した。
- 採用を検討する人には、datatype99 との比較を [requirements.md](../requirements.md#既存の選択肢との比較) で示す。
