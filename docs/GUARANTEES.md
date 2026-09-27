# コンパイル時保証の一覧

> **このファイルは `spec/emit.sh` が `spec/manifest.tsv` とケース断片から生成する。手で編集しないこと。**
> 内容を変えたい場合は `spec/manifest.tsv` と `spec/cases/` を編集して `spec/emit.sh --write` を実行する。

このライブラリが約束しているのは、主に「**どんなコードがコンパイルを通り、どんなコードが通らないか**」である。
この一覧がその仕様の中核であり、判定器はコンパイラそのものになる。実行時の約束のうち保証として固定したいもの
（不正 tag での挙動など）は run-pass として同じ一覧に載せている。

各項目は `spec/run.sh` が CI（gcc / clang × c99 / c11 / c17、`-O0` と `-O2` + サニタイザ）で毎回検証している。
コンパイル失敗を期待するケースは、原因の箇所だけを直した対照（`-DSPEC_FIXED`）が通ることも確かめる。

- **保証** — ライブラリが約束する挙動。破れたらバグとして扱う。
- **既知の限界** — 約束しない（できない）ことを固定したもの。将来検出できるようになってもよいが、
  そのときはこの表と根拠文書をあわせて更新する必要がある。

ビルドフラグは `-std=$STD -pedantic -Wall -Wextra -Werror`。
**`-Werror` を外すと順序取り違えの検出（G-002）が警告止まりになり保証が成立しない。**

## 保証

