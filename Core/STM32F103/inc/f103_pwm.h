#ifndef __F103_PWM_H
#define __F103_PWM_H

#include <stdint.h>

#include "f103_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 配置 PWM 输出引脚为复用推挽输出。 */
void F103_PWM_ConfigPin(void *port, uint16_t pin);
/* 初始化指定定时器的 ARR/PSC。 */
void F103_PWM_InitTimer(uint8_t timId, uint16_t arr, uint16_t psc);
/* 设置指定通道的 CCR 值。 */
void F103_PWM_SetCCR(uint8_t timId, uint8_t channel, uint16_t ccr);

#ifdef __cplusplus
}
#endif

#endif /* __F103_PWM_H */
