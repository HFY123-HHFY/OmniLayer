#ifndef __F103_TIM_H
#define __F103_TIM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void F103_TIM_PeriodicInit(uint8_t timId, uint32_t periodMs);
uint8_t F103_TIM_CheckAndClearUpdateIrq(uint8_t timId);

#ifdef __cplusplus
}
#endif



#endif /* __F103_TIM_H */
