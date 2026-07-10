/*
 * internal_command_demo.c
 *
 * 「内部処理系」のCommandパターン例。
 *
 * 型定義・DISPATCH・ハンドラ本体は internal_command.h に集約されている
 * (tests/test_internal_command.c と共有するため。design_spec.md 4.10節参照)。
 * このファイルは、通信層で受信・デコードされた後の「内部コマンド列」を
 * 模したキューを流すだけの、薄いドライバである。
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
#include "internal_command.h"

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
