/*
 * generic_sum_type.h
 *
 * N分岐の直和型（タグ付き共用体）をコンパイル時に生成するジェネリックマクロライブラリ。
 * C にはパラメトリック多相がないため、「型パラメータの解決」をプリプロセッサの
 * X-Macro展開によってコンパイル時に行う（C++テンプレートの手動再現に相当）。
 *
 * 対応C標準:     C99以降（static inline、指示付き初期化を使う）
 * 対応コンパイラ: GCC / Clang（MSVCは対象外。docs/adr/0007-msvc-out-of-scope.md）
 * 必須ビルドフラグ: -Wall -Wextra -Werror
 *     -Werror を外すと順序取り違えの検出が警告止まりになり、このライブラリの
 *     価値の半分が失われる。docs/GUARANTEES.md 参照。
 *
 * --- ドキュメント ---
 *
 *   何を解くライブラリか・採用条件  docs/requirements.md
 *   シグネチャと使い方              api_reference.md
 *   コンパイル時に何が保証されるか  docs/GUARANTEES.md（spec/ からの生成物）
 *   できないことの一覧              docs/constraints.md
 *   なぜそう設計したか              docs/adr/
 *
 * このコメントは点で引くための要約に留める。制約や判断根拠を二重管理しないため、
 * 詳細は上記の文書を単一のソースとする。
 *
 * --- 使い方の要約 ---
 *
 *   1. 型リストをX-Macroとして1箇所だけで定義する（Rustのenum variant一覧に相当）。
 *      NAME, EXTRA は生成マクロ側から渡される「文脈引数」で、リスト定義側は
 *      素通しするだけでよい。
 *
 *        #define MY_VARIANTS(X, NAME, EXTRA) \
 *            X(NAME, EXTRA, tag1, Type1)      \
 *            X(NAME, EXTRA, tag2, Type2)
 *
 *      ペイロード型は variant 間で重複させないこと（docs/constraints.md C-01）。
 *      たとえ1フィールドでも専用structでラップするのが推奨手段。
 *
 *   2. DEFINE_SUM_TYPE(MyType, MY_VARIANTS)
 *        -> MyType_tag_t, MyType, MyType_new_<tag>(),
 *           MyType_get_<tag>(), MyType_get_<tag>_const() を生成
 *
 *   3. 用途に応じてディスパッチャを生成する。
 *
 *        DEFINE_SUM_MATCH        値を返す純粋変換          RetType (*)(Type*)
 *        DEFINE_SUM_MATCH_CONST  同上のread-only版         RetType (*)(Type const*)
 *        DEFINE_SUM_DISPATCH     ctxを書き換える副作用     void (*)(Type*, Ctx*)
 *        DEFINE_SUM_DESTROY      資源の解放                void (*)(Type*)
 *        DEFINE_SUM_COPY         ディープコピー            Type (*)(const Type*)
 *
 *      DEFINE_SUM_MATCH に RetType=void を渡してはならない（C-08）。void用途は
 *      DEFINE_SUM_DISPATCH か DEFINE_SUM_DESTROY を使う。
 *
 * --- 網羅性検査の仕組み（このライブラリの中核） ---
 *
 * 生成される各ディスパッチ関数の引数リストは VARIANTS から自動生成されるため、
 * 引数の個数は「その時点でのvariant数」と常に一致する。したがって variant を
 * 1個追加すると必須引数も1個増え、渡し忘れは「too few arguments to function」
 * という通常の関数プロトタイプ検査としてコンパイルが失敗する。
 * つまり網羅性は -Wswitch のようなビルドフラグの運用ではなく、C言語標準の
 * 引数チェックだけで強制される。
 *
 * さらに各ハンドラの型は variant ごとのペイロード型で固有になるため、ペイロード型が
 * 異なれば渡す順序を間違えても -Wincompatible-pointer-types で検出できる
 * （名前的型付けの利用。-Werror 必須）。検出できないのは2つ以上のvariantが
 * 互換な型を共有している場合のみ（C-01）。
 *
 * switch文に default: を置いていないのは、tagを手で拡張された場合に -Wswitch を
 * 働かせるため。不正tagの検知は default: ではなく SUM_UNHANDLED_TAG フックで行う
 * （docs/adr/0010-unhandled-tag-hook.md）。
 *
 * --- 差し込めるフック ---
 *
 *   SUM_CTX_LOCK / SUM_CTX_UNLOCK   DEFINE_SUM_DISPATCH のロック区間。既定 no-op
 *   SUM_UNHANDLED_TAG               不正tagの検知。既定 no-op
 *
 * いずれも本ヘッダを include する前に再定義する。ロック区間はハンドラ本体全体を
 * 包むため、同一ctxへの再dispatchは非再帰ロックでデッドロックする（C-09）。
 *
 * --- 未使用関数の警告について ---
 *
 * 生成された全関数を毎回使い切るとは限らない（コンストラクタとdispatchだけ使い
 * ゲッターは使わない、はごく普通）。GCCは未使用のstatic inline関数を警告しないが、
 * Clangは -Wunused-function で警告するため、生成する全関数に SUM_MAYBE_UNUSED を
 * 付与している。
 */
