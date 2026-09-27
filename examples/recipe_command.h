/*
 * recipe_command.h
 *
 * recipe_param_demo.c と tests/test_recipe_command.c から共有される
 * 「ROM/RAMパラメータ取得系」コマンドの型・DISPATCH_RET定義・ハンドラ本体。
 *
 * 型定義を1つの共通ヘッダに1回だけ書き、複数の.cからincludeするパターン
 * (docs/constraints.md C-11、spec G-007)を応用している。
 * 各コマンドは応答(RecipeReply)を戻り値で返す(SUM_DEFINE_DISPATCH_RET。docs/adr/0014)。
 *
 * 参考: PALLASER社 レーザーマーカー 通信コマンド仕様書 Ver.2.0
 *   https://pallaser.co.jp/dwl/mother/R2_0_Communication_Manual.pdf
 *   (§3.5 ファイル操作。本ファイルは同仕様書の一部コマンド名・役割に
 *    着想を得た独自の簡略化例であり、仕様書の転載ではない)
 */
#ifndef RECIPE_COMMAND_H
#define RECIPE_COMMAND_H

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

SUM_DEFINE_TYPE(RecipeCommand, RECIPE_COMMAND_VARIANTS)

/* --- 永続パラメータストア(実機の不揮発メモリに相当するRAM上の模擬) --- */
typedef struct {
    int  used;
    char name[RECIPE_NAME_MAX];
} RecipeSlot;

typedef struct {
    RecipeSlot slots[RECIPE_SLOT_MAX];
    int current;          /* 現在選択中の品種番号。未選択は-1 */
} RecipeStore;

/* --- 各コマンドの応答 ---
 * 実機の応答フレーム(OK / NG + エラー番号 / 読み出し値)に対応する。
 * 結果は戻り値で返し、ストア(ctx)には永続状態だけを置く。 */
typedef enum {
    RECIPE_OK = 0,
    RECIPE_NG_OUT_OF_RANGE,   /* 品種番号が範囲外 */
    RECIPE_NG_NOT_FOUND,      /* 未作成の品種番号 */
    RECIPE_NG_SELECTED        /* 現在選択中のため削除不可 */
} RecipeStatus;

typedef struct {
    RecipeStatus status;
    int value;                /* query の読み出し値。それ以外は 0 */
} RecipeReply;

SUM_DEFINE_DISPATCH_RET(RecipeCommand, RECIPE_COMMAND_VARIANTS,
                        RecipeCommand_execute, RecipeStore, RecipeReply)

static inline RecipeReply recipe_reply(RecipeStatus status, int value) {
    RecipeReply r;
    r.status = status;
    r.value = value;
    return r;
}

static inline int slot_index_valid(int number) {
    return number >= 0 && number < RECIPE_SLOT_MAX;
}

/* --- 各コマンドのハンドラ: 永続ストア(ctx)を読み書きし、応答を返す --- */

static inline RecipeReply on_create(CmdRecipeCreate *c, RecipeStore *store) {
    if (!slot_index_valid(c->number)) {
        printf("[create] NG: 品種番号 %d は範囲外です\n", c->number);
        return recipe_reply(RECIPE_NG_OUT_OF_RANGE, 0);
    }
    RecipeSlot *slot = &store->slots[c->number];
    slot->used = 1;
    snprintf(slot->name, sizeof slot->name, "%.*s", RECIPE_NAME_MAX - 1, c->name);
    printf("[create] 品種番号=%d name=\"%s\" を作成しました\n", c->number, slot->name);
    return recipe_reply(RECIPE_OK, 0);
}

static inline RecipeReply on_delete(CmdRecipeDelete *c, RecipeStore *store) {
    if (!slot_index_valid(c->number) || !store->slots[c->number].used) {
        printf("[delete] NG: 品種番号 %d は存在しません\n", c->number);
        return recipe_reply(RECIPE_NG_NOT_FOUND, 0);
    }
    if (store->current == c->number) {
        /* 実機の仕様書と同様、現在選択中の品種番号を指定するとエラーにする */
        printf("[delete] NG: 品種番号 %d は現在選択中のため削除できません\n", c->number);
        return recipe_reply(RECIPE_NG_SELECTED, 0);
    }
    store->slots[c->number].used = 0;
    store->slots[c->number].name[0] = '\0';
    printf("[delete] 品種番号=%d を削除しました\n", c->number);
    return recipe_reply(RECIPE_OK, 0);
}

static inline RecipeReply on_select(CmdRecipeSelect *c, RecipeStore *store) {
    if (!slot_index_valid(c->number) || !store->slots[c->number].used) {
        printf("[select] NG: 品種番号 %d は存在しません\n", c->number);
        return recipe_reply(RECIPE_NG_NOT_FOUND, 0);
    }
    store->current = c->number;
    printf("[select] 現在品種番号を %d (\"%s\") に切り替えました\n",
           c->number, store->slots[c->number].name);
    return recipe_reply(RECIPE_OK, 0);
}

static inline RecipeReply on_query(CmdRecipeQuery *c, RecipeStore *store) {
    (void)c;
    if (store->current >= 0) {
        printf("[query]  現在品種番号=%d (\"%s\")\n",
               store->current, store->slots[store->current].name);
    } else {
        printf("[query]  現在品種未選択\n");
    }
    return recipe_reply(RECIPE_OK, store->current);
}

#endif /* RECIPE_COMMAND_H */
