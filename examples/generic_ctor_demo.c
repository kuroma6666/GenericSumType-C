/*
 * 型からのイベント構築（タグ名を書かずに、値の型でコンストラクタを自動選択）
 *
 * 【要件】
 *   センサハブは温度・スイッチ・パルスカウントなど複数種のイベントを1つの
 *   イベント型に格納する。生成側でタグ名（_temp/_sw/_pulse）を毎回綴るのは
 *   間違いやすいので、値の型からコンストラクタを自動選択して構築したい。
 *
 * 【仕様】
 *   - SensorEvent は temp / sw / pulse のいずれか（各 payload は専用 struct）。
 *   - 渡した値の型から対応コンストラクタを自動選択して SensorEvent を構築できる。
 *   - C11 の _Generic を使うため C11 以降限定。C99 ビルドではスキップメッセージのみ
 *     （ファイル自身を __STDC_VERSION__ で自己ガード）。
 *
 * 【実装方針】
 *   - DEFINE_SUM_NEW_GENERIC で補助を生成し、SUM_NEW で構築。
 *     利用側の推奨イディオム #define SensorEvent_new(x) SUM_NEW(...) も示す。
 *   - 注意（プリプロセッサの制約）: 複数フィールドの複合リテラルを SUM_NEW や
 *     SensorEvent_new へ「その場で」渡すと、波括弧内のカンマがマクロ引数区切りと
 *     誤認されて壊れる（括弧はカンマを守るが波括弧は守らない）。実務では payload を
 *     いったん名前付き変数に代入して渡すのが安全（本デモもそうしている）。
 *   - 2つ以上の variant が同一 payload 型を共有すると _Generic 連想が重複し
 *     コンパイルエラーになる（design_spec 4.13節）。専用 struct でラップして回避。
 *
 * 仕様の詳細: examples/specs/sensor_event.md
 */
#include <stdio.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#include "generic_sum_type.h"

typedef struct { double celsius; } TempReading;
typedef struct { int gpio; int on; } SwitchEvent;
typedef struct { unsigned count; }  PulseCount;

#define SENSOR_EVENT(X, NAME, EXTRA)     \
    X(NAME, EXTRA, temp,  TempReading)   \
    X(NAME, EXTRA, sw,    SwitchEvent)   \
    X(NAME, EXTRA, pulse, PulseCount)

DEFINE_SUM_TYPE(SensorEvent, SENSOR_EVENT)
DEFINE_SUM_NEW_GENERIC(SensorEvent, SENSOR_EVENT)
#define SensorEvent_new(x) SUM_NEW(SensorEvent, SENSOR_EVENT, (x))

int main(void) {
    /* payload は名前付き変数に入れてから渡す（複合リテラル内カンマ問題の回避） */
    TempReading tr = { .celsius = 25.5 };
    SwitchEvent se = { .gpio = 4, .on = 1 };
    PulseCount  pc = { .count = 128 };

    SensorEvent e1 = SensorEvent_new(tr);   /* 型 TempReading -> _temp を自動選択 */
    SensorEvent e2 = SensorEvent_new(se);   /* 型 SwitchEvent -> _sw   を自動選択 */
    SensorEvent e3 = SensorEvent_new(pc);   /* 型 PulseCount  -> _pulse を自動選択 */

    printf("e1 temp:  celsius=%.1f\n", SensorEvent_get_temp(&e1)->celsius);
    printf("e2 sw:    gpio=%d on=%d\n", SensorEvent_get_sw(&e2)->gpio, SensorEvent_get_sw(&e2)->on);
    printf("e3 pulse: count=%u\n", SensorEvent_get_pulse(&e3)->count);
    return (e1.tag == SensorEvent_temp && e2.tag == SensorEvent_sw && e3.tag == SensorEvent_pulse) ? 0 : 1;
}
#else
int main(void) {
    printf("skip: DEFINE_SUM_NEW_GENERIC/SUM_NEW は C11 以降限定（_Generic 不可のため）\n");
    return 0;
}
#endif