#ifndef GENERIC_SUM_TYPE_H
#define GENERIC_SUM_TYPE_H

#include <stddef.h> /* NULL */

#if defined(__GNUC__) || defined(__clang__)
#define SUM_UNREACHABLE() __builtin_unreachable()
#define SUM_MAYBE_UNUSED __attribute__((unused))
#else
#include <stdlib.h>
#define SUM_UNREACHABLE() abort()
#define SUM_MAYBE_UNUSED
#endif

#ifndef SUM_CTX_LOCK
#define SUM_CTX_LOCK(ctx)   ((void)0)
#endif
#ifndef SUM_CTX_UNLOCK
#define SUM_CTX_UNLOCK(ctx) ((void)0)
#endif

/* 不正なtag（マクロ経由以外でtagを破壊された等）でディスパッチされたときに呼ばれるフック。
 * デフォルトは no-op。開発時は assert 等を差し込んで早期に検知できる。
 *
 *   #define SUM_UNHANDLED_TAG(self) assert(!"generic_sum_type: unhandled tag")
 *   #include "generic_sum_type.h"
 *
 * 全5ディスパッチャ（MATCH / MATCH_CONST / DISPATCH / DESTROY / COPY）が、
 * どのcaseにも一致しなかった場合にこのフックを呼ぶ。値を返す3つ
 * （MATCH / MATCH_CONST / COPY）はフック呼び出し後に SUM_UNREACHABLE() へ進む
 * （戻り値を用意できないため他に手がない）。void を返す2つ
 * （DISPATCH / DESTROY）はフックを呼んで正常に復帰する。
 * この非対称は「戻り値の有無」から導かれる帰結であり、意図的なものである
 * （docs/adr/0010-unhandled-tag-hook.md）。 */
#ifndef SUM_UNHANDLED_TAG
#define SUM_UNHANDLED_TAG(self) ((void)(self))
#endif

/* ---- DEFINE_SUM_TYPE が使うアスペクトマクロ（1マクロ = 1関心事） ---- */

#define SUM_TAG_ENTRY(NAME, EXTRA, TAG, TYPE)    NAME##_##TAG,
#define SUM_UNION_MEMBER(NAME, EXTRA, TAG, TYPE) TYPE TAG;

#define SUM_CTOR(NAME, EXTRA, TAG, TYPE)                             \
    SUM_MAYBE_UNUSED                                                 \
    static inline NAME NAME##_new_##TAG(TYPE v) {                    \
        return (NAME){ .tag = NAME##_##TAG, .as.TAG = v };           \
    }

