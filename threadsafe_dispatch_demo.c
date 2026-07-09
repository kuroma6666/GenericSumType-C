/*
 * SUM_CTX_LOCK / SUM_CTX_UNLOCK フックの使用例。
 * ここではLinux(pthread)向けの実装を示すが、RTOS環境では同じフックを
 * FreeRTOSのセマフォAPI等に差し替えるだけでよい（ライブラリ本体は変更不要）。
 */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>

typedef struct {
    long counter;
    pthread_mutex_t mutex;
} Counter;

/* generic_sum_type.h をincludeする前にロックフックを定義する */
#define SUM_CTX_LOCK(ctx)   pthread_mutex_lock(&(ctx)->mutex)
#define SUM_CTX_UNLOCK(ctx) pthread_mutex_unlock(&(ctx)->mutex)
#include "generic_sum_type.h"

typedef struct { long amount; } CmdIncrement;
typedef struct { long amount; } CmdDecrement;

#define COUNTER_CMD_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, increment, CmdIncrement)  \
    X(NAME, EXTRA, decrement, CmdDecrement)

DEFINE_SUM_TYPE(CounterCmd, COUNTER_CMD_VARIANTS)
DEFINE_SUM_DISPATCH(CounterCmd, COUNTER_CMD_VARIANTS, CounterCmd_dispatch, Counter)

static void on_increment(CmdIncrement *c, Counter *ctx) { ctx->counter += c->amount; }
static void on_decrement(CmdDecrement *c, Counter *ctx) { ctx->counter -= c->amount; }

#define ITER_PER_THREAD 100000

static void *worker_inc(void *arg) {
    Counter *ctx = (Counter *)arg;
    CounterCmd cmd = CounterCmd_new_increment((CmdIncrement){ .amount = 1 });
    for (int i = 0; i < ITER_PER_THREAD; ++i) {
        CounterCmd_dispatch(&cmd, ctx, on_increment, on_decrement);
    }
    return NULL;
}

static void *worker_dec(void *arg) {
    Counter *ctx = (Counter *)arg;
    CounterCmd cmd = CounterCmd_new_decrement((CmdDecrement){ .amount = 1 });
    for (int i = 0; i < ITER_PER_THREAD; ++i) {
        CounterCmd_dispatch(&cmd, ctx, on_increment, on_decrement);
    }
    return NULL;
}

int main(void) {
    Counter ctx = { .counter = 0 };
    pthread_mutex_init(&ctx.mutex, NULL);

    /* +1と-1を行うスレッドをそれぞれ4本ずつ走らせる。ロックが効いていれば
     * 最終的なcounterは必ず0になるはず（効いていなければ競合で値がずれる）。 */
    pthread_t inc_threads[4], dec_threads[4];
    for (int i = 0; i < 4; ++i) pthread_create(&inc_threads[i], NULL, worker_inc, &ctx);
    for (int i = 0; i < 4; ++i) pthread_create(&dec_threads[i], NULL, worker_dec, &ctx);
    for (int i = 0; i < 4; ++i) pthread_join(inc_threads[i], NULL);
    for (int i = 0; i < 4; ++i) pthread_join(dec_threads[i], NULL);

    printf("final counter = %ld (期待値 0, 4スレッド x %d increment と 4スレッド x %d decrement)\n",
           ctx.counter, ITER_PER_THREAD, ITER_PER_THREAD);

    pthread_mutex_destroy(&ctx.mutex);
    return ctx.counter == 0 ? 0 : 1;
}
