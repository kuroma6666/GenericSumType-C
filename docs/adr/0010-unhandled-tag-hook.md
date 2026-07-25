# ADR-0010: 不正 tag の検知をフックとして提供する

- **状態:** 採用
- **関連:** [G-019](../GUARANTEES.md#g-019), [C-05](../constraints.md#c-05)

## 文脈

`self->tag` が不正な値（マクロ経由以外で破壊された等）でディスパッチされたときの挙動が、
マクロごとにばらばらで、かつどの文書にも記載されていなかった。

| マクロ | 従来の挙動 |
|---|---|
| MATCH / MATCH_CONST / COPY | `SUM_UNREACHABLE()` に到達（未定義動作、または `abort()`） |
| DISPATCH | `SUM_CTX_UNLOCK` に到達して正常復帰（何も起きない） |
| DESTROY | **何もせず返る**（資源が解放されないまま黙って通る） |

DESTROY だけ検知の手段が無いのは意図した設計ではなく、単純な考慮漏れだった。
`union` メンバは常に公開でありゲッターを迂回できる（[C-05](../constraints.md#c-05)）以上、
tag の破壊は「起こり得ない」とは言い切れない。

## 決定

`SUM_UNHANDLED_TAG(self)` フックを追加し、**全5ディスパッチャ**がどの case にも一致しなかった
場合にこれを呼ぶ。デフォルトは no-op で、開発時に assert 等を差し込める。

```c
#define SUM_UNHANDLED_TAG(self) assert(!"generic_sum_type: unhandled tag")
#include "generic_sum_type.h"
```

## 根拠

**なぜ DESTROY に `SUM_UNREACHABLE()` を足さないのか。** `__builtin_unreachable()` は未定義動作であり、
コンパイラは「不正 tag は起こり得ない」と仮定して `switch` 周辺を最適化で消す。値を返す関数
（MATCH / MATCH_CONST / COPY）は返す値を用意できないので他に手がないが、**解放パスで UB を踏ませるのは危険**である。
かといって従来の「黙って何もせず返る」は検出不能。フックなら開発時 assert・本番 no-op を選べる。

**なぜ `default:` ラベルを使わないのか。** `switch` に `default:` を置けば同じことができるが、
それをすると [ADR-0001](./0001-x-macro-code-generation.md) が意図した `-Wswitch`（enum 定数を手で増やした場合の
網羅性警告）が無効化されてしまう。したがって:

- **DESTROY** — 各 case は `return;` で抜けるため、`switch` の後ろに到達＝不正 tag。後置するだけでよい
- **DISPATCH** — 各 case は `break;` で抜けて `SUM_CTX_UNLOCK` に進むため、後置では区別できない。
  ローカルフラグ `sum_handled_` を使う。フックが no-op のときフラグは最適化で完全に消えることを
  `-O2` の生成コードで確認済み
- **MATCH / MATCH_CONST / COPY** — フックを呼んでから `SUM_UNREACHABLE()` に進む

## 帰結

- **戻り値の有無に由来する非対称が残る。** void を返す2つ（DISPATCH / DESTROY）はフックを呼んで
  正常復帰し、値を返す3つは復帰できないので UB に進む。これは言語の制約から導かれる帰結であり、
  意図的なものとして [`GUARANTEES.md`](../GUARANTEES.md) に明記する
- 既存の利用側コードへの影響はない（デフォルト no-op、シグネチャ変更なし）
- 不正 tag でハンドラが呼ばれないことは [G-019](../GUARANTEES.md#g-019) で機械検査している