#define SUM_GETTER(NAME, EXTRA, TAG, TYPE)                           \
    SUM_MAYBE_UNUSED                                                 \
    static inline TYPE *NAME##_get_##TAG(NAME *self) {               \
        return self->tag == NAME##_##TAG ? &self->as.TAG : NULL;     \
    }

/* read-only 版ゲッター。const NAME* から呼べ、payload オブジェクト自体を const 化した
 * ポインタ（Type const*）を返す（docs/adr/0009-non-breaking-const-support.md）。可変ゲッター NAME_get_<tag> と併存する。
 * 戻り型に west const（const TYPE*）ではなく east const（TYPE const*）を使うのは、
 * TYPE がポインタ型（例: const char*）のとき west const だと const が指す先に付いて
 * しまい &self->as.TAG（const NAME* 経由 = Type const*）と型不一致になるため。 */
#define SUM_GETTER_CONST(NAME, EXTRA, TAG, TYPE)                     \
    SUM_MAYBE_UNUSED                                                 \
    static inline TYPE const *NAME##_get_##TAG##_const(             \
        const NAME *self) {                                          \
        return self->tag == NAME##_##TAG ? &self->as.TAG : NULL;     \
    }

#define DEFINE_SUM_TYPE(NAME, VARIANTS)                              \
    typedef enum { VARIANTS(SUM_TAG_ENTRY, NAME, _) } NAME##_tag_t;  \
    typedef struct {                                                  \
        NAME##_tag_t tag;                                             \
        union { VARIANTS(SUM_UNION_MEMBER, NAME, _) } as;             \
    } NAME;                                                            \
    VARIANTS(SUM_CTOR, NAME, _)                                        \
    VARIANTS(SUM_GETTER, NAME, _)                                      \
    VARIANTS(SUM_GETTER_CONST, NAME, _)

/* ---- tag 述語ジェネレータ ----
 * 「今この SumType はどの variant か」を問う述語 NAME_is_<TAG>() を1個生成する。
 * Either(left/right) と Result(ok/err) のヘルパはどちらもこれの2回呼び出しに
 * すぎない。tag 名以外に差が無いものを別々の生成ロジックとして持つと、コアの
 * 変更を二重に追従する保守コストが生じるため1箇所にまとめている
 * （docs/adr/0005-either-result-thin-helpers.md と同じ理由）。
 * left/right, ok/err 以外の2択（pending/done 等）にもそのまま使える。 */
#define SUM_DEFINE_PREDICATE(NAME, TAG)                             \
    SUM_MAYBE_UNUSED                                                \
    static inline int NAME##_is_##TAG(const NAME *self) {          \
        return self->tag == NAME##_##TAG;                          \
    }

/* ---- Either イディオム用ヘルパ ----
 * left / right の2 variant を持つ SumType（Either 相当）向けの述語を生成する。
 * DEFINE_SUM_TYPE で tag 名を left / right として定義したうえで本マクロを呼ぶ。
 * Either の使い方: 構築は NAME_new_left / NAME_new_right、取り出しは
 * NAME_get_left / NAME_get_right（read-only は _const 版）、左右の畳み込み(fold)は
 * DEFINE_SUM_MATCH_CONST（左右2ハンドラ）、そして「今どちらか」の述語が本マクロ。
 * left/right 以外の tag 名で定義した SumType に本マクロを使うと、NAME_left /
 * NAME_right という enum 定数が存在せずコンパイルエラーになる（規約違反が
 * そのまま検出される）。 */
#define DEFINE_EITHER_HELPERS(NAME)                                 \
    SUM_DEFINE_PREDICATE(NAME, left)                                \
    SUM_DEFINE_PREDICATE(NAME, right)

