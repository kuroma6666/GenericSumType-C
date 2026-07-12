/*
 * ステッピングモータ制御コマンドのディスパッチ
 *
 * 【要件】
 *   モータ制御器は「速度設定・相対移動・停止」といったコマンドを受け取り、
 *   共有のモータ状態（現在位置・現在速度）を更新する。コマンド適用は状態への
 *   書き込み（副作用）を伴う。コマンド種別を増やしたら、適用処理の更新漏れを
 *   コンパイル時に検出したい。
 *
 * 【仕様】
 *   - MotorCmd は set_speed / move_by / stop のいずれか。
 *       set_speed{ rpm }  -> state.rpm = rpm
 *       move_by{ steps }  -> state.pos += steps
 *       stop{}            -> state.rpm = 0
 *   - MotorState{ pos, rpm } を共有コンテキスト(ctx)として渡し、dispatch で更新する。
 *
 * 【実装方針】
 *   - 副作用（ctx への書き込み）を伴うため DEFINE_SUM_MATCH ではなく
 *     DEFINE_SUM_DISPATCH を使う（ハンドラは payload と ctx を受け取る／戻り値 void）。
 *   - stop はデータを持たないが「1 variant = 1 型」の制約によりダミー struct で表す
 *     （design_spec 4.4節）。
 *
 * 仕様の詳細: examples/specs/motor_command.md
 */
#include <stdio.h>
#include "generic_sum_type.h"

typedef struct { int rpm; }   SetSpeed;
typedef struct { int steps; } MoveBy;
typedef struct { int _unused; } Stop;   /* データなし。ダミー1フィールド */

#define MOTOR_CMD(X, NAME, EXTRA)        \
    X(NAME, EXTRA, set_speed, SetSpeed)  \
    X(NAME, EXTRA, move_by,   MoveBy)    \
    X(NAME, EXTRA, stop,      Stop)

typedef struct { long pos; int rpm; } MotorState;

DEFINE_SUM_TYPE(MotorCmd, MOTOR_CMD)
DEFINE_SUM_DISPATCH(MotorCmd, MOTOR_CMD, MotorCmd_apply, MotorState)

static void on_set_speed(SetSpeed *c, MotorState *s) { s->rpm = c->rpm; }
static void on_move_by(MoveBy *c, MotorState *s)     { s->pos += c->steps; }
static void on_stop(Stop *c, MotorState *s)          { (void)c; s->rpm = 0; }

int main(void) {
    MotorState state = { .pos = 0, .rpm = 0 };
    MotorCmd program[] = {
        MotorCmd_new_set_speed((SetSpeed){ .rpm = 120 }),
        MotorCmd_new_move_by((MoveBy){ .steps = 400 }),
        MotorCmd_new_move_by((MoveBy){ .steps = -150 }),
        MotorCmd_new_stop((Stop){ 0 }),
    };

    for (size_t i = 0; i < sizeof program / sizeof program[0]; ++i) {
        MotorCmd_apply(&program[i], &state, on_set_speed, on_move_by, on_stop);
        printf("step %zu -> pos=%ld rpm=%d\n", i, state.pos, state.rpm);
    }
    return (state.pos == 250 && state.rpm == 0) ? 0 : 1;
}
