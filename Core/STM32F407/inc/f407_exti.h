#ifndef __F407_EXTI_H
#define __F407_EXTI_H

#include <stdint.h>
#include "exti.h"

#ifdef __cplusplus
extern "C" {
#endif

void F407_EXTI_Init(void *port, uint8_t lineIndex, API_EXTI_Trigger_t trigger,
	uint32_t irqn, uint8_t preemptPriority, uint8_t subPriority);
uint8_t F407_EXTI_IsPendingAndClear(uint8_t lineIndex);

#ifdef __cplusplus
}
#endif

#endif /* __F407_EXTI_H */
