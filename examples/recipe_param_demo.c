/*
 * recipe_param_demo.c
 *
 * 「ROM/RAMパラメータ取得系」のCommandパターン例。
 *
 * 実在するレーザーマーカーの通信コマンド仕様書(PALLASER社 レーザーマーカー
 * 通信コマンド仕様書 Ver.2.0, §3.5 ファイル操作)では、印字条件一式を
 * 「品種」という番号付きスロットに保存し、W,MNW(新規作成)/W,MDL(削除)/
 * W,MNO・R,MNO(現在品種番号の変更/取得)といったコマンドで不揮発メモリ上の
 * パラメータセットを操作する。本ファイルはこの"永続パラメータスロットを
 * 番号で読み書きする"という構造に着想を得て、独自に簡略化して書き起こした
 * 例であり、実際のコマンドの詳細なバイト列仕様までは再現していない。
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
#include <string.h>
#include "generic_sum_type.h"

#define RECIPE_SLOT_MAX 8
#define RECIPE_NAME_MAX 32

/* --- 各コマンドのpayload --- */
typedef struct { int number; char name[RECIPE_NAME_MAX]; } CmdRecipeCreate; /* MNW: Memory=A,Name=B */
typedef struct { int number; }                             CmdRecipeDelete; /* MDL: Memory=A */
typedef struct { int number; }                             CmdRecipeSelect; /* MNO(W): 現在品種番号を変更 */
typedef struct { int reserved; }                            CmdRecipeQuery;  /* MNO(R): 現在品種番号を取得 */

#define RECIPE_COMMAND_VARIANTS(X, NAME, EXTRA)  \
    X(NAME, EXTRA, create, CmdRecipeCreate)      \
    X(NAME, EXTRA, delete, CmdRecipeDelete)      \
    X(NAME, EXTRA, select, CmdRecipeSelect)      \
    X(NAME, EXTRA, query,  CmdRecipeQuery)

DEFINE_SUM_TYPE(RecipeCommand, RECIPE_COMMAND_VARIANTS)

/* --- 永続パラメータストア(実機の不揮発メモリに相当するRAM上の模擬) --- */
typedef struct {
    int  used;
    char name[RECIPE_NAME_MAX];
} RecipeSlot;

typedef struct {
    RecipeSlot slots[RECIPE_SLOT_MAX];
    int current;          /* 現在選択中の品種番号。未選択は-1 */
    int last_query_result; /* queryコマンドの結果をここに書き戻す(out-param) */
} RecipeStore;

DEFINE_SUM_DISPATCH(RecipeCommand, RECIPE_COMMAND_VARIANTS,
                     RecipeCommand_dispatch, RecipeStore)

static int slot_index_valid(int number) {
    return number >= 0 && number < RECIPE_SLOT_MAX;
}

/* --- 各コマンドのハンドラ: 永続ストア(ctx)を読み書きする --- */

static void on_create(CmdRecipeCreate *c, RecipeStore *store) {
    if (!slot_index_valid(c->number)) {
        printf("[create] NG: 品種番号 %d は範囲外です\n", c->number);
        return;
    }
    RecipeSlot *slot = &store->slots[c->number];
    slot->used = 1;
    strncpy(slot->name, c->name, RECIPE_NAME_MAX - 1);
    slot->name[RECIPE_NAME_MAX - 1] = '\0';
    printf("[create] 品種番号=%d name=\"%s\" を作成しました\n", c->number, slot->name);
}

static void on_delete(CmdRecipeDelete *c, RecipeStore *store) {
    if (!slot_index_valid(c->number) || !store->slots[c->number].used) {
        printf("[delete] NG: 品種番号 %d は存在しません\n", c->number);
        return;
    }
    if (store->current == c->number) {
        /* 実機の仕様書と同様、現在選択中の品種番号を指定するとエラーにする */
        printf("[delete] NG: 品種番号 %d は現在選択中のため削除できません\n", c->number);
        return;
    }
    store->slots[c->number].used = 0;
    store->slots[c->number].name[0] = '\0';
    printf("[delete] 品種番号=%d を削除しました\n", c->number);
}

static void on_select(CmdRecipeSelect *c, RecipeStore *store) {
    if (!slot_index_valid(c->number) || !store->slots[c->number].used) {
        printf("[select] NG: 品種番号 %d は存在しません\n", c->number);
        return;
    }
    store->current = c->number;
    printf("[select] 現在品種番号を %d (\"%s\") に切り替えました\n",
           c->number, store->slots[c->number].name);
}

static void on_query(CmdRecipeQuery *c, RecipeStore *store) {
    (void)c;
    store->last_query_result = store->current;
    if (store->current >= 0) {
        printf("[query]  現在品種番号=%d (\"%s\")\n",
               store->current, store->slots[store->current].name);
    } else {
        printf("[query]  現在品種未選択\n");
    }
}

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
