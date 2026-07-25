#include "command_type.h"

void worker_process(Command cmd, GameState *gs);

int main(void) {
    GameState gs = { .hp = 100, .x = 0, .y = 0 };
    Command c1 = Command_new_move((CmdMove){ .dx = 3, .dy = 4 });
    Command c2 = Command_new_attack((CmdAttack){ .target = 20 });
    worker_process(c1, &gs);
    worker_process(c2, &gs);
    return (gs.hp == 80 && gs.x == 3 && gs.y == 4) ? 0 : 1;
}
