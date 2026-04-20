#ifndef __F407_TIM_H
#define __F407_TIM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void F407_TIM_PeriodicInit(uint8_t timId, uint32_t periodMs);

#ifdef __cplusplus
}
#endif

#endif /* __F407_TIM_H */
