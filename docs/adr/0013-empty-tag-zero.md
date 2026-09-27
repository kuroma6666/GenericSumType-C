# ADR-0013: tag 値 0 を空状態として予約し、DESTROY 後は空状態に戻す

- **状態:** 採用
- **関連:** [G-020](../GUARANTEES.md#g-020), [G-023](../GUARANTEES.md#g-023), [C-03](../constraints.md#c-03), [C-10](../constraints.md#c-10)

## 文脈

v1 では最初の variant の tag 値が 0 だった。そのため、ゼロ初期化した値（`T t = {0};`、静的記憶域、`memset`、bss）は
「先頭 variant で payload がゼロ」という正当な値に見えた。初期化し忘れを、実行時にも検出できなかった。

同じ理由で、`SUM_DEFINE_NEW_GENERIC` のプレースホルダ用関数が返す `{0}` も、正当な値になっていた。

また DESTROY は解放後の値に手を付けなかったので、同じ値に2回 DESTROY を呼ぶと二重解放になった。

## 決定

- enum の先頭に `NAME_TAG__EMPTY = 0` を置く。variant の tag 値は 1 から始まる。
- 空状態の扱いを次のとおりにする。

| 操作 | 空状態での挙動 |
|---|---|
| ゲッター / 述語 | `NULL` / 偽（どの variant でもない） |
| DESTROY | 何もせず返る（フックも呼ばない。`free(NULL)` と同じ扱い） |
| MATCH / MATCH_CONST / COPY / COPY_CHECKED / DISPATCH_RET | 不正 tag と同じく、フックを呼んで `SUM_TRAP()`（[ADR-0011](./0011-default-label-and-trap.md)） |
| DISPATCH / DISPATCH_LOCKED | 不正 tag と同じく、フックを呼んで復帰 |

- DESTROY は、ハンドラを呼んだ後に `self->tag` を空状態に戻す。
- 空状態の値を作る `SUM_EMPTY(NAME)` を提供する。
  `{ NAME_TAG__EMPTY }` という書き方は clang の `-Wmissing-field-initializers` に引っかかるため、指示付き初期化子による複合リテラルにした。

## 根拠

- 初期化し忘れと、解放後の使用（use-after-destroy）を、MATCH 系の trap として表面化させられる。
- 同じ値への2回目の DESTROY が no-op になる。CBMC で検証した結果、v1 のヘッダでは二重解放として検出された（`double_destroy_case`）ハーネスが、
  v2 では `VERIFICATION SUCCESSFUL` になる（[verify/README.md](../../verify/README.md)）。
- DESTROY で空状態を許すのは、「確保に失敗した経路でもまとめて後始末する」という書き方（`T t = SUM_EMPTY(T); ... goto cleanup;`）を安全にするため。

## 帰結

- **tag の数値が v1 から 1 ずれる。** tag の数値を保存したり通信で送ったりしている利用者は、変換が必要になる（C-10 は tag 値を安定 ABI として約束していない）。
- 構造体コピーで作った別の変数には効かない。コピーした後にそれぞれを DESTROY すれば、依然として二重解放になる（[C-03](../constraints.md#c-03)、CBMC の `double_free_case`）。
- `sizeof(NAME)` は変わらない（enum に定数が1つ増えるだけ）。
