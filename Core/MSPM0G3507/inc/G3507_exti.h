#ifndef __G3507_EXTI_H
#define __G3507_EXTI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void G3507_EXTI_Init(void *port, uint32_t pin, uint32_t trigger,
	uint32_t irqn, uint8_t preemptPriority, uint8_t subPriority);
uint8_t G3507_EXTI_IsPendingAndClear(void *port, uint32_t pin);

#ifdef __cplusplus
}
#endif

#endif /* __G3507_EXTI_H */
