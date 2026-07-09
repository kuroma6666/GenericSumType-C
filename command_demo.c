#include <stdio.h>
#include <stddef.h>
#include "generic_sum_type.h"

/* --- コマンドごとのpayload --- */
typedef struct { int dx, dy; }             CmdMove;
typedef struct { int target_id, damage; }  CmdAttack;
typedef struct { int target_id, amount; }  CmdHeal;
typedef struct { int code; }               CmdQuit;

/* コマンド種別リスト（Rustなら enum Command { Move{..}, Attack{..}, Heal{..}, Quit{..} } ） */
#define COMMAND_VARIANTS(X, NAME, EXTRA)  \
    X(NAME, EXTRA, move,   CmdMove)       \
    X(NAME, EXTRA, attack, CmdAttack)     \
    X(NAME, EXTRA, heal,   CmdHeal)       \
    X(NAME, EXTRA, quit,   CmdQuit)

DEFINE_SUM_TYPE(Command, COMMAND_VARIANTS)

/* ディスパッチが書き換える共有状態（コマンド実行のコンテキスト） */
typedef struct {
    int player_x, player_y;
    int player_hp;
    int running;
} GameState;

DEFINE_SUM_DISPATCH(Command, COMMAND_VARIANTS, Command_dispatch, GameState)

/* --- 各コマンドのハンドラ: payload + 共有状態(ctx) を受け取り副作用を起こす --- */

static void on_move(CmdMove *m, GameState *st) {
    st->player_x += m->dx;
    st->player_y += m->dy;
    printf("[move]   -> (%d, %d)\n", st->player_x, st->player_y);
}

static void on_attack(CmdAttack *a, GameState *st) {
    (void)st;
    printf("[attack] target=%d damage=%d\n", a->target_id, a->damage);
}

static void on_heal(CmdHeal *h, GameState *st) {
    st->player_hp += h->amount;
    printf("[heal]   target=%d amount=%d (HP=%d)\n", h->target_id, h->amount, st->player_hp);
}

static void on_quit(CmdQuit *q, GameState *st) {
    printf("[quit]   code=%d\n", q->code);
    st->running = 0;
}

int main(void) {
    GameState state = { .player_x = 0, .player_y = 0, .player_hp = 100, .running = 1 };

    /* コマンドキュー：実運用ならネットワークやファイルから逐次読み込むイメージ */
    Command queue[] = {
        Command_new_move((CmdMove){ .dx = 1, .dy = 0 }),
        Command_new_attack((CmdAttack){ .target_id = 42, .damage = 10 }),
        Command_new_heal((CmdHeal){ .target_id = 1, .amount = 5 }),
        Command_new_move((CmdMove){ .dx = 0, .dy = 2 }),
        Command_new_quit((CmdQuit){ .code = 0 }),
    };
    size_t queue_len = sizeof(queue) / sizeof(queue[0]);

    for (size_t i = 0; i < queue_len && state.running; ++i) {
        Command_dispatch(&queue[i], &state, on_move, on_attack, on_heal, on_quit);
    }

    printf("final state: pos=(%d,%d) hp=%d\n", state.player_x, state.player_y, state.player_hp);
    return 0;
}
