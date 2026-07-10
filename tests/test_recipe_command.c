/*
 * tests/test_recipe_command.c
 *
 * examples/recipe_command.h(ROM/RAMパラメータ取得系Commandパターン)の単体テスト。
 * test_generic_sum_type.c と同じ方針(外部フレームワークなし、assert()のみ)。
 *
 * ビルド: gcc -std=c11 -pedantic -Wall -Wextra -Werror -Iinclude \
 *             -o test_recipe_command tests/test_recipe_command.c
 * 実行:   ./test_recipe_command
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../examples/recipe_command.h"

static int tests_run = 0;
#define RUN(test) do { printf("- %-55s ... ", #test); test(); tests_run++; printf("OK\n"); } while (0)

static RecipeStore fresh_store(void) {
    RecipeStore store = { .slots = { { 0 } }, .current = -1, .last_query_result = -1 };
    return store;
}

/* ============ 1. 各ハンドラを直接呼び出して永続ストアの読み書きを検証 ============ */

static void test_create_adds_slot(void) {
    RecipeStore store = fresh_store();
    on_create(&(CmdRecipeCreate){ .number = 3, .name = "TEST-A" }, &store);
    assert(store.slots[3].used == 1);
    assert(strcmp(store.slots[3].name, "TEST-A") == 0);
}

static void test_create_rejects_out_of_range_number(void) {
    RecipeStore store = fresh_store();
    on_create(&(CmdRecipeCreate){ .number = 999, .name = "X" }, &store);
    /* 範囲外は無視されるだけで、他のスロットに影響しないことを確認する */
    for (int i = 0; i < RECIPE_SLOT_MAX; ++i) {
        assert(store.slots[i].used == 0);
    }
}

static void test_select_switches_current(void) {
    RecipeStore store = fresh_store();
    on_create(&(CmdRecipeCreate){ .number = 1, .name = "A" }, &store);
    on_select(&(CmdRecipeSelect){ .number = 1 }, &store);
    assert(store.current == 1);
}

static void test_select_rejects_unused_slot(void) {
    RecipeStore store = fresh_store();
    on_select(&(CmdRecipeSelect){ .number = 5 }, &store); /* 未作成のスロット */
    assert(store.current == -1); /* 変化しない */
}

static void test_delete_removes_slot(void) {
    RecipeStore store = fresh_store();
    on_create(&(CmdRecipeCreate){ .number = 2, .name = "B" }, &store);
    on_delete(&(CmdRecipeDelete){ .number = 2 }, &store);
    assert(store.slots[2].used == 0);
}

static void test_delete_rejects_current_slot(void) {
    RecipeStore store = fresh_store();
    on_create(&(CmdRecipeCreate){ .number = 1, .name = "A" }, &store);
    on_select(&(CmdRecipeSelect){ .number = 1 }, &store);
    on_delete(&(CmdRecipeDelete){ .number = 1 }, &store); /* 選択中なのでNG */
    assert(store.slots[1].used == 1); /* 削除されていない */
    assert(store.current == 1);
}

static void test_query_writes_result_to_ctx(void) {
    RecipeStore store = fresh_store();
    on_create(&(CmdRecipeCreate){ .number = 4, .name = "Q" }, &store);
    on_select(&(CmdRecipeSelect){ .number = 4 }, &store);
    on_query(&(CmdRecipeQuery){ .reserved = 0 }, &store);
    assert(store.last_query_result == 4);
}

/* ============ 2. DEFINE_SUM_DISPATCH自体が正しいハンドラへ振り分けるか ============ */

static void test_dispatch_routes_to_correct_handler(void) {
    RecipeStore store = fresh_store();
    RecipeCommand cmd = RecipeCommand_new_create((CmdRecipeCreate){ .number = 0, .name = "Z" });

    RecipeCommand_dispatch(&cmd, &store, on_create, on_delete, on_select, on_query);

    assert(store.slots[0].used == 1);
    assert(strcmp(store.slots[0].name, "Z") == 0);
}

/* ============ 3. recipe_param_demo.c と同じコマンド列を流した結果 ============ */

static void test_full_command_sequence_matches_expected_final_state(void) {
    RecipeStore store = fresh_store();

    RecipeCommand queue[] = {
        RecipeCommand_new_create((CmdRecipeCreate){ .number = 1, .name = "BEARING200" }),
        RecipeCommand_new_create((CmdRecipeCreate){ .number = 2, .name = "SHAFT-A" }),
        RecipeCommand_new_select((CmdRecipeSelect){ .number = 1 }),
        RecipeCommand_new_query((CmdRecipeQuery){ .reserved = 0 }),
        RecipeCommand_new_delete((CmdRecipeDelete){ .number = 1 }),   /* 選択中なのでNGになる */
        RecipeCommand_new_select((CmdRecipeSelect){ .number = 2 }),
        RecipeCommand_new_delete((CmdRecipeDelete){ .number = 1 }),   /* 選択が外れたので削除できる */
        RecipeCommand_new_query((CmdRecipeQuery){ .reserved = 0 }),
    };
    size_t n = sizeof(queue) / sizeof(queue[0]);

    for (size_t i = 0; i < n; ++i) {
        RecipeCommand_dispatch(&queue[i], &store, on_create, on_delete, on_select, on_query);
    }

    assert(store.current == 2);
    assert(store.last_query_result == 2);
    assert(store.slots[1].used == 0); /* 最終的には削除済み */
    assert(store.slots[2].used == 1);
}

int main(void) {
    printf("recipe_command.h 単体テスト\n");
    RUN(test_create_adds_slot);
    RUN(test_create_rejects_out_of_range_number);
    RUN(test_select_switches_current);
    RUN(test_select_rejects_unused_slot);
    RUN(test_delete_removes_slot);
    RUN(test_delete_rejects_current_slot);
    RUN(test_query_writes_result_to_ctx);
    RUN(test_dispatch_routes_to_correct_handler);
    RUN(test_full_command_sequence_matches_expected_final_state);
    printf("%d件全て成功\n", tests_run);
    return 0;
}