| ID | 期待結果 | 対象 | 内容 | 根拠 |
|---|---|---|---|---|
| [G-001](#g-001) | コンパイル失敗 | C99 以降すべて | ハンドラを渡し忘れる | [ADR-0001](adr/0001-x-macro-code-generation.md) |
| [G-002](#g-002) | コンパイル失敗 | C99 以降すべて | 型が異なる variant 間でハンドラの順序を取り違える | [ADR-0001](adr/0001-x-macro-code-generation.md) |
| [G-004](#g-004) | コンパイル成功 | C99 以降すべて | 網羅・順序ともに正しい呼び出しが通る | [ADR-0001](adr/0001-x-macro-code-generation.md) |
| [G-006](#g-006) | コンパイル成功 | C99 以降すべて | typedef で単一トークン化した関数ポインタ型は渡せる | [C-02](constraints.md#c-02) |
| [G-007](#g-007) | ビルド・実行成功 | C99 以降すべて | 共通ヘッダを複数の翻訳単位から include して共有する | [C-11](constraints.md#c-11) |
| [G-008](#g-008) | コンパイル失敗 | C11 以降のみ | SUM_NEW にどの variant とも一致しない型を渡す | [ADR-0008](adr/0008-generic-constructor-selection.md) |
| [G-009](#g-009) | コンパイル失敗 | C11 以降のみ | ペイロード型の重複を SUM_DEFINE_NEW_GENERIC の時点で検出する | [ADR-0008](adr/0008-generic-constructor-selection.md) |
| [G-010](#g-010) | コンパイル成功 | C11 以降のみ | SUM_NEW が値の型からコンストラクタを選ぶ | [ADR-0008](adr/0008-generic-constructor-selection.md) |
| [G-011](#g-011) | コンパイル失敗 | C99 以降すべて | MATCH_CONST のハンドラがペイロードを書き換える | [ADR-0009](adr/0009-non-breaking-const-support.md) |
| [G-012](#g-012) | コンパイル成功 | C99 以降すべて | const NAME* から const 版の match / getter を呼ぶ | [ADR-0009](adr/0009-non-breaking-const-support.md) |
| [G-013](#g-013) | コンパイル失敗 | C99 以降すべて | Either ヘルパを left/right 以外の tag に使う | [ADR-0005](adr/0005-either-result-thin-helpers.md) |
| [G-014](#g-014) | コンパイル成功 | C99 以降すべて | Either ヘルパを left/right の tag に使う | [ADR-0005](adr/0005-either-result-thin-helpers.md) |
| [G-015](#g-015) | コンパイル失敗 | C99 以降すべて | Result ヘルパを ok/err 以外の tag に使う | [ADR-0005](adr/0005-either-result-thin-helpers.md) |
| [G-016](#g-016) | コンパイル成功 | C99 以降すべて | Result ヘルパを ok/err の tag に使う | [ADR-0005](adr/0005-either-result-thin-helpers.md) |
| [G-017](#g-017) | コンパイル失敗 | C11 以降のみ | typedef 別名で同一型になった 2 variant を検出する | [C-01](constraints.md#c-01) |
| [G-019](#g-019) | ビルド・実行成功 | C99 以降すべて | 不正 tag で SUM_UNHANDLED_TAG が発火しハンドラは呼ばれない | [ADR-0010](adr/0010-unhandled-tag-hook.md) |
| [G-020](#g-020) | ビルド・実行成功 | C99 以降すべて | 値を返すディスパッチャは不正 tag・空状態でフック後 SUM_TRAP に進みハンドラを呼ばない | [ADR-0011](adr/0011-default-label-and-trap.md) |
| [G-021](#g-021) | コンパイル失敗 | C99 以降すべて | ハンドラに定数 NULL を渡す | [ADR-0016](adr/0016-nonnull-handlers.md) |
| [G-023](#g-023) | ビルド・実行成功 | C99 以降すべて | ゼロ初期化値は空状態で、DESTROY 後も空状態に戻り2回目は no-op | [ADR-0013](adr/0013-empty-tag-zero.md) |
| [G-024](#g-024) | コンパイル失敗 | C99 以降すべて | DISPATCH_RET のハンドラを渡し忘れる | [ADR-0014](adr/0014-dispatch-ret-and-locked.md) |
| [G-025](#g-025) | ビルド・実行成功 | C99 以降すべて | DISPATCH_LOCKED で同一 TU 内に型ごとのロック方針を持てる | [ADR-0014](adr/0014-dispatch-ret-and-locked.md) |
| [G-026](#g-026) | コンパイル失敗 | C99 以降すべて | COPY_CHECKED の型が異なるコピー関数を入れ替える | [ADR-0015](adr/0015-copy-checked.md) |
| [G-027](#g-027) | コンパイル成功 | C99 以降すべて | 生成コードが -Wswitch-default / -Wswitch-enum を出さない | [ADR-0011](adr/0011-default-label-and-trap.md) |
| [G-028](#g-028) | コンパイル成功 | C99 以降すべて | v1 で衝突した tag 名（get_a, new_a, tag_t, is_a）を並べても衝突しない | [ADR-0012](adr/0012-naming-v2.md) |
| [G-030](#g-030) | ビルド・実行成功 | C99 以降すべて | v1 のマクロ名と SUM_LEGACY_TAG_NAMES で v1 のコードが通る | [ADR-0012](adr/0012-naming-v2.md) |
| [G-031](#g-031) | コンパイル失敗 | C99 以降すべて | SUM_NO_LEGACY_NAMES で v1 のマクロ名が消える | [ADR-0012](adr/0012-naming-v2.md) |
| [G-032](#g-032) | ビルド・実行成功 | C99 以降すべて | payload から struct NAME* で自身の型を指せる | [ADR-0017](adr/0017-self-reference-by-pointer.md) |

## 既知の限界

| ID | 期待結果 | 対象 | 内容 | 根拠 |
|---|---|---|---|---|
| [G-003](#g-003) | コンパイル成功 | C99 以降すべて | 型が同一の variant 間の順序取り違えは検出できない | [C-01](constraints.md#c-01) |
| [G-005](#g-005) | コンパイル失敗 | C99 以降すべて | 関数ポインタ型を typedef せずペイロードに渡す | [C-02](constraints.md#c-02) |
| [G-018](#g-018) | コンパイル成功 | C99 のみ | C99 ではペイロード型の重複を検出できない | [C-01](constraints.md#c-01) |
| [G-022](#g-022) | コンパイル成功 | C99 以降すべて | 変数経由の NULL ハンドラは検出できない | [ADR-0016](adr/0016-nonnull-handlers.md) |
| [G-029](#g-029) | コンパイル失敗 | C99 以降すべて | 末尾が _const の tag 名は const ゲッター名と衝突する | [C-14](constraints.md#c-14) |

---

## 各ケースの詳細

<a id="g-001"></a>

### G-001 — ハンドラを渡し忘れる

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル失敗 | C99 以降すべて | [ADR-0001](adr/0001-x-macro-code-generation.md) |

```c
/* ハンドラを1つ渡し忘れる。生成関数の引数個数は variant 数と常に一致するため、
 * 通常の関数プロトタイプ検査（too few arguments）で落ちる。 */
#include "generic_sum_type.h"
typedef struct { int a, b; } PA;
typedef struct { int a, b; } PB;
typedef struct { int a, b; } PC;
#define V(X, NAME, EXTRA) \
    X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB) X(NAME, EXTRA, c, PC)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_DISPATCH(T, V, T_dispatch, int)
static void ha(PA *p, int *c) { (void)p; (void)c; }
static void hb(PB *p, int *c) { (void)p; (void)c; }
static void hc(PC *p, int *c) { (void)p; (void)c; }
int main(void) {
    T t = T_new_a((PA){ 0, 0 });
    int ctx = 0;
#ifndef SPEC_FIXED
    T_dispatch(&t, &ctx, ha);           /* b, c 用のハンドラが無い */
    (void)hb; (void)hc;
#else
    T_dispatch(&t, &ctx, ha, hb, hc);
#endif
    return 0;
}
```

<sub>ケース断片: [`spec/cases/G-001.c`](../spec/cases/G-001.c)</sub>

<a id="g-002"></a>

### G-002 — 型が異なる variant 間でハンドラの順序を取り違える

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル失敗 | C99 以降すべて | [ADR-0001](adr/0001-x-macro-code-generation.md) |

```c
/* ペイロード型が異なる2 variant のハンドラを入れ替えて渡す。
 * 関数ポインタの型が variant ごとに固有なので -Wincompatible-pointer-types（-Werror でエラー）。 */
#include "generic_sum_type.h"
typedef struct { int target, dmg; } CmdAttack;
typedef struct { int target, amt; } CmdHeal;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, attack, CmdAttack) X(NAME, EXTRA, heal, CmdHeal)
SUM_DEFINE_TYPE(Cmd, V)
SUM_DEFINE_DISPATCH(Cmd, V, Cmd_dispatch, int)
static void on_attack(CmdAttack *a, int *c) { (void)a; (void)c; }
static void on_heal(CmdHeal *h, int *c) { (void)h; (void)c; }
int main(void) {
    Cmd c = Cmd_new_attack((CmdAttack){ 1, 2 });
    int ctx = 0;
#ifndef SPEC_FIXED
    Cmd_dispatch(&c, &ctx, on_heal, on_attack);   /* 順序が逆 */
#else
    Cmd_dispatch(&c, &ctx, on_attack, on_heal);
#endif
    return 0;
}
```

<sub>ケース断片: [`spec/cases/G-002.c`](../spec/cases/G-002.c)</sub>

<a id="g-003"></a>

### G-003 — 型が同一の variant 間の順序取り違えは検出できない

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 既知の限界 | コンパイル成功 | C99 以降すべて | [C-01](constraints.md#c-01) |

```c
/* 2 variant が同一のペイロード型を持つと、ハンドラの関数ポインタ型も一致するため
 * 順序を入れ替えてもコンパイラには区別できない。検出できないことの固定。 */
#include "generic_sum_type.h"
#include <stdint.h>
#define V(X, NAME, EXTRA) X(NAME, EXTRA, score, int32_t) X(NAME, EXTRA, penalty, int32_t)
SUM_DEFINE_TYPE(Cmd, V)
SUM_DEFINE_DISPATCH(Cmd, V, Cmd_dispatch, int)
static void on_score(int32_t *v, int *c) { (void)v; (void)c; }
static void on_penalty(int32_t *v, int *c) { (void)v; (void)c; }
int main(void) {
    Cmd c = Cmd_new_score(10);
    int ctx = 0;
    Cmd_dispatch(&c, &ctx, on_penalty, on_score);   /* 取り違えているが通る */
    return 0;
}
```

<sub>ケース断片: [`spec/cases/G-003.c`](../spec/cases/G-003.c)</sub>

<a id="g-004"></a>

### G-004 — 網羅・順序ともに正しい呼び出しが通る

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル成功 | C99 以降すべて | [ADR-0001](adr/0001-x-macro-code-generation.md) |

```c
/* 網羅・順序ともに正しい呼び出し。誤検出（正しいコードが落ちる）がないことの確認。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_DISPATCH(T, V, T_dispatch, int)
static void ha(PA *p, int *c) { (void)p; (void)c; }
static void hb(PB *p, int *c) { (void)p; (void)c; }
int main(void) {
    T t = T_new_a((PA){ 1 });
    int ctx = 0;
    T_dispatch(&t, &ctx, ha, hb);
    return 0;
}
```

<sub>ケース断片: [`spec/cases/G-004.c`](../spec/cases/G-004.c)</sub>

<a id="g-005"></a>

### G-005 — 関数ポインタ型を typedef せずペイロードに渡す

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 既知の限界 | コンパイル失敗 | C99 以降すべて | [C-02](constraints.md#c-02) |

```c
/* 生成コードは「TYPE 識別子;」という宣言パターンを前提にしている。関数ポインタ型は
 * 識別子を型の内側（TYPE (*識別子)(...)）に置く必要があり、このパターンに合わない。
 * マクロ引数のコンマ分割が原因ではない点に注意（括弧で保護されている）。対照は G-006 の回避策。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
#ifndef SPEC_FIXED
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, cb, void (*)(int, int))
#else
typedef void (*Callback)(int, int);
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, cb, Callback)
#endif
SUM_DEFINE_TYPE(T, V)
int main(void) { return 0; }
```

<sub>ケース断片: [`spec/cases/G-005.c`](../spec/cases/G-005.c)</sub>

<a id="g-006"></a>

### G-006 — typedef で単一トークン化した関数ポインタ型は渡せる

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル成功 | C99 以降すべて | [C-02](constraints.md#c-02) |

```c
/* typedef で型名を単一トークンにすれば、関数ポインタ型もペイロードに使える（G-005 の回避策）。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef void (*Callback)(int, int);
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, cb, Callback)
SUM_DEFINE_TYPE(T, V)
static void my_cb(int x, int y) { (void)x; (void)y; }
int main(void) {
    T t = T_new_cb(my_cb);
    Callback *pc = T_get_cb(&t);
    return pc ? 0 : 1;
}
```

<sub>ケース断片: [`spec/cases/G-006.c`](../spec/cases/G-006.c)</sub>

<a id="g-007"></a>

### G-007 — 共通ヘッダを複数の翻訳単位から include して共有する

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | ビルド・実行成功 | C99 以降すべて | [C-11](constraints.md#c-11) |

`command_type.h`:

```c
/* 型定義・生成マクロ呼び出しを1つの共通ヘッダにまとめ、複数の .c から include する
 * 推奨運用。生成関数はすべて static inline（内部リンケージ）なので、翻訳単位間で
 * 共有される外部シンボルは存在しない。 */
#ifndef COMMAND_TYPE_H
#define COMMAND_TYPE_H
#include "generic_sum_type.h"

typedef struct { int dx, dy; } CmdMove;
typedef struct { int target; }  CmdAttack;

#define CMD_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, move, CmdMove)    \
    X(NAME, EXTRA, attack, CmdAttack)

SUM_DEFINE_TYPE(Command, CMD_VARIANTS)

typedef struct { int hp; int x, y; } GameState;

SUM_DEFINE_DISPATCH(Command, CMD_VARIANTS, Command_dispatch, GameState)

#endif
```

`main.c`:

```c
#include "command_type.h"

void worker_process(Command cmd, GameState *gs);

int main(void) {
    GameState gs = { .hp = 100, .x = 0, .y = 0 };
    Command c1 = Command_new_move((CmdMove){ .dx = 3, .dy = 4 });
    Command c2 = Command_new_attack((CmdAttack){ .target = 20 });
    worker_process(c1, &gs);
    worker_process(c2, &gs);
    return (gs.hp == 80 && gs.x == 3 && gs.y == 4) ? 0 : 1;
}
```

`worker.c`:

```c
#include "command_type.h"

static void on_move(CmdMove *m, GameState *gs) { gs->x += m->dx; gs->y += m->dy; }
static void on_attack(CmdAttack *a, GameState *gs) { gs->hp -= a->target; }

void worker_process(Command cmd, GameState *gs) {
    Command_dispatch(&cmd, gs, on_move, on_attack);
}
```

<sub>ケース断片: [`spec/cases/G-007/`](../spec/cases/G-007)</sub>

<a id="g-008"></a>

### G-008 — SUM_NEW にどの variant とも一致しない型を渡す

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル失敗 | C11 以降のみ | [ADR-0008](adr/0008-generic-constructor-selection.md) |

```c
/* SUM_NEW にどの variant のペイロード型とも一致しない値を渡す。
 * _Generic に default 連想を用意していないため選択できず落ちる。 */
#include "generic_sum_type.h"
typedef struct { int v; } IntBox;
typedef struct { const char *v; } StrBox;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, i, IntBox) X(NAME, EXTRA, s, StrBox)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_NEW_GENERIC(T, V)
int main(void) {
#ifndef SPEC_FIXED
    T t = SUM_NEW(T, V, 42);             /* IntBox でも StrBox でもない生の int */
#else
    T t = SUM_NEW(T, V, ((IntBox){ 42 }));
#endif
    return t.tag == T_TAG_i ? 0 : 1;
}
```

<sub>ケース断片: [`spec/cases/G-008.c`](../spec/cases/G-008.c)</sub>

<a id="g-009"></a>

### G-009 — ペイロード型の重複を SUM_DEFINE_NEW_GENERIC の時点で検出する

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル失敗 | C11 以降のみ | [ADR-0008](adr/0008-generic-constructor-selection.md) |

```c
/* ペイロード型が重複していると _Generic の連想リストが重複する。
 * SUM_DEFINE_NEW_GENERIC が生成する probe 関数がその場で _Generic を展開するため、
 * SUM_NEW を一度も呼ばなくても定義時点で落ちる。 */
#include "generic_sum_type.h"
typedef struct { int v; } IntBox;
typedef struct { int v; } OtherBox;
#ifndef SPEC_FIXED
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, IntBox) X(NAME, EXTRA, b, IntBox)
#else
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, IntBox) X(NAME, EXTRA, b, OtherBox)
#endif
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_NEW_GENERIC(T, V)
int main(void) { return 0; }   /* SUM_NEW は呼ばない */
```

<sub>ケース断片: [`spec/cases/G-009.c`](../spec/cases/G-009.c)</sub>

<a id="g-010"></a>

### G-010 — SUM_NEW が値の型からコンストラクタを選ぶ

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル成功 | C11 以降のみ | [ADR-0008](adr/0008-generic-constructor-selection.md) |

```c
/* SUM_NEW が値の型だけからコンストラクタを選ぶ（正常系）。 */
#include "generic_sum_type.h"
typedef struct { int v; } IntBox;
typedef struct { const char *v; } StrBox;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, i, IntBox) X(NAME, EXTRA, s, StrBox)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_NEW_GENERIC(T, V)
int main(void) {
    T a = SUM_NEW(T, V, ((IntBox){ 1 }));
    T b = SUM_NEW(T, V, ((StrBox){ "x" }));
    return (a.tag == b.tag) ? 1 : 0;
}
```

<sub>ケース断片: [`spec/cases/G-010.c`](../spec/cases/G-010.c)</sub>

<a id="g-011"></a>

### G-011 — MATCH_CONST のハンドラがペイロードを書き換える

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル失敗 | C99 以降すべて | [ADR-0009](adr/0009-non-breaking-const-support.md) |

```c
/* MATCH_CONST のハンドラはペイロードを Type const* で受け取るため、書き換えられない。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_MATCH_CONST(T, V, T_area, int)
static int on_a(const PA *p) { return p->a; }
#ifndef SPEC_FIXED
static int on_b(const PB *p) { p->b = 9; return p->b; }   /* const への代入 */
#else
static int on_b(const PB *p) { return p->b; }
#endif
int main(void) { T t = T_new_a((PA){ 1 }); return T_area(&t, on_a, on_b); }
```

<sub>ケース断片: [`spec/cases/G-011.c`](../spec/cases/G-011.c)</sub>

<a id="g-012"></a>

### G-012 — const NAME* から const 版の match / getter を呼ぶ

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル成功 | C99 以降すべて | [ADR-0009](adr/0009-non-breaking-const-support.md) |

```c
/* const NAME* を保持したまま read-only な match / getter を呼べる
 * （可変版 SUM_DEFINE_MATCH は discards const で落ちる）。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_MATCH_CONST(T, V, T_area, int)
static int on_a(const PA *p) { return p->a; }
static int on_b(const PB *p) { return p->b; }
static int use(const T *t) {
    const PA *pa = T_get_a_const(t);
    (void)pa;
    return T_area(t, on_a, on_b);
}
int main(void) { T t = T_new_a((PA){ 7 }); return use(&t) == 7 ? 0 : 1; }
```

<sub>ケース断片: [`spec/cases/G-012.c`](../spec/cases/G-012.c)</sub>

<a id="g-013"></a>

### G-013 — Either ヘルパを left/right 以外の tag に使う

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル失敗 | C99 以降すべて | [ADR-0005](adr/0005-either-result-thin-helpers.md) |

```c
/* Either ヘルパは left/right という tag 規約に依存する。
 * 別の tag 名に使うと enum 定数が存在せず落ちる（規約違反がそのまま検出される）。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#ifndef SPEC_FIXED
#define V(X, NAME, EXTRA) X(NAME, EXTRA, foo, PA) X(NAME, EXTRA, bar, PB)
#else
#define V(X, NAME, EXTRA) X(NAME, EXTRA, left, PA) X(NAME, EXTRA, right, PB)
#endif
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_EITHER_HELPERS(T)   /* T_TAG_left / T_TAG_right が無い */
int main(void) { return 0; }
```

<sub>ケース断片: [`spec/cases/G-013.c`](../spec/cases/G-013.c)</sub>

<a id="g-014"></a>

### G-014 — Either ヘルパを left/right の tag に使う

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル成功 | C99 以降すべて | [ADR-0005](adr/0005-either-result-thin-helpers.md) |

```c
/* Either ヘルパを規約通り left/right の tag に使う（正常系）。 */
#include "generic_sum_type.h"
typedef struct { int code; } Err;
typedef struct { int val; }  Ok;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, left, Err) X(NAME, EXTRA, right, Ok)
SUM_DEFINE_TYPE(Result, V)
SUM_DEFINE_EITHER_HELPERS(Result)
static int use(const Result *r) { return Result_is_left(r) ? 0 : 1; }
int main(void) { Result r = Result_new_right((Ok){ .val = 1 }); return use(&r); }
```

<sub>ケース断片: [`spec/cases/G-014.c`](../spec/cases/G-014.c)</sub>

<a id="g-015"></a>

### G-015 — Result ヘルパを ok/err 以外の tag に使う

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル失敗 | C99 以降すべて | [ADR-0005](adr/0005-either-result-thin-helpers.md) |

```c
/* Result ヘルパは ok/err という tag 規約に依存する（G-013 の Result 版）。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#ifndef SPEC_FIXED
#define V(X, NAME, EXTRA) X(NAME, EXTRA, foo, PA) X(NAME, EXTRA, bar, PB)
#else
#define V(X, NAME, EXTRA) X(NAME, EXTRA, ok, PA) X(NAME, EXTRA, err, PB)
#endif
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_RESULT_HELPERS(T)   /* T_TAG_ok / T_TAG_err が無い */
int main(void) { return 0; }
```

<sub>ケース断片: [`spec/cases/G-015.c`](../spec/cases/G-015.c)</sub>

<a id="g-016"></a>

### G-016 — Result ヘルパを ok/err の tag に使う

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル成功 | C99 以降すべて | [ADR-0005](adr/0005-either-result-thin-helpers.md) |

```c
/* Result ヘルパを規約通り ok/err の tag に使う（正常系）。 */
#include "generic_sum_type.h"
typedef struct { int v; }    Ok;
typedef struct { int code; } Err;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, ok, Ok) X(NAME, EXTRA, err, Err)
SUM_DEFINE_TYPE(Res, V)
SUM_DEFINE_RESULT_HELPERS(Res)
static int use(const Res *r) { return Res_is_ok(r) ? 0 : 1; }
int main(void) { Res r = Res_new_ok((Ok){ .v = 1 }); return use(&r); }
```

<sub>ケース断片: [`spec/cases/G-016.c`](../spec/cases/G-016.c)</sub>

<a id="g-017"></a>

### G-017 — typedef 別名で同一型になった 2 variant を検出する

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル失敗 | C11 以降のみ | [C-01](constraints.md#c-01) |

```c
/* 「ペイロードに素の基本型を直接使わない」というガイドラインを守っていても、
 * typedef は別名であって別型ではないため、不変条件（variant 間でペイロード型が
 * 重複しない）は破れる。ガイドラインでは防げず、機械検査だけが捕まえる例。
 * C99 では同じコードが通ってしまう（G-018）。対照は struct でラップした形。 */
#include "generic_sum_type.h"
#include <stdint.h>
#ifndef SPEC_FIXED
typedef int32_t DeviceId;
typedef int32_t SensorId;   /* 別名だが DeviceId と同じ型 */
#else
typedef struct { int32_t v; } DeviceId;
typedef struct { int32_t v; } SensorId;
#endif
#define V(X, NAME, EXTRA) X(NAME, EXTRA, device, DeviceId) X(NAME, EXTRA, sensor, SensorId)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_NEW_GENERIC(T, V)
int main(void) { return 0; }
```

<sub>ケース断片: [`spec/cases/G-017.c`](../spec/cases/G-017.c)</sub>

<a id="g-018"></a>

### G-018 — C99 ではペイロード型の重複を検出できない

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 既知の限界 | コンパイル成功 | C99 のみ | [C-01](constraints.md#c-01) |

```c
/* G-017 と同じ設計ミスだが、C99 には _Generic が無いため SUM_DEFINE_NEW_GENERIC
 * 自体が存在せず、機械検査が働かない。C99 では設計ガイドラインとレビューに頼るしかない、
 * という std 依存の限界を固定する。 */
#include "generic_sum_type.h"
#include <stdint.h>
typedef int32_t DeviceId;
typedef int32_t SensorId;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, device, DeviceId) X(NAME, EXTRA, sensor, SensorId)
SUM_DEFINE_TYPE(T, V)
int main(void) { return 0; }
```

<sub>ケース断片: [`spec/cases/G-018.c`](../spec/cases/G-018.c)</sub>

<a id="g-019"></a>

### G-019 — 不正 tag で SUM_UNHANDLED_TAG が発火しハンドラは呼ばれない

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | ビルド・実行成功 | C99 以降すべて | [ADR-0010](adr/0010-unhandled-tag-hook.md) |

```c
/* 不正 tag（マクロ経由以外で破壊された tag）でディスパッチしたとき、
 * void を返す DISPATCH / DESTROY は SUM_UNHANDLED_TAG を呼んで正常復帰する。
 * ハンドラは一つも呼ばれない。値を返すディスパッチャは G-020。 */
#include <stdio.h>
static int sum_unhandled_fired = 0;
#define SUM_UNHANDLED_TAG(self) do { (void)(self); sum_unhandled_fired++; } while (0)
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
typedef struct { int calls; } Ctx;
SUM_DEFINE_DISPATCH(T, V, T_dispatch, Ctx)
SUM_DEFINE_DESTROY(T, V, T_destroy)
static void da(PA *p, Ctx *c) { (void)p; c->calls++; }
static void db(PB *p, Ctx *c) { (void)p; c->calls++; }
static void za(PA *p) { (void)p; }
static void zb(PB *p) { (void)p; }
int main(void) {
    T t = T_new_a((PA){ 1 });
    Ctx c = { 0 };
    T_dispatch(&t, &c, da, db);
    T_destroy(&t, za, zb);
    if (sum_unhandled_fired != 0 || c.calls != 1) {
        printf("正常 tag でフックが発火した: fired=%d calls=%d\n", sum_unhandled_fired, c.calls);
        return 1;
    }
    t.tag = (T_tag_t)99;
    T_dispatch(&t, &c, da, db);
    T_destroy(&t, za, zb);
    if (sum_unhandled_fired != 2 || c.calls != 1) {
        printf("不正 tag でフックが期待通り発火しない: fired=%d calls=%d\n", sum_unhandled_fired, c.calls);
        return 1;
    }
    return 0;
}
```

<sub>ケース断片: [`spec/cases/G-019.c`](../spec/cases/G-019.c)</sub>

<a id="g-020"></a>

### G-020 — 値を返すディスパッチャは不正 tag・空状態でフック後 SUM_TRAP に進みハンドラを呼ばない

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | ビルド・実行成功 | C99 以降すべて | [ADR-0011](adr/0011-default-label-and-trap.md) |

```c
/* 値を返すディスパッチャ（MATCH / MATCH_CONST / COPY / COPY_CHECKED / DISPATCH_RET）は、
 * 不正 tag と空状態でフックを呼んだ後 SUM_TRAP() に進み、ハンドラを1つも呼ばない。
 * v1 ではここが __builtin_unreachable() で、-O2 で無関係なハンドラが呼ばれていた。
 * SUM_TRAP は「戻らない」ことが契約なので、ここでは longjmp で検査側へ戻す。 */
#include <setjmp.h>
#include <stdio.h>
static jmp_buf env;
static int fired = 0;
#define SUM_UNHANDLED_TAG(self) ((void)(self), fired++)
#define SUM_TRAP() longjmp(env, 1)
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_MATCH(T, V, T_m, int)
SUM_DEFINE_MATCH_CONST(T, V, T_mc, int)
SUM_DEFINE_COPY(T, V, T_copy)
SUM_DEFINE_COPY_CHECKED(T, V, T_try_copy)
typedef struct { int n; } Ctx;
SUM_DEFINE_DISPATCH_RET(T, V, T_run, Ctx, int)
static int calls = 0;
static int ma(PA *p) { (void)p; calls++; return 1; }
static int mb(PB *p) { (void)p; calls++; return 2; }
static int ca(PA const *p) { (void)p; calls++; return 1; }
static int cb(PB const *p) { (void)p; calls++; return 2; }
static PA pa(const PA *p) { calls++; return *p; }
static PB pb(const PB *p) { calls++; return *p; }
static int ka(PA *d, const PA *s) { calls++; *d = *s; return 0; }
static int kb(PB *d, const PB *s) { calls++; *d = *s; return 0; }
static int ra(PA *p, Ctx *c) { (void)p; (void)c; calls++; return 1; }
static int rb(PB *p, Ctx *c) { (void)p; (void)c; calls++; return 2; }
int main(void) {
    T bad = T_new_b((PB){ 1 });
    T empty = SUM_EMPTY(T);
    T dst = T_new_a((PA){ 0 });
    Ctx c = { 0 };
    volatile int traps = 0;
    bad.tag = (T_tag_t)99;
    if (setjmp(env) == 0) { (void)T_m(&bad, ma, mb); } else { traps++; }
    if (setjmp(env) == 0) { (void)T_m(&empty, ma, mb); } else { traps++; }
    if (setjmp(env) == 0) { (void)T_mc(&bad, ca, cb); } else { traps++; }
    if (setjmp(env) == 0) { (void)T_copy(&bad, pa, pb); } else { traps++; }
    if (setjmp(env) == 0) { (void)T_try_copy(&dst, &bad, ka, kb); } else { traps++; }
    if (setjmp(env) == 0) { (void)T_run(&bad, &c, ra, rb); } else { traps++; }
    if (traps != 6 || fired != 6 || calls != 0) {
        printf("traps=%d fired=%d calls=%d\n", traps, fired, calls);
        return 1;
    }
    return 0;
}
```

<sub>ケース断片: [`spec/cases/G-020.c`](../spec/cases/G-020.c)</sub>

<a id="g-021"></a>

### G-021 — ハンドラに定数 NULL を渡す

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル失敗 | C99 以降すべて | [ADR-0016](adr/0016-nonnull-handlers.md) |

```c
/* ハンドラの代わりに定数 NULL を渡す。生成関数は __attribute__((nonnull)) 付きなので
 * -Wnonnull（-Wall に含まれる。-Werror でエラー）で落ちる。v1 では通って実行時に落ちていた。 */
#include <stddef.h>
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_MATCH(T, V, T_m, int)
static int ma(PA *p) { return p->a; }
static int mb(PB *p) { return p->b; }
int main(void) {
    T t = T_new_a((PA){ 1 });
#ifndef SPEC_FIXED
    (void)mb;
    return T_m(&t, ma, NULL);            /* variant 追加時の「とりあえず NULL」 */
#else
    return T_m(&t, ma, mb);
#endif
}
```

<sub>ケース断片: [`spec/cases/G-021.c`](../spec/cases/G-021.c)</sub>

<a id="g-022"></a>

### G-022 — 変数経由の NULL ハンドラは検出できない

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 既知の限界 | コンパイル成功 | C99 以降すべて | [ADR-0016](adr/0016-nonnull-handlers.md) |

```c
/* 変数を経由した NULL は nonnull 属性では検出できない（コンパイラは値を追えない）。
 * 検出できないことの固定。 */
#include <stddef.h>
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_MATCH(T, V, T_m, int)
static int ma(PA *p) { return p->a; }
int use(int (*hb)(PB *));
int use(int (*hb)(PB *)) {
    T t = T_new_a((PA){ 1 });
    return T_m(&t, ma, hb);              /* hb が NULL でも通る */
}
```

<sub>ケース断片: [`spec/cases/G-022.c`](../spec/cases/G-022.c)</sub>

<a id="g-023"></a>

### G-023 — ゼロ初期化値は空状態で、DESTROY 後も空状態に戻り2回目は no-op

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | ビルド・実行成功 | C99 以降すべて | [ADR-0013](adr/0013-empty-tag-zero.md) |

```c
/* tag 値 0 は空状態（NAME_TAG__EMPTY）。ゼロ初期化した値はどの variant でもなく、
 * ゲッターは NULL を返し、DESTROY は何もしない（フックも呼ばない）。
 * DESTROY は解放後に空状態へ戻すため、同じ値への2回目の DESTROY は no-op になる。 */
#include <stdio.h>
#include <string.h>
static int fired = 0;
#define SUM_UNHANDLED_TAG(self) ((void)(self), fired++)
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_DESTROY(T, V, T_destroy)
static int freed = 0;
static void da(PA *p) { (void)p; freed++; }
static void db(PB *p) { (void)p; freed++; }
static T g_static;                               /* 静的記憶域はゼロ初期化される */
int main(void) {
    T z;
    T t = T_new_b((PB){ 1 });
    memset(&z, 0, sizeof z);
    if (z.tag != T_TAG__EMPTY || g_static.tag != T_TAG__EMPTY) { puts("zero != EMPTY"); return 1; }
    if (T_get_a(&z) != NULL || T_get_b(&z) != NULL) { puts("getter on EMPTY"); return 1; }
    T_destroy(&z, da, db);
    T_destroy(&t, da, db);
    T_destroy(&t, da, db);
    if (freed != 1 || fired != 0 || t.tag != T_TAG__EMPTY) {
        printf("freed=%d fired=%d tag=%d\n", freed, fired, (int)t.tag);
        return 1;
    }
    return 0;
}
```

<sub>ケース断片: [`spec/cases/G-023.c`](../spec/cases/G-023.c)</sub>

<a id="g-024"></a>

### G-024 — DISPATCH_RET のハンドラを渡し忘れる

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル失敗 | C99 以降すべて | [ADR-0014](adr/0014-dispatch-ret-and-locked.md) |

```c
/* DISPATCH_RET も他のディスパッチャと同じく、ハンドラの渡し忘れが too few arguments で落ちる。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
typedef struct { int n; } Ctx;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_DISPATCH_RET(T, V, T_run, Ctx, int)
static int ra(PA *p, Ctx *c) { return p->a + c->n; }
static int rb(PB *p, Ctx *c) { return p->b + c->n; }
int main(void) {
    T t = T_new_a((PA){ 1 });
    Ctx c = { 0 };
#ifndef SPEC_FIXED
    (void)rb;
    return T_run(&t, &c, ra);
#else
    return T_run(&t, &c, ra, rb) == 1 ? 0 : 1;
#endif
}
```

<sub>ケース断片: [`spec/cases/G-024.c`](../spec/cases/G-024.c)</sub>

<a id="g-025"></a>

### G-025 — DISPATCH_LOCKED で同一 TU 内に型ごとのロック方針を持てる

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | ビルド・実行成功 | C99 以降すべて | [ADR-0014](adr/0014-dispatch-ret-and-locked.md) |

```c
/* DISPATCH_LOCKED はロックを定義箇所で受け取るため、同じ翻訳単位の中で型ごとに
 * 異なるロック方針を持てる（ロックを持たない Ctx 型も同居できる）。 */
#include <stdio.h>
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
typedef struct { int n; int held; int acquired; } Shared;
typedef struct { int n; } Local;                 /* ロック用メンバを持たない */
static void shared_lock(Shared *s) { s->held++; s->acquired++; }
static void shared_unlock(Shared *s) { s->held--; }
SUM_DEFINE_DISPATCH_LOCKED(T, V, T_apply_shared, Shared, shared_lock, shared_unlock)
SUM_DEFINE_DISPATCH_LOCKED(T, V, T_apply_local, Local, SUM_NO_LOCK, SUM_NO_LOCK)
static void sa(PA *p, Shared *s) { s->n += p->a * s->held; }
static void sb(PB *p, Shared *s) { s->n += p->b * s->held; }
static void la(PA *p, Local *l) { l->n += p->a; }
static void lb(PB *p, Local *l) { l->n += p->b; }
int main(void) {
    T t = T_new_a((PA){ 5 });
    Shared s = { 0, 0, 0 };
    Local l = { 0 };
    T_apply_shared(&t, &s, sa, sb);
    T_apply_local(&t, &l, la, lb);
    if (s.n != 5 || s.held != 0 || s.acquired != 1 || l.n != 5) {
        printf("s.n=%d held=%d acquired=%d l.n=%d\n", s.n, s.held, s.acquired, l.n);
        return 1;
    }
    return 0;
}
```

<sub>ケース断片: [`spec/cases/G-025.c`](../spec/cases/G-025.c)</sub>

<a id="g-026"></a>

### G-026 — COPY_CHECKED の型が異なるコピー関数を入れ替える

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル失敗 | C99 以降すべて | [ADR-0015](adr/0015-copy-checked.md) |

```c
/* COPY_CHECKED のコピー関数は (dst, src) の順に variant 固有の型を取る。
 * 型が異なる2 variant のコピー関数を入れ替えると -Wincompatible-pointer-types で落ちる。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_COPY_CHECKED(T, V, T_try_copy)
static int ka(PA *d, const PA *s) { *d = *s; return 0; }
static int kb(PB *d, const PB *s) { *d = *s; return 0; }
int main(void) {
    T src = T_new_a((PA){ 1 });
    T dst = SUM_EMPTY(T);
#ifndef SPEC_FIXED
    return T_try_copy(&dst, &src, kb, ka);
#else
    return T_try_copy(&dst, &src, ka, kb);
#endif
}
```

<sub>ケース断片: [`spec/cases/G-026.c`](../spec/cases/G-026.c)</sub>

<a id="g-027"></a>

### G-027 — 生成コードが -Wswitch-default / -Wswitch-enum を出さない

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル成功 | C99 以降すべて | [ADR-0011](adr/0011-default-label-and-trap.md) |

```c
/* 生成コードは -Wswitch-default / -Wswitch-enum を出さない（全 switch に default: と
 * 空状態の case を持つ）。MISRA C:2012 Rule 16.4 を課す利用側のビルドに入れられることの固定。 */
#pragma GCC diagnostic error "-Wswitch-default"
#pragma GCC diagnostic error "-Wswitch-enum"
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
typedef struct { int n; } Ctx;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_MATCH(T, V, T_m, int)
SUM_DEFINE_MATCH_CONST(T, V, T_mc, int)
SUM_DEFINE_DISPATCH(T, V, T_d, Ctx)
SUM_DEFINE_DISPATCH_RET(T, V, T_r, Ctx, int)
SUM_DEFINE_DESTROY(T, V, T_destroy)
SUM_DEFINE_COPY(T, V, T_copy)
SUM_DEFINE_COPY_CHECKED(T, V, T_try_copy)
int main(void) { return 0; }
```

<sub>ケース断片: [`spec/cases/G-027.c`](../spec/cases/G-027.c)</sub>

<a id="g-028"></a>

### G-028 — v1 で衝突した tag 名（get_a, new_a, tag_t, is_a）を並べても衝突しない

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル成功 | C99 以降すべて | [ADR-0012](adr/0012-naming-v2.md) |

```c
/* tag 定数は NAME_TAG_<tag>、生成関数は NAME_new_ / NAME_get_ / NAME_is_ で始まるため、
 * v1 で衝突していた tag 名（get_a, new_a, tag_t, is_a）を同じ型に並べても衝突しない。 */
#include "generic_sum_type.h"
typedef struct { int v; } P1;
typedef struct { int v; } P2;
typedef struct { int v; } P3;
typedef struct { int v; } P4;
typedef struct { int v; } P5;
#define V(X, NAME, EXTRA) \
    X(NAME, EXTRA, a, P1) X(NAME, EXTRA, get_a, P2) X(NAME, EXTRA, new_a, P3) \
    X(NAME, EXTRA, tag_t, P4) X(NAME, EXTRA, is_a, P5)
SUM_DEFINE_TYPE(T, V)
SUM_DEFINE_PREDICATE(T, a)
int main(void) {
    T t = T_new_get_a((P2){ 1 });
    return (T_get_get_a(&t) != NULL && !T_is_a(&t) && t.tag == T_TAG_get_a) ? 0 : 1;
}
```

<sub>ケース断片: [`spec/cases/G-028.c`](../spec/cases/G-028.c)</sub>

<a id="g-029"></a>

### G-029 — 末尾が _const の tag 名は const ゲッター名と衝突する

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 既知の限界 | コンパイル失敗 | C99 以降すべて | [C-14](constraints.md#c-14) |

```c
/* tag a の const ゲッター NAME_get_a_const と、tag a_const のゲッター NAME_get_a_const は
 * 同名になる。末尾が _const の tag 名は予約（docs/constraints.md C-14）。 */
#include "generic_sum_type.h"
typedef struct { int v; } P1;
typedef struct { int v; } P2;
#ifndef SPEC_FIXED
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, P1) X(NAME, EXTRA, a_const, P2)
#else
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, P1) X(NAME, EXTRA, a_fixed, P2)
#endif
SUM_DEFINE_TYPE(T, V)
int main(void) { return 0; }
```

<sub>ケース断片: [`spec/cases/G-029.c`](../spec/cases/G-029.c)</sub>

<a id="g-030"></a>

### G-030 — v1 のマクロ名と SUM_LEGACY_TAG_NAMES で v1 のコードが通る

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | ビルド・実行成功 | C99 以降すべて | [ADR-0012](adr/0012-naming-v2.md) |

```c
/* v1 のマクロ名は非推奨の別名として残り、SUM_LEGACY_TAG_NAMES を定義すれば v1 の
 * tag 定数名 NAME_<tag> も使える。v1 のコードを段階的に移行できることの固定。 */
#define SUM_LEGACY_TAG_NAMES
#include "generic_sum_type.h"
typedef struct { int v; } IntBox;
typedef struct { int code; } ErrBox;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, ok, IntBox) X(NAME, EXTRA, err, ErrBox)
DEFINE_SUM_TYPE(Res, V)
DEFINE_RESULT_HELPERS(Res)
DEFINE_SUM_MATCH(Res, V, Res_fold, int)
static int on_ok(IntBox *b) { return b->v; }
static int on_err(ErrBox *e) { return -e->code; }
int main(void) {
    Res r = Res_new_ok((IntBox){ 3 });
    int k = 0;
    switch (r.tag) {
        case Res_ok:  k = 1; break;             /* v1 の tag 定数名 */
        case Res_err: k = 2; break;
        default:      break;
    }
    return (k == 1 && Res_is_ok(&r) && Res_fold(&r, on_ok, on_err) == 3) ? 0 : 1;
}
```

<sub>ケース断片: [`spec/cases/G-030.c`](../spec/cases/G-030.c)</sub>

<a id="g-031"></a>

### G-031 — SUM_NO_LEGACY_NAMES で v1 のマクロ名が消える

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル失敗 | C99 以降すべて | [ADR-0012](adr/0012-naming-v2.md) |

```c
/* SUM_NO_LEGACY_NAMES を定義すると v1 のマクロ名は提供されない。
 * 移行完了後に v1 名の残存をビルドで検出するための切り替え。 */
#define SUM_NO_LEGACY_NAMES
#include "generic_sum_type.h"
typedef struct { int v; } P1;
typedef struct { int v; } P2;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, P1) X(NAME, EXTRA, b, P2)
#ifndef SPEC_FIXED
DEFINE_SUM_TYPE(T, V)
#else
SUM_DEFINE_TYPE(T, V)
#endif
int main(void) { return 0; }
```

<sub>ケース断片: [`spec/cases/G-031.c`](../spec/cases/G-031.c)</sub>

<a id="g-032"></a>

### G-032 — payload から struct NAME* で自身の型を指せる

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | ビルド・実行成功 | C99 以降すべて | [ADR-0017](adr/0017-self-reference-by-pointer.md) |

```c
/* SUM_DEFINE_TYPE は struct にタグ名 NAME を付けるため、payload から struct NAME* で
 * 自身の型を指せる（値としての自己埋め込みは言語上不可。C-04）。所有権は管理しない。 */
#include "generic_sum_type.h"
typedef struct { int v; } Num;
typedef struct { struct Expr *lhs, *rhs; } Add;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, num, Num) X(NAME, EXTRA, add, Add)
SUM_DEFINE_TYPE(Expr, V)
SUM_DEFINE_MATCH_CONST(Expr, V, Expr_eval, int)
static int eval_num(const Num *n) { return n->v; }
static int eval_add(const Add *a) {
    return Expr_eval(a->lhs, eval_num, eval_add) + Expr_eval(a->rhs, eval_num, eval_add);
}
int main(void) {
    Expr one = Expr_new_num((Num){ 1 });
    Expr two = Expr_new_num((Num){ 2 });
    Expr sum = Expr_new_add((Add){ &one, &two });
    return Expr_eval(&sum, eval_num, eval_add) == 3 ? 0 : 1;
}
```

<sub>ケース断片: [`spec/cases/G-032.c`](../spec/cases/G-032.c)</sub>
