# 要件 — 何を解くライブラリか

この文書は「このライブラリを**採用してよい状況**と、**採用すべきでない状況**」を定める。
個々の設計判断の根拠は [`adr/`](./adr/)、実装できていないことの一覧は
[`constraints.md`](./constraints.md)、コンパイル時に何が保証されるかは
[`GUARANTEES.md`](./GUARANTEES.md) にある。

## 解く問題

C には代数的直和型がない。そのため「値が N 種類のうちどれか一つ」という構造を書くと、
タグ付き共用体を手書きし、`switch` で分岐することになる。この書き方には次の欠陥がある。

**variant を1つ増やしたとき、対応漏れが黙って通る。** 既存の `switch` に `case` を足し忘れても
コンパイルは成功し、実行時に「何もしない」という形で表面化する。`-Wswitch` は多少助けになるが、
`switch` が複数箇所に散っていれば全部に届く保証はなく、ビルドフラグの運用に依存する。

このライブラリは、**variant の追加漏れと取り違えをコンパイルエラーとして検出可能にする**ことを
主目的とする。網羅性は標準 C の関数プロトタイプ検査（引数の個数）で強制し、「残り全部」を受ける
ワイルドカードを書く手段を持たない。したがって variant を足すと、**すべての**呼び出し箇所が
ビルドエラーになる（[ADR-0018](./adr/0018-exhaustiveness-by-arity.md)）。

副次的に、不正 tag・未初期化値（空状態）・定数 NULL ハンドラといった実行時の誤りを、
未定義動作ではなく検出可能な形（フック・trap・コンパイルエラー）で表面化させる
（[ADR-0011](./adr/0011-default-label-and-trap.md), [ADR-0013](./adr/0013-empty-tag-zero.md), [ADR-0016](./adr/0016-nonnull-handlers.md)）。

## 想定利用者

次の条件を**すべて**満たす開発者を想定している。

- C99 以降しか使えない（C++ が使えない、あるいは使わない方針である）
- GCC または Clang でビルドしている
- コマンド種別・メッセージ種別・状態など、**今後 variant が増えることが確実な**直和型を扱う
- 増やしたときの対応漏れを、レビューではなくビルドで潰したい
- `-Wall -Wextra -Werror` を運用できる（**これが前提条件。外すと保証が成立しない**）

典型例は、通信コマンドのディスパッチ、機器の状態遷移、パーサのトークン種別といった
組み込み・通信系の処理である。

## 採用すべきでない状況

