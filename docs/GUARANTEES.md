# コンパイル時保証の一覧

> **このファイルは `spec/emit.sh` が `spec/manifest.tsv` とケース断片から生成する。手で編集しないこと。**
> 内容を変えたい場合は `spec/manifest.tsv` と `spec/cases/` を編集して `spec/emit.sh --write` を実行する。

`generic_sum_type.h` のランタイム挙動は `switch` 一つ分しかなく、このライブラリが
実際に約束しているのは「**どんなコードがコンパイルを通り、どんなコードが通らないか**」である。
したがってこの一覧がこのライブラリの仕様の中核であり、その判定器はコンパイラそのものになる。

各項目は `spec/run.sh` が CI（gcc / clang × c99 / c11 / c17）で毎回実行して検証している。

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
| [G-009](#g-009) | コンパイル失敗 | C11 以降のみ | ペイロード型の重複を DEFINE_SUM_NEW_GENERIC の時点で検出する | [ADR-0008](adr/0008-generic-constructor-selection.md) |
| [G-010](#g-010) | コンパイル成功 | C11 以降のみ | SUM_NEW が値の型からコンストラクタを選ぶ | [ADR-0008](adr/0008-generic-constructor-selection.md) |
| [G-011](#g-011) | コンパイル失敗 | C99 以降すべて | MATCH_CONST のハンドラがペイロードを書き換える | [ADR-0009](adr/0009-non-breaking-const-support.md) |
| [G-012](#g-012) | コンパイル成功 | C99 以降すべて | const NAME* から const 版の match / getter を呼ぶ | [ADR-0009](adr/0009-non-breaking-const-support.md) |
| [G-013](#g-013) | コンパイル失敗 | C99 以降すべて | Either ヘルパを left/right 以外の tag に使う | [ADR-0005](adr/0005-either-result-thin-helpers.md) |
| [G-014](#g-014) | コンパイル成功 | C99 以降すべて | Either ヘルパを left/right の tag に使う | [ADR-0005](adr/0005-either-result-thin-helpers.md) |
| [G-015](#g-015) | コンパイル失敗 | C99 以降すべて | Result ヘルパを ok/err 以外の tag に使う | [ADR-0005](adr/0005-either-result-thin-helpers.md) |
| [G-016](#g-016) | コンパイル成功 | C99 以降すべて | Result ヘルパを ok/err の tag に使う | [ADR-0005](adr/0005-either-result-thin-helpers.md) |
| [G-017](#g-017) | コンパイル失敗 | C11 以降のみ | typedef 別名で同一型になった 2 variant を検出する | [C-01](constraints.md#c-01) |
| [G-019](#g-019) | ビルド・実行成功 | C99 以降すべて | 不正 tag で SUM_UNHANDLED_TAG が発火しハンドラは呼ばれない | [ADR-0010](adr/0010-unhandled-tag-hook.md) |
| [G-020](#g-020) | コンパイル成功 | C99 以降すべて | 生成コードの仮引数が利用側の大域名と衝突せず -Wshadow を出さない | [ADR-0011](adr/0011-reserved-internal-identifiers.md) |
| [G-022](#g-022) | コンパイル成功 | C99 以降すべて | C-14 の pragma 回避策で -Wswitch-default 下でも生成 switch が通る | [C-14](constraints.md#c-14) |

## 既知の限界

| ID | 期待結果 | 対象 | 内容 | 根拠 |
|---|---|---|---|---|
| [G-003](#g-003) | コンパイル成功 | C99 以降すべて | 型が同一の variant 間の順序取り違えは検出できない | [C-01](constraints.md#c-01) |
| [G-005](#g-005) | コンパイル失敗 | C99 以降すべて | 関数ポインタ型を typedef せずペイロードに渡す | [C-02](constraints.md#c-02) |
| [G-018](#g-018) | コンパイル成功 | C99 のみ | C99 ではペイロード型の重複を検出できない | [C-01](constraints.md#c-01) |
| [G-021](#g-021) | コンパイル失敗 | C99 以降すべて | -Wswitch-default を error にすると default のない生成 switch が落ちる | [C-14](constraints.md#c-14) |

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
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_DISPATCH(T, V, T_dispatch, int)
static void h(PA *p, int *c) { (void)p; (void)c; }
int main(void) {
    T t = T_new_a((PA){ 0, 0 });
    int ctx = 0;
    T_dispatch(&t, &ctx, h);   /* b, c 用のハンドラが無い */
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
 * 関数ポインタの型が variant ごとに固有なので -Wincompatible-pointer-types。 */
#include "generic_sum_type.h"
typedef struct { int target, dmg; } CmdAttack;
typedef struct { int target, amt; } CmdHeal;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, attack, CmdAttack) X(NAME, EXTRA, heal, CmdHeal)
DEFINE_SUM_TYPE(Cmd, V)
DEFINE_SUM_DISPATCH(Cmd, V, Cmd_dispatch, int)
static void on_attack(CmdAttack *a, int *c) { (void)a; (void)c; }
static void on_heal(CmdHeal *h, int *c) { (void)h; (void)c; }
int main(void) {
    Cmd c = Cmd_new_attack((CmdAttack){ 1, 2 });
    int ctx = 0;
    Cmd_dispatch(&c, &ctx, on_heal, on_attack);   /* 順序が逆 */
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
DEFINE_SUM_TYPE(Cmd, V)
DEFINE_SUM_DISPATCH(Cmd, V, Cmd_dispatch, int)
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
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_DISPATCH(T, V, T_dispatch, int)
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
 * マクロ引数のコンマ分割が原因ではない点に注意（括弧で保護されている）。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, cb, void (*)(int, int))
DEFINE_SUM_TYPE(T, V)
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
DEFINE_SUM_TYPE(T, V)
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

DEFINE_SUM_TYPE(Command, CMD_VARIANTS)

typedef struct { int hp; int x, y; } GameState;

DEFINE_SUM_DISPATCH(Command, CMD_VARIANTS, Command_dispatch, GameState)

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
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_NEW_GENERIC(T, V)
int main(void) {
    T t = SUM_NEW(T, V, 42);   /* IntBox でも StrBox でもない生の int */
    return t.tag;
}
```

<sub>ケース断片: [`spec/cases/G-008.c`](../spec/cases/G-008.c)</sub>

<a id="g-009"></a>

### G-009 — ペイロード型の重複を DEFINE_SUM_NEW_GENERIC の時点で検出する

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル失敗 | C11 以降のみ | [ADR-0008](adr/0008-generic-constructor-selection.md) |

```c
/* ペイロード型が重複していると _Generic の連想リストが重複する。
 * DEFINE_SUM_NEW_GENERIC が生成する probe 関数がその場で _Generic を展開するため、
 * SUM_NEW を一度も呼ばなくても定義時点で落ちる。 */
#include "generic_sum_type.h"
typedef struct { int v; } IntBox;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, IntBox) X(NAME, EXTRA, b, IntBox)
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_NEW_GENERIC(T, V)
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
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_NEW_GENERIC(T, V)
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
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_MATCH_CONST(T, V, T_area, int)
static int on_a(const PA *p) { return p->a; }
static int on_b(const PB *p) { p->b = 9; return p->b; }   /* const への代入 */
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
 * （可変版 DEFINE_SUM_MATCH は discards const で落ちる）。 */
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_MATCH_CONST(T, V, T_area, int)
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
#define V(X, NAME, EXTRA) X(NAME, EXTRA, foo, PA) X(NAME, EXTRA, bar, PB)
DEFINE_SUM_TYPE(T, V)
DEFINE_EITHER_HELPERS(T)   /* T_left / T_right が無い */
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
DEFINE_SUM_TYPE(Result, V)
DEFINE_EITHER_HELPERS(Result)
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
#define V(X, NAME, EXTRA) X(NAME, EXTRA, foo, PA) X(NAME, EXTRA, bar, PB)
DEFINE_SUM_TYPE(T, V)
DEFINE_RESULT_HELPERS(T)   /* T_ok / T_err が無い */
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
DEFINE_SUM_TYPE(Res, V)
DEFINE_RESULT_HELPERS(Res)
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
 * C99 では同じコードが通ってしまう（G-018）。 */
#include "generic_sum_type.h"
#include <stdint.h>
typedef int32_t DeviceId;
typedef int32_t SensorId;   /* 別名だが DeviceId と同じ型 */
#define V(X, NAME, EXTRA) X(NAME, EXTRA, device, DeviceId) X(NAME, EXTRA, sensor, SensorId)
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_NEW_GENERIC(T, V)
int main(void) { return 0; }
```

<sub>ケース断片: [`spec/cases/G-017.c`](../spec/cases/G-017.c)</sub>

<a id="g-018"></a>

### G-018 — C99 ではペイロード型の重複を検出できない

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 既知の限界 | コンパイル成功 | C99 のみ | [C-01](constraints.md#c-01) |

```c
/* G-017 と同じ設計ミスだが、C99 には _Generic が無いため DEFINE_SUM_NEW_GENERIC
 * 自体が存在せず、機械検査が働かない。C99 では設計ガイドラインとレビューに頼るしかない、
 * という std 依存の限界を固定する。 */
#include "generic_sum_type.h"
#include <stdint.h>
typedef int32_t DeviceId;
typedef int32_t SensorId;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, device, DeviceId) X(NAME, EXTRA, sensor, SensorId)
DEFINE_SUM_TYPE(T, V)
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
 * ハンドラは一つも呼ばれない。値を返す MATCH / MATCH_CONST / COPY は
 * フック呼び出し後 SUM_UNREACHABLE() に進むため、ここでは検証しない。 */
#include <stdio.h>
static int sum_unhandled_fired = 0;
#define SUM_UNHANDLED_TAG(self) do { (void)(self); sum_unhandled_fired++; } while (0)
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
DEFINE_SUM_TYPE(T, V)
typedef struct { int calls; } Ctx;
DEFINE_SUM_DISPATCH(T, V, T_dispatch, Ctx)
DEFINE_SUM_DESTROY(T, V, T_destroy)
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

### G-020 — 生成コードの仮引数が利用側の大域名と衝突せず -Wshadow を出さない

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル成功 | C99 以降すべて | [ADR-0011](adr/0011-reserved-internal-identifiers.md) |

```c
/* 生成コードの仮引数・ローカル変数は sum_ で始まり _ で終わる名前に限られる。
 * 利用側が self / ctx / v や NAME_on_<tag> / NAME_copy_<tag> という名前を大域に
 * 持っていても、生成関数の仮引数がそれを隠して -Wshadow を出すことはない。
 * -Wshadow は -Wall / -Wextra に含まれないため、この TU だけ error に昇格して検査する。 */
#pragma GCC diagnostic error "-Wshadow"
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
typedef struct { int n; } Ctx;
static Ctx *ctx;   /* 生成コードが仮引数名に使いそうな名前を大域に置く */
static int self;
static int v;
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_MATCH(T, V, T_match, int)
DEFINE_SUM_MATCH_CONST(T, V, T_match_const, int)
DEFINE_SUM_DISPATCH(T, V, T_dispatch, Ctx)
DEFINE_SUM_DESTROY(T, V, T_destroy)
DEFINE_SUM_COPY(T, V, T_copy)
SUM_DEFINE_PREDICATE(T, a)
static void T_on_a(PA *p, Ctx *c) { (void)p; c->n++; }   /* NAME_on_<tag> という自然な命名 */
static void T_on_b(PB *p, Ctx *c) { (void)p; c->n++; }
SUM_DEFINE_IDENTITY_COPY(T_copy_a, PA)                    /* NAME_copy_<tag> も同様 */
SUM_DEFINE_IDENTITY_COPY(T_copy_b, PB)
static int ma(PA *p) { return p->a; }
static int mb(PB *p) { return p->b; }
static int ca(const PA *p) { return p->a; }
static int cb(const PB *p) { return p->b; }
static void za(PA *p) { (void)p; }
static void zb(PB *p) { (void)p; }
int main(void) {
    Ctx c = { 0 };
    ctx = &c; self = 0; v = 0;
    T t = T_new_a((PA){ 1 });
    T_dispatch(&t, &c, T_on_a, T_on_b);
    T u = T_copy(&t, T_copy_a, T_copy_b);
    T_destroy(&u, za, zb);
    return T_match(&t, ma, mb) + T_match_const(&t, ca, cb) + T_is_a(&t) + c.n + self + v - 4;
}
```

<sub>ケース断片: [`spec/cases/G-020.c`](../spec/cases/G-020.c)</sub>

<a id="g-021"></a>

### G-021 — -Wswitch-default を error にすると default のない生成 switch が落ちる

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 既知の限界 | コンパイル失敗 | C99 以降すべて | [C-14](constraints.md#c-14) |

```c
/* 生成される switch は意図的に default: を持たない（ADR-0010: NAME_tag_t を手で拡張された
 * ときに -Wswitch の網羅性警告を殺さないため）。その帰結として、-Wswitch-default を error に
 * 昇格した TU では DEFINE_SUM_* を呼んだ時点で落ちる。これを既知の限界として固定する。
 * 呼び出し側での回避策（pragma で囲む）が機能することは G-022 で確かめる。
 * 注意: -Wswitch-default を知らない Clang 17 以前では -Wunknown-warning-option が -Werror で
 * error になり、別の理由でコンパイルが失敗する（結果だけ見ると同じ）。CI の Clang は 18。 */
#pragma GCC diagnostic error "-Wswitch-default"
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_MATCH(T, V, T_match, int)   /* 生成 switch に default: が無いので error */
static int ma(PA *p) { return p->a; }
static int mb(PB *p) { return p->b; }
int main(void) { T t = T_new_a((PA){ 1 }); return T_match(&t, ma, mb) - 1; }
```

<sub>ケース断片: [`spec/cases/G-021.c`](../spec/cases/G-021.c)</sub>

<a id="g-022"></a>

### G-022 — C-14 の pragma 回避策で -Wswitch-default 下でも生成 switch が通る

| 種別 | 期待結果 | 対象 | 根拠 |
|---|---|---|---|
| 保証 | コンパイル成功 | C99 以降すべて | [C-14](constraints.md#c-14) |

```c
/* C-14 の回避策。生成マクロの呼び出し箇所だけを診断 pragma で囲むと、-Wswitch-default を
 * error に昇格した TU でも通る。診断にはマクロ定義側（ヘッダ）ではなく展開位置（呼び出し側）の
 * pragma 状態が適用される、という前提の確認でもある。DEFINE_SUM_TYPE は switch を生成しないので
 * 囲む必要がない。利用側自身の switch（main 内）は規約どおり default を持つ。
 * 注意: Clang 17 以前は -Wswitch-default を知らず、このケースは -Wunknown-warning-option で落ちる。
 * その場合は Clang 18 以降で再実行すること。 */
#pragma GCC diagnostic error "-Wswitch-default"
#include "generic_sum_type.h"
typedef struct { int a; } PA;
typedef struct { int b; } PB;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, a, PA) X(NAME, EXTRA, b, PB)
DEFINE_SUM_TYPE(T, V)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-default"
DEFINE_SUM_MATCH(T, V, T_match, int)
DEFINE_SUM_DISPATCH(T, V, T_dispatch, int)
#pragma GCC diagnostic pop
static int ma(PA *p) { return p->a; }
static int mb(PB *p) { return p->b; }
static void da(PA *p, int *c) { (void)p; (*c)++; }
static void db(PB *p, int *c) { (void)p; (*c)++; }
int main(void) {
    T t = T_new_a((PA){ 1 });
    int calls = 0;
    T_dispatch(&t, &calls, da, db);
    switch (calls) {
        case 1:  return T_match(&t, ma, mb) - 1;
        default: return 1;
    }
}
```

<sub>ケース断片: [`spec/cases/G-022.c`](../spec/cases/G-022.c)</sub>
