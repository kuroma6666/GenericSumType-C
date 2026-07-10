/*
 * recipe_command.h
 *
 * recipe_param_demo.c と tests/test_recipe_command.c から共有される
 * 「ROM/RAMパラメータ取得系」コマンドの型・DISPATCH定義・ハンドラ本体。
 *
 * design_spec.md 4.10節で検証済みの「型定義を1つの共通ヘッダに1回だけ書き、
 * 複数の.cからincludeする」というパターンを応用している。
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

SUM_MAYBE_UNUSED
static inline int slot_index_valid(int number) {
    return number >= 0 && number < RECIPE_SLOT_MAX;
}

/* --- 各コマンドのハンドラ: 永続ストア(ctx)を読み書きする --- */

SUM_MAYBE_UNUSED
static inline void on_create(CmdRecipeCreate *c, RecipeStore *store) {
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

SUM_MAYBE_UNUSED
static inline void on_delete(CmdRecipeDelete *c, RecipeStore *store) {
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

SUM_MAYBE_UNUSED
static inline void on_select(CmdRecipeSelect *c, RecipeStore *store) {
    if (!slot_index_valid(c->number) || !store->slots[c->number].used) {
        printf("[select] NG: 品種番号 %d は存在しません\n", c->number);
        return;
    }
    store->current = c->number;
    printf("[select] 現在品種番号を %d (\"%s\") に切り替えました\n",
           c->number, store->slots[c->number].name);
}

SUM_MAYBE_UNUSED
static inline void on_query(CmdRecipeQuery *c, RecipeStore *store) {
    (void)c;
    store->last_query_result = store->current;
    if (store->current >= 0) {
        printf("[query]  現在品種番号=%d (\"%s\")\n",
               store->current, store->slots[store->current].name);
    } else {
        printf("[query]  現在品種未選択\n");
    }
}

#endif /* RECIPE_COMMAND_H */
