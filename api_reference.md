# generic_sum_type.h API リファレンス

このドキュメントは `generic_sum_type.h` が提供するマクロ・関数の**シグネチャと使い方**をまとめたリファレンスである。各マクロを採用した設計判断・トレードオフ・実装していないことは [docs/adr/](./docs/adr/) と [docs/constraints.md](./docs/constraints.md) を参照。「何ができるか」を素早く引くための一覧であり、「なぜそうなっているか」は [docs/adr/](./docs/adr/) 側に分離している。

## 目次

- [0. 前提: 型リストの定義規約](#0-前提-型リストの定義規約)
- [1. `DEFINE_SUM_TYPE`](#1-define_sum_type)
- [2. `DEFINE_SUM_MATCH`](#2-define_sum_match)
  - [2.1 `DEFINE_SUM_MATCH_CONST`(read-only 版)](#21-define_sum_match_constread-only-版)
  - [2.2 Either イディオム(`DEFINE_EITHER_HELPERS`)](#22-either-イディオムdefine_either_helpers)
  - [2.3 Result イディオム(`DEFINE_RESULT_HELPERS`)](#23-result-イディオムdefine_result_helpers)
- [3. `DEFINE_SUM_DISPATCH`](#3-define_sum_dispatch)
- [4. `DEFINE_SUM_DESTROY`](#4-define_sum_destroy)
- [5. `SUM_DEFINE_NOOP_DESTROY`](#5-sum_define_noop_destroy)
- [6. `DEFINE_SUM_COPY`](#6-define_sum_copy)
- [7. `SUM_DEFINE_IDENTITY_COPY`](#7-sum_define_identity_copy)
- [8. `SUM_CTX_LOCK` / `SUM_CTX_UNLOCK`](#8-sum_ctx_lock--sum_ctx_unlock)
- [9. `DEFINE_SUM_NEW_GENERIC` / `SUM_NEW`(C11以降限定)](#9-define_sum_new_generic--sum_newc11以降限定)
- [10. 内部マクロ(直接呼び出さないもの)](#10-内部マクロ直接呼び出さないもの)
- [11. 早見表](#11-早見表)

---

## 0. 前提: 型リストの定義規約

すべての `DEFINE_SUM_*` マクロは、事前に定義した「型リストマクロ」(X-Macro)を第2引数に取る。型リストは1箇所だけで定義し、複数の `DEFINE_SUM_*` から使い回す。

```c
#define MY_VARIANTS(X, NAME, EXTRA)  \
    X(NAME, EXTRA, tag1, Type1)      \
    X(NAME, EXTRA, tag2, Type2)      \
    X(NAME, EXTRA, tag3, Type3)
```

| プレースホルダ | 意味 |
|---|---|
| `X` | 呼び出し側(`DEFINE_SUM_*`)が差し込む「1 variantあたりの展開規則」 |
| `NAME` | SumType名。呼び出し側から渡される。型リスト定義側は素通しするだけでよい |
| `EXTRA` | 呼び出しマクロごとに意味が変わる文脈引数(`RetType`, `CtxType` など、または未使用なら `_`) |
| `tagN` | variantのタグ名(小文字スネークケース推奨。`NAME##_##tagN` という識別子が生成されるため) |
| `TypeN` | そのvariantのペイロード型。**素の基本型(`int`, `double`等)を直接使わず専用structでラップすることを推奨**([C-01](./docs/constraints.md#c-01): ハンドラ順序取り違え検出のため) |

型リスト中に **関数ポインタ型など複雑な宣言子を持つ型**を直接書くと壊れる(原因はマクロのコンマ分割ではなく、`TYPE 識別子;` という宣言パターンが関数ポインタの宣言子構文と噛み合わないため。実機検証済み、[C-02](./docs/constraints.md#c-02)参照)。`typedef` で単一トークンの型名にしてから渡すこと。

---

## 1. `DEFINE_SUM_TYPE`

```c
DEFINE_SUM_TYPE(NAME, VARIANTS)
```

型定義そのものを生成する。**最初に1回だけ呼ぶ。**

| 生成される識別子 | シグネチャ | 説明 |
|---|---|---|
| `NAME_tag_t` | `typedef enum { NAME_tag1, NAME_tag2, ... } NAME_tag_t;` | タグのenum |
| `NAME` | `typedef struct { NAME_tag_t tag; union { ... } as; } NAME;` | タグ付きunion本体 |
| `NAME_new_<tag>` | `NAME NAME_new_<tag>(Type v)` | コンストラクタ(variantごとに1個生成) |
| `NAME_get_<tag>` | `Type *NAME_get_<tag>(NAME *self)` | タグが一致すればアドレス、しなければ`NULL`を返すゲッター |
| `NAME_get_<tag>_const` | `Type const *NAME_get_<tag>_const(const NAME *self)` | 上記の read-only 版。`const NAME*` から呼べ、`Type const*` を返す(payload を書き換えられない)。可変版と併存([ADR-0009](./docs/adr/0009-non-breaking-const-support.md)) |

```c
DEFINE_SUM_TYPE(Shape, SHAPE_VARIANTS)

Shape s = Shape_new_circle((Circle){ .radius = 2.0 });
Circle *c = Shape_get_circle(&s);   /* 一致するのでNULLでない */
Rectangle *r = Shape_get_rectangle(&s); /* 不一致なのでNULL */
```

---

## 2. `DEFINE_SUM_MATCH`

```c
DEFINE_SUM_MATCH(NAME, VARIANTS, MATCH_FN, RET_TYPE)
```

「payloadを受け取り値を返す」純粋関数群への振り分けを生成する。**同じSumTypeに対して関数名を変えて何度でも呼べる**(用途違いのmatch関数を複数持てる)。

| 生成される識別子 | シグネチャ |
|---|---|
| `MATCH_FN` | `RET_TYPE MATCH_FN(NAME *self, RET_TYPE (*on_tag1)(Type1*), RET_TYPE (*on_tag2)(Type2*), ...)` |

- ハンドラの並び順は型リストの定義順と一致させる(渡し忘れ・順序取り違えはコンパイルエラーで検出される。[C-01](./docs/constraints.md#c-01))。
- `RET_TYPE = void` は**非推奨**(`-pedantic`環境で警告。4.7節参照)。void用途は `DEFINE_SUM_DISPATCH` を使う。

```c
DEFINE_SUM_MATCH(Shape, SHAPE_VARIANTS, Shape_area, double)
DEFINE_SUM_MATCH(Shape, SHAPE_VARIANTS, Shape_print, int)  /* 同じShapeに2個目のmatch */

double area_circle(Circle *c) { return 3.14159265358979 * c->radius * c->radius; }
double area_rectangle(Rectangle *r) { return r->width * r->height; }
double area_triangle(Triangle *t) { return t->base * t->height / 2.0; }

double a = Shape_area(&s, area_circle, area_rectangle, area_triangle);
```

### 2.1 `DEFINE_SUM_MATCH_CONST`(read-only 版)

```c
DEFINE_SUM_MATCH_CONST(NAME, VARIANTS, MATCH_FN, RET_TYPE)
```

`DEFINE_SUM_MATCH` の read-only 版。`self` を `const NAME*` で受け、各ハンドラも payload を `Type const*` で受け取る。`const NAME*` を保持したまま面積計算・分類・整形などの純粋変換を呼びたい場合に使う(可変版 `DEFINE_SUM_MATCH` は `const NAME*` を受け付けず `discards const` になる)。payload を書き換える用途では従来の可変版を使うこと。

| 生成される識別子 | シグネチャ |
|---|---|
| `MATCH_FN` | `RET_TYPE MATCH_FN(const NAME *self, RET_TYPE (*on_tag1)(Type1 const*), RET_TYPE (*on_tag2)(Type2 const*), ...)` |

- 網羅性検査・順序取り違え検出は可変版と同一の仕組みで働く([C-01](./docs/constraints.md#c-01))。
- 戻り型に east const(`Type const*`)を使うのは、`TYPE` がポインタ型(例: `const char*`)でも payload オブジェクト自体に const を付けるため(west const `const Type*` だと指す先に const が付き型不一致になる。[ADR-0009](./docs/adr/0009-non-breaking-const-support.md))。

```c
DEFINE_SUM_MATCH_CONST(Shape, SHAPE_VARIANTS, Shape_area, double)

double area_circle(const Circle *c) { return 3.14159265358979 * c->radius * c->radius; }
double area_rectangle(const Rectangle *r) { return r->width * r->height; }
double area_triangle(const Triangle *t) { return t->base * t->height / 2.0; }

double area_of(const Shape *s) {   /* const Shape* のまま呼べる */
    return Shape_area(s, area_circle, area_rectangle, area_triangle);
}
```


---

## 2.2 Either イディオム(`DEFINE_EITHER_HELPERS`)

`Either<L, R>`(Rust の `Result`、関数型の Either 相当)は「`left` か `right` の2択」に固定された直和型なので、`DEFINE_SUM_TYPE` の tag 名を `left` / `right` にするだけで表現できる。`DEFINE_EITHER_HELPERS(NAME)` は、その規約に乗った述語を生成する薄いヘルパ。

```c
DEFINE_EITHER_HELPERS(NAME)   /* DEFINE_SUM_TYPE(左右2 variant) の後に呼ぶ */
```

| 生成される識別子 | シグネチャ |
|---|---|
| `NAME_is_left`  | `int NAME_is_left(const NAME *self)` |
| `NAME_is_right` | `int NAME_is_right(const NAME *self)` |

Either の各操作は既存 API の組み合わせで完結する。

| 操作 | 使うもの |
|---|---|
| 構築 | `NAME_new_left(L)` / `NAME_new_right(R)`(`DEFINE_SUM_TYPE`) |
| 述語 | `NAME_is_left` / `NAME_is_right`(`DEFINE_EITHER_HELPERS`) |
| 取り出し | `NAME_get_left(_const)` / `NAME_get_right(_const)`(`DEFINE_SUM_TYPE`) |
| 畳み込み(fold) | `DEFINE_SUM_MATCH_CONST` の左右2ハンドラ |

- `left`/`right` 以外の tag 名の SumType に `DEFINE_EITHER_HELPERS` を使うと、`NAME_left` / `NAME_right` という enum 定数が無くコンパイルエラーになる(規約違反がそのまま検出される)。
- **L と R は別の型にする**(専用 struct でラップ)。同一型だと fold ハンドラの左右取り違え検出が効かない([C-01](./docs/constraints.md#c-01))。データを持たない Left/Right にはダミー struct を使う([C-06](./docs/constraints.md#c-06))。
- 実例は [`examples/either_demo.c`](./examples/either_demo.c)。

```c
typedef struct { int code; const char *msg; } Err;  /* Left  */
typedef struct { int value; }                 Ok;   /* Right */
#define RESULT_EITHER(X, NAME, EXTRA) \
    X(NAME, EXTRA, left,  Err)        \
    X(NAME, EXTRA, right, Ok)
DEFINE_SUM_TYPE(Result, RESULT_EITHER)
DEFINE_EITHER_HELPERS(Result)
DEFINE_SUM_MATCH_CONST(Result, RESULT_EITHER, Result_fold, int)
```


---

## 2.3 Result イディオム(`DEFINE_RESULT_HELPERS`)

`Result<T, E>`(Rust の `Result`)は Either の `ok` / `err` 版。tag 名を `ok` / `err` にした2 variant を `DEFINE_SUM_TYPE` で定義し、`DEFINE_RESULT_HELPERS(NAME)` で述語を生やす。

| 生成される識別子 | シグネチャ |
|---|---|
| `NAME_is_ok`  | `int NAME_is_ok(const NAME *self)` |
| `NAME_is_err` | `int NAME_is_err(const NAME *self)` |

| 操作 | 使うもの |
|---|---|
| 構築 | `NAME_new_ok(T)` / `NAME_new_err(E)` |
| 述語 | `NAME_is_ok` / `NAME_is_err` |
| 取り出し | `NAME_get_ok(_const)` / `NAME_get_err(_const)` |
| 畳み込み(fold) | `DEFINE_SUM_MATCH_CONST` の ok/err 2ハンドラ |
| unwrap_or 等 | 利用側で手書き（`T` 型を知る必要があるためヘルパ化しない） |

- **C での限界(ROP):** `map: Result<T,E> → Result<U,E>` / `and_then: T → Result<U,E>` / `map_err: E → F` のように **payload 型が変わる**関数合成は、変換先が別の具体 Result 型になり C では汎用化できない([C-13](./docs/constraints.md#c-13))。同型変換(`T → Result<T,E>`)や目標型を明示した手書きは可能。
- `ok`/`err` 以外の tag 名に使うと `NAME_ok`/`NAME_err` が無くコンパイルエラー(規約違反の検出)。
- **T と E は別の型にする**(専用 struct)。同一型だと fold の取り違え検出が効かない([C-01](./docs/constraints.md#c-01))。
- 実例は [`examples/result_demo.c`](./examples/result_demo.c)。

---

## 3. `DEFINE_SUM_DISPATCH`

```c
DEFINE_SUM_DISPATCH(NAME, VARIANTS, DISPATCH_FN, CTX_TYPE)
```

「payload + 共有ctxへの書き込み」という副作用を伴うハンドラへの振り分けを生成する。戻り値は常に`void`。ロックフック(8節)を自動で挟む。

| 生成される識別子 | シグネチャ |
|---|---|
| `DISPATCH_FN` | `void DISPATCH_FN(NAME *self, CTX_TYPE *ctx, void (*on_tag1)(Type1*, CTX_TYPE*), void (*on_tag2)(Type2*, CTX_TYPE*), ...)` |

内部動作: `SUM_CTX_LOCK(ctx)` → `switch(self->tag)` → `SUM_CTX_UNLOCK(ctx)`。

**注意:** `CTX_TYPE` にその場で `struct Foo` を書かず、必ず事前に `typedef` した型名を渡すこと。マクロ展開位置ごとに別の匿名構造体が生成され `-Wincompatible-pointer-types` になる([C-01](./docs/constraints.md#c-01)、実際に踏んだ既知の落とし穴)。

```c
typedef struct { long counter; } Counter;   /* 先にtypedefする */
DEFINE_SUM_DISPATCH(Cmd, CMD_VARIANTS, Cmd_dispatch, Counter)

void on_increment(CmdIncrement *p, Counter *ctx) { (void)p; ctx->counter++; }
void on_decrement(CmdDecrement *p, Counter *ctx) { (void)p; ctx->counter--; }

Cmd_dispatch(&cmd, &counter, on_increment, on_decrement);
```

---

## 4. `DEFINE_SUM_DESTROY`

```c
DEFINE_SUM_DESTROY(NAME, VARIANTS, DESTROY_FN)
```

ポインタ資源(heap確保した文字列・バッファ等)を持つvariantの解放を、網羅性検査付きで行う。ctxを取らない点が `DEFINE_SUM_DISPATCH` と異なる。

| 生成される識別子 | シグネチャ |
|---|---|
| `DESTROY_FN` | `void DESTROY_FN(NAME *self, void (*on_tag1)(Type1*), void (*on_tag2)(Type2*), ...)` |

**呼び出し忘れを検出する仕組みはない**(Cにownership checkerは存在しないため。[C-03](./docs/constraints.md#c-03))。呼べば全variantを網羅的に処理できることまでが保証範囲。

```c
DEFINE_SUM_DESTROY(Message, MESSAGE_VARIANTS, Message_destroy)

void destroy_text(MsgText *t) { free(t->str); }
/* MsgNumberは資源を持たないので5節のヘルパーを使う */

Message_destroy(&msg, destroy_text, MsgNumber_noop_destroy);
```

---

## 5. `SUM_DEFINE_NOOP_DESTROY`

```c
SUM_DEFINE_NOOP_DESTROY(FN_NAME, TYPE)
```

資源を持たないvariant向けに「何もしないデストラクタ」を生成するヘルパー。`DEFINE_SUM_DESTROY` に渡すハンドラを全variant分揃えるためだけに使う。

| 生成される識別子 | シグネチャ |
|---|---|
| `FN_NAME` | `void FN_NAME(TYPE *v)` (中身は`(void)v;`のみ) |

```c
SUM_DEFINE_NOOP_DESTROY(MsgNumber_noop_destroy, MsgNumber)
```

---

## 6. `DEFINE_SUM_COPY`

```c
DEFINE_SUM_COPY(NAME, VARIANTS, COPY_FN)
```

ポインタ資源を持つvariantのディープコピー(例: `strdup`)を、網羅性検査付きで行う。

| 生成される識別子 | シグネチャ |
|---|---|
| `COPY_FN` | `NAME COPY_FN(const NAME *self, Type1 (*copy_tag1)(const Type1*), Type2 (*copy_tag2)(const Type2*), ...)` |

```c
DEFINE_SUM_COPY(Message, MESSAGE_VARIANTS, Message_copy)

MsgText copy_text(const MsgText *t) { return (MsgText){ .str = strdup(t->str) }; }
/* MsgNumberは値そのものでよいので7節のヘルパーを使う */

Message copy = Message_copy(&msg, copy_text, MsgNumber_identity_copy);
```

---

## 7. `SUM_DEFINE_IDENTITY_COPY`

```c
SUM_DEFINE_IDENTITY_COPY(FN_NAME, TYPE)
```

値そのものを複製すればよい(ポインタ資源を持たない)variant向けに「浅いコピー」関数を生成するヘルパー。`DEFINE_SUM_COPY` に渡すハンドラを全variant分揃えるためだけに使う。

| 生成される識別子 | シグネチャ |
|---|---|
| `FN_NAME` | `TYPE FN_NAME(const TYPE *v)` (中身は`return *v;`のみ) |

```c
SUM_DEFINE_IDENTITY_COPY(MsgNumber_identity_copy, MsgNumber)
```

---

## 8. `SUM_CTX_LOCK` / `SUM_CTX_UNLOCK`

`DEFINE_SUM_DISPATCH` が生成する関数が switch 文の前後で呼ぶフックマクロ。デフォルトは no-op。ロックが必要な場合は **`#include "generic_sum_type.h"` より前に** 再定義する。

```c
#define SUM_CTX_LOCK(ctx)   pthread_mutex_lock(&(ctx)->mutex)
#define SUM_CTX_UNLOCK(ctx) pthread_mutex_unlock(&(ctx)->mutex)
#include "generic_sum_type.h"
```

再定義しなければ排他制御なしで動く(コンパイルエラーにはならない)。複数スレッドから使うctxかどうかはコードレビューで確認すること([C-09](./docs/constraints.md#c-09))。

**ロック区間はハンドラ本体全体を包む。** そのためハンドラ内から同一 `ctx` へ再 dispatch すると、非再帰ロックでは自己デッドロックする([C-09](./docs/constraints.md#c-09))。

---

## 8.1 `SUM_UNHANDLED_TAG`

`self->tag` がどの variant にも一致しなかった場合に呼ばれるフックマクロ。デフォルトは no-op。`SUM_CTX_LOCK` と同様、`#include` より前に再定義する。

```c
#include <assert.h>
#define SUM_UNHANDLED_TAG(self) assert(!"generic_sum_type: unhandled tag")
#include "generic_sum_type.h"
```

`union` メンバは常に公開でありゲッターを迂回できる([C-05](./docs/constraints.md#c-05))ため、tag の破壊は起こり得ないとは言い切れない。開発ビルドで `assert` を差し込んでおくと早期に検知できる。

**全5ディスパッチャが呼ぶが、その後の挙動は戻り値の有無で分かれる。**

| マクロ | 不一致時の挙動 |
|---|---|
| `DEFINE_SUM_DISPATCH` | フックを呼び、ハンドラを1つも呼ばずに正常復帰(`SUM_CTX_UNLOCK` は実行される) |
| `DEFINE_SUM_DESTROY` | フックを呼び、ハンドラを1つも呼ばずに正常復帰 |
| `DEFINE_SUM_MATCH` / `_CONST` / `DEFINE_SUM_COPY` | フックを呼んだ後 `SUM_UNREACHABLE()` へ進む(**返す値を用意できないため**) |

この非対称は言語の制約から導かれる帰結であり意図的なものである([ADR-0010](./docs/adr/0010-unhandled-tag-hook.md))。回帰検査は [G-019](./docs/GUARANTEES.md#g-019)。

---

## 8.2 `SUM_DEFINE_PREDICATE`

```c
SUM_DEFINE_PREDICATE(NAME, TAG)
```

「今この SumType はどの variant か」を問う述語を1個生成する。

| 生成される識別子 | シグネチャ |
|---|---|
| `NAME_is_<TAG>` | `int NAME_is_<TAG>(const NAME *self)` |

`DEFINE_EITHER_HELPERS` / `DEFINE_RESULT_HELPERS` はどちらもこれの2回呼び出しにすぎない([ADR-0005](./docs/adr/0005-either-result-thin-helpers.md))。`left`/`right`、`ok`/`err` 以外の2択にもそのまま使える。

```c
SUM_DEFINE_PREDICATE(Job, pending)
SUM_DEFINE_PREDICATE(Job, done)
```

---

## 9. `DEFINE_SUM_NEW_GENERIC` / `SUM_NEW`(C11以降限定)

```c
DEFINE_SUM_NEW_GENERIC(NAME, VARIANTS)   // 補助の型・関数を1回だけ生成
SUM_NEW(NAME, VARIANTS, x)               // 呼び出し側はこの形で使う
```

C11の`_Generic`を使い、渡した値の**型**だけから`NAME_new_<tag>()`を自動選択する糖衣構文。タグ名(`i`/`s`/`f`等)を呼び出し側が覚えて綴る必要がなくなる。

**`__STDC_VERSION__ >= 201112L`でガードされており、C99ビルドではこの2マクロとも存在しない。** `#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L` で囲って使うこと。

| 生成される識別子 | シグネチャ | 説明 |
|---|---|---|
| `NAME_GenericPlaceholder_`(struct) | - | `_Generic`の連想リストの先頭コンマ問題を解決するための内部プレースホルダ型 |
| `NAME_new_unreachable_` | `NAME NAME_new_unreachable_(struct NAME_GenericPlaceholder_)` | 上記プレースホルダ用の内部関数。通常は選択されない |
| `NAME_assert_distinct_payloads_` | `void NAME_assert_distinct_payloads_(void)` | ペイロード型の重複を定義時点で検査するための内部 probe 関数(下記) |

**このマクロを呼ぶだけで[不変条件 C-01](./docs/constraints.md#c-01)(variant 間でペイロード型が重複しない)が機械検査される。** `_Generic` は互換な型が連想リストに2つあると ISO C 6.5.1.1 の制約違反になるため、probe 関数がその場で `_Generic` を1回展開して診断を発火させる。**`SUM_NEW` を一度も呼ばなくても検出される**([G-009](./docs/GUARANTEES.md#g-009), [G-017](./docs/GUARANTEES.md#g-017))。

そのため `SUM_NEW` を使う予定がなくても、**この不変条件を検査させる目的だけで `DEFINE_SUM_NEW_GENERIC` を呼んでおく価値がある**。C99 ビルドではこの検査は働かない([G-018](./docs/GUARANTEES.md#g-018))。

**`NAME_new(x)`のような専用マクロは生成できない**(Cのプリプロセッサはマクロ展開結果から新しい`#define`を起こせないため)。そのため呼び出し側は`SUM_NEW(NAME, VARIANTS, x)`という形で、`NAME`と`VARIANTS`を毎回明示する。

**推奨イディオム: 利用側で1行のショートカットマクロを書く。** ライブラリが`NAME_new(x)`を自動生成することはできないが、利用側が`DEFINE_SUM_NEW_GENERIC`の直後に自分で`#define NAME_new(x) SUM_NEW(NAME, VARIANTS, x)`と1行書くことは制限されない(これは通常のユーザーコードによる`#define`であり、マクロ展開結果から生成されたものではないため、上記の制約に抵触しない)。C99/C11双方で動作確認済み([ADR-0008](./docs/adr/0008-generic-constructor-selection.md))。

```c
typedef struct { int32_t v; } IntBox;
typedef struct { const char *v; } StrBox;
#define IOS_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, i, IntBox)        \
    X(NAME, EXTRA, s, StrBox)

DEFINE_SUM_TYPE(IntOrStr, IOS_VARIANTS)
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
DEFINE_SUM_NEW_GENERIC(IntOrStr, IOS_VARIANTS)
/* 推奨イディオム: 呼び出し側でショートカットマクロを1行定義する */
#define IntOrStr_new(x) SUM_NEW(IntOrStr, IOS_VARIANTS, x)
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
IntOrStr a = IntOrStr_new(((IntBox){ 42 }));
IntOrStr b = IntOrStr_new(((StrBox){ "hi" }));
#endif
```

**注意点([ADR-0008](./docs/adr/0008-generic-constructor-selection.md)に検証結果を記載):**

- `_Generic`は制御式の型が**完全一致**しないと選択できない(暗黙の型変換に頼れない)。ポインタが指す先のconst有無も別の型として扱われる(`char*`と`const char*`は別キー)。ペイロードを専用structでラップする設計ガイドライン([C-01](./docs/constraints.md#c-01))に従っていれば問題にならない。
- 2つ以上のvariantが同一のペイロード型を共有していると、`_Generic`の連想リストが重複しコンパイルエラーになる。**`DEFINE_SUM_NEW_GENERIC`を呼んだ時点で検出される**(probe 関数がその場で`_Generic`を展開するため。`SUM_NEW`の呼び出しは不要)。`typedef`は別名を作るだけで別の型を作らないため、`typedef int32_t A; typedef int32_t B;` のように「素の基本型を避ける」ガイドラインを守っていても重複しうる。この検査だけがそれを捕まえる([C-01](./docs/constraints.md#c-01))。
- このマクロを使うファイルをC99マトリクスのCIジョブに含める場合、ファイル自体を`#if __STDC_VERSION__ >= 201112L`で自己ガードし、C99実行時は代替処理(またはスキップ)をする必要がある。詳細は[`examples/generic_ctor_demo.c`](./examples/generic_ctor_demo.c)を参照。

---

## 10. 内部マクロ(直接呼び出さないもの)

利用者が直接書くことは通常ないが、生成コードの挙動に関わるため参考として記載する。

| マクロ | 役割 |
|---|---|
| `SUM_UNREACHABLE()` | `DEFINE_SUM_MATCH`/`DEFINE_SUM_COPY` の switch 末尾で使う。GCC/Clangでは`__builtin_unreachable()`、それ以外では`abort()`にフォールバック |
| `SUM_MAYBE_UNUSED` | 生成される全`static inline`関数に付与される`__attribute__((unused))`。Clangの`-Wunused-function`対策([VERIFICATION_LOG.md](./VERIFICATION_LOG.md) 7節) |
| `SUM_TAG_ENTRY` / `SUM_UNION_MEMBER` / `SUM_CTOR` / `SUM_GETTER` | `DEFINE_SUM_TYPE`が内部で使うアスペクトマクロ |
| `SUM_MATCH_PARAM` / `SUM_MATCH_CASE` | `DEFINE_SUM_MATCH`が内部で使うアスペクトマクロ |
| `SUM_DISPATCH_PARAM` / `SUM_DISPATCH_CASE` | `DEFINE_SUM_DISPATCH`が内部で使うアスペクトマクロ |
| `SUM_DESTROY_PARAM` / `SUM_DESTROY_CASE` | `DEFINE_SUM_DESTROY`が内部で使うアスペクトマクロ |
| `SUM_COPY_PARAM` / `SUM_COPY_CASE` | `DEFINE_SUM_COPY`が内部で使うアスペクトマクロ |
| `SUM_GENERIC_ASSOC` | `DEFINE_SUM_NEW_GENERIC`が内部で使うアスペクトマクロ(C11以降限定) |

---

## 11. 早見表

| マクロ | 用途 | ハンドラのシグネチャ | ctx | 戻り値 |
|---|---|---|---|---|
| `DEFINE_SUM_TYPE` | 型定義そのもの | - | - | - |
| `DEFINE_SUM_MATCH` | 値を返す純粋変換 | `RetType (*)(Type*)` | なし | `RetType` |
| `DEFINE_SUM_MATCH_CONST` | 値を返す純粋変換(read-only) | `RetType (*)(Type const*)` | なし | `RetType` |
| `DEFINE_SUM_DISPATCH` | 副作用を伴う処理 | `void (*)(Type*, CtxType*)` | あり(ロックフック付き) | `void`固定 |
| `DEFINE_SUM_DESTROY` | リソース解放 | `void (*)(Type*)` | なし | `void`固定 |
| `DEFINE_SUM_COPY` | ディープコピー | `Type (*)(const Type*)` | なし | `NAME`(コピー結果) |
| `DEFINE_SUM_NEW_GENERIC` + `SUM_NEW` | 型からコンストラクタを自動選択 + ペイロード型重複の検査(C11以降限定) | - | - | `NAME` |
| `SUM_DEFINE_PREDICATE` | `NAME_is_<TAG>()` 述語の生成 | - | - | `int` |
| `DEFINE_EITHER_HELPERS` / `DEFINE_RESULT_HELPERS` | 上記の `left`/`right`・`ok`/`err` 版 | - | - | `int` |

フック(いずれも `#include` より前に再定義。既定は no-op):

| フック | 呼ばれる場所 |
|---|---|
| `SUM_CTX_LOCK` / `SUM_CTX_UNLOCK` | `DEFINE_SUM_DISPATCH` のロック区間 |
| `SUM_UNHANDLED_TAG` | 全5ディスパッチャで tag がどの variant にも一致しなかったとき |

実装例は [`examples/`](./examples) 配下の各デモを参照(`demo.c`, `shape_demo.c`, `command_demo.c`, `resource_demo.c`, `threadsafe_dispatch_demo.c`, `generic_ctor_demo.c`)。設計判断・既知の制約は [docs/adr/](./docs/adr/) と [docs/constraints.md](./docs/constraints.md) を参照。
