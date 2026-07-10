/*
 * tests/test_internal_command.c
 *
 * examples/internal_command.h(内部処理系Commandパターン)の単体テスト。
 * test_generic_sum_type.c と同じ方針(外部フレームワークなし、assert()のみ)。
 *
 * ハンドラは internal_command.h 側で static inline として定義されているため、
 * このテストファイルからも demo と全く同じ実装を直接呼び出せる
 * (design_spec.md 4.10節: 複数TUでの共有パターンの応用)。
 *
 * ビルド: gcc -std=c11 -pedantic -Wall -Wextra -Werror -Iinclude \
 *             -o test_internal_command tests/test_internal_command.c
 * 実行:   ./test_internal_command
 */
#include <assert.h>
#include <stdio.h>
#include "../examples/internal_command.h"

static int tests_run = 0;
#define RUN(test) do { printf("- %-55s ... ", #test); test(); tests_run++; printf("OK\n"); } while (0)

static LaserDeviceState fresh_state(void) {
    LaserDeviceState st = {
        .printing = 0, .print_mode = 0, .running = 0,
        .guide_on = 0, .guide_kind = 0, .error_active = 0,
    };
    return st;
}

/* ============ 1. 各ハンドラを直接呼び出して状態遷移を検証 ============ */

static void test_print_start_updates_state(void) {
    LaserDeviceState st = fresh_state();
    on_print_start(&(CmdPrintStart){ .kind = 1 }, &st);
    assert(st.printing == 1);
    assert(st.print_mode == 1);
}

static void test_print_stop_updates_state(void) {
    LaserDeviceState st = fresh_state();
    st.printing = 1;
    on_print_stop(&(CmdPrintStop){ .reserved = 0 }, &st);
    assert(st.printing == 0);
}

static void test_run_control_updates_state(void) {
    LaserDeviceState st = fresh_state();
    on_run_control(&(CmdRunControl){ .mode = 1 }, &st);
    assert(st.running == 1);
    on_run_control(&(CmdRunControl){ .mode = 0 }, &st);
    assert(st.running == 0);
}

static void test_error_clear_resets_flag(void) {
    LaserDeviceState st = fresh_state();
    st.error_active = 1;
    on_error_clear(&(CmdErrorClear){ .reserved = 0 }, &st);
    assert(st.error_active == 0);
}

static void test_guide_start_updates_state(void) {
    LaserDeviceState st = fresh_state();
    on_guide_start(&(CmdGuideStart){ .kind = 2 }, &st);
    assert(st.guide_on == 1);
    assert(st.guide_kind == 2);
}

/* テスト作成時に発見した不整合の回帰テスト:
 * 範囲外のkind(0〜2以外)を渡した場合、以前は表示名の選択でだけ
 * 「不明」扱いにしつつ、st->guide_kindには範囲外の値をそのまま
 * 書き込んでしまっていた。internal_command.h側で「範囲外なら状態を
 * 変更しない」ように修正済み。これが崩れたら検出できるようにする。 */
static void test_guide_start_rejects_invalid_kind(void) {
    LaserDeviceState st = fresh_state();
    on_guide_start(&(CmdGuideStart){ .kind = 99 }, &st);
    assert(st.guide_on == 0);
    assert(st.guide_kind == 0);
}

static void test_guide_stop_updates_state(void) {
    LaserDeviceState st = fresh_state();
    st.guide_on = 1;
    on_guide_stop(&(CmdGuideStop){ .reserved = 0 }, &st);
    assert(st.guide_on == 0);
}

/* ============ 2. DEFINE_SUM_DISPATCH自体が正しいハンドラへ振り分けるか ============ */

static void test_dispatch_routes_to_correct_handler(void) {
    LaserDeviceState st = fresh_state();
    InternalCommand cmd = InternalCommand_new_run_control((CmdRunControl){ .mode = 1 });

    InternalCommand_dispatch(&cmd, &st,
                              on_print_start, on_print_stop, on_run_control,
                              on_error_clear, on_guide_start, on_guide_stop);

    assert(st.running == 1);
    /* run_control以外のハンドラが誤って呼ばれていないことも確認する */
    assert(st.printing == 0);
    assert(st.guide_on == 0);
}

/* ============ 3. internal_command_demo.c と同じコマンド列を流した結果 ============ */

static void test_full_command_sequence_matches_expected_final_state(void) {
    LaserDeviceState st = fresh_state();
    st.error_active = 1;

    InternalCommand queue[] = {
        InternalCommand_new_error_clear((CmdErrorClear){ .reserved = 0 }),
        InternalCommand_new_guide_start((CmdGuideStart){ .kind = 1 }),
        InternalCommand_new_run_control((CmdRunControl){ .mode = 1 }),
        InternalCommand_new_print_start((CmdPrintStart){ .kind = 1 }),
        InternalCommand_new_guide_stop((CmdGuideStop){ .reserved = 0 }),
        InternalCommand_new_print_stop((CmdPrintStop){ .reserved = 0 }),
        InternalCommand_new_run_control((CmdRunControl){ .mode = 0 }),
    };
    size_t n = sizeof(queue) / sizeof(queue[0]);

    for (size_t i = 0; i < n; ++i) {
        InternalCommand_dispatch(&queue[i], &st,
                                  on_print_start, on_print_stop, on_run_control,
                                  on_error_clear, on_guide_start, on_guide_stop);
    }

    assert(st.printing == 0);
    assert(st.running == 0);
    assert(st.guide_on == 0);
    assert(st.error_active == 0);
}

int main(void) {
    printf("internal_command.h 単体テスト\n");
    RUN(test_print_start_updates_state);
    RUN(test_print_stop_updates_state);
    RUN(test_run_control_updates_state);
    RUN(test_error_clear_resets_flag);
    RUN(test_guide_start_updates_state);
    RUN(test_guide_start_rejects_invalid_kind);
    RUN(test_guide_stop_updates_state);
    RUN(test_dispatch_routes_to_correct_handler);
    RUN(test_full_command_sequence_matches_expected_final_state);
    printf("%d件全て成功\n", tests_run);
    return 0;
}
