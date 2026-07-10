/*
 * protocol_frame.h
 *
 * protocol_frame_demo.c と tests/test_protocol_frame.c から共有される
 * 「通信処理系」フレームの型・MATCH定義・分類ロジック・チェックサム計算。
 *
 * design_spec.md 4.10節で検証済みの「型定義を1つの共通ヘッダに1回だけ書き、
 * 複数の.cからincludeする」というパターンを応用している。
 *
 * 参考: PALLASER社 レーザーマーカー 通信コマンド仕様書 Ver.2.0
 *   https://pallaser.co.jp/dwl/mother/R2_0_Communication_Manual.pdf
 *   (§2 通信仕様。本ファイルは同仕様書のフレーム構造・NGエラー番号・
 *    チェックサム計算式に着想を得た独自の簡略化例であり、仕様書の転載ではない)
 */
#ifndef PROTOCOL_FRAME_H
#define PROTOCOL_FRAME_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "generic_sum_type.h"

/* --- NGエラー番号(仕様書2.1.3 NGエラーの表に対応) --- */
#define NG_T001_STX_ERROR     1  /* STXエラー */
#define NG_T002_COMMAND_ERROR 2  /* コマンドエラー */
#define NG_T003_FORMAT_ERROR  3  /* フォーマットエラー */
#define NG_T004_CONTENT_ERROR 4  /* コンテンツエラー */

SUM_MAYBE_UNUSED
static inline const char *ng_error_name(int code) {
    switch (code) {
        case NG_T001_STX_ERROR:     return "T001 STXエラー";
        case NG_T002_COMMAND_ERROR: return "T002 コマンドエラー";
        case NG_T003_FORMAT_ERROR:  return "T003 フォーマットエラー";
        case NG_T004_CONTENT_ERROR: return "T004 コンテンツエラー";
        default:                    return "未知のエラー番号";
    }
}

/* --- チェックサム計算(仕様書2.3.3 チェックサム計算式) ---
 * 送信データ(デリミタを除く)の各バイトのASCIIコード値を単純合計し、
 * 256で割った余り(オーバーフローは無視)を2桁の16進数文字列(大文字)にする。
 */
SUM_MAYBE_UNUSED
static inline unsigned char compute_checksum(const char *data, size_t len) {
    unsigned int sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum += (unsigned char)data[i];
    }
    return (unsigned char)(sum % 256);
}

SUM_MAYBE_UNUSED
static inline void format_checksum_hex(unsigned char csum, char out[3]) {
    static const char hex[] = "0123456789ABCDEF";
    out[0] = hex[(csum >> 4) & 0xF];
    out[1] = hex[csum & 0xF];
    out[2] = '\0';
}

/* --- 受信フレームのpayload ---
 * command/argsは元のコマンド文字列をそのまま保持する(内部処理層でのデコードは
 * このファイルの責務外)。NGフレームだけはエラー番号を整数で保持する。
 */
typedef struct { char command[8]; char args[64]; } FrameWriteRequest; /* W,CMD,args... */
typedef struct { char command[8]; char args[64]; } FrameReadRequest;  /* R,CMD,args... */
typedef struct { char payload[64]; }                FrameOkReply;     /* OK[,vals...] */
typedef struct { int error_code; }                  FrameNgReply;     /* NG,Txxx */

#define FRAME_VARIANTS(X, NAME, EXTRA)          \
    X(NAME, EXTRA, write_request, FrameWriteRequest) \
    X(NAME, EXTRA, read_request,  FrameReadRequest)  \
    X(NAME, EXTRA, ok_reply,      FrameOkReply)       \
    X(NAME, EXTRA, ng_reply,      FrameNgReply)

DEFINE_SUM_TYPE(Frame, FRAME_VARIANTS)
DEFINE_SUM_MATCH(Frame, FRAME_VARIANTS, Frame_describe, int)

SUM_MAYBE_UNUSED
static inline int describe_write(FrameWriteRequest *f) {
    printf("[FRAME] Write要求  command=%-4s args=\"%s\"\n", f->command, f->args);
    return 0;
}
SUM_MAYBE_UNUSED
static inline int describe_read(FrameReadRequest *f) {
    printf("[FRAME] Read要求   command=%-4s args=\"%s\"\n", f->command, f->args);
    return 0;
}
SUM_MAYBE_UNUSED
static inline int describe_ok(FrameOkReply *f) {
    printf("[FRAME] OK応答     payload=\"%s\"\n", f->payload);
    return 0;
}
SUM_MAYBE_UNUSED
static inline int describe_ng(FrameNgReply *f) {
    printf("[FRAME] NG応答     %s\n", ng_error_name(f->error_code));
    return 0;
}

/* --- 受信した1行(デリミタ除去済み)をFrameへ分類するクラシファイア ---
 * 「,」区切りの先頭トークンでW/R/OK/NGを判別する、簡略化したパーサー。
 * 実際の仕様書ではSTX/ETX付加の有無やチェックサム検証も併せて行うが、
 * ここではフレーム種別の判定ロジックに焦点を絞っている。
 */
SUM_MAYBE_UNUSED
static inline Frame classify_line(const char *line) {
    if (strncmp(line, "W,", 2) == 0) {
        FrameWriteRequest f = { { 0 }, { 0 } };
        const char *rest = line + 2;
        const char *comma = strchr(rest, ',');
        size_t cmd_len = comma ? (size_t)(comma - rest) : strlen(rest);
        if (cmd_len >= sizeof(f.command)) cmd_len = sizeof(f.command) - 1;
        memcpy(f.command, rest, cmd_len);
        if (comma) {
            size_t args_len = strlen(comma + 1);
            if (args_len >= sizeof(f.args)) args_len = sizeof(f.args) - 1;
            memcpy(f.args, comma + 1, args_len);
        }
        return Frame_new_write_request(f);
    }
    if (strncmp(line, "R,", 2) == 0) {
        FrameReadRequest f = { { 0 }, { 0 } };
        const char *rest = line + 2;
        const char *comma = strchr(rest, ',');
        size_t cmd_len = comma ? (size_t)(comma - rest) : strlen(rest);
        if (cmd_len >= sizeof(f.command)) cmd_len = sizeof(f.command) - 1;
        memcpy(f.command, rest, cmd_len);
        if (comma) {
            size_t args_len = strlen(comma + 1);
            if (args_len >= sizeof(f.args)) args_len = sizeof(f.args) - 1;
            memcpy(f.args, comma + 1, args_len);
        }
        return Frame_new_read_request(f);
    }
    if (strncmp(line, "NG,T", 4) == 0) {
        int code = 0;
        sscanf(line + 4, "%3d", &code);
        return Frame_new_ng_reply((FrameNgReply){ .error_code = code });
    }
    /* 上記いずれにも当てはまらなければOK応答とみなす(簡略化) */
    FrameOkReply f = { { 0 } };
    size_t len = strlen(line);
    if (len >= sizeof(f.payload)) len = sizeof(f.payload) - 1;
    memcpy(f.payload, line, len);
    return Frame_new_ok_reply(f);
}

#endif /* PROTOCOL_FRAME_H */
