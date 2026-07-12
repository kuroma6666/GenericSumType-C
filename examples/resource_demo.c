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
 *   - 破棄・複製とも網羅性検査付き（variant 追加時にハンドラ追加を強制）。
 *
 * 【実装方針】
 *   - DEFINE_SUM_DESTROY / DEFINE_SUM_COPY を使用。
 *   - 資源を持たない number には SUM_DEFINE_NOOP_DESTROY / SUM_DEFINE_IDENTITY_COPY。
 *   - AddressSanitizer 付きでリーク・二重解放がないことを確認する。
 *
 * 仕様の詳細: examples/specs/log_record.md
 */
#define _POSIX_C_SOURCE 200809L /* strdup は POSIX 拡張 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generic_sum_type.h"

typedef struct { char *msg; } LogText;   /* heap を所有 */
typedef struct { int value; } LogMetric; /* POD */

#define LOG_RECORD(X, NAME, EXTRA) \
    X(NAME, EXTRA, text,   LogText)      \
    X(NAME, EXTRA, number, LogMetric)

DEFINE_SUM_TYPE(LogRecord, LOG_RECORD)
DEFINE_SUM_DESTROY(LogRecord, LOG_RECORD, LogRecord_destroy)
DEFINE_SUM_COPY(LogRecord, LOG_RECORD, LogRecord_copy)

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

int main(void) {
    printf("--- text レコード: ディープコピー ---\n");
    LogRecord original = LogRecord_new_text((LogText){ .msg = strdup("link down") });
    LogRecord copy = LogRecord_copy(&original, copy_text, copy_number);

    LogText *o = LogRecord_get_text(&original);
    LogText *c = LogRecord_get_text(&copy);
    printf("original=%p copy=%p -> %s\n", (void *)o->msg, (void *)c->msg,
           o->msg != c->msg ? "別アドレス(ディープコピー成功)" : "同一(バグ)");

    LogRecord_destroy(&original, destroy_text, destroy_number);
    LogRecord_destroy(&copy, destroy_text, destroy_number);

    printf("--- number レコード: identity copy ---\n");
    LogRecord n = LogRecord_new_number((LogMetric){ .value = 42 });
    LogRecord n_copy = LogRecord_copy(&n, copy_text, copy_number);
    printf("metric original=%d copy=%d\n",
           LogRecord_get_number(&n)->value, LogRecord_get_number(&n_copy)->value);
    LogRecord_destroy(&n, destroy_text, destroy_number);
    LogRecord_destroy(&n_copy, destroy_text, destroy_number);
    return 0;
}
