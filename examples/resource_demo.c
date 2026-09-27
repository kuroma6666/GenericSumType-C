/*
 * ログ収集レコードの解放とディープコピー（所有権のある可変長ペイロード）
 *
 * 【要件】
 *   ログ収集器のレコードは「テキストログ（heap 確保した文字列を所有）」と
 *   「数値メトリクス（POD、所有権なし）」が混在する。レコードを破棄するときは
 *   所有する heap を確実に解放し、複製するときはテキストをディープコピーして
 *   別々に破棄できるようにしたい。リーク・二重解放を起こさないこと。
 *
 * 【仕様】
 *   - LogRecord は text（char* を所有）/ number（int の POD）のいずれか。
 *   - LogRecord_destroy(): variant ごとに後始末（text は free、number は no-op）。
 *   - LogRecord_copy():    variant ごとに複製（text は strdup、number は値コピー）。
 *   - LogRecord_try_copy(): 複製の失敗（確保失敗）を戻り値で返し、失敗時は複製先を変えない。
 *   - destroy 後のレコードは空状態になり、同じレコードへの2回目の destroy は何もしない。
 *   - 破棄・複製とも網羅性検査付き（variant 追加時にハンドラ追加を強制）。
 *
 * 【実装方針】
 *   - SUM_DEFINE_DESTROY / SUM_DEFINE_COPY / SUM_DEFINE_COPY_CHECKED を使用。
 *   - 資源を持たない number には SUM_DEFINE_NOOP_DESTROY / SUM_DEFINE_IDENTITY_COPY。
 *   - AddressSanitizer 付きでリーク・二重解放がないことを確認する。
 *
 * 仕様の詳細: examples/specs/log_record.md
 */
#define _POSIX_C_SOURCE 200809L /* strdup は POSIX 拡張 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generic_sum_type.h"

typedef struct { char *msg; } LogText;   /* heap を所有 */
typedef struct { int value; } LogMetric; /* POD */

#define LOG_RECORD(X, NAME, EXTRA) \
    X(NAME, EXTRA, text,   LogText)      \
    X(NAME, EXTRA, number, LogMetric)

SUM_DEFINE_TYPE(LogRecord, LOG_RECORD)
SUM_DEFINE_DESTROY(LogRecord, LOG_RECORD, LogRecord_destroy)
SUM_DEFINE_COPY(LogRecord, LOG_RECORD, LogRecord_copy)
SUM_DEFINE_COPY_CHECKED(LogRecord, LOG_RECORD, LogRecord_try_copy)

static void destroy_text(LogText *t) {
    printf("  free(%p) \"%s\"\n", (void *)t->msg, t->msg);
    free(t->msg);
    t->msg = NULL;
}
SUM_DEFINE_NOOP_DESTROY(destroy_number, LogMetric)

static LogText copy_text(const LogText *t) {
    char *dup = strdup(t->msg);
    printf("  strdup: %p -> %p\n", (const void *)t->msg, (void *)dup);
    return (LogText){ .msg = dup };
}
SUM_DEFINE_IDENTITY_COPY(copy_number, LogMetric)

/* 確保失敗を模擬するためのスイッチ（実機では strdup が NULL を返す状況） */
static int g_simulate_oom = 0;

static int try_copy_text(LogText *dst, const LogText *src) {
    char *dup = g_simulate_oom ? NULL : strdup(src->msg);
    if (dup == NULL) return ENOMEM;
    dst->msg = dup;
    return 0;
}
static int try_copy_number(LogMetric *dst, const LogMetric *src) { *dst = *src; return 0; }

int main(void) {
    printf("--- text レコード: ディープコピー ---\n");
    LogRecord original = LogRecord_new_text((LogText){ .msg = strdup("link down") });
    LogRecord copy = LogRecord_copy(&original, copy_text, copy_number);

    LogText *o = LogRecord_get_text(&original);
    LogText *c = LogRecord_get_text(&copy);
    printf("original=%p copy=%p -> %s\n", (void *)o->msg, (void *)c->msg,
           o->msg != c->msg ? "別アドレス(ディープコピー成功)" : "同一(バグ)");

    printf("--- try_copy: 確保失敗時は複製先を変えない ---\n");
    LogRecord dst = LogRecord_new_number((LogMetric){ .value = 7 });
    g_simulate_oom = 1;
    int rc = LogRecord_try_copy(&dst, &original, try_copy_text, try_copy_number);
    printf("rc=%d (ENOMEM=%d) dst は number のまま: %s\n", rc, ENOMEM,
           LogRecord_get_number(&dst) != NULL ? "yes" : "no");
    g_simulate_oom = 0;
    rc = LogRecord_try_copy(&dst, &original, try_copy_text, try_copy_number);
    const LogText *dup = LogRecord_get_text_const(&dst);
    if (rc != 0 || dup == NULL) {
        printf("try_copy 失敗: rc=%d\n", rc);
        return 1;
    }
    printf("rc=%d dst=\"%s\"\n", rc, dup->msg);

    LogRecord_destroy(&original, destroy_text, destroy_number);
    LogRecord_destroy(&original, destroy_text, destroy_number);   /* 空状態なので何もしない */
    LogRecord_destroy(&copy, destroy_text, destroy_number);
    LogRecord_destroy(&dst, destroy_text, destroy_number);

    printf("--- number レコード: identity copy ---\n");
    LogRecord n = LogRecord_new_number((LogMetric){ .value = 42 });
    LogRecord n_copy = LogRecord_copy(&n, copy_text, copy_number);
    printf("metric original=%d copy=%d\n",
           LogRecord_get_number(&n)->value, LogRecord_get_number(&n_copy)->value);
    LogRecord_destroy(&n, destroy_text, destroy_number);
    LogRecord_destroy(&n_copy, destroy_text, destroy_number);
    return 0;
}
