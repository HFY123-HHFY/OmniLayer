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

#define SYS_EXTI_INVALID_IRQN (0xFFFFFFFFUL)

/* 系统层初始化 */
void SYS_Init(void);

/* EXTI 公共辅助：线号计算、IRQ 映射、线组判断。 */
uint8_t SYS_EXTI_GetLineIndex(uint32_t pin);
uint32_t SYS_EXTI_GetIrqn(void *port, uint32_t pin);
uint8_t SYS_EXTI_LineInGroup(uint32_t pin, uint8_t startLine, uint8_t endLine);

#ifdef __cplusplus
}
#endif

#endif /* __SYS_H */
