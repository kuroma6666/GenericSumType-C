# generic_sum_type.h API リファレンス

`generic_sum_type.h` が提供するマクロと、生成される関数の**シグネチャと使い方**の一覧。
「なぜそうなっているか」は [docs/adr/](./docs/adr/)、「できないこと」は [docs/constraints.md](./docs/constraints.md)、
v1 からの移行は [CHANGELOG.md](./CHANGELOG.md) を参照。

## 目次

- [0. 型リストの定義規約](#0-型リストの定義規約)
- [1. `SUM_DEFINE_TYPE`](#1-sum_define_type)
- [2. 空状態と `SUM_EMPTY`](#2-空状態と-sum_empty)
- [3. `SUM_DEFINE_MATCH` / `SUM_DEFINE_MATCH_CONST`](#3-sum_define_match--sum_define_match_const)
- [4. `SUM_DEFINE_DISPATCH` 系](#4-sum_define_dispatch-系)
- [5. `SUM_DEFINE_DESTROY` / `SUM_DEFINE_NOOP_DESTROY`](#5-sum_define_destroy--sum_define_noop_destroy)
- [6. `SUM_DEFINE_COPY` / `SUM_DEFINE_IDENTITY_COPY`](#6-sum_define_copy--sum_define_identity_copy)
- [7. `SUM_DEFINE_COPY_CHECKED`](#7-sum_define_copy_checked)
- [8. 述語と Either / Result ヘルパ](#8-述語と-either--result-ヘルパ)
- [9. `SUM_DEFINE_NEW_GENERIC` / `SUM_NEW`（C11 以降）](#9-sum_define_new_generic--sum_newc11-以降)
- [10. フックと設定マクロ](#10-フックと設定マクロ)
- [11. 不正 tag・空状態での挙動](#11-不正-tag空状態での挙動)
- [12. v1 名の別名](#12-v1-名の別名)
- [13. 早見表](#13-早見表)

---

## 0. 型リストの定義規約

すべての `SUM_DEFINE_*` は、事前に定義した型リストマクロ（X-Macro）を第2引数に取る。型リストは1箇所だけで定義する。

```c
#define MY_VARIANTS(X, NAME, EXTRA)  \
    X(NAME, EXTRA, tag1, Type1)      \
    X(NAME, EXTRA, tag2, Type2)
```

| 要素 | 意味 |
|---|---|
| `X`, `NAME`, `EXTRA` | 生成マクロ側が渡す文脈。型リスト側はそのまま素通しする |
| `tagN` | variant 名。`_` 始まりと、他の tag 名 + `_const` は予約（[C-14](./docs/constraints.md#c-14)） |
| `TypeN` | ペイロード型。**variant 間で互換な型を重複させない**（[C-01](./docs/constraints.md#c-01)）。専用 struct でラップするのが推奨手段。関数ポインタ型は typedef してから渡す（[C-02](./docs/constraints.md#c-02)） |

---

## 1. `SUM_DEFINE_TYPE`

```c
SUM_DEFINE_TYPE(NAME, VARIANTS)
```

| 生成される識別子 | シグネチャ / 定義 |
|---|---|
| `NAME_tag_t` | `typedef enum { NAME_TAG__EMPTY = 0, NAME_TAG_tag1, NAME_TAG_tag2, ... } NAME_tag_t;` |
| `NAME` | `typedef struct NAME { NAME_tag_t tag; union { Type1 tag1; ... } as; } NAME;` |
| `NAME_new_<tag>` | `NAME NAME_new_<tag>(Type v)` |
| `NAME_get_<tag>` | `Type *NAME_get_<tag>(NAME *self)` — tag が一致しなければ `NULL` |
| `NAME_get_<tag>_const` | `Type const *NAME_get_<tag>_const(const NAME *self)`（[ADR-0009](./docs/adr/0009-non-breaking-const-support.md)） |

- struct にはタグ名 `NAME` が付くため、payload から `struct NAME *` で自身を指せる（[ADR-0017](./docs/adr/0017-self-reference-by-pointer.md)）。
- tag 定数を直接比較する場合は `v.tag == NAME_TAG_<tag>`。

```c
SUM_DEFINE_TYPE(Shape, SHAPE_VARIANTS)

Shape s = Shape_new_circle((Circle){ .radius = 2.0 });
Circle *c = Shape_get_circle(&s);        /* 一致するので非 NULL */
Rectangle *r = Shape_get_rectangle(&s);  /* 不一致なので NULL */
```

---

## 2. 空状態と `SUM_EMPTY`

tag 値 0（`NAME_TAG__EMPTY`）は、どの variant でもない**空状態**（[ADR-0013](./docs/adr/0013-empty-tag-zero.md)）。

- ゼロ初期化した値（静的記憶域、`memset`）と、`SUM_DEFINE_DESTROY` で解放した後の値が空状態になる。
- `SUM_EMPTY(NAME)` は空状態の値を作る複合リテラル（`T t = SUM_EMPTY(T);`）。
- 空状態でのゲッター・述語・DESTROY・その他のディスパッチャの挙動は[11節](#11-不正-tag空状態での挙動)。

---

## 3. `SUM_DEFINE_MATCH` / `SUM_DEFINE_MATCH_CONST`

```c
SUM_DEFINE_MATCH(NAME, VARIANTS, FN, RET_TYPE)
SUM_DEFINE_MATCH_CONST(NAME, VARIANTS, FN, RET_TYPE)
```

payload を受け取って値を返す、純粋な変換への振り分け。同じ型に対して関数名を変えて何度でも定義できる。

| マクロ | 生成される関数 |
|---|---|
| `SUM_DEFINE_MATCH` | `RET_TYPE FN(NAME *self, RET_TYPE (*on_tag1)(Type1 *), ...)` |
| `SUM_DEFINE_MATCH_CONST` | `RET_TYPE FN(const NAME *self, RET_TYPE (*on_tag1)(Type1 const *), ...)` |

- ハンドラは型リストの定義順に渡す。渡し忘れ（[G-001](./docs/GUARANTEES.md#g-001)）と、型の異なる variant 間の順序取り違え（[G-002](./docs/GUARANTEES.md#g-002)）はコンパイルエラー。
- `RET_TYPE` に `void` は渡せない（[C-08](./docs/constraints.md#c-08)）。

```c
SUM_DEFINE_MATCH_CONST(Shape, SHAPE_VARIANTS, Shape_area, double)

static double area_circle(const Circle *c) { return 3.14159 * c->radius * c->radius; }
static double area_rect(const Rectangle *r) { return r->width * r->height; }
double a = Shape_area(&s, area_circle, area_rect);
```

---

## 4. `SUM_DEFINE_DISPATCH` 系

payload と共有コンテキスト（ctx）を受け取る、副作用を伴う振り分け。ロック区間はハンドラ本体全体を包む（[C-09](./docs/constraints.md#c-09)）。

| マクロ | 生成される関数 | ロック |
|---|---|---|
| `SUM_DEFINE_DISPATCH(NAME, V, FN, CTX)` | `void FN(NAME *self, CTX *ctx, void (*on_tag1)(Type1 *, CTX *), ...)` | `SUM_CTX_LOCK` / `SUM_CTX_UNLOCK` フック |
| `SUM_DEFINE_DISPATCH_RET(NAME, V, FN, CTX, RET)` | `RET FN(NAME *self, CTX *ctx, RET (*on_tag1)(Type1 *, CTX *), ...)` | 同上 |
| `SUM_DEFINE_DISPATCH_LOCKED(NAME, V, FN, CTX, LOCK, UNLOCK)` | `void FN(...)`（`SUM_DEFINE_DISPATCH` と同じ） | 引数で渡す |
| `SUM_DEFINE_DISPATCH_RET_LOCKED(NAME, V, FN, CTX, RET, LOCK, UNLOCK)` | `RET FN(...)`（`_RET` と同じ） | 引数で渡す |

- `LOCK` / `UNLOCK` は、ctx を1引数に取る関数か関数形式マクロ。ロック不要なら `SUM_NO_LOCK` を渡して明示する。
- 新しいコードでロックが必要な場合は、`_LOCKED` 版を推奨する（[ADR-0014](./docs/adr/0014-dispatch-ret-and-locked.md)）。
- `CTX` と `RET` には、事前に `typedef` した型名を渡す（その場で `struct Foo` と書くと、展開位置ごとに別の型になる）。
- `RET` に `void` は渡せない。
- ctx は非 NULL が前提（[ADR-0016](./docs/adr/0016-nonnull-handlers.md)）。

```c
typedef struct { RecipeStatus status; int value; } RecipeReply;
SUM_DEFINE_DISPATCH_RET(RecipeCommand, RECIPE_VARIANTS, RecipeCommand_execute, RecipeStore, RecipeReply)

RecipeReply r = RecipeCommand_execute(&cmd, &store, on_create, on_delete, on_select, on_query);
```

```c
static void counter_lock(Counter *c)   { pthread_mutex_lock(&c->mutex); }
static void counter_unlock(Counter *c) { pthread_mutex_unlock(&c->mutex); }
SUM_DEFINE_DISPATCH_LOCKED(CounterCmd, COUNTER_CMD, CounterCmd_apply, Counter,
                           counter_lock, counter_unlock)
```

---

## 5. `SUM_DEFINE_DESTROY` / `SUM_DEFINE_NOOP_DESTROY`

```c
SUM_DEFINE_DESTROY(NAME, VARIANTS, FN)
SUM_DEFINE_NOOP_DESTROY(FN_NAME, TYPE)
```

| マクロ | 生成される関数 |
|---|---|
| `SUM_DEFINE_DESTROY` | `void FN(NAME *self, void (*on_tag1)(Type1 *), ...)` |
| `SUM_DEFINE_NOOP_DESTROY` | `void FN_NAME(TYPE *v)`（何もしない。資源を持たない variant 用） |

- ハンドラを呼んだ後、`self` を空状態に戻す。同じ値への2回目の呼び出しは no-op になる（[G-023](./docs/GUARANTEES.md#g-023)）。
- 空状態の値に対しては何もしない（`free(NULL)` と同じ扱い）。
- 呼び出し忘れと、構造体コピーした別変数の二重解放は検出できない（[C-03](./docs/constraints.md#c-03)）。

---

## 6. `SUM_DEFINE_COPY` / `SUM_DEFINE_IDENTITY_COPY`

```c
SUM_DEFINE_COPY(NAME, VARIANTS, FN)
SUM_DEFINE_IDENTITY_COPY(FN_NAME, TYPE)
```

| マクロ | 生成される関数 |
|---|---|
| `SUM_DEFINE_COPY` | `NAME FN(const NAME *self, Type1 (*copy_tag1)(const Type1 *), ...)` |
| `SUM_DEFINE_IDENTITY_COPY` | `TYPE FN_NAME(const TYPE *v)`（`return *v;`） |

コピーの失敗を扱う場合は、次節の `SUM_DEFINE_COPY_CHECKED` を使う。

---

## 7. `SUM_DEFINE_COPY_CHECKED`

```c
SUM_DEFINE_COPY_CHECKED(NAME, VARIANTS, FN)
```

| 生成される関数 |
|---|
| `int FN(NAME *dst, const NAME *src, int (*copy_tag1)(Type1 *dst, const Type1 *src), ...)` |

- コピー関数は 0 で成功、0 以外で失敗を返し、FN はその値をそのまま返す。
- 失敗時は `*dst` を変更しない。部分的に確保した資源の後始末はコピー関数の責務（[ADR-0015](./docs/adr/0015-copy-checked.md)）。
- 成功時は `*dst` を上書きする。`*dst` が資源を所有している場合は、事前に DESTROY しておくこと。

```c
static int copy_text(LogText *dst, const LogText *src) {
    char *p = strdup(src->msg);
    if (p == NULL) return ENOMEM;
    dst->msg = p;
    return 0;
}
static int copy_metric(LogMetric *dst, const LogMetric *src) { *dst = *src; return 0; }

if (LogRecord_try_copy(&dup, &orig, copy_text, copy_metric) != 0) { /* dup は変更されていない */ }
```

---

## 8. 述語と Either / Result ヘルパ

| マクロ | 生成される関数 |
|---|---|
| `SUM_DEFINE_PREDICATE(NAME, TAG)` | `int NAME_is_<TAG>(const NAME *self)` |
| `SUM_DEFINE_EITHER_HELPERS(NAME)` | `NAME_is_left` / `NAME_is_right`（tag 名 `left` / `right` が前提） |
| `SUM_DEFINE_RESULT_HELPERS(NAME)` | `NAME_is_ok` / `NAME_is_err`（tag 名 `ok` / `err` が前提） |

- 規約外の tag 名に使うと `NAME_TAG_left` などが存在せずコンパイルエラーになる（[G-013](./docs/GUARANTEES.md#g-013), [G-015](./docs/GUARANTEES.md#g-015)）。
- 構築は `NAME_new_left` / `NAME_new_ok`、取り出しはゲッター、fold は `SUM_DEFINE_MATCH_CONST`（[ADR-0005](./docs/adr/0005-either-result-thin-helpers.md)）。
- payload 型が変わる `map` / `and_then` は汎用化できない（[C-13](./docs/constraints.md#c-13)）。

---

## 9. `SUM_DEFINE_NEW_GENERIC` / `SUM_NEW`（C11 以降）

```c
SUM_DEFINE_NEW_GENERIC(NAME, VARIANTS)   /* 補助の型・関数を1回だけ生成 */
SUM_NEW(NAME, VARIANTS, x)               /* x の型から NAME_new_<tag> を選ぶ */
```

- `__STDC_VERSION__ >= 201112L` のときだけ定義される。C99 と併用するファイルでは `#if` で囲む。
- `SUM_DEFINE_NEW_GENERIC` を呼んだ時点で、ペイロード型の重複（[C-01](./docs/constraints.md#c-01) の不変条件違反）がコンパイルエラーになる（[G-009](./docs/GUARANTEES.md#g-009), [G-017](./docs/GUARANTEES.md#g-017)）。`SUM_NEW` を使わなくても、この検査のために呼ぶ価値がある。
- 型は完全一致が必要（暗黙変換に頼れない）。一致する variant が無ければコンパイルエラー（[G-008](./docs/GUARANTEES.md#g-008)）。
- 推奨イディオム: `#define NAME_new(x) SUM_NEW(NAME, VARIANTS, x)` を利用側で1行書く（[ADR-0008](./docs/adr/0008-generic-constructor-selection.md)）。

| 内部で生成される識別子 | 用途 |
|---|---|
| `struct NAME_GenericPlaceholder_` | `_Generic` 連想リストの先頭用プレースホルダ |
| `NAME_new_unreachable_` | 上記の連想先。空状態を返す（通常は選択されない） |
| `NAME_assert_distinct_payloads_` | 定義時点で `_Generic` を展開して重複を検査する probe |

---

## 10. フックと設定マクロ

いずれも `#include "generic_sum_type.h"` より前に定義する。

| マクロ | 既定 | 用途 |
|---|---|---|
| `SUM_CTX_LOCK(ctx)` / `SUM_CTX_UNLOCK(ctx)` | no-op | `SUM_DEFINE_DISPATCH` / `_RET` のロック。`SUM_DEFINE_*` を展開した時点で評価される |
| `SUM_UNHANDLED_TAG(self)` | no-op | 不正 tag・空状態の通知（開発時に `assert` 等を差し込む） |
| `SUM_TRAP()` | GCC/Clang: `__builtin_trap()`、他: `abort()` | 値を返すディスパッチャの不正 tag 時の停止。**戻ってはならない** |
| `SUM_ASSUME_VALID_TAG` | 未定義 | 定義すると不正 tag を未定義動作として最適化する（フックも trap も呼ばない） |
| `SUM_NO_LEGACY_NAMES` | 未定義 | 定義すると v1 のマクロ名を提供しない |
| `SUM_LEGACY_TAG_NAMES` | 未定義 | 定義すると v1 の tag 定数名 `NAME_<tag>` も生成する |

| その他 | 内容 |
|---|---|
| `SUM_NO_LOCK(ctx)` | `_LOCKED` 版に「ロックしない」ことを明示して渡す no-op |
| `SUM_VERSION_MAJOR` / `_MINOR` / `_PATCH` | ヘッダのバージョン（2.0.0） |

```c
#include <assert.h>
#define SUM_UNHANDLED_TAG(self) assert(!"generic_sum_type: unhandled tag")
#define SUM_TRAP()              board_fault_handler()   /* 戻らない関数 */
#include "generic_sum_type.h"
```

---

## 11. 不正 tag・空状態での挙動

[ADR-0011](./docs/adr/0011-default-label-and-trap.md) と [ADR-0013](./docs/adr/0013-empty-tag-zero.md) による。
ハンドラが呼ばれることはない。

| 対象 | 空状態 | 不正 tag（範囲外の値） |
|---|---|---|
| ゲッター / 述語 | `NULL` / 偽 | `NULL` / 偽 |
| `SUM_DEFINE_DESTROY` | 何もしない | フック → 復帰 |
| `SUM_DEFINE_DISPATCH` / `_LOCKED` | フック → 復帰（UNLOCK は実行される） | 同左 |
| `MATCH` / `MATCH_CONST` / `COPY` / `COPY_CHECKED` / `DISPATCH_RET(_LOCKED)` | フック → `SUM_TRAP()` | 同左 |

`SUM_ASSUME_VALID_TAG` を定義した場合、ディスパッチャ（空状態の DESTROY を除く）が不正 tag・空状態を受けると未定義動作になる。ゲッター・述語は影響を受けない。

---

## 12. v1 名の別名

`SUM_NO_LEGACY_NAMES` を定義しない限り、次の v1 名が非推奨の別名として使える（[ADR-0012](./docs/adr/0012-naming-v2.md)）。

| v1 | v2 |
|---|---|
| `DEFINE_SUM_TYPE` / `_MATCH` / `_MATCH_CONST` / `_DISPATCH` / `_DESTROY` / `_COPY` / `_NEW_GENERIC` | `SUM_DEFINE_TYPE` / ... |
| `DEFINE_EITHER_HELPERS` / `DEFINE_RESULT_HELPERS` | `SUM_DEFINE_EITHER_HELPERS` / `SUM_DEFINE_RESULT_HELPERS` |
| `SUM_MAYBE_UNUSED` / `SUM_UNREACHABLE` | 内部マクロ `SUM__MAYBE_UNUSED` / `SUM__UNREACHABLE` |
| tag 定数 `NAME_<tag>` | `NAME_TAG_<tag>`（`SUM_LEGACY_TAG_NAMES` 定義時のみ旧名も生成） |

---

## 13. 早見表

| マクロ | 用途 | ハンドラ | ctx | 戻り値 |
|---|---|---|---|---|
| `SUM_DEFINE_TYPE` | 型定義 | — | — | — |
| `SUM_DEFINE_MATCH` | 値を返す変換 | `R (*)(Type *)` | なし | `R` |
| `SUM_DEFINE_MATCH_CONST` | 値を返す変換（read-only） | `R (*)(Type const *)` | なし | `R` |
| `SUM_DEFINE_DISPATCH(_LOCKED)` | 副作用 | `void (*)(Type *, Ctx *)` | あり | `void` |
| `SUM_DEFINE_DISPATCH_RET(_LOCKED)` | 副作用 + 結果 | `R (*)(Type *, Ctx *)` | あり | `R` |
| `SUM_DEFINE_DESTROY` | 資源解放 | `void (*)(Type *)` | なし | `void` |
| `SUM_DEFINE_COPY` | ディープコピー | `Type (*)(const Type *)` | なし | `NAME` |
| `SUM_DEFINE_COPY_CHECKED` | 失敗し得るディープコピー | `int (*)(Type *, const Type *)` | なし | `int` |
| `SUM_DEFINE_PREDICATE` / `_EITHER_HELPERS` / `_RESULT_HELPERS` | tag の述語 | — | — | `int` |
| `SUM_DEFINE_NEW_GENERIC` + `SUM_NEW` | 型からの構築・重複検査（C11 以降） | — | — | `NAME` |

実装例は [`examples/`](./examples/) を参照。
