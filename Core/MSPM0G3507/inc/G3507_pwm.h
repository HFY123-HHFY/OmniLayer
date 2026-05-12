#ifndef __G3507_PWM_H
#define __G3507_PWM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 配置指定底层定时器通道对应的 PWM 引脚复用。 */
void G3507_PWM_ConfigPin(uint8_t coreTimId, uint8_t coreChannel);
/* 初始化指定底层定时器的 PWM 周期。 */
void G3507_PWM_InitTimer(uint8_t coreTimId, uint16_t arr, uint16_t psc);
/* 设置指定底层定时器通道的比较值。 */
void G3507_PWM_SetCCR(uint8_t coreTimId, uint8_t coreChannel, uint16_t ccr);

#ifdef __cplusplus
}
#endif

#endif /* __G3507_PWM_H */
