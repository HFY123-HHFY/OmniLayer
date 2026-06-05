#ifndef __F407_ENCODER_H
#define __F407_ENCODER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * F407 编码器底层驱动：
 * - 使用定时器编码器模式（TIM1 / TIM4）；
 * - GPIO 需配置为 AF 模式以接入定时器通道；
 * - 计数器自动累加方向计数，无需中断。
 */

/* coreId 即 TIM 编号：TIM1=0, TIM4=3 */
void    F407_Encoder_Init(uint8_t coreId);
int16_t F407_Encoder_GetCount(uint8_t coreId);

#ifdef __cplusplus
}
#endif

#endif /* __F407_ENCODER_H */
