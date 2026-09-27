/*
 * generic_sum_type.h — N 分岐の直和型（タグ付き共用体）を X-Macro で生成する。
 *
 * 対応: C99 以降 / GCC・Clang（MSVC は対象外。docs/adr/0007）
 * 必須ビルドフラグ: -Wall -Wextra -Werror
 *
 * 文書（このコメントは索引に留め、内容は各文書を単一ソースとする）
 *   採用条件・非目標              docs/requirements.md
 *   シグネチャと使い方            api_reference.md
 *   コンパイル時に保証されること  docs/GUARANTEES.md（spec/ からの生成物）
 *   できないこと                  docs/constraints.md
 *   設計判断                      docs/adr/
 *   v1 からの移行                 CHANGELOG.md
 *
 * 利用者が include 前に定義できるもの
 *   SUM_CTX_LOCK(ctx) / SUM_CTX_UNLOCK(ctx)  SUM_DEFINE_DISPATCH(_RET) のロック（既定 no-op）
 *   SUM_UNHANDLED_TAG(self)                  不正 tag の通知（既定 no-op）
 *   SUM_TRAP()                               値を返すディスパッチャの不正 tag 時の停止（戻ってはならない）
 *   SUM_ASSUME_VALID_TAG                     定義すると不正 tag を未定義動作として最適化する
 *   SUM_NO_LEGACY_NAMES                      定義すると v1 のマクロ名を提供しない
 *   SUM_LEGACY_TAG_NAMES                     定義すると v1 の tag 定数名 NAME_<tag> も生成する
 */
#ifndef GENERIC_SUM_TYPE_H
#define GENERIC_SUM_TYPE_H

#define SUM_VERSION_MAJOR 2
#define SUM_VERSION_MINOR 0
#define SUM_VERSION_PATCH 0

#include <stddef.h> /* NULL */

/* ======================================================================
 * 処理系依存部（docs/constraints.md C-07）
 * ====================================================================== */

#if defined(__GNUC__) || defined(__clang__)
#define SUM__MAYBE_UNUSED __attribute__((unused))
#define SUM__NONNULL      __attribute__((nonnull))
#define SUM__UNREACHABLE() __builtin_unreachable()
#define SUM__DEFAULT_TRAP() __builtin_trap()
#else
#include <stdlib.h>
#define SUM__MAYBE_UNUSED
#define SUM__NONNULL
#define SUM__UNREACHABLE() abort()
#define SUM__DEFAULT_TRAP() abort()
#endif

/* ======================================================================
 * フック（docs/adr/0003, 0011）
 * ====================================================================== */

#ifndef SUM_CTX_LOCK
#define SUM_CTX_LOCK(ctx) ((void)(ctx))
#endif
#ifndef SUM_CTX_UNLOCK
#define SUM_CTX_UNLOCK(ctx) ((void)(ctx))
#endif

/* SUM_DEFINE_DISPATCH_LOCKED 等に「ロックしない」ことを明示して渡すための no-op */
#define SUM_NO_LOCK(ctx) ((void)(ctx))

#ifndef SUM_UNHANDLED_TAG
#define SUM_UNHANDLED_TAG(self) ((void)(self))
#endif

#ifndef SUM_TRAP
#define SUM_TRAP() SUM__DEFAULT_TRAP()
#endif

/* 不正 tag を受けたときの switch の default 節。
 *   void を返すもの   : フックを呼んで復帰する
 *   値を返すもの      : フックを呼んで SUM_TRAP() で停止する。SUM_TRAP が誤って戻った
 *                       場合も未定義動作にしないため、無限ループで受ける
 * SUM_ASSUME_VALID_TAG 定義時はどちらも到達不能として扱う（利用者の明示的な選択）。 */
