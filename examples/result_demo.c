/*
 * 注文処理の Result<OrderId, OrderError>（ドメインエラーは値、技術例外と分離）
 *
 * 【要件】
 *   注文作成は在庫確認・決済などで失敗しうる。これらは想定内のドメインエラー
 *   なので、例外ではなく Result の「値」として返し、呼び出し側で分岐・変換したい。
 *   （DB 接続断のような技術的エラーは別途 例外で扱う、という設計の Result 側。）
 *
 * 【仕様】
 *   - Result は ok=OrderId / err=OrderError のいずれか。
 *   - is_ok/is_err で分岐、fold で「注文ID or エラーコード」に畳み込む。
 *   - unwrap_or でデフォルト、同型 and_then（OrderId -> Result）で段階適用。
 *
 * 【実装方針】
 *   - DEFINE_SUM_TYPE(tag=ok/err) + DEFINE_RESULT_HELPERS（is_ok/is_err）。
 *   - fold は DEFINE_SUM_MATCH_CONST、取り出しは const ゲッター。
 *   - 型が変わる合成（map: Result<T,E>->Result<U,E> / map_err）は C では汎用化
 *     できない（design_spec 2.10節）。同型 and_then か目標型明示の手書きに限る。
 *
 * 仕様の詳細: examples/specs/order_result.md
 */
#include <stdio.h>
#include "generic_sum_type.h"

typedef struct { int order_id; }               OrderId;
typedef struct { int kind; const char *reason; } OrderError; /* kind: 業務エラー種別 */

enum { ERR_INSUFFICIENT_STOCK = 1, ERR_PAYMENT_FAILED = 2 };

#define ORDER_RESULT(X, NAME, EXTRA) \
    X(NAME, EXTRA, ok,  OrderId)     \
    X(NAME, EXTRA, err, OrderError)

DEFINE_SUM_TYPE(OrderResult, ORDER_RESULT)
DEFINE_RESULT_HELPERS(OrderResult)
DEFINE_SUM_MATCH_CONST(OrderResult, ORDER_RESULT, OrderResult_fold, int)

static int fold_ok (const OrderId *o)    { return o->order_id; }
static int fold_err(const OrderError *e) { printf("  err(%d): %s\n", e->kind, e->reason); return -e->kind; }

static OrderId OrderResult_unwrap_or(const OrderResult *r, OrderId dflt) {
    return r->tag == OrderResult_ok ? r->as.ok : dflt;
}

/* 在庫確認（OrderId -> Result）。同型 and_then で段階適用できる例 */
static OrderResult check_stock(const OrderId *o) {
    if (o->order_id % 7 == 0)
        return OrderResult_new_err((OrderError){ ERR_INSUFFICIENT_STOCK, "out of stock" });
    return OrderResult_new_ok(*o);
}
static OrderResult OrderResult_and_then(const OrderResult *r, OrderResult (*f)(const OrderId *)) {
    return r->tag == OrderResult_ok ? f(&r->as.ok) : *r;
}

int main(void) {
    OrderResult a = OrderResult_new_ok((OrderId){ .order_id = 1002 });
    OrderResult b = OrderResult_new_err((OrderError){ ERR_PAYMENT_FAILED, "card declined" });

    printf("a: is_ok=%d fold=%d\n", OrderResult_is_ok(&a), OrderResult_fold(&a, fold_ok, fold_err));
    printf("b: is_err=%d fold=%d\n", OrderResult_is_err(&b), OrderResult_fold(&b, fold_ok, fold_err));
    printf("unwrap_or(b, #0) = %d\n", OrderResult_unwrap_or(&b, (OrderId){0}).order_id);

    OrderResult chained = OrderResult_and_then(&a, check_stock); /* 1002 は7の倍数でない=在庫OK */
    printf("and_then(a) is_ok=%d order_id=%d\n",
           OrderResult_is_ok(&chained),
           OrderResult_is_ok(&chained) ? chained.as.ok.order_id : -1);
    return 0;
}
