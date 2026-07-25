# ADR-0009: const 対応を破壊的変更ではなく追加で行う

- **状態:** 採用
- **関連:** [G-011](../GUARANTEES.md#g-011), [G-012](../GUARANTEES.md#g-012)

## 文脈

`DEFINE_SUM_MATCH` と `NAME_get_<tag>()` はいずれも非 const の `NAME *` を取り、
ハンドラも可変の `TYPE *` を受け取っていた。このため面積計算・print・ゲッターのような
**意味的には read-only な操作**でも、呼び出し側が `const NAME *` を保持していると
const を捨てるしかなかった（`discards 'const' qualifier` で落ちる）。

「payload だけを受け取る純粋な変換関数を想定」という MATCH の設計意図と、実際の型シグネチャが
可変アクセスを要求している点が食い違っていた。

## 決定

既存の可変版をそのまま温存し、read-only 用途に const 版を**追加**する。

- `DEFINE_SUM_MATCH_CONST(NAME, VARIANTS, MATCH_FN, RET_TYPE)`
- `NAME_get_<tag>_const(const NAME *self)`

## 根拠

MATCH 全体を `const NAME *` + ハンドラ `const TYPE *` に一斉変更する案は型として最も正しいが、
**既存の全ハンドラの署名改修を強制する破壊的変更**になる。利用側コードの回帰リスクに見合わないため見送った。

DISPATCH / DESTROY は副作用（payload や ctx の書き換え）を持つことが前提であり、
可変シグネチャのままで正しい。const 化の対象は read-only を意図する MATCH とゲッターに限る。

## east const を使う理由

戻り型に west const（`const TYPE*`）ではなく east const（`TYPE const*`）を使っている。
`TYPE` がポインタ型（例: `const char*`）のとき、west const だと const が指す先に付いてしまい、
`&self->as.TAG`（`const NAME*` 経由なので `TYPE const*`）と型不一致になるため。

## 帰結

- API 表面積が増え、「可変版と const 版のどちらを使うか」の選択ルールを周知する必要がある。
  ルールは明快で、**payload や共有状態を書き換えるなら可変版、書き換えないなら const 版**
- const 版のハンドラが payload を書き換えようとするとコンパイルエラーになる
  （[G-011](../GUARANTEES.md#g-011)）