#ifdef SUM_ASSUME_VALID_TAG
#define SUM__ON_BAD_TAG_VOID(self)  SUM__UNREACHABLE()
#define SUM__ON_BAD_TAG_VALUE(self) SUM__UNREACHABLE()
#else
#define SUM__ON_BAD_TAG_VOID(self)  SUM_UNHANDLED_TAG(self)
#define SUM__ON_BAD_TAG_VALUE(self)                                 \
    do { SUM_UNHANDLED_TAG(self); SUM_TRAP(); for (;;) { } } while (0)
#endif

/* ======================================================================
 * SUM_DEFINE_TYPE
 *
 * tag 値 0 は SUM_DEFINE_TYPE が予約する空状態 NAME_TAG__EMPTY。
 * ゼロ初期化した値と、SUM_DEFINE_DESTROY で解放した後の値がこの状態になる（docs/adr/0013）。
 * ====================================================================== */

#define SUM__TAG_ENTRY(NAME, EXTRA, TAG, TYPE)    NAME##_TAG_##TAG,
#define SUM__UNION_MEMBER(NAME, EXTRA, TAG, TYPE) TYPE TAG;

#ifdef SUM_LEGACY_TAG_NAMES
#define SUM__LEGACY_TAG_ENTRY(NAME, EXTRA, TAG, TYPE) NAME##_##TAG = NAME##_TAG_##TAG,
#else
#define SUM__LEGACY_TAG_ENTRY(NAME, EXTRA, TAG, TYPE)
#endif

#define SUM__CTOR(NAME, EXTRA, TAG, TYPE)                           \
    SUM__MAYBE_UNUSED                                               \
    static inline NAME NAME##_new_##TAG(TYPE v) {                   \
        return (NAME){ .tag = NAME##_TAG_##TAG, .as.TAG = v };      \
    }

#define SUM__GETTER(NAME, EXTRA, TAG, TYPE)                         \
    SUM__MAYBE_UNUSED SUM__NONNULL                                  \
    static inline TYPE *NAME##_get_##TAG(NAME *self) {              \
        return self->tag == NAME##_TAG_##TAG ? &self->as.TAG : NULL; \
    }

/* 戻り型が east const（TYPE const*）なのは、TYPE がポインタ型でも payload 自体を
 * const にするため（docs/adr/0009）。 */
#define SUM__GETTER_CONST(NAME, EXTRA, TAG, TYPE)                   \
    SUM__MAYBE_UNUSED SUM__NONNULL                                  \
    static inline TYPE const *NAME##_get_##TAG##_const(             \
        const NAME *self) {                                         \
        return self->tag == NAME##_TAG_##TAG ? &self->as.TAG : NULL; \
    }

/* struct にタグ名 NAME を付けるのは、payload から struct NAME* で自身を指せるようにするため
 * （docs/adr/0017）。 */
#define SUM_DEFINE_TYPE(NAME, VARIANTS)                             \
    typedef enum {                                                  \
        NAME##_TAG__EMPTY = 0,                                      \
        VARIANTS(SUM__TAG_ENTRY, NAME, _)                           \
        VARIANTS(SUM__LEGACY_TAG_ENTRY, NAME, _)                    \
    } NAME##_tag_t;                                                 \
    typedef struct NAME {                                           \
        NAME##_tag_t tag;                                           \
        union { VARIANTS(SUM__UNION_MEMBER, NAME, _) } as;          \
    } NAME;                                                         \
    VARIANTS(SUM__CTOR, NAME, _)                                    \
    VARIANTS(SUM__GETTER, NAME, _)                                  \
    VARIANTS(SUM__GETTER_CONST, NAME, _)

/* 空状態の値（複合リテラル）。T t = SUM_EMPTY(T); */
#define SUM_EMPTY(NAME) ((NAME){ .tag = NAME##_TAG__EMPTY })

/* ======================================================================
 * 述語と Either / Result ヘルパ（docs/adr/0005）
 * ====================================================================== */

