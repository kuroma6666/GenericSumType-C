# ADR-0015: 失敗を返せるディープコピー `SUM_DEFINE_COPY_CHECKED` を追加する

- **状態:** 採用（[ADR-0004](./0004-destroy-copy-separate-macros.md) を拡張）
- **関連:** [G-026](../GUARANTEES.md#g-026), [C-03](../constraints.md#c-03)

## 文脈

`SUM_DEFINE_COPY` のコピー関数の型は `TYPE (*)(const TYPE*)` で、失敗を伝える経路が無い。
`strdup` が `NULL` を返した場合、利用者は「NULL を持つ正常な値」を受け取ることになり、失敗を検出できない。

## 決定

`SUM_DEFINE_COPY_CHECKED(NAME, VARIANTS, FN)` を追加する。

```c
int FN(NAME *dst, const NAME *src, int (*copy_<tag>)(Type *dst, const Type *src), ...);
```

- コピー関数は、0 で成功、0 以外で失敗を返す。FN はその値をそのまま返す。
- **失敗時は `*dst` を変更しない。** 一時変数でコピーを組み立て、成功したときだけ `*dst` に代入する。
  `dst == src` でも正しく動く。
- 成功時は `*dst` を上書きする。`*dst` が所有していた資源は解放しない（ライブラリは所有の有無を知らない）。
- 部分的に確保した資源の後始末は、コピー関数の責務とする（ライブラリは payload の内部構造を知らない）。
- `src` が不正 tag・空状態の場合は、値を返すほかのディスパッチャと同じく、フックを呼んで `SUM_TRAP()` で停止する（[ADR-0011](./0011-default-label-and-trap.md)）。
  エラーコードとして返すと、コピー関数が返す値と区別できないため。

## 検討した代替案

| 案 | 不採用の理由 |
|---|---|
| 既存の `SUM_DEFINE_COPY` のシグネチャを変える | 既存のコピー関数がすべて壊れる。失敗しないコピー（値型のみ）には現行の形が簡潔 |
| 失敗時に `*dst` を空状態にする | 呼び出し側が持っていた値を黙って失わせる。「失敗したら何も変えない」ほうが呼び出し側の推論が単純 |

## 帰結

- コピー関数の引数の順序は `(dst, src)` で、`memcpy` と同じ。variant ごとに型が固有なので、取り違えは `-Wincompatible-pointer-types` で落ちる（[G-026](../GUARANTEES.md#g-026)）。
- 失敗しない variant 用の定型ヘルパは用意していない（`*dst = *src; return 0;` の1行で済むため）。
