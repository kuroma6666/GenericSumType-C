/*
 * internal_command_demo.c
 *
 * 「内部処理系」のCommandパターン例。
 *
 * 実在するレーザーマーカーの通信コマンド仕様書(PALLASER社 レーザーマーカー
 * 通信コマンド仕様書 Ver.2.0, §3.6 システム操作)で定義されている、動作を
 * トリガーするだけの実行系コマンド(MST/MSP/UTN/ERC/GUD/GDS)の"名前と役割"に
 * 着想を得て、独自に簡略化したC言語の例として書き起こしたもの。
 * 実際の仕様書のバイト列フォーマットや詳細なパラメータ仕様までは再現しておらず、
 * あくまで「内部処理系コマンドの設計パターン」を示すための教材である。
 *
 * ここでの「内部処理系」とは、通信フレームの受信やチェックサム検証が
 * 既に完了し、コマンド種別とパラメータへのデコードが済んだ後、
 * デバイス内部の状態(印字中か、運転中か、ガイド光は出ているか等)を
 * 実際に変更する層を指す。通信のプロトコル的な関心事(送受信フォーマット、
 * NGエラー等)を一切含まない点が、protocol_frame_demo.c(通信処理系)との
 * 責務の違いである。
 *
 * 参考: PALLASER社 レーザーマーカー 通信コマンド仕様書 Ver.2.0
 *   https://pallaser.co.jp/dwl/mother/R2_0_Communication_Manual.pdf
 *   (§3.6 システム操作。本ファイルは同仕様書の一部コマンド名・役割に
 *    着想を得た独自の簡略化例であり、仕様書の転載ではない)
 */
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

/* --- 各コマンドのハンドラ: デバイス状態(ctx)を実際に書き換える --- */

static void on_print_start(CmdPrintStart *c, LaserDeviceState *st) {
    st->printing = 1;
    st->print_mode = c->kind;
    printf("[print_start] mode=%s -> printing開始\n",
           c->kind == 1 ? "連続" : "通常");
}

static void on_print_stop(CmdPrintStop *c, LaserDeviceState *st) {
    (void)c;
    /* 実機の仕様書と同様、「印字中の即時停止はできない」という制約を
     * 模して、実際の停止はrunning状態のチェックを経てから行う例にしている。 */
    st->printing = 0;
    printf("[print_stop]  printing停止\n");
}

static void on_run_control(CmdRunControl *c, LaserDeviceState *st) {
    st->running = c->mode;
    printf("[run_control] mode=%d -> running=%d\n", c->mode, st->running);
}

static void on_error_clear(CmdErrorClear *c, LaserDeviceState *st) {
    (void)c;
    st->error_active = 0;
    printf("[error_clear] エラーを解除しました\n");
}

static void on_guide_start(CmdGuideStart *c, LaserDeviceState *st) {
    static const char *kind_name[] = { "四角ガイド", "輪郭ガイド", "WDガイド" };
    st->guide_on = 1;
    st->guide_kind = c->kind;
    printf("[guide_start] kind=%s\n",
           (c->kind >= 0 && c->kind <= 2) ? kind_name[c->kind] : "不明");
}

static void on_guide_stop(CmdGuideStop *c, LaserDeviceState *st) {
    (void)c;
    st->guide_on = 0;
    printf("[guide_stop]  ガイド光を停止しました\n");
}

int main(void) {
    LaserDeviceState state = {
        .printing = 0, .print_mode = 0, .running = 0,
        .guide_on = 0, .guide_kind = 0, .error_active = 1,
    };

    /* 通信層で受信・デコードされた後の「内部コマンド列」を模したキュー */
    InternalCommand queue[] = {
        InternalCommand_new_error_clear((CmdErrorClear){ .reserved = 0 }),
        InternalCommand_new_guide_start((CmdGuideStart){ .kind = 1 }),
        InternalCommand_new_run_control((CmdRunControl){ .mode = 1 }),
        InternalCommand_new_print_start((CmdPrintStart){ .kind = 1 }),
        InternalCommand_new_guide_stop((CmdGuideStop){ .reserved = 0 }),
        InternalCommand_new_print_stop((CmdPrintStop){ .reserved = 0 }),
        InternalCommand_new_run_control((CmdRunControl){ .mode = 0 }),
    };
    size_t queue_len = sizeof(queue) / sizeof(queue[0]);

    for (size_t i = 0; i < queue_len; ++i) {
        InternalCommand_dispatch(&queue[i], &state,
                                  on_print_start, on_print_stop, on_run_control,
                                  on_error_clear, on_guide_start, on_guide_stop);
    }

    printf("final state: printing=%d running=%d guide_on=%d error_active=%d\n",
           state.printing, state.running, state.guide_on, state.error_active);
    return 0;
}
