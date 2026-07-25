/* 「ペイロードに素の基本型を直接使わない」というガイドラインを守っていても、
 * typedef は別名であって別型ではないため、不変条件（variant 間でペイロード型が
 * 重複しない）は破れる。ガイドラインでは防げず、機械検査だけが捕まえる例。
 * C99 では同じコードが通ってしまう（G-018）。 */
#include "generic_sum_type.h"
#include <stdint.h>
typedef int32_t DeviceId;
typedef int32_t SensorId;   /* 別名だが DeviceId と同じ型 */
#define V(X, NAME, EXTRA) X(NAME, EXTRA, device, DeviceId) X(NAME, EXTRA, sensor, SensorId)
DEFINE_SUM_TYPE(T, V)
DEFINE_SUM_NEW_GENERIC(T, V)
int main(void) { return 0; }
