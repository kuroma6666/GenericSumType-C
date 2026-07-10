/*
 * tests/test_protocol_frame.c
 *
 * examples/protocol_frame.h(通信処理系Commandパターン)の単体テスト。
 * test_generic_sum_type.c と同じ方針(外部フレームワークなし、assert()のみ)。
 *
 * describe_*系(printfで人間向けに整形するだけの関数)は副作用が標準出力の
 * 見た目にしかないため、ここでは検証しない。テストの対象は classify_line()
 * (フレーム分類)と compute_checksum()/format_checksum_hex()
 * (仕様書2.3.3のチェックサム計算)という、戻り値を持つ純粋なロジックに絞る。
 *
 * ビルド: gcc -std=c11 -pedantic -Wall -Wextra -Werror -Iinclude \
 *             -o test_protocol_frame tests/test_protocol_frame.c
 * 実行:   ./test_protocol_frame
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../examples/protocol_frame.h"

static int tests_run = 0;
#define RUN(test) do { printf("- %-55s ... ", #test); test(); tests_run++; printf("OK\n"); } while (0)

/* ============ 1. classify_line(): フレーム種別の判定 ============ */

static void test_classify_write_request(void) {
    Frame f = classify_line("W,MST,Kind=1");
    FrameWriteRequest *w = Frame_get_write_request(&f);
    assert(w != NULL);
    assert(strcmp(w->command, "MST") == 0);
    assert(strcmp(w->args, "Kind=1") == 0);
}

static void test_classify_write_request_without_args(void) {
    Frame f = classify_line("W,ERC");
    FrameWriteRequest *w = Frame_get_write_request(&f);
    assert(w != NULL);
    assert(strcmp(w->command, "ERC") == 0);
    assert(strcmp(w->args, "") == 0); /* コンマがない = 引数なし */
}

static void test_classify_read_request(void) {
    Frame f = classify_line("R,STA");
    FrameReadRequest *r = Frame_get_read_request(&f);
    assert(r != NULL);
    assert(strcmp(r->command, "STA") == 0);
}

static void test_classify_ng_reply(void) {
    Frame f = classify_line("NG,T003");
    FrameNgReply *ng = Frame_get_ng_reply(&f);
    assert(ng != NULL);
    assert(ng->error_code == NG_T003_FORMAT_ERROR);
}

static void test_classify_ok_reply_fallback(void) {
    /* W,/R,/NG,T のいずれにも当てはまらない行はOK応答とみなす(簡略化仕様) */
    Frame f = classify_line("OK,A,0,Ready");
    FrameOkReply *ok = Frame_get_ok_reply(&f);
    assert(ok != NULL);
    assert(strcmp(ok->payload, "OK,A,0,Ready") == 0);
}

/* ============ 2. チェックサム計算(仕様書2.3.3) ============ */

static void test_checksum_of_empty_string_is_zero(void) {
    unsigned char csum = compute_checksum("", 0);
    assert(csum == 0);
    char hex[3];
    format_checksum_hex(csum, hex);
    assert(strcmp(hex, "00") == 0);
}

static void test_checksum_matches_known_value(void) {
    /* 実際にビルド・実行して得た既知の結果を回帰テストとして固定する。
     * 計算式自体は仕様書2.3.3(バイト総和を256で割った余りを16進2桁化、
     * オーバーフローは無視)に基づく。 */
    const char sample[] = "R,PWC,Power=A,Freq=B,Pulse=C";
    unsigned char csum = compute_checksum(sample, strlen(sample));
    char hex[3];
    format_checksum_hex(csum, hex);
    assert(csum == 0x0D);
    assert(strcmp(hex, "0D") == 0);
}

static void test_checksum_hex_uses_uppercase_and_ignores_overflow(void) {
    /* 256バイト分の 0xFF を足すとオーバーフローするが、mod 256により
     * 単に折り返されるだけで、破綻しないことを確認する。 */
    char data[300];
    memset(data, 0xFF, sizeof(data));
    unsigned char csum = compute_checksum(data, sizeof(data));
    char hex[3];
    format_checksum_hex(csum, hex);
    /* 大文字A~Fのみを使うこと(仕様書の指定通り) */
    assert(hex[0] < 'a' || hex[0] > 'z');
    assert(hex[1] < 'a' || hex[1] > 'z');
}

/* ============ 3. NGエラー番号 -> 名称の対応 ============ */

static void test_ng_error_name_known_codes(void) {
    assert(strcmp(ng_error_name(NG_T001_STX_ERROR), "T001 STXエラー") == 0);
    assert(strcmp(ng_error_name(NG_T004_CONTENT_ERROR), "T004 コンテンツエラー") == 0);
}

static void test_ng_error_name_unknown_code_is_safe(void) {
    /* 未知のエラー番号でもクラッシュせず、フォールバック文字列を返す */
    const char *name = ng_error_name(999);
    assert(name != NULL);
}

int main(void) {
    printf("protocol_frame.h 単体テスト\n");
    RUN(test_classify_write_request);
    RUN(test_classify_write_request_without_args);
    RUN(test_classify_read_request);
    RUN(test_classify_ng_reply);
    RUN(test_classify_ok_reply_fallback);
    RUN(test_checksum_of_empty_string_is_zero);
    RUN(test_checksum_matches_known_value);
    RUN(test_checksum_hex_uses_uppercase_and_ignores_overflow);
    RUN(test_ng_error_name_known_codes);
    RUN(test_ng_error_name_unknown_code_is_safe);
    printf("%d件全て成功\n", tests_run);
    return 0;
}
