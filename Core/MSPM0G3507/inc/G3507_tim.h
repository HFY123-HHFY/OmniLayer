#ifndef __G3507_TIM_H
#define __G3507_TIM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void G3507_TIM_PeriodicInit(uint8_t timId, uint32_t periodMs);
uint8_t G3507_TIM_CheckAndClearUpdateIrq(uint8_t timId);

#ifdef __cplusplus
}
#endif

#endif /* __G3507_TIM_H */
