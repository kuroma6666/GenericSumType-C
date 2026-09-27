# ADR-0017: struct にタグ名を付け、payload から自身の型をポインタで指せるようにする

- **状態:** 採用（[ADR-0006](./0006-no-recursive-sum-types.md) の決定は維持。その帰結として書かれた内容を実装で満たす）
- **関連:** [G-032](../GUARANTEES.md#g-032), [C-04](../constraints.md#c-04)

## 文脈

ADR-0006 は、再帰的な直和型を「対応しない」と決めた。その根拠は、値としての自己埋め込みが言語上成立しないことと、
所有権の管理がライブラリの範囲外であることの2点。ADR-0006 はその帰結として「ポインタ経由で手動管理すること自体は妨げない」とも書いていた。

しかし v1 の `SUM_DEFINE_TYPE` は、無名の struct（`typedef struct { ... } NAME;`）を生成していた。そのため payload から
`struct NAME *` を書けず、ポインタ経由の自己参照もできなかった。datatype99 など、同じ種類のライブラリはポインタ経由の再帰を許している
（[先行事例の比較](../requirements.md#既存の選択肢との比較)）。

## 決定

生成する struct にタグ名を付ける（`typedef struct NAME { ... } NAME;`）。payload 型の中では `struct NAME *` と書いて自身を指せる。

```c
typedef struct { struct Expr *lhs, *rhs; } Add;
#define EXPR(X, NAME, EXTRA) X(NAME, EXTRA, num, Num) X(NAME, EXTRA, add, Add)
SUM_DEFINE_TYPE(Expr, EXPR)
```

## 帰結

- 所有権は管理しない（ADR-0006 のとおり）。確保と解放は利用者が DESTROY のハンドラなどで書く。
- C99 では typedef の再定義が許されないので、payload 側で `typedef struct Expr Expr;` と前方宣言することはできない。`struct Expr *` と書く。
- タグ名 `struct NAME` が新たに使用済みになる。同じ名前の struct を利用者が別に定義していると衝突する（v1 から移行する際にのみ問題になり得る）。
