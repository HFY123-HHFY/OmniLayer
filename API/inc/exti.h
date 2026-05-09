#ifndef __API_EXTI_H
#define __API_EXTI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	API_EXTI_TRIGGER_RISING = 0x01U,
	API_EXTI_TRIGGER_FALLING = 0x02U
} API_EXTI_Trigger_t;

/* 初始化 EXTI 线：端口映射、触发沿、NVIC 优先级。 */
void API_EXTI_Init(void *port, uint32_t pin, API_EXTI_Trigger_t trigger,
	uint32_t irqn, uint8_t preemptPriority, uint8_t subPriority);

/* 查询并清除指定 EXTI 线 pending 标志，命中返回 1。 */
uint8_t API_EXTI_IsPendingAndClear(uint8_t lineIndex);

#ifdef __cplusplus
}
#endif

#endif /* __API_EXTI_H */