| 状況 | 使うべきもの | 理由 |
|---|---|---|
| C++ が使える | `std::variant` | 標準にある。移植コストを払う理由がない（[ADR-0007](./adr/0007-msvc-out-of-scope.md)） |
| 所有権・ドロップ順序まで保証したい | Rust の `enum` | 呼び出し忘れ自体を防ぐ仕組みは C には作れない（[C-03](./constraints.md#c-03)） |
| 再帰的な直和型（連結リスト・構文木）を所有権込みで扱いたい | Rust、既存の侵入型データ構造ライブラリ | 値としての自己参照は言語上書けない。ポインタ経由は書けるが所有権は管理しない（[C-04](./constraints.md#c-04)） |
| 一部の呼び出し箇所で「残り全部」をまとめて扱いたい | [datatype99](https://github.com/Hirrolot/datatype99) | 本ライブラリはワイルドカードを持たない（下記「既存の選択肢との比較」） |
| MSVC / TCC でもビルドする | [datatype99](https://github.com/Hirrolot/datatype99) | 本ライブラリは GCC / Clang のみ（[ADR-0007](./adr/0007-msvc-out-of-scope.md)） |
| MSVC でビルドする | `std::variant` | 対応しない（[ADR-0007](./adr/0007-msvc-out-of-scope.md)） |
| variant 数が固定で今後増えない | 素のタグ付き共用体 | 追加漏れが起きないなら、このライブラリの主目的が発生しない |
| `-Werror` を運用できない | — | 順序取り違えの検出（[G-002](./GUARANTEES.md#g-002)）が警告止まりになり保証が崩れる |

最後の2行が重要で、**このライブラリは「variant が増える」ことと「`-Werror` を運用する」ことの
両方が成り立つときにだけ価値がある**。どちらか一方でも欠けるなら、X-Macro を導入する
複雑さ（[C-12](./constraints.md#c-12) のデバッグ容易性のトレードオフ）に見合わない。

## 非目標

意図的に対象外としたもの。「まだ実装していない」ではなく「実装しない」と決めたものを挙げる。

- **デストラクタの自動呼び出し。** `DEFINE_SUM_DESTROY` は「呼べば網羅的に処理される」ところまでを
  保証する。呼ぶこと自体の強制は C では作れない（[C-03](./constraints.md#c-03)）
- **再帰的直和型**（[C-04](./constraints.md#c-04)）
- **カプセル化。** `union` メンバは常に公開であり、ゲッターを迂回できる（[C-05](./constraints.md#c-05)）
- **所有権の管理。** ポインタ経由の自己参照は書けるが、確保・解放の責任は利用者が持つ（[ADR-0017](./adr/0017-self-reference-by-pointer.md)）
- **payload 型が変わる関数合成（Railway Oriented Programming）。** `map` / `and_then` は
  変換先が別の具体型になるため C では汎用化できない（[C-13](./constraints.md#c-13)）
- **MSVC 対応**（[ADR-0007](./adr/0007-msvc-out-of-scope.md)）
- **ロック粒度の決定。** ロックを差し込む口（フック / `_LOCKED` 版）を提供するところまでが責務で、
  どの粒度でロックするかはアプリケーション側が決める（[ADR-0003](./adr/0003-lock-hook-injection.md), [ADR-0014](./adr/0014-dispatch-ret-and-locked.md)）

## 既存の選択肢との比較

2026-09 に GitHub の topic `tagged-unions` と Web 検索で調べた範囲では、C のマクロだけで直和型を提供し
網羅性も検査するものは [datatype99](https://github.com/Hirrolot/datatype99)（1.5k★、v1.6.5 / 2025-03）だけだった
（[sumtype.h](https://github.com/grego/sumtype.h) と [c-match](https://github.com/elias-michaias/c-match) は網羅性を検査しない）。
項目はいずれも実測または各 README の記載による。

| 項目 | 本ライブラリ | datatype99 |
|---|---|---|
| 網羅性の検査 | 引数の個数（`too few arguments`） | `-Wswitch`（`-Wall -Werror` でエラー。フラグなしでは黙って通る） |
| ワイルドカード | なし（全呼び出し箇所で全腕が必須） | `otherwise`（書くとその箇所の検出が無効） |
| 腕の書き方 | 関数ポインタを位置引数で渡す | `of(Variant, x...)` の名前付きインライン文 |
| 順序取り違え | 型が異なれば検出、同型なら不可（[C-01](./constraints.md#c-01)） | 問題として存在しない |
| 腕からローカル変数を使う | 不可（ctx 経由。[ADR-0014](./adr/0014-dispatch-ret-and-locked.md)） | 可 |
| 1 variant の要素 | 1型（[C-06](./constraints.md#c-06)） | 複数可 |
| 自己参照 | ポインタ経由で可 | ポインタ経由で可 |
| variant 名のスコープ | 型ごと（`NAME_TAG_<tag>`） | 全体で1つ（2つの型に `Ok` を定義すると衝突） |
| const 正しさ | `MATCH_CONST` / `_const` ゲッター（[G-011](./GUARANTEES.md#g-011)） | `match` 内で const を外す（README に「const の書き換えは安全でない」と明記） |
| 不正 tag・未初期化値 | フック後に trap（[G-020](./GUARANTEES.md#g-020)） | switch を抜けて利用側コードへ |
| 依存 | なし | Metalang99 |
| 処理系 | C99〜、GCC / Clang | C99〜、GCC / Clang / MSVC / TCC、C++11〜 |
| コンパイル時間（最小ファイル1件の実測） | 0.03 s | 0.22 s |

**本ライブラリを選ぶ理由になるのは次の場合に限る:** variant の追加時に全呼び出し箇所の更新を
ワイルドカードの逃げ道なしに強制したい、依存を増やせない、variant 名を型ごとに閉じたい、const 正しさが要る。
ワイルドカードやローカル変数を使える書き味を優先するなら datatype99 のほうが適する。

## 成功条件

このライブラリが目的を果たしているかは、次の2点で判断する。

1. **型リストに1行足すだけで variant を増やせ、対応漏れがあるすべての呼び出し箇所が
   ビルドエラーになる。** → [G-001](./GUARANTEES.md#g-001)
2. **正しく書かれたコードは、特別なビルドフラグなしにそのまま通る。** → [G-004](./GUARANTEES.md#g-004)

どちらも [`GUARANTEES.md`](./GUARANTEES.md) の一覧として機械検査されており、
CI が gcc / clang × c99 / c11 / c17 × default / sanitize / strict の18通りで毎回確かめている（[scripts/check.sh](../scripts/check.sh)）。