#define SUM_DEFINE_PREDICATE(NAME, TAG)                             \
    SUM__MAYBE_UNUSED SUM__NONNULL                                  \
    static inline int NAME##_is_##TAG(const NAME *self) {           \
        return self->tag == NAME##_TAG_##TAG;                       \
    }

#define SUM_DEFINE_EITHER_HELPERS(NAME)                             \
    SUM_DEFINE_PREDICATE(NAME, left)                                \
    SUM_DEFINE_PREDICATE(NAME, right)

#define SUM_DEFINE_RESULT_HELPERS(NAME)                             \
    SUM_DEFINE_PREDICATE(NAME, ok)                                  \
    SUM_DEFINE_PREDICATE(NAME, err)

/* ======================================================================
 * SUM_DEFINE_MATCH / SUM_DEFINE_MATCH_CONST
 * RET_TYPE に void は渡せない（docs/constraints.md C-08）。
 * ====================================================================== */

#define SUM__MATCH_PARAM(NAME, RET_TYPE, TAG, TYPE)                 \
    , RET_TYPE (*NAME##_on_##TAG)(TYPE *)

#define SUM__MATCH_CONST_PARAM(NAME, RET_TYPE, TAG, TYPE)           \
    , RET_TYPE (*NAME##_on_##TAG)(TYPE const *)

#define SUM__MATCH_CASE(NAME, RET_TYPE, TAG, TYPE)                  \
    case NAME##_TAG_##TAG: return NAME##_on_##TAG(&self->as.TAG);

#define SUM_DEFINE_MATCH(NAME, VARIANTS, MATCH_FN, RET_TYPE)        \
    SUM__MAYBE_UNUSED SUM__NONNULL                                  \
    static inline RET_TYPE MATCH_FN(NAME *self                      \
        VARIANTS(SUM__MATCH_PARAM, NAME, RET_TYPE)) {               \
        switch (self->tag) {                                        \
            VARIANTS(SUM__MATCH_CASE, NAME, RET_TYPE)               \
            case NAME##_TAG__EMPTY: default: SUM__ON_BAD_TAG_VALUE(self); \
        }                                                           \
    }

#define SUM_DEFINE_MATCH_CONST(NAME, VARIANTS, MATCH_FN, RET_TYPE)  \
    SUM__MAYBE_UNUSED SUM__NONNULL                                  \
    static inline RET_TYPE MATCH_FN(const NAME *self                \
        VARIANTS(SUM__MATCH_CONST_PARAM, NAME, RET_TYPE)) {         \
        switch (self->tag) {                                        \
            VARIANTS(SUM__MATCH_CASE, NAME, RET_TYPE)               \
            case NAME##_TAG__EMPTY: default: SUM__ON_BAD_TAG_VALUE(self); \
        }                                                           \
    }

/* ======================================================================
 * SUM_DEFINE_DISPATCH 系（docs/adr/0002, 0003, 0014）
 *
 * ロック区間はハンドラ本体全体を包む（同一 ctx への再 dispatch は非再帰ロックで
 * デッドロックする。docs/constraints.md C-09）。
 * LOCK / UNLOCK は SUM_DEFINE_* を展開した時点で評価される。
 * ====================================================================== */

#define SUM__DISPATCH_PARAM(NAME, CTX_TYPE, TAG, TYPE)              \
    , void (*NAME##_on_##TAG)(TYPE *, CTX_TYPE *)

#define SUM__DISPATCH_CASE(NAME, CTX_TYPE, TAG, TYPE)               \
    case NAME##_TAG_##TAG:                                          \
        NAME##_on_##TAG(&self->as.TAG, ctx);                        \
        break;

#define SUM__DISPATCH_IMPL(NAME, VARIANTS, FN, CTX_TYPE, LOCK, UNLOCK) \
    SUM__MAYBE_UNUSED SUM__NONNULL                                  \
    static inline void FN(NAME *self, CTX_TYPE *ctx                 \
        VARIANTS(SUM__DISPATCH_PARAM, NAME, CTX_TYPE)) {            \
        LOCK(ctx);                                                  \
        switch (self->tag) {                                        \
            VARIANTS(SUM__DISPATCH_CASE, NAME, CTX_TYPE)            \
            case NAME##_TAG__EMPTY: default: SUM__ON_BAD_TAG_VOID(self); break; \
        }                                                           \
        UNLOCK(ctx);                                                \
    }

/* RET_CTX は「戻り値型, ctx 型」を括弧で包んだ組。X-Macro の EXTRA 引数は1つしか
 * 通せないため、2つの型を1引数に束ねて各アスペクトマクロで展開する。 */
#define SUM__RC_RET(RET_CTX) SUM__RC_RET_ RET_CTX
#define SUM__RC_RET_(RET, CTX) RET
#define SUM__RC_CTX(RET_CTX) SUM__RC_CTX_ RET_CTX
#define SUM__RC_CTX_(RET, CTX) CTX

#define SUM__DISPATCH_RET_PARAM(NAME, RET_CTX, TAG, TYPE)           \
    , SUM__RC_RET(RET_CTX) (*NAME##_on_##TAG)(TYPE *, SUM__RC_CTX(RET_CTX) *)

#define SUM__DISPATCH_RET_CASE(NAME, RET_CTX, TAG, TYPE)            \
    case NAME##_TAG_##TAG:                                          \
        sum__ret_ = NAME##_on_##TAG(&self->as.TAG, ctx);            \
        break;

#define SUM__DISPATCH_RET_IMPL(NAME, VARIANTS, FN, CTX_TYPE, RET_TYPE, LOCK, UNLOCK) \
    SUM__MAYBE_UNUSED SUM__NONNULL                                  \
    static inline RET_TYPE FN(NAME *self, CTX_TYPE *ctx             \
        VARIANTS(SUM__DISPATCH_RET_PARAM, NAME, (RET_TYPE, CTX_TYPE))) { \
        RET_TYPE sum__ret_;                                         \
        LOCK(ctx);                                                  \
        switch (self->tag) {                                        \
            VARIANTS(SUM__DISPATCH_RET_CASE, NAME, (RET_TYPE, CTX_TYPE)) \
            case NAME##_TAG__EMPTY: default: SUM__ON_BAD_TAG_VALUE(self); \
        }                                                           \
        UNLOCK(ctx);                                                \
        return sum__ret_;                                           \
    }

/* ロックは SUM_CTX_LOCK / SUM_CTX_UNLOCK フック */
#define SUM_DEFINE_DISPATCH(NAME, VARIANTS, FN, CTX_TYPE)           \
    SUM__DISPATCH_IMPL(NAME, VARIANTS, FN, CTX_TYPE, SUM_CTX_LOCK, SUM_CTX_UNLOCK)

#define SUM_DEFINE_DISPATCH_RET(NAME, VARIANTS, FN, CTX_TYPE, RET_TYPE) \
    SUM__DISPATCH_RET_IMPL(NAME, VARIANTS, FN, CTX_TYPE, RET_TYPE,  \
                           SUM_CTX_LOCK, SUM_CTX_UNLOCK)

/* ロックを定義箇所で明示する。LOCK / UNLOCK は ctx を1引数に取る関数か関数形式マクロ。
 * ロック不要なら SUM_NO_LOCK を渡す。 */
#define SUM_DEFINE_DISPATCH_LOCKED(NAME, VARIANTS, FN, CTX_TYPE, LOCK, UNLOCK) \
    SUM__DISPATCH_IMPL(NAME, VARIANTS, FN, CTX_TYPE, LOCK, UNLOCK)

#define SUM_DEFINE_DISPATCH_RET_LOCKED(NAME, VARIANTS, FN, CTX_TYPE, RET_TYPE, LOCK, UNLOCK) \
    SUM__DISPATCH_RET_IMPL(NAME, VARIANTS, FN, CTX_TYPE, RET_TYPE, LOCK, UNLOCK)

/* ======================================================================
 * SUM_DEFINE_DESTROY（docs/adr/0004, 0013）
 *
 * 空状態（NAME_TAG__EMPTY）は何もせず返る。解放後は空状態に戻すため、同じ値への
 * 2回目の呼び出しは no-op になる（値をコピーした別変数には効かない。C-03）。
 * ====================================================================== */

#define SUM__DESTROY_PARAM(NAME, EXTRA, TAG, TYPE)                  \
    , void (*NAME##_on_##TAG)(TYPE *)

#define SUM__DESTROY_CASE(NAME, EXTRA, TAG, TYPE)                   \
    case NAME##_TAG_##TAG: NAME##_on_##TAG(&self->as.TAG); break;

#define SUM_DEFINE_DESTROY(NAME, VARIANTS, DESTROY_FN)              \
    SUM__MAYBE_UNUSED SUM__NONNULL                                  \
    static inline void DESTROY_FN(NAME *self                        \
        VARIANTS(SUM__DESTROY_PARAM, NAME, _)) {                    \
        switch (self->tag) {                                        \
            case NAME##_TAG__EMPTY: return;                         \
            VARIANTS(SUM__DESTROY_CASE, NAME, _)                    \
            default: SUM__ON_BAD_TAG_VOID(self); return;            \
        }                                                           \
        self->tag = NAME##_TAG__EMPTY;                              \
    }

#define SUM_DEFINE_NOOP_DESTROY(FN_NAME, TYPE)                      \
    SUM__MAYBE_UNUSED SUM__NONNULL                                  \
    static inline void FN_NAME(TYPE *v) { (void)v; }

/* ======================================================================
 * SUM_DEFINE_COPY / SUM_DEFINE_COPY_CHECKED（docs/adr/0004, 0015）
 * ====================================================================== */

#define SUM__COPY_PARAM(NAME, EXTRA, TAG, TYPE)                     \
    , TYPE (*NAME##_copy_##TAG)(const TYPE *)

#define SUM__COPY_CASE(NAME, EXTRA, TAG, TYPE)                      \
    case NAME##_TAG_##TAG:                                          \
        return NAME##_new_##TAG(NAME##_copy_##TAG(&self->as.TAG));

#define SUM_DEFINE_COPY(NAME, VARIANTS, COPY_FN)                    \
    SUM__MAYBE_UNUSED SUM__NONNULL                                  \
    static inline NAME COPY_FN(const NAME *self                     \
        VARIANTS(SUM__COPY_PARAM, NAME, _)) {                       \
        switch (self->tag) {                                        \
            VARIANTS(SUM__COPY_CASE, NAME, _)                       \
            case NAME##_TAG__EMPTY: default: SUM__ON_BAD_TAG_VALUE(self); \
        }                                                           \
    }

#define SUM_DEFINE_IDENTITY_COPY(FN_NAME, TYPE)                     \
    SUM__MAYBE_UNUSED SUM__NONNULL                                  \
    static inline TYPE FN_NAME(const TYPE *v) { return *v; }

/* ハンドラは 0 で成功、非 0 で失敗を返す。失敗時は *dst を変更せず、その値を返す。
 * 部分的に確保した資源の後始末はハンドラの責務。 */
#define SUM__COPY_CHECKED_PARAM(NAME, EXTRA, TAG, TYPE)             \
    , int (*NAME##_copy_##TAG)(TYPE *, const TYPE *)

#define SUM__COPY_CHECKED_CASE(NAME, EXTRA, TAG, TYPE)              \
    case NAME##_TAG_##TAG:                                          \
        sum__tmp_.tag = NAME##_TAG_##TAG;                           \
        sum__rc_ = NAME##_copy_##TAG(&sum__tmp_.as.TAG, &src->as.TAG); \
        break;

#define SUM_DEFINE_COPY_CHECKED(NAME, VARIANTS, COPY_FN)            \
    SUM__MAYBE_UNUSED SUM__NONNULL                                  \
    static inline int COPY_FN(NAME *dst, const NAME *src            \
        VARIANTS(SUM__COPY_CHECKED_PARAM, NAME, _)) {               \
        NAME sum__tmp_ = { .tag = NAME##_TAG__EMPTY };              \
        int sum__rc_;                                               \
        switch (src->tag) {                                         \
            VARIANTS(SUM__COPY_CHECKED_CASE, NAME, _)               \
            case NAME##_TAG__EMPTY: default: SUM__ON_BAD_TAG_VALUE(src); \
        }                                                           \
        if (sum__rc_ == 0) { *dst = sum__tmp_; }                    \
        return sum__rc_;                                            \
    }

/* ======================================================================
 * SUM_DEFINE_NEW_GENERIC / SUM_NEW（C11 以降。docs/adr/0008）
 *
 * _Generic の連想リストは末尾コンマを許さないため、各エントリを先頭コンマ形式で
 * 展開し、先頭に専用のプレースホルダ型を1つ置く。probe 関数がその場で _Generic を
 * 展開するため、ペイロード型の重複は SUM_NEW を呼ばなくても定義時点で検出される。
 * ====================================================================== */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

#define SUM__GENERIC_ASSOC(NAME, EXTRA, TAG, TYPE)                  \
    , TYPE: NAME##_new_##TAG

#define SUM_DEFINE_NEW_GENERIC(NAME, VARIANTS)                      \
    struct NAME##_GenericPlaceholder_ { char sum__unused_; };       \
    SUM__MAYBE_UNUSED                                               \
    static inline NAME NAME##_new_unreachable_(                     \
        struct NAME##_GenericPlaceholder_ sum__unused_arg_) {       \
        NAME sum__empty_ = { .tag = NAME##_TAG__EMPTY };            \
        (void)sum__unused_arg_;                                     \
        return sum__empty_;                                         \
    }                                                               \
    SUM__MAYBE_UNUSED                                               \
    static inline void NAME##_assert_distinct_payloads_(void) {     \
        struct NAME##_GenericPlaceholder_ sum__probe_ = { 0 };      \
        (void)SUM_NEW(NAME, VARIANTS, sum__probe_);                 \
    }

#define SUM_NEW(NAME, VARIANTS, x) _Generic((x),                    \
    struct NAME##_GenericPlaceholder_: NAME##_new_unreachable_      \
    VARIANTS(SUM__GENERIC_ASSOC, NAME, ())                          \
)(x)

#endif /* __STDC_VERSION__ >= 201112L */

/* ======================================================================
 * v1 名の別名（非推奨。docs/adr/0012、CHANGELOG.md）
 * ====================================================================== */
#ifndef SUM_NO_LEGACY_NAMES
#define DEFINE_SUM_TYPE           SUM_DEFINE_TYPE
#define DEFINE_SUM_MATCH          SUM_DEFINE_MATCH
#define DEFINE_SUM_MATCH_CONST    SUM_DEFINE_MATCH_CONST
#define DEFINE_SUM_DISPATCH       SUM_DEFINE_DISPATCH
#define DEFINE_SUM_DESTROY        SUM_DEFINE_DESTROY
#define DEFINE_SUM_COPY           SUM_DEFINE_COPY
#define DEFINE_EITHER_HELPERS     SUM_DEFINE_EITHER_HELPERS
#define DEFINE_RESULT_HELPERS     SUM_DEFINE_RESULT_HELPERS
#define SUM_MAYBE_UNUSED          SUM__MAYBE_UNUSED
#define SUM_UNREACHABLE           SUM__UNREACHABLE
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define DEFINE_SUM_NEW_GENERIC    SUM_DEFINE_NEW_GENERIC
#endif
#endif /* SUM_NO_LEGACY_NAMES */

#endif /* GENERIC_SUM_TYPE_H */
