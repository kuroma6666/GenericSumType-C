/*
 * protocol_frame_demo.c
 *
 * 「通信処理系」のCommandパターン例。
 *
 * 実在するレーザーマーカーの通信コマンド仕様書(PALLASER社 レーザーマーカー
 * 通信コマンド仕様書 Ver.2.0, §2 通信仕様)で定義されている、
 *   ・W(Write)/R(Read)コマンドのASCIIフォーマット
 *   ・NGエラー(T001 STXエラー, T002 コマンドエラー,
 *              T003 フォーマットエラー, T004 コンテンツエラー)
 *   ・チェックサム計算式(バイト列の総和を256で割った余りを2桁16進ASCII化、
 *     A〜Fは大文字。オーバーフローは無視)
 * という"通信フレームの構造"に着想を得て、独自に簡略化して書き起こした例。
 * 実際の仕様書のSTX/ETX付加設定や具体的なバイト列そのものではなく、
 * 「通信フレームを解釈する層」をどう設計するかを示すための教材である。
 *
 * ここでの「通信処理系」とは、受信した1行(すでにSTX/ETX/CR等のデリミタは
 * 除去済みとする)を、
 *   ・Writeコマンド要求か
 *   ・Readコマンド要求か
 *   ・OK応答か
 *   ・NG応答(エラー番号付き)か
 * のいずれかに分類し、内容を整形する層を指す。ここで解釈された結果を
 * internal_command_demo.c のようなコマンド固有ハンドラへ渡すのは、
 * さらに上位の層の責務であり、本ファイルはその一歩手前
 * (フレームの構造を理解する層)だけを担当する。この責務分割により、
 * 「通信フォーマットが変わったときの修正」と「デバイスの振る舞いが
 * 変わったときの修正」が互いに影響しないようにできる。
 *
 * internal_command_demo.c / recipe_param_demo.c はいずれも
 * DEFINE_SUM_DISPATCH(共有ctxへの副作用)を使っているのに対し、
 * 本ファイルは DEFINE_SUM_MATCH(受信フレーム→文字列への純粋な変換)を
 * 使っている。フレーム解釈自体は「今何を受信したか」を判定するだけの
 * 副作用のない処理であるため、ctxを持たないMATCHの方が契約として素直、
 * という設計判断の実例でもある(api_reference.md 早見表を参照)。
 *
 * 参考: PALLASER社 レーザーマーカー 通信コマンド仕様書 Ver.2.0
 *   https://pallaser.co.jp/dwl/mother/R2_0_Communication_Manual.pdf
 *   (§2 通信仕様。本ファイルは同仕様書のフレーム構造・NGエラー番号・
 *    チェックサム計算式に着想を得た独自の簡略化例であり、仕様書の転載ではない)
 */
#include <stdio.h>
#include <string.h>
#include "generic_sum_type.h"

/* --- NGエラー番号(仕様書2.1.3 NGエラーの表に対応) --- */
#define NG_T001_STX_ERROR     1  /* STXエラー */
#define NG_T002_COMMAND_ERROR 2  /* コマンドエラー */
#define NG_T003_FORMAT_ERROR  3  /* フォーマットエラー */
#define NG_T004_CONTENT_ERROR 4  /* コンテンツエラー */

static const char *ng_error_name(int code) {
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
static unsigned char compute_checksum(const char *data, size_t len) {
    unsigned int sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum += (unsigned char)data[i];
    }
    return (unsigned char)(sum % 256);
}

static void format_checksum_hex(unsigned char csum, char out[3]) {
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

static int describe_write(FrameWriteRequest *f) {
    printf("[FRAME] Write要求  command=%-4s args=\"%s\"\n", f->command, f->args);
    return 0;
}
static int describe_read(FrameReadRequest *f) {
    printf("[FRAME] Read要求   command=%-4s args=\"%s\"\n", f->command, f->args);
    return 0;
}
static int describe_ok(FrameOkReply *f) {
    printf("[FRAME] OK応答     payload=\"%s\"\n", f->payload);
    return 0;
}
static int describe_ng(FrameNgReply *f) {
    printf("[FRAME] NG応答     %s\n", ng_error_name(f->error_code));
    return 0;
}

/* --- 受信した1行(デリミタ除去済み)をFrameへ分類するクラシファイア ---
 * 「,」区切りの先頭トークンでW/R/OK/NGを判別する、簡略化したパーサー。
 * 実際の仕様書ではSTX/ETX付加の有無やチェックサム検証も併せて行うが、
 * ここではフレーム種別の判定ロジックに焦点を絞っている。
 */
static Frame classify_line(const char *line) {
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

int main(void) {
    /* ホスト側から届いた(想定の)受信済みライン群。実運用ならシリアル/TCP
     * から1行ずつ読み込むイメージ。 */
    const char *received_lines[] = {
        "W,MST,Kind=1",
        "R,STA",
        "OK,A,0,Ready",
        "NG,T003",
    };
    size_t n = sizeof(received_lines) / sizeof(received_lines[0]);

    for (size_t i = 0; i < n; ++i) {
        Frame f = classify_line(received_lines[i]);
        Frame_describe(&f, describe_write, describe_read, describe_ok, describe_ng);
    }

    /* チェックサム計算のデモ(仕様書2.3.3の例と同じ考え方) */
    const char sample[] = "R,PWC,Power=A,Freq=B,Pulse=C";
    unsigned char csum = compute_checksum(sample, strlen(sample));
    char hex[3];
    format_checksum_hex(csum, hex);
    printf("[CHECKSUM] \"%s\" -> %s\n", sample, hex);

    return 0;
}
