/*
 * internal_command.h
 *
 * internal_command_demo.c と tests/test_internal_command.c から共有される
 * 「内部処理系」コマンドの型・DISPATCH定義・ハンドラ本体。
 *
 * design_spec.md 4.10節で検証済みの「型定義を1つの共通ヘッダに1回だけ書き、
 * 複数の.cからincludeする」というパターンを応用し、デモ本体(main()のみ)と
 * 単体テスト(ハンドラ/DISPATCHを直接assertする)がロジックを共有できるように
 * 分離している。ハンドラは各TUで同一定義になるよう static inline にしており、
 * ライブラリ自身が生成する関数と同じ形になる。
 *
 * 参考: PALLASER社 レーザーマーカー 通信コマンド仕様書 Ver.2.0
 *   https://pallaser.co.jp/dwl/mother/R2_0_Communication_Manual.pdf
 *   (§3.6 システム操作。本ファイルは同仕様書の一部コマンド名・役割に
 *    着想を得た独自の簡略化例であり、仕様書の転載ではない)
 */
#ifndef INTERNAL_COMMAND_H
#define INTERNAL_COMMAND_H

#include <stdio.h>
#include "generic_sum_type.h"

/* --- 各コマンドのpayload ---
 * MSP(印字停止)やERC(エラー解除)、GDS(ガイド光停止)はパラメータを持たない
 * コマンドだが、「素の基本型を直接使わずstructでラップする」という設計指針
 * (design_spec.md 3節)に倣い、単なるトリガーであっても専用structを与える。
 */
typedef struct { int kind; }       CmdPrintStart;   /* MST: 0=通常, 1=連続 */
typedef struct { int reserved; }   CmdPrintStop;    /* MSP: パラメータなし */
typedef struct { int mode; }       CmdRunControl;   /* UTN: 0=停止, 1=開始 */
typedef struct { int reserved; }   CmdErrorClear;   /* ERC: パラメータなし */
typedef struct { int kind; }       CmdGuideStart;   /* GUD: 0=四角,1=輪郭,2=WD */
typedef struct { int reserved; }   CmdGuideStop;    /* GDS: パラメータなし */

#define INTERNAL_COMMAND_VARIANTS(X, NAME, EXTRA)     \
    X(NAME, EXTRA, print_start,  CmdPrintStart)       \
    X(NAME, EXTRA, print_stop,   CmdPrintStop)        \
    X(NAME, EXTRA, run_control,  CmdRunControl)       \
    X(NAME, EXTRA, error_clear,  CmdErrorClear)       \
    X(NAME, EXTRA, guide_start,  CmdGuideStart)       \
    X(NAME, EXTRA, guide_stop,   CmdGuideStop)

DEFINE_SUM_TYPE(InternalCommand, INTERNAL_COMMAND_VARIANTS)

/* デバイスの内部状態。通信層とは完全に切り離されており、
 * 「今デバイスがどう振る舞っているか」だけを表す。 */
typedef struct {
    int printing;       /* 印字中か */
    int print_mode;      /* 0=通常, 1=連続 */
    int running;          /* 運転中か */
    int guide_on;          /* ガイド光を出力中か */
    int guide_kind;         /* 0=四角,1=輪郭,2=WD */
    int error_active;        /* エラーが発生中か */
} LaserDeviceState;

DEFINE_SUM_DISPATCH(InternalCommand, INTERNAL_COMMAND_VARIANTS,
                     InternalCommand_dispatch, LaserDeviceState)

/* --- 各コマンドのハンドラ: デバイス状態(ctx)を実際に書き換える ---
 * static inline にすることで、demo/testの両TUに同一定義を展開しつつ、
 * 未使用の場合の警告(SUM_MAYBE_UNUSED相当)を避けている。
 */

SUM_MAYBE_UNUSED
static inline void on_print_start(CmdPrintStart *c, LaserDeviceState *st) {
    st->printing = 1;
    st->print_mode = c->kind;
    printf("[print_start] mode=%s -> printing開始\n",
           c->kind == 1 ? "連続" : "通常");
}

SUM_MAYBE_UNUSED
static inline void on_print_stop(CmdPrintStop *c, LaserDeviceState *st) {
    (void)c;
    /* 実機の仕様書と同様、「印字中の即時停止はできない」という制約を
     * 模して、実際の停止はrunning状態のチェックを経てから行う例にしている。 */
    st->printing = 0;
    printf("[print_stop]  printing停止\n");
}

SUM_MAYBE_UNUSED
static inline void on_run_control(CmdRunControl *c, LaserDeviceState *st) {
    st->running = c->mode;
    printf("[run_control] mode=%d -> running=%d\n", c->mode, st->running);
}

SUM_MAYBE_UNUSED
static inline void on_error_clear(CmdErrorClear *c, LaserDeviceState *st) {
    (void)c;
    st->error_active = 0;
    printf("[error_clear] エラーを解除しました\n");
}

SUM_MAYBE_UNUSED
static inline void on_guide_start(CmdGuideStart *c, LaserDeviceState *st) {
    static const char *kind_name[] = { "四角ガイド", "輪郭ガイド", "WDガイド" };
    /* テスト作成時に気づいた点: 以前はkindの範囲チェックを表示名の選択にしか
     * 使っておらず、範囲外の値(例: 99)でもguide_kindへそのまま書き込んで
     * いた。ctxを直接assertするテストを書くと決めたことで「表示は安全でも
     * 状態が壊れる」という不整合が可視化されたため、範囲外の値は無視して
     * 状態を書き換えない(NGとして扱う)ように修正した。 */
    if (c->kind < 0 || c->kind > 2) {
        printf("[guide_start] NG: kind=%d は範囲外です\n", c->kind);
        return;
    }
    st->guide_on = 1;
    st->guide_kind = c->kind;
    printf("[guide_start] kind=%s\n", kind_name[c->kind]);
}

SUM_MAYBE_UNUSED
static inline void on_guide_stop(CmdGuideStop *c, LaserDeviceState *st) {
    (void)c;
    st->guide_on = 0;
    printf("[guide_stop]  ガイド光を停止しました\n");
}

#endif /* INTERNAL_COMMAND_H */
