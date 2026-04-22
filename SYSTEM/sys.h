#ifndef __SYS_H
#define __SYS_H

#include <stdint.h>

#include "Enroll.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	SYS_EXTI_TRIGGER_RISING = 0x01U, // 上升沿触发
	SYS_EXTI_TRIGGER_FALLING = 0x02U // 下降沿触发
} SYS_EXTI_Trigger_t;

/* 系统层初始化：保留为系统入口，当前不做默认中断注册。 */
void SYS_Init(void);

/* 注册一条 EXTI 线并配置触发沿/优先级。 */
void SYS_EXTI_Register(void *port, uint16_t pin, SYS_EXTI_Trigger_t trigger,
	uint8_t preemptPriority, uint8_t subPriority);

/*
 * 处理某个 EXTI 线组（例如 5~9）：
 * 返回 1 表示本次确实处理到注册线中断，返回 0 表示未命中。
 */
uint8_t SYS_EXTI_IRQHandlerGroup(uint8_t startLine, uint8_t endLine);

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
void F103_SYS_EXTI_Init(void *port, uint8_t lineIndex, SYS_EXTI_Trigger_t trigger,
	uint32_t irqn, uint8_t preemptPriority, uint8_t subPriority);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
void F407_SYS_EXTI_Init(void *port, uint8_t lineIndex, SYS_EXTI_Trigger_t trigger,
	uint32_t irqn, uint8_t preemptPriority, uint8_t subPriority);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SYS_H */
