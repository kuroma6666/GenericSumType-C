# ADR-0012: 公開名を `SUM_` 接頭辞に統一し、tag 定数を `NAME_TAG_<tag>` にする

- **状態:** 採用
- **関連:** [G-028](../GUARANTEES.md#g-028), [G-029](../GUARANTEES.md#g-029), [G-030](../GUARANTEES.md#g-030), [G-031](../GUARANTEES.md#g-031), [C-14](../constraints.md#c-14)

## 文脈

v1 には命名の問題が3つあった。

1. **tag 定数と生成関数が同じ接頭辞を使っている。** enum 定数 `NAME_<tag>` と、生成関数 `NAME_new_*`、`NAME_get_*`、`NAME_is_*`、
   型名 `NAME_tag_t` が同じ名前空間にあった。そのため tag を `get_a` や `tag_t` と名付けると衝突した
   （`'T_get_a' redeclared as different kind of symbol`）。エラーにはなるが、原因を推測しにくい。
2. **公開マクロの語順が揃っていない。** `DEFINE_SUM_*` と `SUM_DEFINE_*` が混在していた。さらに `DEFINE_RESULT_HELPERS` には
   ライブラリ固有の接頭辞が無く、他のコードと衝突しやすかった。
3. **公開マクロと内部マクロの見た目が同じ。** `SUM_CTOR` や `SUM_MATCH_PARAM` は内部用だが、公開 API と区別できなかった。
   examples も内部用の `SUM_MAYBE_UNUSED` を公開 API のように使っていた。

## 決定

| 対象 | v1 | v2 |
|---|---|---|
| 生成マクロ | `DEFINE_SUM_TYPE` など、`DEFINE_EITHER_HELPERS` / `DEFINE_RESULT_HELPERS` | `SUM_DEFINE_TYPE` など。**公開名はすべて `SUM_` で始める** |
| tag 定数 | `NAME_<tag>` | `NAME_TAG_<tag>`。空状態は `NAME_TAG__EMPTY`（[ADR-0013](./0013-empty-tag-zero.md)） |
| 内部マクロ | `SUM_CTOR`, `SUM_MAYBE_UNUSED` など | `SUM__CTOR`, `SUM__MAYBE_UNUSED` など（`SUM__` は内部用） |

生成関数の接頭辞（`new_` / `get_` / `is_`）は小文字、tag 定数の接頭辞 `TAG_` は大文字なので、
両者が同名になることはない（[G-028](../GUARANTEES.md#g-028)）。

**移行手段**（[CHANGELOG.md](../../CHANGELOG.md)）:

- v1 のマクロ名は非推奨の別名として残す。`SUM_NO_LEGACY_NAMES` を定義すると提供されなくなる（[G-031](../GUARANTEES.md#g-031)）。
- `SUM_LEGACY_TAG_NAMES` を定義すると、v1 の tag 定数名 `NAME_<tag>` も同じ値の enum 定数として生成する（[G-030](../GUARANTEES.md#g-030)）。
  有効にすると v1 と同じ衝突が起こり得るので、移行期間だけ使うものとする。

## 検討した代替案

| 案 | 不採用の理由 |
|---|---|
| tag 定数名は変えず、予約 tag 名を文書で禁止するだけにする | 衝突の原因が残る。移行手段（`SUM_LEGACY_TAG_NAMES`）があるので、破壊的変更のコストは吸収できる |
| 旧名を使ったときに非推奨の警告を出す（`_Pragma("GCC warning ...")`） | このライブラリは `-Werror` を前提にしている（[requirements.md](../requirements.md)）。警告を出すと、旧名が即座にビルドエラーになる |
| VARIANTS の定義形式を `X(NAME, EXTRA, tag, Type)` から `X(C, tag, Type)`（C は括弧で束ねた文脈）に縮める | 利用者のすべての型リスト定義が壊れるわりに、得られるのは見た目の改善だけ。現状を維持する |

## 帰結

- 残る予約名は2種類: 末尾が `_const` で、もう一方の tag の const ゲッター名と一致する tag 名（[G-029](../GUARANTEES.md#g-029)）と、
  `_` で始まる tag 名（`NAME_TAG__EMPTY` と衝突し得る）。いずれも [C-14](../constraints.md#c-14) に記載した。
- `SUM_MAYBE_UNUSED` を公開 API として提供するのをやめた。examples のヘッダ内関数は `static inline` のままで警告が出ない
  （ヘッダ内の未使用 `static inline` は gcc / clang とも警告しない）。
