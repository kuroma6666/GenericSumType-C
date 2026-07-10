/*
 * protocol_frame_demo.c
 *
 * 「通信処理系」のCommandパターン例。
 *
 * 型定義・MATCH・分類ロジック・チェックサム計算は protocol_frame.h に
 * 集約されている(tests/test_protocol_frame.c と共有するため)。このファイルは
 * 受信済みライン群を分類して整形するだけの、薄いドライバである。
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
#include "protocol_frame.h"

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
