# ADR-0005: Either / Result を専用ジェネレータではなく薄いヘルパで実現する

- **状態:** 採用
- **関連:** [G-013](../GUARANTEES.md#g-013)〜[G-016](../GUARANTEES.md#g-016), [C-13](../constraints.md#c-13)

## 文脈

`Either<L, R>` と `Result<T, E>` は variant が2つに固定された直和型である。
`DEFINE_EITHER(NAME, L, R)` のような専用ジェネレータを1マクロで用意すれば、利用側は1行で書ける。

## 決定

専用ジェネレータは作らず、`DEFINE_SUM_TYPE` をそのまま使う。Either / Result 固有なのは
「`left`/`right`、`ok`/`err` という tag 規約」と「述語」だけに絞り、
`DEFINE_EITHER_HELPERS` / `DEFINE_RESULT_HELPERS` として提供する。

## 根拠

専用ジェネレータならエルゴノミクスは良いが、enum・struct・コンストラクタ・ゲッターの生成ロジックを
`DEFINE_SUM_TYPE` と**別に持つ**ことになる。コアの変更（`SUM_MAYBE_UNUSED` の付与方針変更、
east const 化のような修正）を二重に追従する保守コストが生じる。

構築は `NAME_new_left` / `NAME_new_ok`、取り出しは既存のゲッター、fold は
`DEFINE_SUM_MATCH_CONST` で賄えるため、新設が必要なのは述語だけだった。

**この原則は Either と Result の間にも適用する。** 両者は tag 名以外に差がないため、
述語の生成ロジックを2つ持つのは同じ誤りになる。`SUM_DEFINE_PREDICATE(NAME, TAG)` を
1つ用意し、両ヘルパはその2回呼び出しにした。副次的に `pending`/`done` のような
別の2択にもそのまま使える。

## 帰結

- tag 規約に依存するため、`left`/`right` 以外の tag 名に `DEFINE_EITHER_HELPERS` を使うと
  `NAME_left` / `NAME_right` という enum 定数が無くコンパイルエラーになる。
  **規約違反がそのまま検出される**（[G-013](../GUARANTEES.md#g-013), [G-015](../GUARANTEES.md#g-015)）
- L と R（T と E）が同一型だと fold ハンドラの左右取り違え検出が効かない
  （[C-01](../constraints.md#c-01) がそのまま該当）
- `map` / `and_then` のような **payload 型が変わる**関数合成は提供できない
  （[C-13](../constraints.md#c-13)）