/* ---- Result イディオム用ヘルパ ----
 * ok / err の2 variant を持つ SumType（Result<T,E> 相当）向けの述語を生成する。
 * Either（left/right）の ok/err 版。DEFINE_SUM_TYPE で tag 名を ok / err として
 * 定義したうえで本マクロを呼ぶ。構築は NAME_new_ok / NAME_new_err、取り出しは
 * NAME_get_ok / NAME_get_err（read-only は _const 版）、fold は
 * DEFINE_SUM_MATCH_CONST（ok/err 2ハンドラ）。
 * 注意: map / and_then のような「payload 型が変わる」関数合成(ROP)は、変換先が
 * 別の具体 Result 型になるため C では汎用化できない（docs/constraints.md C-13）。
 * 同型変換や、目標型を明示した手書きの合成は可能。 */
#define DEFINE_RESULT_HELPERS(NAME)                                 \
    SUM_DEFINE_PREDICATE(NAME, ok)                                  \
    SUM_DEFINE_PREDICATE(NAME, err)

/* ---- DEFINE_SUM_MATCH が使うアスペクトマクロ ---- */

#define SUM_MATCH_PARAM(NAME, RET_TYPE, TAG, TYPE)                   \
    , RET_TYPE (*NAME##_on_##TAG)(TYPE *)

#define SUM_MATCH_CASE(NAME, RET_TYPE, TAG, TYPE)                    \
    case NAME##_##TAG: return NAME##_on_##TAG(&self->as.TAG);

#define DEFINE_SUM_MATCH(NAME, VARIANTS, MATCH_FN, RET_TYPE)                \
    SUM_MAYBE_UNUSED                                                        \
    static inline RET_TYPE MATCH_FN(NAME *self                             \
        VARIANTS(SUM_MATCH_PARAM, NAME, RET_TYPE)) {                        \
        switch (self->tag) {                                                \
            VARIANTS(SUM_MATCH_CASE, NAME, RET_TYPE)                        \
        }                                                                    \
        SUM_UNHANDLED_TAG(self);                                             \
        SUM_UNREACHABLE();                                                   \
    }

/* ---- DEFINE_SUM_MATCH_CONST が使うアスペクトマクロ ----
 * DEFINE_SUM_MATCH の read-only 版。self を const NAME* で受け、各ハンドラも
 * payload を Type const*（east const）で受け取る。面積計算・分類・整形など
 * payload を書き換えない純粋な変換を、const NAME* を保持したまま呼びたい場合に使う。
 * payload を書き換える用途では従来の DEFINE_SUM_MATCH（可変版）を使うこと。
 * east const を使う理由は SUM_GETTER_CONST と同じ（TYPE がポインタ型でも
 * payload オブジェクト自体に const を付けるため）。
 * 網羅性検査・順序取り違え検出は可変版と同一の仕組みで働く（docs/GUARANTEES.md G-001, G-002）。
 */

#define SUM_MATCH_CONST_PARAM(NAME, RET_TYPE, TAG, TYPE)             \
    , RET_TYPE (*NAME##_on_##TAG)(TYPE const *)

#define SUM_MATCH_CONST_CASE(NAME, RET_TYPE, TAG, TYPE)              \
    case NAME##_##TAG: return NAME##_on_##TAG(&self->as.TAG);

#define DEFINE_SUM_MATCH_CONST(NAME, VARIANTS, MATCH_FN, RET_TYPE)          \
    SUM_MAYBE_UNUSED                                                        \
    static inline RET_TYPE MATCH_FN(const NAME *self                       \
        VARIANTS(SUM_MATCH_CONST_PARAM, NAME, RET_TYPE)) {                  \
        switch (self->tag) {                                                \
            VARIANTS(SUM_MATCH_CONST_CASE, NAME, RET_TYPE)                  \
        }                                                                    \
        SUM_UNHANDLED_TAG(self);                                             \
        SUM_UNREACHABLE();                                                   \
    }

/* ---- DEFINE_SUM_DISPATCH が使うアスペクトマクロ ----
 * DEFINE_SUM_MATCH との違いは、各ハンドラが payload に加えて
 * 共有コンテキスト(ctx)へのポインタも受け取る点。コマンド実行のような
 * 「副作用を伴うディスパッチ」を表現するために分けている。
 */

#define SUM_DISPATCH_PARAM(NAME, CTX_TYPE, TAG, TYPE)                \
    , void (*NAME##_on_##TAG)(TYPE *, CTX_TYPE *)

#define SUM_DISPATCH_CASE(NAME, CTX_TYPE, TAG, TYPE)                 \
    case NAME##_##TAG:                                                \
        NAME##_on_##TAG(&self->as.TAG, ctx);                          \
        sum_handled_ = 1;                                             \
        break;

/* 不正tag時に SUM_UNHANDLED_TAG を呼ぶために sum_handled_ フラグを使う。
 * switch に default: を置けば同じことができるが、それをすると -Wswitch による
 * 「enum定数を手で増やした場合の網羅性警告」が無効化されてしまうため採らない。
 * ローカルフラグはフックが no-op のとき最適化で完全に消える
 * （docs/adr/0010-unhandled-tag-hook.md）。 */
#define DEFINE_SUM_DISPATCH(NAME, VARIANTS, DISPATCH_FN, CTX_TYPE)         \
    SUM_MAYBE_UNUSED                                                       \
    static inline void DISPATCH_FN(NAME *self, CTX_TYPE *ctx              \
        VARIANTS(SUM_DISPATCH_PARAM, NAME, CTX_TYPE)) {                    \
        int sum_handled_ = 0;                                              \
        SUM_CTX_LOCK(ctx);                                                  \
        switch (self->tag) {                                               \
            VARIANTS(SUM_DISPATCH_CASE, NAME, CTX_TYPE)                    \
        }                                                                   \
        if (!sum_handled_) { SUM_UNHANDLED_TAG(self); }                    \
        SUM_CTX_UNLOCK(ctx);                                                \
    }

/* ---- DEFINE_SUM_DESTROY が使うアスペクトマクロ ----
 * ポインタ資源（heap確保された文字列・バッファ等）を持つvariantを解放するための
 * デストラクタディスパッチ。ctxを取らない点、戻り値がvoid固定である点がDISPATCHと異なる。
 * 資源を持たないvariantにはSUM_DEFINE_NOOP_DESTROYで生成した関数を渡す。
 */

#define SUM_DESTROY_PARAM(NAME, EXTRA, TAG, TYPE)                    \
    , void (*NAME##_on_##TAG)(TYPE *)

#define SUM_DESTROY_CASE(NAME, EXTRA, TAG, TYPE)                     \
    case NAME##_##TAG: NAME##_on_##TAG(&self->as.TAG); return;

#define DEFINE_SUM_DESTROY(NAME, VARIANTS, DESTROY_FN)                     \
    SUM_MAYBE_UNUSED                                                       \
    static inline void DESTROY_FN(NAME *self                              \
        VARIANTS(SUM_DESTROY_PARAM, NAME, _)) {                            \
        switch (self->tag) {                                               \
            VARIANTS(SUM_DESTROY_CASE, NAME, _)                            \
        }                                                                   \
        /* 各caseは return; で抜けるため、ここに到達＝不正tag */             \
        SUM_UNHANDLED_TAG(self);                                            \
    }

/* 資源を持たないvariant向けの「何もしないデストラクタ」を生成するヘルパー */
#define SUM_DEFINE_NOOP_DESTROY(FN_NAME, TYPE)                       \
    SUM_MAYBE_UNUSED                                                 \
    static inline void FN_NAME(TYPE *v) { (void)v; }

/* ---- DEFINE_SUM_COPY が使うアスペクトマクロ ----
 * ポインタ資源を持つvariantのディープコピー（例: strdup）を行うためのコピーディスパッチ。
 * 値そのものを複製するだけでよいvariantにはSUM_DEFINE_IDENTITY_COPYで生成した関数を渡す。
 */

#define SUM_COPY_PARAM(NAME, EXTRA, TAG, TYPE)                       \
    , TYPE (*NAME##_copy_##TAG)(const TYPE *)

#define SUM_COPY_CASE(NAME, EXTRA, TAG, TYPE)                        \
    case NAME##_##TAG:                                                \
        return (NAME){ .tag = NAME##_##TAG,                           \
                        .as.TAG = NAME##_copy_##TAG(&self->as.TAG) };

#define DEFINE_SUM_COPY(NAME, VARIANTS, COPY_FN)                           \
    SUM_MAYBE_UNUSED                                                       \
    static inline NAME COPY_FN(const NAME *self                           \
        VARIANTS(SUM_COPY_PARAM, NAME, _)) {                               \
        switch (self->tag) {                                               \
            VARIANTS(SUM_COPY_CASE, NAME, _)                               \
        }                                                                   \
        SUM_UNHANDLED_TAG(self);                                            \
        SUM_UNREACHABLE();                                                  \
    }

/* 値そのものを複製するだけでよいvariant向けの「浅いコピー」を生成するヘルパー */
#define SUM_DEFINE_IDENTITY_COPY(FN_NAME, TYPE)                      \
    SUM_MAYBE_UNUSED                                                 \
    static inline TYPE FN_NAME(const TYPE *v) { return *v; }

/* ---- DEFINE_SUM_NEW_GENERIC / SUM_NEW が使うアスペクトマクロ（C11以降限定） ----
 * ヘッダ冒頭のコメント「DEFINE_SUM_NEW_GENERIC / SUM_NEW について」を参照。
 * C99ビルドではこのブロック全体が存在しない扱いになる。
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

#define SUM_GENERIC_ASSOC(NAME, EXTRA, TAG, TYPE) \
    , TYPE: NAME##_new_##TAG

/* _Genericの連想リストは末尾コンマを許さない文法のため（実機検証済み）、
 * VARIANTSからのX-Macro展開は「各エントリが先頭コンマを持つ」形にしている。
 * リストの先頭にはライブラリ専用のプレースホルダ型を1個だけ手動で置き、
 * 帳尻を合わせる。このプレースホルダ型はユーザーの型と衝突しない専用の
 * 匿名でない構造体であり、実際に選択されることは想定していない
 * （選択された場合は0初期化した値を返すだけで、クラッシュはしない）。
 */
#define DEFINE_SUM_NEW_GENERIC(NAME, VARIANTS)                              \
    struct NAME##_GenericPlaceholder_ { char sum_generic_unused_; };        \
    SUM_MAYBE_UNUSED                                                        \
    static inline NAME NAME##_new_unreachable_(                            \
        struct NAME##_GenericPlaceholder_ sum_generic_unused_arg_) {        \
        (void)sum_generic_unused_arg_;                                      \
        NAME sum_generic_zeroed_ = { 0 };                                   \
        return sum_generic_zeroed_;                                        \
    }                                                                       \
    SUM_MAYBE_UNUSED                                                        \
    static inline void NAME##_assert_distinct_payloads_(void) {            \
        struct NAME##_GenericPlaceholder_ sum_probe_ = { 0 };               \
        (void)SUM_NEW(NAME, VARIANTS, sum_probe_);                          \
    }

/* 呼び出し側はこの形で使う: SUM_NEW(NAME, VARIANTS, x)
 * NAMEとVARIANTSを毎回明示するのは、マクロ展開結果から新しい#defineを
 * 生成する手段がCのプリプロセッサに存在しないための制約であり、
 * DEFINE_SUM_TYPE等のように「1回呼べばNAME_new(x)という専用の名前が
 * 使えるようになる」形にはできない（docs/adr/0008-generic-constructor-selection.md）。
 */
#define SUM_NEW(NAME, VARIANTS, x) _Generic((x),                            \
    struct NAME##_GenericPlaceholder_: NAME##_new_unreachable_              \
    VARIANTS(SUM_GENERIC_ASSOC, NAME, ())                                   \
)(x)

#endif /* __STDC_VERSION__ >= 201112L */

#endif /* GENERIC_SUM_TYPE_H */
