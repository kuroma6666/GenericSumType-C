#include "command_type.h"

static void on_move(CmdMove *m, GameState *gs) { gs->x += m->dx; gs->y += m->dy; }
static void on_attack(CmdAttack *a, GameState *gs) { gs->hp -= a->target; }

void worker_process(Command cmd, GameState *gs) {
    Command_dispatch(&cmd, gs, on_move, on_attack);
}
