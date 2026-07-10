/*
 * recipe_param_demo.c
 *
 * 「ROM/RAMパラメータ取得系」のCommandパターン例。
 *
 * 型定義・DISPATCH・ハンドラ本体は recipe_command.h に集約されている
 * (tests/test_recipe_command.c と共有するため)。このファイルは、
 * 通信層でデコードされた後の「ROM/RAMパラメータ操作コマンド列」を
 * 模したキューを流すだけの、薄いドライバである。
 *
 * ここでの「ROM/RAMパラメータ取得系」とは、実行するたびに副作用が起こる
 * 内部処理系(internal_command_demo.c)や、フレームを解釈するだけの
 * 通信処理系(protocol_frame_demo.c)とは異なり、
 *   ・複数の名前付きスロットを持つ永続ストレージがある
 *   ・番号を指定して作成・削除・選択・参照する
 *   ・「現在選択中のスロット」という状態を持つ
 * という、設定値の永続化そのものを扱う層を指す。実機では不揮発メモリ
 * (フラッシュ/EEPROM等)に対応するが、本デモでは配列によるRAM上の
 * シミュレーションに置き換えている。
 *
 * 参考: PALLASER社 レーザーマーカー 通信コマンド仕様書 Ver.2.0
 *   https://pallaser.co.jp/dwl/mother/R2_0_Communication_Manual.pdf
 *   (§3.5 ファイル操作。本ファイルは同仕様書の一部コマンド名・役割に
 *    着想を得た独自の簡略化例であり、仕様書の転載ではない)
 */
#include <stdio.h>
#include "recipe_command.h"

int main(void) {
    RecipeStore store = { .slots = { { 0 } }, .current = -1, .last_query_result = -1 };

    /* 通信層でデコードされた後の「ROM/RAMパラメータ操作コマンド列」を模したキュー */
    RecipeCommand queue[] = {
        RecipeCommand_new_create((CmdRecipeCreate){ .number = 1, .name = "BEARING200" }),
        RecipeCommand_new_create((CmdRecipeCreate){ .number = 2, .name = "SHAFT-A" }),
        RecipeCommand_new_select((CmdRecipeSelect){ .number = 1 }),
        RecipeCommand_new_query((CmdRecipeQuery){ .reserved = 0 }),
        RecipeCommand_new_delete((CmdRecipeDelete){ .number = 1 }),   /* 選択中なのでNGになる例 */
        RecipeCommand_new_select((CmdRecipeSelect){ .number = 2 }),
        RecipeCommand_new_delete((CmdRecipeDelete){ .number = 1 }),   /* 選択が外れたので削除できる */
        RecipeCommand_new_query((CmdRecipeQuery){ .reserved = 0 }),
    };
    size_t queue_len = sizeof(queue) / sizeof(queue[0]);

    for (size_t i = 0; i < queue_len; ++i) {
        RecipeCommand_dispatch(&queue[i], &store, on_create, on_delete, on_select, on_query);
    }

    printf("final: current=%d last_query_result=%d\n", store.current, store.last_query_result);
    return 0;
}
