#define _POSIX_C_SOURCE 200809L /* strdup() はPOSIX拡張でありISO Cの一部ではないため必要 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generic_sum_type.h"

/* --- variantごとのpayload --- */
typedef struct { char *str; } MsgText;    /* heap資源を所有するvariant */
typedef struct { int value; }  MsgNumber; /* POD（資源を持たない）variant */

#define MESSAGE_VARIANTS(X, NAME, EXTRA) \
    X(NAME, EXTRA, text,   MsgText)      \
    X(NAME, EXTRA, number, MsgNumber)

DEFINE_SUM_TYPE(Message, MESSAGE_VARIANTS)
DEFINE_SUM_DESTROY(Message, MESSAGE_VARIANTS, Message_destroy)
DEFINE_SUM_COPY(Message, MESSAGE_VARIANTS, Message_copy)

/* --- destroyハンドラ --- */
static void destroy_text(MsgText *t) {
    printf("  free(%p) \"%s\"\n", (void *)t->str, t->str);
    free(t->str);
    t->str = NULL;
}
SUM_DEFINE_NOOP_DESTROY(destroy_number, MsgNumber) /* 資源を持たないのでno-opでよい */

/* --- copyハンドラ --- */
static MsgText copy_text(const MsgText *t) {
    char *dup = strdup(t->str);
    printf("  strdup: %p (元) -> %p (複製)\n", (void *)t->str, (void *)dup);
    return (MsgText){ .str = dup };
}
SUM_DEFINE_IDENTITY_COPY(copy_number, MsgNumber) /* 値そのものを複製すればよい */

int main(void) {
    printf("--- Textバリアント: ディープコピーの確認 ---\n");
    Message original = Message_new_text((MsgText){ .str = strdup("hello") });
    Message copy = Message_copy(&original, copy_text, copy_number);

    MsgText *ot = Message_get_text(&original);
    MsgText *ct = Message_get_text(&copy);
    printf("original.str=%p copy.str=%p -> %s\n",
           (void *)ot->str, (void *)ct->str,
           ot->str != ct->str ? "別アドレス(ディープコピー成功)" : "同一アドレス(バグ)");

    Message_destroy(&original, destroy_text, destroy_number);
    Message_destroy(&copy, destroy_text, destroy_number);

    printf("--- Numberバリアント: identity copyの確認 ---\n");
    Message n = Message_new_number((MsgNumber){ .value = 42 });
    Message n_copy = Message_copy(&n, copy_text, copy_number);
    printf("number original=%d copy=%d\n",
           Message_get_number(&n)->value, Message_get_number(&n_copy)->value);

    Message_destroy(&n, destroy_text, destroy_number);
    Message_destroy(&n_copy, destroy_text, destroy_number);

    return 0;
}
