/*
 * generic_sum_type.h
 *
 * N分岐の直和型（タグ付き共用体）をコンパイル時に生成するジェネリックマクロライブラリ。
 * C にはパラメトリック多相がないため、「型パラメータの解決」をプリプロセッサの
 * X-Macro展開によってコンパイル時に行う（C++テンプレートの手動再現に相当）。
 *
 * 対応C標準: C99以降
 * 対応コンパイラ: GCC / Clang（設計仕様書 design_spec.md を参照。MSVCは対象外とする）
 *
 * --- 使い方 ---
 *
 *   1. 型リストをX-Macroとして1箇所だけで定義する（Rustのenum variant一覧に相当）。
 *      NAME, EXTRA は生成マクロ側から渡される「文脈引数」で、リスト定義側は
 *      素通しするだけでよい（各DEFINE_SUM_*マクロから同じリストを再利用するために存在する）。
 *
 *        #define MY_VARIANTS(X, NAME, EXTRA) \
 *            X(NAME, EXTRA, tag1, Type1)      \
 *            X(NAME, EXTRA, tag2, Type2)      \
 *            ...
 *
 *   2. DEFINE_SUM_TYPE(MyType, MY_VARIANTS)
 *        -> MyType_tag_t（enum）, MyType（タグ付きunion構造体）,
 *           MyType_new_<tag>(), MyType_get_<tag>() を生成する。
 *
 *   3. DEFINE_SUM_MATCH(MyType, MY_VARIANTS, match_fn_name, RetType)
 *        -> match_fn_name(MyType *self, RetType (*on_tag1)(Type1*), ...) を生成する。
 *        1つのSumTypeに対して用途ごとに複数のmatch関数を、関数名を変えて複数回定義できる。
 *        各ハンドラは「payloadだけ」を受け取る純粋な変換関数を想定している。
 *        RetTypeにvoidを渡すのは非推奨（-pedantic環境でISO C違反の警告が出る。用途としては
 *        DEFINE_SUM_DISPATCHかDEFINE_SUM_DESTROYを使うこと）。
 *        read-only用途（payloadを書き換えない）で self を const NAME* のまま渡したい場合は
 *        DEFINE_SUM_MATCH_CONST を使う（ハンドラは Type1 const* を受け取る）。
 *        DEFINE_SUM_TYPE は可変ゲッター NAME_get_<tag>() に加え、const NAME* から
 *        呼べる NAME_get_<tag>_const()（Type const* を返す）も生成する。
 *
 *   4. DEFINE_SUM_DISPATCH(MyType, MY_VARIANTS, dispatch_fn_name, CtxType)
 *        -> dispatch_fn_name(MyType *self, CtxType *ctx,
 *             void (*on_tag1)(Type1*, CtxType*), ...) を生成する。
 *        コマンドディスパッチのように「payloadに加えて共有状態(ctx)を書き換える副作用」を
 *        伴うハンドラ向け。戻り値は常にvoidに固定している。
 *        SUM_CTX_LOCK/SUM_CTX_UNLOCK フック（後述）でロック区間を挟める。
 *
 *   5. DEFINE_SUM_DESTROY(MyType, MY_VARIANTS, destroy_fn_name)
 *        -> destroy_fn_name(MyType *self, void (*on_tag1)(Type1*), ...) を生成する。
 *        ポインタ資源（heap確保された文字列やバッファなど）を持つvariantを解放するための
 *        デストラクタディスパッチ。資源を持たないvariantにはSUM_DEFINE_NOOP_DESTROYで
 *        生成した何もしない関数を渡せばよい。
 *
 *   6. DEFINE_SUM_COPY(MyType, MY_VARIANTS, copy_fn_name)
 *        -> copy_fn_name(const MyType *self, Type1 (*copy_tag1)(const Type1*), ...) -> MyType
 *        を生成する。ポインタ資源を持つvariantのディープコピー（例: strdup）を行うための
 *        コピーディスパッチ。値そのものを複製するだけでよいvariantにはSUM_DEFINE_IDENTITY_COPY
 *        で生成した関数を渡せばよい。
 *
 * --- 網羅性検査の仕組み ---
 *
 * 生成される各ディスパッチ関数の引数リストは VARIANTS リストから自動生成されるため、
 * 引数の個数は「その時点でのvariant数」と常に一致する。したがって：
 *
 *   - variant を1個追加すると、生成される関数の必須引数も1個増える。
 *   - 既存の呼び出し側がハンドラを1個渡し忘れていれば、
 *     「too few arguments to function」という通常の関数呼び出しの
 *     型検査エラーとしてコンパイルが失敗する。
 *
 * つまり網羅性は "-Wswitch を有効にする" といったビルドフラグの運用に頼るのではなく、
 * 通常のC言語の関数プロトタイプの引数チェックだけで強制される。
 *
 * さらに、各ハンドラの型は variant ごとのペイロード型で固有になっているため、
 * 2つのvariantのペイロード型が異なっていれば、渡す順序を間違えても
 * -Wincompatible-pointer-types（-Werror下ではコンパイルエラー）で検出できる
 * （構造的に同一レイアウトでも別のtypedefであれば別の型として扱われるC言語の
 *   名前的型付けを利用している）。検出できないのは2つ以上のvariantが文字通り
 * 同一の型をペイロードに使っている場合のみであり、これを避けるため
 * 「ペイロードに素の基本型を直接使わず専用structでラップする」ことを設計ガイドラインとする。
 *
 * switch文自体には default: を置いていない。これは *_tag_t を誰かが
 * マクロ経由以外で直接いじった場合の保険であり、-Wswitch (通常 -Wall に含まれる)
 * を有効にしていれば、その場合も未処理ケースが警告される。
 *
 * --- スレッドセーフティについて ---
 *
 * 本ライブラリ自体は特定のロックプリミティブに依存しない。RTOS（FreeRTOSのセマフォ等）と
 * Linux（pthread_mutex等）では利用できるロック機構が異なるため、環境依存のロック実装を
 * ライブラリ側に埋め込むと可搬性を損なう。そこで、DEFINE_SUM_DISPATCHが生成する関数は
 * 実行区間の前後で SUM_CTX_LOCK(ctx) / SUM_CTX_UNLOCK(ctx) を呼ぶようにしておき、
 * これらはデフォルトでは何もしない（no-op）マクロとして定義している。
 * ロックが必要な場合は、本ヘッダをincludeする前に利用側で以下のように再定義する。
 *
 *   #define SUM_CTX_LOCK(ctx)   pthread_mutex_lock(&(ctx)->mutex)
 *   #define SUM_CTX_UNLOCK(ctx) pthread_mutex_unlock(&(ctx)->mutex)
 *   #include "generic_sum_type.h"
 *
 * ロックが不要な用途では追加コストはゼロ（no-opはコンパイラが最適化で消す）。
 * どの粒度でロックするか（dispatch呼び出し単位か、もっと粗い単位か）は
 * アプリケーション側の責務であり、本ライブラリはフックの提供にとどめる。
 *
 * 【再入（recursive dispatch）によるデッドロックに注意】
 * ロック区間は SUM_CTX_LOCK(ctx) からハンドラ本体を含む switch 全体を経て
 * SUM_CTX_UNLOCK(ctx) までを1つの区間とする（ハンドラ実行中もロックを保持し続ける）。
 * このためハンドラが同一 ctx に対して直接／間接に別の dispatch を呼び返すと、
 * SUM_CTX_LOCK に非再帰ロック（pthread_mutexのデフォルト等）を差し込んでいる場合は
 * 自己デッドロックする。回避策は次のいずれか:
 *   - ハンドラ内で同一 ctx への再 dispatch を行わない（再 dispatch はロック区間の外、
 *     すなわち呼び出し元のスコープで発行する）。推奨。
 *   - 再入が避けられない設計では、SUM_CTX_LOCK に再帰対応プリミティブ
 *     （PTHREAD_MUTEX_RECURSIVE で初期化した mutex 等）を差し込む。
 * なお self->tag が不正な値でも switch はどの case にも入らず SUM_CTX_UNLOCK に到達するため、
 * LOCK/UNLOCK の対応は崩れない（ロック解放漏れは起きない）。詳細は design_spec.md 4.8節。
 *
 * --- DEFINE_SUM_NEW_GENERIC / SUM_NEW（C11以降限定のオプトイン機能）について ---
 *
 * C11の _Generic（ジェネリック選択）を使い、渡した値の「型」だけからコンストラクタを
 * 自動選択する糖衣構文。DEFINE_SUM_TYPEが生成する NAME_new_<tag>() を、タグ名を
 * 意識せず呼べるようにする。
 *
 *   DEFINE_SUM_NEW_GENERIC(MyType, MY_VARIANTS)   // 補助の型・関数を1回だけ生成
 *   MyType v = SUM_NEW(MyType, MY_VARIANTS, (Type1){ ... });
 *
 * 他のDEFINE_SUM_*と違い、NAME_new(x)のような専用マクロは生成できない。Cの
 * プリプロセッサはマクロ展開結果の中から新しい#defineディレクティブを起こす
 * 機能を持たないため（実機検証済み。design_spec.md 4.13節）、
 * DEFINE_SUM_NEW_GENERICが生成できるのは補助の型・関数のみであり、実際の
 * ディスパッチはNAMEとVARIANTSを毎回明示するSUM_NEWという単一の共通マクロが
 * 担う。
 *
 * 制約（design_spec.md 4.13節に検証結果を記載）:
 *   - C11以上必須。__STDC_VERSION__ でガードしており、C99ビルドでは
 *     DEFINE_SUM_NEW_GENERIC / SUM_NEW いずれも定義されない。
 *   - _Genericは制御式の型が完全一致しないと選択できない（暗黙の型変換に
 *     頼れない）。ポインタが指す先のconst有無も別の型として扱われる
 *     （char*とconst char*は別キー）。ペイロードを専用structでラップする
 *     設計ガイドライン（3節）に従っていれば問題にならない。
 *   - 2つ以上のvariantが同一のペイロード型を共有していると、_Genericの
 *     連想リストが重複し、コンパイルエラーになる。ただしこれは
 *     DEFINE_SUM_NEW_GENERIC単体を呼んだだけでは検出されない。
 *     DEFINE_SUM_NEW_GENERICは補助の型・関数を生成するだけで_Generic自体を
 *     まだ展開しないため、実際にSUM_NEW(NAME, VARIANTS, x)を呼んで
 *     _Genericが展開された箇所で初めて検出される（実機検証で確認済み。
 *     当初「DEFINE_SUM_NEW_GENERICを呼ぶだけで検出できる」と誤解していたが、
 *     テスト作成時に実際には検出されないことが分かり訂正した）。
 *
 * --- 未使用関数の警告について ---
 *
 * DEFINE_SUM_TYPE等が生成する各関数は「利用側が必要なものだけを使う」前提であり、
 * 生成された全関数を毎回使い切るとは限らない（例えばコンストラクタとdispatchだけ使い、
 * ゲッターは使わない、というのはごく普通の使い方である）。GCCは未使用のstatic inline関数を
 * 警告しないが、Clangは -Wunused-function で警告する（実機で確認済みの実在するコンパイラ間差異）。
 * そのため生成される全 static inline 関数に SUM_MAYBE_UNUSED を付与し、
 * 両コンパイラで意図せぬ警告が出ないようにしている。
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
 * ポインタ（Type const*）を返す（4.14節）。可変ゲッター NAME_get_<tag> と併存する。
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
    SUM_MAYBE_UNUSED                                                \
    static inline int NAME##_is_left(const NAME *self) {           \
        return self->tag == NAME##_left;                           \
    }                                                               \
    SUM_MAYBE_UNUSED                                                \
    static inline int NAME##_is_right(const NAME *self) {          \
        return self->tag == NAME##_right;                          \
    }

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
        SUM_UNREACHABLE();                                                   \
    }

/* ---- DEFINE_SUM_MATCH_CONST が使うアスペクトマクロ ----
 * DEFINE_SUM_MATCH の read-only 版。self を const NAME* で受け、各ハンドラも
 * payload を Type const*（east const）で受け取る。面積計算・分類・整形など
 * payload を書き換えない純粋な変換を、const NAME* を保持したまま呼びたい場合に使う。
 * payload を書き換える用途では従来の DEFINE_SUM_MATCH（可変版）を使うこと。
 * east const を使う理由は SUM_GETTER_CONST と同じ（TYPE がポインタ型でも
 * payload オブジェクト自体に const を付けるため）。
 * 網羅性検査・順序取り違え検出は可変版と同一の仕組みで働く（3節）。
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
    case NAME##_##TAG: NAME##_on_##TAG(&self->as.TAG, ctx); break;

#define DEFINE_SUM_DISPATCH(NAME, VARIANTS, DISPATCH_FN, CTX_TYPE)         \
    SUM_MAYBE_UNUSED                                                       \
    static inline void DISPATCH_FN(NAME *self, CTX_TYPE *ctx              \
        VARIANTS(SUM_DISPATCH_PARAM, NAME, CTX_TYPE)) {                    \
        SUM_CTX_LOCK(ctx);                                                  \
        switch (self->tag) {                                               \
            VARIANTS(SUM_DISPATCH_CASE, NAME, CTX_TYPE)                    \
        }                                                                   \
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
    }

/* 呼び出し側はこの形で使う: SUM_NEW(NAME, VARIANTS, x)
 * NAMEとVARIANTSを毎回明示するのは、マクロ展開結果から新しい#defineを
 * 生成する手段がCのプリプロセッサに存在しないための制約であり、
 * DEFINE_SUM_TYPE等のように「1回呼べばNAME_new(x)という専用の名前が
 * 使えるようになる」形にはできない（design_spec.md 4.13節）。
 */
#define SUM_NEW(NAME, VARIANTS, x) _Generic((x),                            \
    struct NAME##_GenericPlaceholder_: NAME##_new_unreachable_              \
    VARIANTS(SUM_GENERIC_ASSOC, NAME, ())                                   \
)(x)

#endif /* __STDC_VERSION__ >= 201112L */

#endif /* GENERIC_SUM_TYPE_H */
