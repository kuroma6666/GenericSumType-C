# ADR-0014: 戻り値付き DISPATCH とロック明示版 DISPATCH を追加する

- **状態:** 採用（[ADR-0002](./0002-separate-match-and-dispatch.md) と [ADR-0003](./0003-lock-hook-injection.md) を拡張）
- **関連:** [G-024](../GUARANTEES.md#g-024), [G-025](../GUARANTEES.md#g-025), [C-08](../constraints.md#c-08), [C-09](../constraints.md#c-09)

## 文脈

**戻り値:** MATCH は ctx を取れず、DISPATCH は戻り値が `void` に固定されている。「装置の状態を書き換えてエラーコードを返す」という
コマンド処理の典型形が書けなかった。v1 の `examples/recipe_command.h` は、query の結果を `ctx->last_query_result` に書き戻して回避していた。
その結果、ctx が「共有状態」と「関数の戻り値」の2つの役割を負っていた。

**ロック:** `SUM_CTX_LOCK` / `SUM_CTX_UNLOCK` は、翻訳単位全体で1つのマクロとして定義される。そのため `(ctx)->mutex` を持たない Ctx 型を
同じ翻訳単位で DISPATCH すると、コンパイルが通らない。C-09 は「ロックの要否で翻訳単位を分ける」ことを運用ルールにしていた。
しかしフックは `SUM_DEFINE_DISPATCH` を展開した時点で評価されるので、DEFINE の呼び出しの間で `#undef` / 再定義すれば同じ翻訳単位でも切り替えられる。
つまり翻訳単位の分割は必須ではなかった（この訂正は [VERIFICATION_LOG.md](../../VERIFICATION_LOG.md) に記録）。
加えて、ロックの書き忘れが定義箇所から見えなかった。

## 決定

次の3つを追加する。既存の `SUM_DEFINE_DISPATCH` は変更しない。

| マクロ | ハンドラ | 戻り値 | ロック |
|---|---|---|---|
| `SUM_DEFINE_DISPATCH_RET(NAME, V, FN, CTX, RET)` | `RET (*)(Type*, CTX*)` | `RET` | `SUM_CTX_LOCK` / `SUM_CTX_UNLOCK` フック |
| `SUM_DEFINE_DISPATCH_LOCKED(NAME, V, FN, CTX, LOCK, UNLOCK)` | `void (*)(Type*, CTX*)` | `void` | 引数で渡す |
| `SUM_DEFINE_DISPATCH_RET_LOCKED(NAME, V, FN, CTX, RET, LOCK, UNLOCK)` | `RET (*)(Type*, CTX*)` | `RET` | 引数で渡す |

- `LOCK` / `UNLOCK` には、ctx を1引数に取る関数か関数形式マクロを渡す。ロックが不要なら `SUM_NO_LOCK` を渡し、
  「ロックしない」ことを定義箇所で明示する。
- DISPATCH_RET は、ハンドラの戻り値をローカル変数に受けてからアンロックし、その値を返す。
  v1 のロック解放漏れバグ（早期 return）と同じ誤りを避けるため。
- 内部では、生成器を void 版と戻り値版の2つだけ持つ。公開マクロ4つはその薄いラッパーにする。

## 検討した代替案

| 案 | 不採用の理由 |
|---|---|
| 既存の DISPATCH に `RET` 引数を足して統合する | 既存呼び出しがすべて壊れる。また `RET=void` のとき、C-08（void 関数で値を return できない）と同じ問題が出る |
| MATCH に ctx 引数を足す | ADR-0002 が分けた契約（純粋な変換か、副作用か）を崩す |
| グローバルフックを廃止してロックを必須引数にする | 既存呼び出しがすべて壊れる。ロック不要な用途が多数派なので、既定 no-op のフックも残す価値がある |

## 帰結

- 生成マクロの種類が増えた（DISPATCH 系が4つ）。どれを選ぶかは「戻り値が要るか」「ロックを型ごとに決めたいか」の2軸で決まる（[api_reference.md](../../api_reference.md) 早見表）。
- 新しいコードでロックが必要な場合は、`_LOCKED` 版を推奨する。ロックの書き忘れが定義箇所から見え、同じ翻訳単位に異なるロック方針を置ける（[G-025](../GUARANTEES.md#g-025)）。
- `RET` に `void` は渡せない（C-08 と同じ）。
- `examples/recipe_command.h` を DISPATCH_RET に書き換え、`last_query_result` を ctx から取り除いた。
