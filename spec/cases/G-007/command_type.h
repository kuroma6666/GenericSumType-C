/* 型定義・生成マクロ呼び出しを1つの共通ヘッダにまとめ、複数の .c から include する
 * 推奨運用。生成関数はすべて static inline（内部リンケージ）なので、翻訳単位間で
 * 共有される外部シンボルは存在しない。 */
#ifndef COMMAND_TYPE_H
#define COMMAND_TYPE_H
#include "generic_sum_type.h"

typedef struct { int dx, dy; } CmdMove;
typedef struct { int target; }  CmdAttack;

#define CMD_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, move, CmdMove)    \
    X(NAME, EXTRA, attack, CmdAttack)

SUM_DEFINE_TYPE(Command, CMD_VARIANTS)

typedef struct { int hp; int x, y; } GameState;

SUM_DEFINE_DISPATCH(Command, CMD_VARIANTS, Command_dispatch, GameState)

#endif
