/* G-017 と同じ設計ミスだが、C99 には _Generic が無いため SUM_DEFINE_NEW_GENERIC
 * 自体が存在せず、機械検査が働かない。C99 では設計ガイドラインとレビューに頼るしかない、
 * という std 依存の限界を固定する。 */
#include "generic_sum_type.h"
#include <stdint.h>
typedef int32_t DeviceId;
typedef int32_t SensorId;
#define V(X, NAME, EXTRA) X(NAME, EXTRA, device, DeviceId) X(NAME, EXTRA, sensor, SensorId)
SUM_DEFINE_TYPE(T, V)
int main(void) { return 0; }
