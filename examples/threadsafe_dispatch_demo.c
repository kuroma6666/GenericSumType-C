/*
 * 共有テレメトリカウンタの並行更新（ロックフックの検証）
 *
 * 【要件】
 *   複数のワーカースレッド（受信系・送信系）が、1つの共有カウンタに対して
 *   「加算コマンド」「減算コマンド」を適用する。カウンタ更新は read-modify-write
 *   であり、排他制御がないと lost update で値がずれる。ライブラリ本体を特定の
 *   ロック機構に依存させず、環境（pthread / RTOS）に応じたロックを差し込みたい。
 *
 * 【仕様】
 *   - CounterCmd は increment / decrement のいずれか（それぞれ amount を持つ）。
 *   - dispatch はコマンドを共有 Counter に適用する。dispatch 区間は排他する。
 *   - +1 と -1 を同数のスレッドで回し、最終値が必ず 0 になること（ロックが効いている証拠）。
 *
 * 【実装方針】
 *   - SUM_DEFINE_DISPATCH_LOCKED に pthread_mutex のロック/アンロック関数を渡す。
 *     ロックは定義箇所で型ごとに決まり、同じ翻訳単位の他の DISPATCH に影響しない
 *     （docs/adr/0014）。
 *   - RTOS では渡す関数をセマフォ等に差し替えるだけでよい（ライブラリ本体は不変）。
 *   - 注意: ハンドラ内から同一 ctx へ再 dispatch すると非再帰ロックでデッドロックする
 *     （docs/constraints.md C-09）。本デモは再入しない。
 *
 * 仕様の詳細: examples/specs/telemetry_counter.md
 *
 * ci-flags: -pthread
 */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include "generic_sum_type.h"

typedef struct {
    long counter;
    pthread_mutex_t mutex;
} Counter;

static void counter_lock(Counter *c)   { pthread_mutex_lock(&c->mutex); }
static void counter_unlock(Counter *c) { pthread_mutex_unlock(&c->mutex); }

typedef struct { long amount; } CmdIncrement;
typedef struct { long amount; } CmdDecrement;

#define COUNTER_CMD(X, NAME, EXTRA)          \
    X(NAME, EXTRA, increment, CmdIncrement)  \
    X(NAME, EXTRA, decrement, CmdDecrement)

SUM_DEFINE_TYPE(CounterCmd, COUNTER_CMD)
SUM_DEFINE_DISPATCH_LOCKED(CounterCmd, COUNTER_CMD, CounterCmd_apply, Counter,
                           counter_lock, counter_unlock)

static void on_increment(CmdIncrement *c, Counter *ctx) { ctx->counter += c->amount; }
static void on_decrement(CmdDecrement *c, Counter *ctx) { ctx->counter -= c->amount; }

#define ITER_PER_THREAD 100000

static void *worker_inc(void *arg) {
    Counter *ctx = (Counter *)arg;
    CounterCmd cmd = CounterCmd_new_increment((CmdIncrement){ .amount = 1 });
    for (int i = 0; i < ITER_PER_THREAD; ++i)
        CounterCmd_apply(&cmd, ctx, on_increment, on_decrement);
    return NULL;
}
static void *worker_dec(void *arg) {
    Counter *ctx = (Counter *)arg;
    CounterCmd cmd = CounterCmd_new_decrement((CmdDecrement){ .amount = 1 });
    for (int i = 0; i < ITER_PER_THREAD; ++i)
        CounterCmd_apply(&cmd, ctx, on_increment, on_decrement);
    return NULL;
}

int main(void) {
    Counter ctx = { .counter = 0 };
    pthread_mutex_init(&ctx.mutex, NULL);

    pthread_t inc[4], dec[4];
    for (int i = 0; i < 4; ++i) pthread_create(&inc[i], NULL, worker_inc, &ctx);
    for (int i = 0; i < 4; ++i) pthread_create(&dec[i], NULL, worker_dec, &ctx);
    for (int i = 0; i < 4; ++i) pthread_join(inc[i], NULL);
    for (int i = 0; i < 4; ++i) pthread_join(dec[i], NULL);

    printf("final counter = %ld (期待値 0)\n", ctx.counter);
    pthread_mutex_destroy(&ctx.mutex);
    return ctx.counter == 0 ? 0 : 1;
}
